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
// 文件名称: ise_server_udp.cpp
// 功能描述: UDP服务器的实现
///////////////////////////////////////////////////////////////////////////////

#include "ise_server_udp.h"
#include "ise_errmsgs.h"
#include "ise_application.h"

using namespace ise;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class ThreadTimeOutChecker

ThreadTimeOutChecker::ThreadTimeOutChecker(Thread *thread) :
    thread_(thread),
    startTime_(0),
    started_(false),
    timeoutSecs_(0)
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 检测线程是否已超时，若超时则通知其退出
//-----------------------------------------------------------------------------
bool ThreadTimeOutChecker::check()
{
    bool result = false;

    if (started_ && timeoutSecs_ > 0)
    {
        if ((UINT)time(NULL) - startTime_ >= timeoutSecs_)
        {
            if (!thread_->isTerminated()) thread_->terminate();
            result = true;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 返回是否已开始计时
//-----------------------------------------------------------------------------
bool ThreadTimeOutChecker::getStarted()
{
    AutoLocker locker(lock_);

    return started_;
}

///////////////////////////////////////////////////////////////////////////////
// class UdpPacket

void UdpPacket::setPacketBuffer(void *pPakcetBuffer, int packetSize)
{
    if (packetBuffer_)
    {
        free(packetBuffer_);
        packetBuffer_ = NULL;
    }

    packetBuffer_ = malloc(packetSize);
    if (!packetBuffer_)
        iseThrowMemoryException();

    memcpy(packetBuffer_, pPakcetBuffer, packetSize);
}

//-----------------------------------------------------------------------------
// 描述: 开始计时
//-----------------------------------------------------------------------------
void ThreadTimeOutChecker::start()
{
    AutoLocker locker(lock_);

    startTime_ = time(NULL);
    started_ = true;
}

//-----------------------------------------------------------------------------
// 描述: 停止计时
//-----------------------------------------------------------------------------
void ThreadTimeOutChecker::stop()
{
    AutoLocker locker(lock_);

    started_ = false;
}

///////////////////////////////////////////////////////////////////////////////
// class UdpRequestQueue

//-----------------------------------------------------------------------------
// 描述: 构造函数
// 参数:
//   ownGroup - 指定所属组别
//-----------------------------------------------------------------------------
UdpRequestQueue::UdpRequestQueue(UdpRequestGroup *ownGroup)
{
    int groupIndex;

    ownGroup_ = ownGroup;
    groupIndex = ownGroup->getGroupIndex();
    capacity_ = iseApp().getIseOptions().getUdpRequestQueueCapacity(groupIndex);
    effWaitTime_ = iseApp().getIseOptions().getUdpRequestEffWaitTime();
    packetCount_ = 0;
}

//-----------------------------------------------------------------------------
// 描述: 向队列中添加数据包
//-----------------------------------------------------------------------------
void UdpRequestQueue::addPacket(UdpPacket *pPacket)
{
    if (capacity_ <= 0) return;
    bool removed = false;

    {
        AutoLocker locker(lock_);

        if (packetCount_ >= capacity_)
        {
            UdpPacket *p;
            p = packetList_.front();
            delete p;
            packetList_.pop_front();
            packetCount_--;
            removed = true;
        }

        packetList_.push_back(pPacket);
        packetCount_++;
    }

    if (!removed) semaphore_.increase();
}

//-----------------------------------------------------------------------------
// 描述: 从队列中取出数据包 (取出后应自行释放，若失败则返回 NULL)
// 备注: 若队列中尚没有数据包，则一直等待。
//-----------------------------------------------------------------------------
UdpPacket* UdpRequestQueue::extractPacket()
{
    semaphore_.wait();

    {
        AutoLocker locker(lock_);
        UdpPacket *p, *result = NULL;

        while (packetCount_ > 0)
        {
            p = packetList_.front();
            packetList_.pop_front();
            packetCount_--;

            if (time(NULL) - (UINT)p->recvTimeStamp_ <= (UINT)effWaitTime_)
            {
                result = p;
                break;
            }
            else
            {
                delete p;
            }
        }

        return result;
    }
}

//-----------------------------------------------------------------------------
// 描述: 清空队列
//-----------------------------------------------------------------------------
void UdpRequestQueue::clear()
{
    AutoLocker locker(lock_);
    UdpPacket *p;

    for (UINT i = 0; i < packetList_.size(); i++)
    {
        p = packetList_[i];
        delete p;
    }

    packetList_.clear();
    packetCount_ = 0;
    semaphore_.reset();
}

//-----------------------------------------------------------------------------
// 描述: 增加信号量的值，使等待数据的线程中断等待
//-----------------------------------------------------------------------------
void UdpRequestQueue::breakWaiting(int semCount)
{
    for (int i = 0; i < semCount; i++)
        semaphore_.increase();
}

///////////////////////////////////////////////////////////////////////////////
// class UdpWorkerThread

UdpWorkerThread::UdpWorkerThread(UdpWorkerThreadPool *threadPool) :
    ownPool_(threadPool),
    timeoutChecker_(this)
{
    setFreeOnTerminate(true);
    // 启用超时检测
    timeoutChecker_.setTimeOutSecs(iseApp().getIseOptions().getUdpWorkerThreadTimeOut());

    ownPool_->registerThread(this);
}

UdpWorkerThread::~UdpWorkerThread()
{
    ownPool_->unregisterThread(this);
}

//-----------------------------------------------------------------------------
// 描述: 线程的执行函数
//-----------------------------------------------------------------------------
void UdpWorkerThread::execute()
{
    int groupIndex;
    UdpRequestQueue *requestQueue;
    UdpPacket *packet;

    groupIndex = ownPool_->getRequestGroup().getGroupIndex();
    requestQueue = &(ownPool_->getRequestGroup().getRequestQueue());

    while (!isTerminated())
    try
    {
        packet = requestQueue->extractPacket();
        if (packet)
        {
            std::auto_ptr<UdpPacket> AutoPtr(packet);
            AutoInvoker autoInvoker(timeoutChecker_);

            // 分派数据包
            if (!isTerminated())
                iseApp().getIseBusiness().dispatchUdpPacket(*this, groupIndex, *packet);
        }
    }
    catch (Exception&)
    {}
}

//-----------------------------------------------------------------------------
// 描述: 执行 terminate() 前的附加操作
//-----------------------------------------------------------------------------
void UdpWorkerThread::doTerminate()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 执行 kill() 前的附加操作
//-----------------------------------------------------------------------------
void UdpWorkerThread::doKill()
{
    // nothing
}

///////////////////////////////////////////////////////////////////////////////
// class UdpWorkerThreadPool

UdpWorkerThreadPool::UdpWorkerThreadPool(UdpRequestGroup *ownGroup) :
    ownGroup_(ownGroup)
{
    // nothing
}

UdpWorkerThreadPool::~UdpWorkerThreadPool()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 注册线程
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::registerThread(UdpWorkerThread *thread)
{
    threadList_.add(thread);
}

//-----------------------------------------------------------------------------
// 描述: 注销线程
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::unregisterThread(UdpWorkerThread *thread)
{
    threadList_.remove(thread);
}

//-----------------------------------------------------------------------------
// 描述: 根据负载情况动态调整线程数量
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::AdjustThreadCount()
{
    int packetCount, threadCount;
    int minThreads, maxThreads, deltaThreads;
    int packetAlertLine;

    // 取得线程数量上下限
    iseApp().getIseOptions().getUdpWorkerThreadCount(
        ownGroup_->getGroupIndex(), minThreads, maxThreads);
    // 取得请求队列中数据包数量警戒线
    packetAlertLine = iseApp().getIseOptions().getUdpRequestQueueAlertLine();

    // 检测线程是否工作超时
    checkThreadTimeout();
    // 强行杀死僵死的线程
    killZombieThreads();

    // 取得数据包数量和线程数量
    packetCount = ownGroup_->getRequestQueue().getCount();
    threadCount = threadList_.getCount();

    // 保证线程数量在上下限范围之内
    if (threadCount < minThreads)
    {
        createThreads(minThreads - threadCount);
        threadCount = minThreads;
    }
    if (threadCount > maxThreads)
    {
        terminateThreads(threadCount - maxThreads);
        threadCount = maxThreads;
    }

    // 如果请求队列中的数量超过警戒线，则尝试增加线程数量
    if (threadCount < maxThreads && packetCount >= packetAlertLine)
    {
        deltaThreads = ise::min(maxThreads - threadCount, 3);
        createThreads(deltaThreads);
    }

    // 如果请求队列为空，则尝试减少线程数量
    if (threadCount > minThreads && packetCount == 0)
    {
        deltaThreads = 1;
        terminateThreads(deltaThreads);
    }
}

//-----------------------------------------------------------------------------
// 描述: 通知所有线程退出
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::terminateAllThreads()
{
    threadList_.terminateAllThreads();

    AutoLocker locker(threadList_.getLock());

    // 使线程从等待中解脱，尽快退出
    getRequestGroup().getRequestQueue().breakWaiting(threadList_.getCount());
}

//-----------------------------------------------------------------------------
// 描述: 等待所有线程退出
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::waitForAllThreads()
{
    terminateAllThreads();

    int killedCount = 0;
    threadList_.waitForAllThreads(MAX_THREAD_WAIT_FOR_SECS, &killedCount);

    if (killedCount)
        logger().writeFmt(SEM_THREAD_KILLED, killedCount, "udp worker");
}

//-----------------------------------------------------------------------------
// 描述: 创建 count 个线程
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::createThreads(int count)
{
    for (int i = 0; i < count; i++)
    {
        UdpWorkerThread *thread;
        thread = new UdpWorkerThread(this);
        thread->run();
    }
}

//-----------------------------------------------------------------------------
// 描述: 终止 count 个线程
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::terminateThreads(int count)
{
    AutoLocker locker(threadList_.getLock());

    int termCount = 0;
    if (count > threadList_.getCount())
        count = threadList_.getCount();

    for (int i = threadList_.getCount() - 1; i >= 0; i--)
    {
        UdpWorkerThread *thread;
        thread = (UdpWorkerThread*)threadList_[i];
        if (thread->isIdle())
        {
            thread->terminate();
            termCount++;
            if (termCount >= count) break;
        }
    }
}

//-----------------------------------------------------------------------------
// 描述: 检测线程是否工作超时 (超时线程: 因某一请求进入工作状态但长久未完成的线程)
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::checkThreadTimeout()
{
    AutoLocker locker(threadList_.getLock());

    for (int i = 0; i < threadList_.getCount(); i++)
    {
        UdpWorkerThread *thread;
        thread = (UdpWorkerThread*)threadList_[i];
        thread->getTimeoutChecker().check();
    }
}

//-----------------------------------------------------------------------------
// 描述: 强行杀死僵死的线程 (僵死线程: 已被通知退出但长久不退出的线程)
//-----------------------------------------------------------------------------
void UdpWorkerThreadPool::killZombieThreads()
{
    AutoLocker locker(threadList_.getLock());

    for (int i = threadList_.getCount() - 1; i >= 0; i--)
    {
        UdpWorkerThread *thread;
        thread = (UdpWorkerThread*)threadList_[i];
        if (thread->getTermElapsedSecs() >= MAX_THREAD_TERM_SECS)
        {
            thread->kill();
            threadList_.remove(thread);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// class UdpRequestGroup

UdpRequestGroup::UdpRequestGroup(MainUdpServer *ownMainUdpSvr, int groupIndex) :
    ownMainUdpSvr_(ownMainUdpSvr),
    groupIndex_(groupIndex),
    requestQueue_(this),
    threadPool_(this)
{
    // nothing
}

///////////////////////////////////////////////////////////////////////////////
// class MainUdpServer

MainUdpServer::MainUdpServer()
{
    initUdpServer();
    initRequestGroupList();
}

MainUdpServer::~MainUdpServer()
{
    clearRequestGroupList();
}

//-----------------------------------------------------------------------------
// 描述: 开启服务器
//-----------------------------------------------------------------------------
void MainUdpServer::open()
{
    udpServer_.open();
}

//-----------------------------------------------------------------------------
// 描述: 关闭服务器
//-----------------------------------------------------------------------------
void MainUdpServer::close()
{
    terminateAllWorkerThreads();
    waitForAllWorkerThreads();

    udpServer_.close();
}

//-----------------------------------------------------------------------------
// 描述: 根据负载情况动态调整工作者线程数量
//-----------------------------------------------------------------------------
void MainUdpServer::adjustWorkerThreadCount()
{
    for (UINT i = 0; i < requestGroupList_.size(); i++)
        requestGroupList_[i]->getThreadPool().AdjustThreadCount();
}

//-----------------------------------------------------------------------------
// 描述: 通知所有工作者线程退出
//-----------------------------------------------------------------------------
void MainUdpServer::terminateAllWorkerThreads()
{
    for (UINT i = 0; i < requestGroupList_.size(); i++)
        requestGroupList_[i]->getThreadPool().terminateAllThreads();
}

//-----------------------------------------------------------------------------
// 描述: 等待所有工作者线程退出
//-----------------------------------------------------------------------------
void MainUdpServer::waitForAllWorkerThreads()
{
    for (UINT i = 0; i < requestGroupList_.size(); i++)
        requestGroupList_[i]->getThreadPool().waitForAllThreads();
}

//-----------------------------------------------------------------------------
// 描述: 初始化 udpServer_
//-----------------------------------------------------------------------------
void MainUdpServer::initUdpServer()
{
    udpServer_.setRecvDataCallback(boost::bind(&MainUdpServer::onRecvData, this, _1, _2, _3));
}

//-----------------------------------------------------------------------------
// 描述: 初始化请求组别列表
//-----------------------------------------------------------------------------
void MainUdpServer::initRequestGroupList()
{
    clearRequestGroupList();

    requestGroupCount_ = iseApp().getIseOptions().getUdpRequestGroupCount();
    for (int groupIndex = 0; groupIndex < requestGroupCount_; groupIndex++)
    {
        UdpRequestGroup *p;
        p = new UdpRequestGroup(this, groupIndex);
        requestGroupList_.push_back(p);
    }
}

//-----------------------------------------------------------------------------
// 描述: 清空请求组别列表
//-----------------------------------------------------------------------------
void MainUdpServer::clearRequestGroupList()
{
    for (UINT i = 0; i < requestGroupList_.size(); i++)
        delete requestGroupList_[i];
    requestGroupList_.clear();
}

//-----------------------------------------------------------------------------
// 描述: 收到数据包
//-----------------------------------------------------------------------------
void MainUdpServer::onRecvData(void *packetBuffer, int packetSize, const InetAddress& peerAddr)
{
    int groupIndex;

    // 先进行数据包分类，得到组别号
    iseApp().getIseBusiness().classifyUdpPacket(packetBuffer, packetSize, groupIndex);

    // 如果组别号合法
    if (groupIndex >= 0 && groupIndex < requestGroupCount_)
    {
        UdpPacket *p = new UdpPacket();
        if (p)
        {
            p->recvTimeStamp_ = (UINT)time(NULL);
            p->peerAddr_ = peerAddr;
            p->packetSize_ = packetSize;
            p->setPacketBuffer(packetBuffer, packetSize);

            // 添加到请求队列中
            requestGroupList_[groupIndex]->getRequestQueue().addPacket(p);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
