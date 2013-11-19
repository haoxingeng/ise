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
// ise_event_loop.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_EVENT_LOOP_H_
#define _ISE_EVENT_LOOP_H_

#include "ise/main/ise_options.h"
#include "ise/main/ise_classes.h"
#include "ise/main/ise_sys_utils.h"
#include "ise/main/ise_exceptions.h"
#include "ise/main/ise_thread.h"
#include "ise/main/ise_timer.h"

#ifdef ISE_WINDOWS
#include "ise/main/ise_iocp.h"
#endif
#ifdef ISE_LINUX
#include "ise/main/ise_epoll.h"
#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// classes

class EventLoop;
class EventLoopThread;
class EventLoopList;
class OsEventLoop;

///////////////////////////////////////////////////////////////////////////////
// class EventLoop

class EventLoop : boost::noncopyable
{
public:
    typedef std::vector<Functor> Functors;

    struct FunctorList
    {
        Functors items;
        Mutex mutex;
    };

public:
    EventLoop();
    virtual ~EventLoop();

    void start();
    void stop(bool force, bool waitFor);

    bool isRunning();
    bool isInLoopThread();
    void assertInLoopThread();
    void executeInLoop(const Functor& functor);
    void delegateToLoop(const Functor& functor);
    void addFinalizer(const Functor& finalizer);

    TimerId executeAt(Timestamp time, const TimerCallback& callback);
    TimerId executeAfter(double delay, const TimerCallback& callback);
    TimerId executeEvery(double interval, const TimerCallback& callback);
    void cancelTimer(TimerId timerId);

    THREAD_ID getLoopThreadId() const { return loopThreadId_; };

protected:
    virtual void runLoop(Thread *thread);
    virtual void doLoopWork(Thread *thread) = 0;
    virtual void wakeupLoop() {}

protected:
    void executeDelegatedFunctors();
    void executeFinalizer();

    int calcLoopWaitTimeout();
    void processExpiredTimers();

private:
    TimerId addTimer(Timestamp expiration, double interval, const TimerCallback& callback);

protected:
    EventLoopThread *thread_;
    THREAD_ID loopThreadId_;
    FunctorList delegatedFunctors_;
    FunctorList finalizers_;
    UINT64 lastCheckTimeoutTicks_;
    TimerQueue timerQueue_;

    friend class EventLoopThread;
    friend class IocpObject;
    friend class EpollObject;
};

///////////////////////////////////////////////////////////////////////////////
// class EventLoopThread - 事件循环执行线程

class EventLoopThread : public Thread
{
public:
    EventLoopThread(EventLoop& eventLoop);
protected:
    virtual void execute();
    virtual void afterExecute();
private:
    EventLoop& eventLoop_;
};

///////////////////////////////////////////////////////////////////////////////
// class EventLoopList - 事件循环列表

class EventLoopList : boost::noncopyable
{
public:
    enum { MAX_LOOP_COUNT = 64 };

public:
    explicit EventLoopList(int loopCount);
    virtual ~EventLoopList();

    void start();
    void stop();

    int getCount() { return items_.getCount(); }
    EventLoop* findEventLoop(THREAD_ID loopThreadId);

    EventLoop* getItem(int index) { return items_[index]; }
    EventLoop* operator[] (int index) { return getItem(index); }

protected:
    virtual EventLoop* createEventLoop();
private:
    void setCount(int count);
protected:
    ObjectList<EventLoop> items_;
    int wantLoopCount_;
    Mutex mutex_;
};

///////////////////////////////////////////////////////////////////////////////
// class OsEventLoop - 平台相关的事件循环基类

class OsEventLoop : public EventLoop
{
public:
    OsEventLoop();
    virtual ~OsEventLoop();

protected:
    virtual void doLoopWork(Thread *thread);
    virtual void wakeupLoop();

protected:
#ifdef ISE_WINDOWS
    IocpObject *iocpObject_;
#endif
#ifdef ISE_LINUX
    EpollObject *epollObject_;
#endif
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_EVENT_LOOP_H_
