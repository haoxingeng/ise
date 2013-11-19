/****************************************************************************\
*                                                                            *
*  ISE (Iris Server Engine) Project                                          *
*  http://github.com/haoxingeng/ise                                          *
*                                                                            *
*  Copyright 2013 HaoXinGeng (haoxingeng@gmail.com)                          *
*  All rights reserved.                                                      *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
\****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// 文件名称: ise_event_loop.cpp
// 功能描述: 事件循环实现
///////////////////////////////////////////////////////////////////////////////

#include "ise/main/ise_event_loop.h"
#include "ise/main/ise_err_msgs.h"

using namespace ise;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class EventLoop

EventLoop::EventLoop() :
    thread_(NULL),
    loopThreadId_(0),
    lastCheckTimeoutTicks_(0)
{
    // nothing
}

EventLoop::~EventLoop()
{
    stop(false, true);
}

//-----------------------------------------------------------------------------
// 描述: 启动工作线程
//-----------------------------------------------------------------------------
void EventLoop::start()
{
    if (!thread_)
    {
        thread_ = new EventLoopThread(*this);
        thread_->run();
    }
}

//-----------------------------------------------------------------------------
// 描述: 停止工作线程
//-----------------------------------------------------------------------------
void EventLoop::stop(bool force, bool waitFor)
{
    if (thread_ && thread_->isRunning())
    {
        if (force)
        {
            thread_->kill();
            thread_ = NULL;
            waitFor = false;
        }
        else
        {
            thread_->terminate();
            wakeupLoop();
        }

        if (waitFor)
        {
            thread_->waitFor();
            delete thread_;
            thread_ = NULL;
        }
    }
}

//-----------------------------------------------------------------------------
// 描述: 判断工作线程当前是否正在运行
//-----------------------------------------------------------------------------
bool EventLoop::isRunning()
{
    return (thread_ != NULL && thread_->isRunning());
}

//-----------------------------------------------------------------------------
// 描述: 判断当前调用此方法的线程和此 eventLoop 所属线程是不是同一个线程
//-----------------------------------------------------------------------------
bool EventLoop::isInLoopThread()
{
    return loopThreadId_ == getCurThreadId();
}

//-----------------------------------------------------------------------------
// 描述: 确保当前调用在事件循环线程内
//-----------------------------------------------------------------------------
void EventLoop::assertInLoopThread()
{
    ISE_ASSERT(isInLoopThread());
}

//-----------------------------------------------------------------------------
// 描述: 在事件循环线程中立即执行指定的仿函数
// 备注: 线程安全
//-----------------------------------------------------------------------------
void EventLoop::executeInLoop(const Functor& functor)
{
    if (isInLoopThread())
        functor();
    else
        delegateToLoop(functor);
}

//-----------------------------------------------------------------------------
// 描述: 将指定的仿函数委托给事件循环线程执行。线程在完成当前一轮事件循环后再
//       执行被委托的仿函数。
// 备注: 线程安全
//-----------------------------------------------------------------------------
void EventLoop::delegateToLoop(const Functor& functor)
{
    {
        AutoLocker locker(delegatedFunctors_.mutex);
        delegatedFunctors_.items.push_back(functor);
    }

    wakeupLoop();
}

//-----------------------------------------------------------------------------
// 描述: 添加一个清理器 (finalizer) 到事件循环中，在每次循环的最后会执行它们
//-----------------------------------------------------------------------------
void EventLoop::addFinalizer(const Functor& finalizer)
{
    AutoLocker locker(finalizers_.mutex);
    finalizers_.items.push_back(finalizer);
}

//-----------------------------------------------------------------------------
// 描述: 添加定时器 (指定时间执行)
//-----------------------------------------------------------------------------
TimerId EventLoop::executeAt(Timestamp time, const TimerCallback& callback)
{
    return addTimer(time, 0, callback);
}

//-----------------------------------------------------------------------------
// 描述: 添加定时器 (在 delay 秒后执行)
//-----------------------------------------------------------------------------
TimerId EventLoop::executeAfter(double delay, const TimerCallback& callback)
{
    Timestamp time(Timestamp::now() + delay * MICROSECS_PER_SECOND);
    return addTimer(time, 0, callback);
}

//-----------------------------------------------------------------------------
// 描述: 添加定时器 (每 interval 秒循环执行)
//-----------------------------------------------------------------------------
TimerId EventLoop::executeEvery(double interval, const TimerCallback& callback)
{
    Timestamp time(Timestamp::now() + interval * MICROSECS_PER_SECOND);
    return addTimer(time, interval, callback);
}

//-----------------------------------------------------------------------------
// 描述: 取消定时器
//-----------------------------------------------------------------------------
void EventLoop::cancelTimer(TimerId timerId)
{
    executeInLoop(boost::bind(&TimerQueue::cancelTimer, &timerQueue_, timerId));
}

//-----------------------------------------------------------------------------
// 描述: 执行事件循环
//-----------------------------------------------------------------------------
void EventLoop::runLoop(Thread *thread)
{
    bool isTerminated = false;
    while (!isTerminated)
    {
        try
        {
	        if (thread->isTerminated())
            {
                isTerminated = true;
                wakeupLoop();
            }

            doLoopWork(thread);
            executeDelegatedFunctors();
            executeFinalizer();
        }
        catch (Exception& e)
        {
            logger().writeException(e);
        }
        catch (...)
        {}
    }
}

//-----------------------------------------------------------------------------
// 描述: 执行被委托的仿函数
//-----------------------------------------------------------------------------
void EventLoop::executeDelegatedFunctors()
{
    Functors functors;
    {
        AutoLocker locker(delegatedFunctors_.mutex);
        functors.swap(delegatedFunctors_.items);
    }

    for (size_t i = 0; i < functors.size(); ++i)
        functors[i]();
}

//-----------------------------------------------------------------------------
// 描述: 执行所有清理器
//-----------------------------------------------------------------------------
void EventLoop::executeFinalizer()
{
    Functors finalizers;
    {
        AutoLocker locker(finalizers_.mutex);
        finalizers.swap(finalizers_.items);
    }

    for (size_t i = 0; i < finalizers.size(); ++i)
        finalizers[i]();
}

//-----------------------------------------------------------------------------
// 描述: 在事件循环进入等待前，计算等待超时时间 (毫秒)
//-----------------------------------------------------------------------------
int EventLoop::calcLoopWaitTimeout()
{
    int result = TIMEOUT_INFINITE;
    Timestamp expiration;

    if (timerQueue_.getNearestExpiration(expiration))
    {
        Timestamp now(Timestamp::now());
        if (expiration <= now)
            result = 0;
        else
            result = ise::max((int)((expiration - now) / MICROSECS_PER_MILLISEC), 1);
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 事件循环等待完毕后，处理定时器事件
//-----------------------------------------------------------------------------
void EventLoop::processExpiredTimers()
{
    timerQueue_.processExpiredTimers(Timestamp::now());
}

//-----------------------------------------------------------------------------
// 描述: 添加定时器 (线程安全)
//-----------------------------------------------------------------------------
TimerId EventLoop::addTimer(Timestamp expiration, double interval, const TimerCallback& callback)
{
    Timer *timer = new Timer(expiration, interval, callback);

    // 此处必须调用 delegateToLoop，而不可以是 executeInLoop，因为前者能保证 wakeupLoop，
    // 从而马上重新计算事件循环的等待超时时间。
    delegateToLoop(boost::bind(&TimerQueue::addTimer, &timerQueue_, timer));

    return timer->timerId();
}

///////////////////////////////////////////////////////////////////////////////
// class EventLoopThread

EventLoopThread::EventLoopThread(EventLoop& eventLoop) :
    eventLoop_(eventLoop)
{
    setAutoDelete(false);
}

//-----------------------------------------------------------------------------

void EventLoopThread::execute()
{
    eventLoop_.loopThreadId_ = getThreadId();
    eventLoop_.runLoop(this);
}

//-----------------------------------------------------------------------------

void EventLoopThread::afterExecute()
{
    eventLoop_.loopThreadId_ = 0;
}

///////////////////////////////////////////////////////////////////////////////
// class EventLoopList

EventLoopList::EventLoopList(int loopCount) :
    items_(false, true),
    wantLoopCount_(loopCount)
{
    // nothing
}

EventLoopList::~EventLoopList()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 启动全部 eventLoop 的工作线程
//-----------------------------------------------------------------------------
void EventLoopList::start()
{
    if (getCount() == 0)
        setCount(wantLoopCount_);

    for (int i = 0; i < items_.getCount(); i++)
        items_[i]->start();
}

//-----------------------------------------------------------------------------
// 描述: 全部 eventLoop 停止工作
//-----------------------------------------------------------------------------
void EventLoopList::stop()
{
    const int MAX_WAIT_FOR_SECS = 10;   // (秒)
    const double SLEEP_INTERVAL = 0.5;  // (秒)

    // 通知停止
    for (int i = 0; i < items_.getCount(); i++)
        items_[i]->stop(false, false);

    // 等待停止
    double waitSecs = 0;
    while (waitSecs < MAX_WAIT_FOR_SECS)
    {
        int runningCount = 0;
        for (int i = 0; i < items_.getCount(); i++)
            if (items_[i]->isRunning()) runningCount++;

        if (runningCount == 0) break;

        sleepSeconds(SLEEP_INTERVAL, true);
        waitSecs += SLEEP_INTERVAL;
    }

    // 强行停止
    for (int i = 0; i < items_.getCount(); i++)
        items_[i]->stop(true, true);
}

//-----------------------------------------------------------------------------
// 描述: 根据事件循环线程ID查找对应的事件循环，找不到返回NULL
//-----------------------------------------------------------------------------
EventLoop* EventLoopList::findEventLoop(THREAD_ID loopThreadId)
{
    for (int i = 0; i < (int)items_.getCount(); ++i)
    {
        EventLoop *eventLoop = items_[i];
        if (eventLoop->getLoopThreadId() == loopThreadId)
            return eventLoop;
    }

    return NULL;
}

//-----------------------------------------------------------------------------

EventLoop* EventLoopList::createEventLoop()
{
    return new OsEventLoop();
}

//-----------------------------------------------------------------------------
// 描述: 设置 eventLoop 的个数
//-----------------------------------------------------------------------------
void EventLoopList::setCount(int count)
{
    count = ensureRange(count, 1, (int)MAX_LOOP_COUNT);

    for (int i = 0; i < count; i++)
        items_.add(createEventLoop());
}

///////////////////////////////////////////////////////////////////////////////
// class OsEventLoop

OsEventLoop::OsEventLoop()
{
#ifdef ISE_WINDOWS
    iocpObject_ = new IocpObject(this);
#endif
#ifdef ISE_LINUX
    epollObject_ = new EpollObject(this);
#endif
}

OsEventLoop::~OsEventLoop()
{
    stop(false, true);

#ifdef ISE_WINDOWS
    delete iocpObject_;
#endif
#ifdef ISE_LINUX
    delete epollObject_;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 执行单次事件循环中的工作
//-----------------------------------------------------------------------------
void OsEventLoop::doLoopWork(Thread *thread)
{
#ifdef ISE_WINDOWS
    iocpObject_->work();
#endif
#ifdef ISE_LINUX
    epollObject_->poll();
#endif
}

//-----------------------------------------------------------------------------
// 描述: 唤醒事件循环中的阻塞操作
//-----------------------------------------------------------------------------
void OsEventLoop::wakeupLoop()
{
#ifdef ISE_WINDOWS
    iocpObject_->wakeup();
#endif
#ifdef ISE_LINUX
    epollObject_->wakeup();
#endif
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
