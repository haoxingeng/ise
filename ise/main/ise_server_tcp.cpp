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
// 文件名称: ise_server_tcp.cpp
// 功能描述: TCP服务器的实现
///////////////////////////////////////////////////////////////////////////////

#include "ise/main/ise_server_tcp.h"
#include "ise/main/ise_err_msgs.h"
#include "ise/main/ise_application.h"

using namespace ise;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 预定义分包器

void bytePacketSplitter(const char *data, int bytes, int& retrieveBytes)
{
    retrieveBytes = (bytes > 0 ? 1 : 0);
}

void linePacketSplitter(const char *data, int bytes, int& retrieveBytes)
{
    retrieveBytes = 0;

    const char *p = data;
    int i = 0;
    while (i < bytes)
    {
        if (*p == '\r' || *p == '\n')
        {
            retrieveBytes = i + 1;
            if (i < bytes - 1)
            {
                char next = *(p+1);
                if ((next == '\r' || next == '\n') && next != *p)
                    ++retrieveBytes;
            }
            break;
        }

        ++p;
        ++i;
    }
}

//-----------------------------------------------------------------------------

void nullTerminatedPacketSplitter(const char *data, int bytes, int& retrieveBytes)
{
    const char DELIMITER = '\0';

    retrieveBytes = 0;
    for (int i = 0; i < bytes; ++i)
    {
        if (data[i] == DELIMITER)
        {
            retrieveBytes = i + 1;
            break;
        }
    }
}

//-----------------------------------------------------------------------------

void anyPacketSplitter(const char *data, int bytes, int& retrieveBytes)
{
    retrieveBytes = (bytes > 0 ? bytes : 0);
}

///////////////////////////////////////////////////////////////////////////////
// class IoBuffer

IoBuffer::IoBuffer() :
    buffer_(INITIAL_SIZE),
    readerIndex_(0),
    writerIndex_(0)
{
    // nothing
}

IoBuffer::~IoBuffer()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 向缓存追加写入数据
//-----------------------------------------------------------------------------
void IoBuffer::append(const string& str)
{
    append(str.c_str(), (int)str.length());
}

//-----------------------------------------------------------------------------
// 描述: 向缓存追加写入数据
//-----------------------------------------------------------------------------
void IoBuffer::append(const void *data, int bytes)
{
    if (data && bytes > 0)
    {
        if (getWritableBytes() < bytes)
            makeSpace(bytes);

        ISE_ASSERT(getWritableBytes() >= bytes);

        memmove(getWriterPtr(), data, bytes);
        writerIndex_ += bytes;
    }
}

//-----------------------------------------------------------------------------
// 描述: 向缓存追加 bytes 个字节并填充为'\0'
//-----------------------------------------------------------------------------
void IoBuffer::append(int bytes)
{
    if (bytes > 0)
    {
        string str;
        str.resize(bytes, 0);
        append(str);
    }
}

//-----------------------------------------------------------------------------
// 描述: 从缓存读取 bytes 个字节数据
//-----------------------------------------------------------------------------
void IoBuffer::retrieve(int bytes)
{
    if (bytes > 0)
    {
        ISE_ASSERT(bytes <= getReadableBytes());
        readerIndex_ += bytes;
    }
}

//-----------------------------------------------------------------------------
// 描述: 从缓存读取全部可读数据并存入 str 中
//-----------------------------------------------------------------------------
void IoBuffer::retrieveAll(string& str)
{
    if (getReadableBytes() > 0)
        str.assign(peek(), getReadableBytes());
    else
        str.clear();

    retrieveAll();
}

//-----------------------------------------------------------------------------
// 描述: 从缓存读取全部可读数据
//-----------------------------------------------------------------------------
void IoBuffer::retrieveAll()
{
    readerIndex_ = 0;
    writerIndex_ = 0;
}

//-----------------------------------------------------------------------------

void IoBuffer::swap(IoBuffer& rhs)
{
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
}

//-----------------------------------------------------------------------------
// 描述: 扩展缓存空间以便可再写进 moreBytes 个字节
//-----------------------------------------------------------------------------
void IoBuffer::makeSpace(int moreBytes)
{
    if (getWritableBytes() + getUselessBytes() < moreBytes)
    {
        buffer_.resize(writerIndex_ + moreBytes);
    }
    else
    {
        // 将全部可读数据移至缓存开始处
        int readableBytes = getReadableBytes();
        char *buffer = getBufferPtr();
        memmove(buffer, buffer + readerIndex_, readableBytes);
        readerIndex_ = 0;
        writerIndex_ = readerIndex_ + readableBytes;

        ISE_ASSERT(readableBytes == getReadableBytes());
    }
}

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoop

TcpEventLoop::TcpEventLoop()
{
    // nothing
}

TcpEventLoop::~TcpEventLoop()
{
    tcpConnMap_.clear();
}

//-----------------------------------------------------------------------------
// 描述: 将指定连接注册到此 eventLoop 中
//-----------------------------------------------------------------------------
void TcpEventLoop::addConnection(TcpConnection *connection)
{
    TcpInspectInfo::instance().addConnCount.increment();

    TcpConnectionPtr connPtr(connection);
    tcpConnMap_[connection->getConnectionName()] = connPtr;

    registerConnection(connection);
    delegateToLoop(boost::bind(&IseBusiness::onTcpConnected, &iseApp().iseBusiness(), connPtr));
}

//-----------------------------------------------------------------------------
// 描述: 将指定连接从此 eventLoop 中注销
//-----------------------------------------------------------------------------
void TcpEventLoop::removeConnection(TcpConnection *connection)
{
    TcpInspectInfo::instance().removeConnCount.increment();

    unregisterConnection(connection);

    // 此处 shared_ptr 计数递减，有可能会销毁 TcpConnection 对象
    tcpConnMap_.erase(connection->getConnectionName());
}

//-----------------------------------------------------------------------------
// 描述: 清除全部连接
// 备注:
//   conn->shutdown(true, true) 使 IOCP 返回错误，从而释放 IOCP 持有的 shared_ptr。
//   之后 TcpConnection::errorOccurred() 被调用，进而调用 onTcpDisconnected() 和
//   TcpConnection::setEventLoop(NULL)。
//-----------------------------------------------------------------------------
void TcpEventLoop::clearConnections()
{
    assertInLoopThread();

    for (TcpConnectionMap::iterator iter = tcpConnMap_.begin(); iter != tcpConnMap_.end(); ++iter)
    {
        TcpConnectionPtr conn = tcpConnMap_.begin()->second;
        conn->shutdown(true, true);
    }
}

//-----------------------------------------------------------------------------
// 描述: 执行事件循环
// 注意:
//   * 只有全部连接被销毁后才可以退出循环，不然未销毁连接将对程序退出过程造成
//     麻烦。在 Linux 下典型的错误信息是:
//     'pure virtual method called terminate called without an active exception'.
//-----------------------------------------------------------------------------
void TcpEventLoop::runLoop(Thread *thread)
{
    bool isTerminated = false;
    while (!isTerminated || !tcpConnMap_.empty())
    {
        try
        {
	        if (thread->isTerminated())
	        {
                clearConnections();
                isTerminated = true;
                wakeupLoop();
	        }

            doLoopWork(thread);
            checkTimeout();
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
// 描述: 检查每个TCP连接的超时
//-----------------------------------------------------------------------------
void TcpEventLoop::checkTimeout()
{
    const UINT CHECK_INTERVAL = 1000;  // ms

    UINT64 curTicks = getCurTicks();
    if (getTickDiff(lastCheckTimeoutTicks_, curTicks) >= (UINT64)CHECK_INTERVAL)
    {
        lastCheckTimeoutTicks_ = curTicks;

        for (TcpConnectionMap::iterator iter = tcpConnMap_.begin(); iter != tcpConnMap_.end(); ++iter)
        {
            TcpConnectionPtr& connPtr = iter->second;
            connPtr->checkTimeout(curTicks);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopList

TcpEventLoopList::TcpEventLoopList(int loopCount) :
    EventLoopList(loopCount)
{
    // nothing
}

TcpEventLoopList::~TcpEventLoopList()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 将 connection 挂接到 EventLoop 上
// 参数:
//   index - EventLoop 的序号 (0-based)，为 -1 表示自动选择。
//-----------------------------------------------------------------------------
bool TcpEventLoopList::registerToEventLoop(BaseTcpConnection *connection, int eventLoopIndex)
{
    TcpEventLoop *eventLoop = NULL;

    if (eventLoopIndex >= 0 && eventLoopIndex < getCount())
    {
        AutoLocker locker(mutex_);
        eventLoop = getItem(eventLoopIndex);
    }
    else
    {
        static int s_index = 0;
        AutoLocker locker(mutex_);
        // round-robin
        eventLoop = getItem(s_index);
        s_index = (s_index >= getCount() - 1 ? 0 : s_index + 1);
    }

    bool result = (eventLoop != NULL);
    if (result)
    {
        // 将 ((TcpConnection*)connection)->setEventLoop(eventLoop) 委托给事件循环线程
        eventLoop->delegateToLoop(boost::bind(
            &TcpConnection::setEventLoop,
            static_cast<TcpConnection*>(connection),
            eventLoop));
    }

    return result;
}

//-----------------------------------------------------------------------------

EventLoop* TcpEventLoopList::createEventLoop()
{
#ifdef ISE_WINDOWS
    return new WinTcpEventLoop();
#endif
#ifdef ISE_LINUX
    return new LinuxTcpEventLoop();
#endif
}

///////////////////////////////////////////////////////////////////////////////
// class TcpConnection

TcpConnection::TcpConnection()
{
    init();
    TcpInspectInfo::instance().tcpConnCreateCount.increment();
}

TcpConnection::TcpConnection(TcpServer *tcpServer, SOCKET socketHandle) :
    BaseTcpConnection(socketHandle)
{
    init();

    tcpServer_ = tcpServer;
    tcpServer_->incConnCount();
    TcpInspectInfo::instance().tcpConnCreateCount.increment();
}

TcpConnection::~TcpConnection()
{
    //logger().writeFmt("destroy conn: %s", getConnectionName().c_str());  // debug

    setEventLoop(NULL);

    if (tcpServer_)
        tcpServer_->decConnCount();
    TcpInspectInfo::instance().tcpConnDestroyCount.increment();
}

//-----------------------------------------------------------------------------

void TcpConnection::init()
{
    tcpServer_ = NULL;
    eventLoop_ = NULL;
    isErrorOccurred_ = false;
}

//-----------------------------------------------------------------------------
// 描述: 提交一个发送任务 (线程安全)
// 参数:
//   timeout - 超时值 (毫秒)
//-----------------------------------------------------------------------------
void TcpConnection::send(const void *buffer, size_t size, const Context& context, int timeout)
{
    if (!buffer || size <= 0) return;

    if (eventLoop_ == NULL)
        iseThrowException(SEM_EVENT_LOOP_NOT_SPECIFIED);

    if (getEventLoop()->isInLoopThread())
        postSendTask(buffer, static_cast<int>(size), context, timeout);
    else
    {
        string data((const char*)buffer, size);
        getEventLoop()->delegateToLoop(
            boost::bind(&TcpConnection::postSendTask, this, data, context, timeout));
    }
}

//-----------------------------------------------------------------------------
// 描述: 提交一个接收任务 (线程安全)
// 参数:
//   timeout - 超时值 (毫秒)
//-----------------------------------------------------------------------------
void TcpConnection::recv(const PacketSplitter& packetSplitter, const Context& context, int timeout)
{
    if (!packetSplitter) return;

    if (eventLoop_ == NULL)
        iseThrowException(SEM_EVENT_LOOP_NOT_SPECIFIED);

    if (getEventLoop()->isInLoopThread())
        postRecvTask(packetSplitter, context, timeout);
    else
    {
        getEventLoop()->delegateToLoop(
            boost::bind(&TcpConnection::postRecvTask, this, packetSplitter, context, timeout));
    }
}

//-----------------------------------------------------------------------------

const string& TcpConnection::getConnectionName() const
{
    if (connectionName_.empty() && isConnected())
    {
        static Mutex mutex;
        static SeqNumberAlloc connIdAlloc_;

        AutoLocker locker(mutex);

        connectionName_ = formatString("%s-%s#%s",
            getSocket().getLocalAddr().getDisplayStr().c_str(),
            getSocket().getPeerAddr().getDisplayStr().c_str(),
            intToStr((INT64)connIdAlloc_.allocId()).c_str());
    }

    return connectionName_;
}

//-----------------------------------------------------------------------------

int TcpConnection::getServerIndex() const
{
    return tcpServer_ ? boost::any_cast<int>(tcpServer_->getContext()) : -1;
}

//-----------------------------------------------------------------------------

int TcpConnection::getServerPort() const
{
    return tcpServer_ ? tcpServer_->getLocalPort() : 0;
}

//-----------------------------------------------------------------------------

int TcpConnection::getServerConnCount() const
{
    return tcpServer_ ? tcpServer_->getConnectionCount() : 0;
}

//-----------------------------------------------------------------------------
// 描述: 断开连接
// 备注:
//   如果直接 close socket，Linux下 EPoll 将不会产生通知，从而无法捕获错误，
//   而 shutdown 则没有问题。在 Windows 下，无论是 close 还是 shutdown，
//   只要连接上存在接收或发送动作，IOCP 都可以捕获错误。
//-----------------------------------------------------------------------------
void TcpConnection::doDisconnect()
{
    shutdown(true, false);
}

//-----------------------------------------------------------------------------
// 描述: 连接发生了错误
// 备注:
//   连接在发生错误后已没有利用价值，应当销毁，但不应立马销毁，因为稍后执行的
//   委托给事件循环的仿函数中可能存在对此连接的回调。所以，应该以 addFinalizer()
//   的方式，再次将销毁对象的任务委托给事件循环，在每次循环的末尾执行。
//-----------------------------------------------------------------------------
void TcpConnection::errorOccurred()
{
    if (isErrorOccurred_) return;
    isErrorOccurred_ = true;

    TcpInspectInfo::instance().errorOccurredCount.increment();

    shutdown(true, true);

    ISE_ASSERT(eventLoop_ != NULL);

    getEventLoop()->executeInLoop(boost::bind(
        &IseBusiness::onTcpDisconnected,
        &iseApp().iseBusiness(), shared_from_this()));

    // setEventLoop(NULL) 可使 shared_ptr<TcpConnection> 减少引用计数，进而销毁对象
    getEventLoop()->addFinalizer(boost::bind(
        &TcpConnection::setEventLoop,
        shared_from_this(),   // 保证在调用 setEventLoop(NULL) 期间不会销毁 TcpConnection 对象
        (TcpEventLoop*)NULL));
}

//-----------------------------------------------------------------------------
// 描述: 检查接收和发送任务是否超时
//-----------------------------------------------------------------------------
void TcpConnection::checkTimeout(UINT curTicks)
{
    if (!sendTaskQueue_.empty())
    {
        SendTask& task = sendTaskQueue_.front();

        if (task.startTicks == 0)
            task.startTicks = curTicks;
        else
        {
            if (task.timeout > 0 &&
                (int)getTickDiff(task.startTicks, curTicks) > task.timeout)
            {
                shutdown(true, true);
            }
        }
    }

    if (!recvTaskQueue_.empty())
    {
        RecvTask& task = recvTaskQueue_.front();

        if (task.startTicks == 0)
            task.startTicks = curTicks;
        else
        {
            if (task.timeout > 0 &&
                (int)getTickDiff(task.startTicks, curTicks) > task.timeout)
            {
                shutdown(true, true);
            }
        }
    }
}

//-----------------------------------------------------------------------------

void TcpConnection::postSendTask(const string& data, const Context& context, int timeout)
{
    postSendTask(data.c_str(), (int)data.size(), context, timeout);
}

//-----------------------------------------------------------------------------
// 描述: 设置此连接从属于哪个 eventLoop
//-----------------------------------------------------------------------------
void TcpConnection::setEventLoop(TcpEventLoop *eventLoop)
{
    if (eventLoop != eventLoop_)
    {
        if (eventLoop_)
        {
            TcpEventLoop *temp = eventLoop_;
            eventLoop_ = NULL;
            temp->removeConnection(this);
            eventLoopChanged();
        }

        if (eventLoop)
        {
            eventLoop->assertInLoopThread();
            eventLoop_ = eventLoop;
            eventLoop->addConnection(this);
            eventLoopChanged();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// class TcpClient

BaseTcpConnection* TcpClient::createConnection()
{
    BaseTcpConnection *result = NULL;

#ifdef ISE_WINDOWS
    result = new WinTcpConnection();
#endif
#ifdef ISE_LINUX
    result = new LinuxTcpConnection();
#endif

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 将 connection 挂接到 EventLoop 上
// 参数:
//   index - EventLoop 的序号 (0-based)，为 -1 表示自动选择
// 备注:
//   挂接成功后，TcpClient 将释放对 TcpConnection 的控制权。
//-----------------------------------------------------------------------------
bool TcpClient::registerToEventLoop(int index)
{
    bool result = false;

    if (connection_ != NULL)
    {
        TcpEventLoopList& eventLoopList = iseApp().mainServer().getMainTcpServer().getTcpClientEventLoopList();
        result = eventLoopList.registerToEventLoop(connection_, index);
        if (result)
            connection_ = NULL;
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class TcpServer

TcpServer::TcpServer(int eventLoopCount) :
    eventLoopList_(eventLoopCount)
{
    // nothing
}

//-----------------------------------------------------------------------------

void TcpServer::open()
{
    BaseTcpServer::open();
    eventLoopList_.start();
}

//-----------------------------------------------------------------------------

void TcpServer::close()
{
    eventLoopList_.stop();
    BaseTcpServer::close();
}

//-----------------------------------------------------------------------------
// 描述: 创建连接对象
//-----------------------------------------------------------------------------
BaseTcpConnection* TcpServer::createConnection(SOCKET socketHandle)
{
    BaseTcpConnection *result = NULL;

#ifdef ISE_WINDOWS
    result = new WinTcpConnection(this, socketHandle);
#endif
#ifdef ISE_LINUX
    result = new LinuxTcpConnection(this, socketHandle);
#endif

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 收到连接
// 注意:
//   1. 此回调在TCP服务器监听线程(TcpListenerThread)中执行。
//   2. 对 connection->setEventLoop() 的调用在事件循环线程中执行。
//   3. 为避免程序退出时能尽快清除剩余连接，此时不允许有新连接连入。
//-----------------------------------------------------------------------------
void TcpServer::acceptConnection(BaseTcpConnection *connection)
{
    if (!iseApp().isTerminated())
        eventLoopList_.registerToEventLoop(connection, -1);
    else
        delete connection;
}

///////////////////////////////////////////////////////////////////////////////
// class TcpConnector

TcpConnector::TcpConnector() :
    taskList_(false, true),
    thread_(NULL)
{
    // nothing
}

TcpConnector::~TcpConnector()
{
    stop();
    clear();
}

//-----------------------------------------------------------------------------

void TcpConnector::connect(const InetAddress& peerAddr,
    const CompleteCallback& completeCallback, const Context& context)
{
    AutoLocker locker(mutex_);

    TaskItem *item = new TaskItem();
    item->peerAddr = peerAddr;
    item->completeCallback = completeCallback;
    item->state = ACS_NONE;
    item->context = context;

    taskList_.add(item);
    start();
}

//-----------------------------------------------------------------------------

void TcpConnector::clear()
{
    AutoLocker locker(mutex_);
    taskList_.clear();
}

//-----------------------------------------------------------------------------

void TcpConnector::start()
{
    if (thread_ == NULL)
    {
        thread_ = new WorkerThread(*this);
        thread_->setAutoDelete(true);
        thread_->run();
    }
}

//-----------------------------------------------------------------------------

void TcpConnector::stop()
{
    if (thread_ != NULL)
    {
        thread_->terminate();
        thread_->waitFor();
        thread_ = NULL;
    }
}

//-----------------------------------------------------------------------------

void TcpConnector::work(WorkerThread& thread)
{
    while (!thread.isTerminated() && !taskList_.isEmpty())
    {
        tryConnect();

        int fromIndex = 0;
        FdList fds, connectedFds, failedFds;

        getPendingFdsFromTaskList(fromIndex, fds);
        checkAsyncConnectState(fds, connectedFds, failedFds);

        if (fromIndex >= taskList_.getCount())
            fromIndex = 0;

        for (int i = 0; i < (int)connectedFds.size(); ++i)
        {
            TaskItem *task = findTask(connectedFds[i]);
            if (task != NULL)
            {
                task->state = ACS_CONNECTED;
                task->tcpClient.getConnection().setContext(task->context);
            }
        }

        for (int i = 0; i < (int)failedFds.size(); ++i)
        {
            TaskItem *task = findTask(failedFds[i]);
            if (task != NULL)
            {
                task->state = ACS_FAILED;
                task->tcpClient.disconnect();
            }
        }

        invokeCompleteCallback();
    }
}

//-----------------------------------------------------------------------------

void TcpConnector::tryConnect()
{
    AutoLocker locker(mutex_);

    for (int i = 0; i < taskList_.getCount(); ++i)
    {
        TaskItem *task = taskList_[i];
        if (task->state == ACS_NONE)
        {
            task->state = (ASYNC_CONNECT_STATE)task->tcpClient.asyncConnect(
                ipToString(task->peerAddr.ip), task->peerAddr.port, 0);
        }
    }
}

//-----------------------------------------------------------------------------

void TcpConnector::getPendingFdsFromTaskList(int& fromIndex, FdList& fds)
{
    AutoLocker locker(mutex_);

    fds.clear();
    for (int i = fromIndex; i < taskList_.getCount(); ++i)
    {
        TaskItem *task = taskList_[i];
        if (task->state == ACS_CONNECTING)
        {
            fds.push_back(task->tcpClient.getConnection().getSocket().getHandle());
            if (fds.size() >= FD_SETSIZE)
            {
                fromIndex = i + 1;
                break;
            }
        }
    }
}

//-----------------------------------------------------------------------------

void TcpConnector::checkAsyncConnectState(const FdList& fds,
    FdList& connectedFds, FdList& failedFds)
{
    const int WAIT_TIME = 1;  // ms

    fd_set rset, wset;
    struct timeval tv;

    connectedFds.clear();
    failedFds.clear();

    // init wset & rset
    FD_ZERO(&rset);
    for (int i = 0; i < (int)fds.size(); ++i)
        FD_SET(fds[i], &rset);
    wset = rset;

    // find max fd
    SOCKET maxFd = 0;
    for (int i = 0; i < (int)fds.size(); ++i)
    {
        if (maxFd < fds[i])
            maxFd = fds[i];
    }

    tv.tv_sec = 0;
    tv.tv_usec = WAIT_TIME * 1000;

    int r = select(maxFd + 1, &rset, &wset, NULL, &tv);
    if (r > 0)
    {
        for (int i = 0; i < (int)fds.size(); ++i)
        {
            int state = ACS_CONNECTING;
            SOCKET fd = fds[i];
            if (FD_ISSET(fd, &rset) || FD_ISSET(fd, &wset))
            {
                socklen_t errLen = sizeof(int);
                int errorCode = 0;
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&errorCode, &errLen) < 0 || errorCode)
                    state = ACS_FAILED;
                else
                    state = ACS_CONNECTED;
            }

            if (state == ACS_CONNECTED)
                connectedFds.push_back(fd);
            else if (state == ACS_FAILED)
                failedFds.push_back(fd);
        }
    }
}

//-----------------------------------------------------------------------------

TcpConnector::TaskItem* TcpConnector::findTask(SOCKET fd)
{
    TaskItem *result = NULL;

    for (int i = 0; i < taskList_.getCount(); ++i)
    {
        TaskItem *task = taskList_[i];
        if (task->tcpClient.getConnection().getSocket().getHandle() == fd)
        {
            result = task;
            break;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------

void TcpConnector::invokeCompleteCallback()
{
    TaskList completeList(false, true);

    {
        AutoLocker locker(mutex_);

        for (int i = 0; i < taskList_.getCount(); ++i)
        {
            TaskItem *task = taskList_[i];
            if (task->state == ACS_CONNECTED || task->state == ACS_FAILED)
            {
                taskList_.extract(i);
                completeList.add(task);
            }
        }
    }

    for (int i = 0; i < completeList.getCount(); ++i)
    {
        TaskItem *task = completeList[i];
        if (task->completeCallback)
        {
            bool success = (task->state == ACS_CONNECTED);

            task->completeCallback(success,
                success ? &task->tcpClient.getConnection() : NULL,
                task->peerAddr, task->context);

            if (success)
                task->tcpClient.registerToEventLoop();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_WINDOWS

///////////////////////////////////////////////////////////////////////////////
// class WinTcpConnection

WinTcpConnection::WinTcpConnection()
{
    init();
}

WinTcpConnection::WinTcpConnection(TcpServer *tcpServer, SOCKET socketHandle) :
    TcpConnection(tcpServer, socketHandle)
{
    init();
}

//-----------------------------------------------------------------------------

void WinTcpConnection::init()
{
    isSending_ = false;
    isRecving_ = false;
    bytesSent_ = 0;
    bytesRecved_ = 0;
}

//-----------------------------------------------------------------------------

void WinTcpConnection::eventLoopChanged()
{
    if (getEventLoop() != NULL)
    {
        getEventLoop()->assertInLoopThread();
        tryRecv();
    }
}

//-----------------------------------------------------------------------------
// 描述: 提交一个发送任务
//-----------------------------------------------------------------------------
void WinTcpConnection::postSendTask(const void *buffer, int size,
    const Context& context, int timeout)
{
    sendBuffer_.append(buffer, size);

    SendTask task;
    task.bytes = size;
    task.context = context;
    task.timeout = timeout;

    sendTaskQueue_.push_back(task);

    trySend();
}

//-----------------------------------------------------------------------------
// 描述: 提交一个接收任务
//-----------------------------------------------------------------------------
void WinTcpConnection::postRecvTask(const PacketSplitter& packetSplitter,
    const Context& context, int timeout)
{
    RecvTask task;
    task.packetSplitter = packetSplitter;
    task.context = context;
    task.timeout = timeout;

    recvTaskQueue_.push_back(task);

    tryRecv();
}

//-----------------------------------------------------------------------------

void WinTcpConnection::trySend()
{
    if (isSending_) return;

    int readableBytes = sendBuffer_.getReadableBytes();
    if (readableBytes > 0)
    {
        const int MAX_SEND_SIZE = 1024*32;

        const char *buffer = sendBuffer_.peek();
        int sendSize = ise::min(readableBytes, MAX_SEND_SIZE);

        isSending_ = true;
        getEventLoop()->getIocpObject()->send(
            getSocket().getHandle(),
            (PVOID)buffer, sendSize, 0,
            boost::bind(&WinTcpConnection::onIocpCallback, shared_from_this(), _1),
            this, EMPTY_CONTEXT);
    }
}

//-----------------------------------------------------------------------------

void WinTcpConnection::tryRecv()
{
    if (isRecving_) return;

    const int MAX_BUFFER_SIZE = iseApp().iseOptions().getTcpMaxRecvBufferSize();
    const int MAX_RECV_SIZE = 1024*16;

    if (recvTaskQueue_.empty() && recvBuffer_.getReadableBytes() >= MAX_BUFFER_SIZE)
        return;

    isRecving_ = true;
    recvBuffer_.append(MAX_RECV_SIZE);
    const char *buffer = recvBuffer_.peek() + bytesRecved_;

    getEventLoop()->getIocpObject()->recv(
        getSocket().getHandle(),
        (PVOID)buffer, MAX_RECV_SIZE, 0,
        boost::bind(&WinTcpConnection::onIocpCallback, shared_from_this(), _1),
        this, EMPTY_CONTEXT);
}

//-----------------------------------------------------------------------------

void WinTcpConnection::onIocpCallback(const TcpConnectionPtr& thisObj, const IocpTaskData& taskData)
{
    WinTcpConnection *thisPtr = static_cast<WinTcpConnection*>(thisObj.get());
    if (thisPtr->isErrorOccurred_) return;

    if (taskData.getErrorCode() == 0)
    {
        switch (taskData.getTaskType())
        {
        case ITT_SEND:
            thisPtr->onSendCallback(taskData);
            break;
        case ITT_RECV:
            thisPtr->onRecvCallback(taskData);
            break;
        }
    }
    else
    {
        thisPtr->errorOccurred();
    }
}

//-----------------------------------------------------------------------------

void WinTcpConnection::onSendCallback(const IocpTaskData& taskData)
{
    ISE_ASSERT(taskData.getErrorCode() == 0);

    if (taskData.getBytesTrans() < taskData.getDataSize())
    {
        getEventLoop()->getIocpObject()->send(
            (SOCKET)taskData.getFileHandle(),
            taskData.getEntireDataBuf(),
            taskData.getEntireDataSize(),
            taskData.getDataBuf() - taskData.getEntireDataBuf() + taskData.getBytesTrans(),
            taskData.getCallback(), taskData.getCaller(), taskData.getContext());
    }
    else
    {
        isSending_ = false;
        sendBuffer_.retrieve(taskData.getEntireDataSize());
    }

    bytesSent_ += taskData.getBytesTrans();

    while (!sendTaskQueue_.empty())
    {
        SendTask& task = sendTaskQueue_.front();
        if (bytesSent_ >= task.bytes)
        {
            bytesSent_ -= task.bytes;
            iseApp().iseBusiness().onTcpSendComplete(shared_from_this(), task.context);
            sendTaskQueue_.pop_front();
        }
        else
            break;
    }

    if (!sendTaskQueue_.empty())
        trySend();
}

//-----------------------------------------------------------------------------

void WinTcpConnection::onRecvCallback(const IocpTaskData& taskData)
{
    ISE_ASSERT(taskData.getErrorCode() == 0);

    if (taskData.getBytesTrans() < taskData.getDataSize())
    {
        getEventLoop()->getIocpObject()->recv(
            (SOCKET)taskData.getFileHandle(),
            taskData.getEntireDataBuf(),
            taskData.getEntireDataSize(),
            taskData.getDataBuf() - taskData.getEntireDataBuf() + taskData.getBytesTrans(),
            taskData.getCallback(), taskData.getCaller(), taskData.getContext());
    }
    else
    {
        isRecving_ = false;
    }

    bytesRecved_ += taskData.getBytesTrans();

    while (!recvTaskQueue_.empty())
    {
        RecvTask& task = recvTaskQueue_.front();
        const char *buffer = recvBuffer_.peek();
        bool packetRecved = false;

        if (bytesRecved_ > 0)
        {
            int packetSize = 0;
            task.packetSplitter(buffer, bytesRecved_, packetSize);
            if (packetSize > 0)
            {
                bytesRecved_ -= packetSize;
                iseApp().iseBusiness().onTcpRecvComplete(shared_from_this(),
                    (void*)buffer, packetSize, task.context);
                recvTaskQueue_.pop_front();
                recvBuffer_.retrieve(packetSize);
                packetRecved = true;
            }
        }

        if (!packetRecved)
            break;
    }

    tryRecv();
}

///////////////////////////////////////////////////////////////////////////////
// class WinTcpEventLoop

//-----------------------------------------------------------------------------
// 描述: 将新连接注册到事件循环中
//-----------------------------------------------------------------------------
void WinTcpEventLoop::registerConnection(TcpConnection *connection)
{
    iocpObject_->associateHandle(connection->getSocket().getHandle());
}

//-----------------------------------------------------------------------------
// 描述: 从事件循环中注销连接
//-----------------------------------------------------------------------------
void WinTcpEventLoop::unregisterConnection(TcpConnection *connection)
{
    // nothing
}

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_WINDOWS */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_LINUX

///////////////////////////////////////////////////////////////////////////////
// class LinuxTcpConnection

LinuxTcpConnection::LinuxTcpConnection()
{
    init();
}

LinuxTcpConnection::LinuxTcpConnection(TcpServer *tcpServer, SOCKET socketHandle) :
    TcpConnection(tcpServer, socketHandle)
{
    init();
}

//-----------------------------------------------------------------------------

void LinuxTcpConnection::init()
{
    bytesSent_ = 0;
    enableSend_ = false;
    enableRecv_ = false;
}

//-----------------------------------------------------------------------------

void LinuxTcpConnection::eventLoopChanged()
{
    if (getEventLoop() != NULL)
    {
        getEventLoop()->assertInLoopThread();
        setRecvEnabled(true);
    }
}

//-----------------------------------------------------------------------------
// 描述: 提交一个发送任务
//-----------------------------------------------------------------------------
void LinuxTcpConnection::postSendTask(const void *buffer, int size,
    const Context& context, int timeout)
{
    sendBuffer_.append(buffer, size);

    SendTask task;
    task.bytes = size;
    task.context = context;
    task.timeout = timeout;

    sendTaskQueue_.push_back(task);

    if (!enableSend_)
        setSendEnabled(true);
}

//-----------------------------------------------------------------------------
// 描述: 提交一个接收任务
//-----------------------------------------------------------------------------
void LinuxTcpConnection::postRecvTask(const PacketSplitter& packetSplitter,
    const Context& context, int timeout)
{
    RecvTask task;
    task.packetSplitter = packetSplitter;
    task.context = context;
    task.timeout = timeout;

    recvTaskQueue_.push_back(task);

    if (!enableRecv_)
        setRecvEnabled(true);

    // 注意: 此处必须调用 tryRetrievePacket()，否则会造成接收中止。但是，直接
    // 调用 tryRetrievePacket() 又会造成循环调用，所以必须用 delegateToLoop()，
    // 且必须使用 shared_from_this()，否则在某些情况下会导致程序崩溃。
    getEventLoop()->delegateToLoop(boost::bind(
        &LinuxTcpConnection::afterPostRecvTask, shared_from_this()));
}

//-----------------------------------------------------------------------------
// 描述: 设置“是否监视可发送事件”
//-----------------------------------------------------------------------------
void LinuxTcpConnection::setSendEnabled(bool enabled)
{
    ISE_ASSERT(eventLoop_ != NULL);

    enableSend_ = enabled;
    getEventLoop()->updateConnection(this, enableSend_, enableRecv_);
}

//-----------------------------------------------------------------------------
// 描述: 设置“是否监视可接收事件”
//-----------------------------------------------------------------------------
void LinuxTcpConnection::setRecvEnabled(bool enabled)
{
    ISE_ASSERT(eventLoop_ != NULL);

    enableRecv_ = enabled;
    getEventLoop()->updateConnection(this, enableSend_, enableRecv_);
}

//-----------------------------------------------------------------------------
// 描述: 当“可发送”事件到来时，尝试发送数据
//-----------------------------------------------------------------------------
void LinuxTcpConnection::trySend()
{
    int readableBytes = sendBuffer_.getReadableBytes();
    if (readableBytes <= 0)
    {
        setSendEnabled(false);
        return;
    }

    const char *buffer = sendBuffer_.peek();
    int bytesSent = sendBuffer((void*)buffer, readableBytes, false);
    if (bytesSent < 0)
    {
        errorOccurred();
        return;
    }

    if (bytesSent > 0)
    {
        sendBuffer_.retrieve(bytesSent);
        bytesSent_ += bytesSent;

        while (!sendTaskQueue_.empty())
        {
            SendTask& task = sendTaskQueue_.front();
            if (bytesSent_ >= task.bytes)
            {
                bytesSent_ -= task.bytes;
                iseApp().iseBusiness().onTcpSendComplete(shared_from_this(), task.context);
                sendTaskQueue_.pop_front();
            }
            else
                break;
        }
    }
}

//-----------------------------------------------------------------------------
// 描述: 当“可接收”事件到来时，尝试接收数据
//-----------------------------------------------------------------------------
void LinuxTcpConnection::tryRecv()
{
    const int MAX_BUFFER_SIZE = iseApp().iseOptions().getTcpMaxRecvBufferSize();
    if (recvTaskQueue_.empty() && recvBuffer_.getReadableBytes() >= MAX_BUFFER_SIZE)
    {
        setRecvEnabled(false);
        return;
    }

    const int BUFFER_SIZE = 1024*16;
    char dataBuf[BUFFER_SIZE];

    int bytesRecved = recvBuffer(dataBuf, BUFFER_SIZE, false);
    if (bytesRecved < 0)
    {
        errorOccurred();
        return;
    }

    if (bytesRecved > 0)
        recvBuffer_.append(dataBuf, bytesRecved);

    while (!recvTaskQueue_.empty())
    {
        bool packetRecved = tryRetrievePacket();
        if (!packetRecved)
            break;
    }
}

//-----------------------------------------------------------------------------
// 描述: 尝试从缓存中取出一个完整数据包
//-----------------------------------------------------------------------------
bool LinuxTcpConnection::tryRetrievePacket()
{
    if (recvTaskQueue_.empty()) return false;

    bool result = false;
    RecvTask& task = recvTaskQueue_.front();
    const char *buffer = recvBuffer_.peek();
    int readableBytes = recvBuffer_.getReadableBytes();

    if (readableBytes > 0)
    {
        int packetSize = 0;
        task.packetSplitter(buffer, readableBytes, packetSize);
        if (packetSize > 0)
        {
            iseApp().iseBusiness().onTcpRecvComplete(shared_from_this(),
                (void*)buffer, packetSize, task.context);
            recvTaskQueue_.pop_front();
            recvBuffer_.retrieve(packetSize);
            result = true;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 在 postRecvTask() 中调用此函数
//-----------------------------------------------------------------------------
void LinuxTcpConnection::afterPostRecvTask(const TcpConnectionPtr& thisObj)
{
    LinuxTcpConnection *thisPtr = static_cast<LinuxTcpConnection*>(thisObj.get());
    if (!thisPtr->isErrorOccurred_)
        thisPtr->tryRetrievePacket();
}

///////////////////////////////////////////////////////////////////////////////
// class LinuxTcpEventLoop

LinuxTcpEventLoop::LinuxTcpEventLoop()
{
    epollObject_->setNotifyEventCallback(boost::bind(&LinuxTcpEventLoop::onEpollNotifyEvent, this, _1, _2));
}

LinuxTcpEventLoop::~LinuxTcpEventLoop()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 更新此 eventLoop 中的指定连接的设置
//-----------------------------------------------------------------------------
void LinuxTcpEventLoop::updateConnection(TcpConnection *connection, bool enableSend, bool enableRecv)
{
    epollObject_->updateConnection(connection, enableSend, enableRecv);
}

//-----------------------------------------------------------------------------
// 描述: 将新连接注册到事件循环中
//-----------------------------------------------------------------------------
void LinuxTcpEventLoop::registerConnection(TcpConnection *connection)
{
    epollObject_->addConnection(connection, false, false);
}

//-----------------------------------------------------------------------------
// 描述: 从事件循环中注销连接
//-----------------------------------------------------------------------------
void LinuxTcpEventLoop::unregisterConnection(TcpConnection *connection)
{
    epollObject_->removeConnection(connection);
}

//-----------------------------------------------------------------------------
// 描述: EPoll 事件回调
//-----------------------------------------------------------------------------
void LinuxTcpEventLoop::onEpollNotifyEvent(BaseTcpConnection *connection,
    EpollObject::EVENT_TYPE eventType)
{
    LinuxTcpConnection *conn = static_cast<LinuxTcpConnection*>(connection);

    if (eventType == EpollObject::ET_ALLOW_SEND)
        conn->trySend();
    else if (eventType == EpollObject::ET_ALLOW_RECV)
        conn->tryRecv();
    else if (eventType == EpollObject::ET_ERROR)
        conn->errorOccurred();
}

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_LINUX */

///////////////////////////////////////////////////////////////////////////////
// class MainTcpServer

MainTcpServer::MainTcpServer() :
    isActive_(false)
{
    createTcpServerList();
}

MainTcpServer::~MainTcpServer()
{
    close();
    destroyTcpServerList();
}

//-----------------------------------------------------------------------------
// 描述: 开启服务器
//-----------------------------------------------------------------------------
void MainTcpServer::open()
{
    if (!isActive_)
    {
        try
        {
            doOpen();
            isActive_ = true;
        }
        catch (...)
        {
            doClose();
            throw;
        }
    }
}

//-----------------------------------------------------------------------------
// 描述: 关闭服务器
//-----------------------------------------------------------------------------
void MainTcpServer::close()
{
    if (isActive_)
    {
        doClose();
        isActive_ = false;
    }
}

//-----------------------------------------------------------------------------

TcpServer& MainTcpServer::getTcpServer(int index)
{
    ISE_ASSERT(index >= 0 && index < (int)tcpServerList_.size());
    return *tcpServerList_[index];
}

//-----------------------------------------------------------------------------
// 描述: 延迟创建 tcpClientEventLoopList_
//-----------------------------------------------------------------------------
TcpEventLoopList& MainTcpServer::getTcpClientEventLoopList()
{
    static Mutex s_mutex;
    AutoLocker locker(s_mutex);

    if (!tcpClientEventLoopList_)
    {
        int eventLoopCount = iseApp().iseOptions().getTcpClientEventLoopCount();
        tcpClientEventLoopList_.reset(new TcpEventLoopList(eventLoopCount));
        if (isActive_)
            tcpClientEventLoopList_->start();
    }

    return *tcpClientEventLoopList_;
}

//-----------------------------------------------------------------------------
// 描述: 在全局范围内，根据事件循环线程ID查找对应的事件循环，找不到返回NULL
//-----------------------------------------------------------------------------
EventLoop* MainTcpServer::findEventLoop(THREAD_ID loopThreadId)
{
    EventLoop *result = NULL;

    for (int svrIdx = 0; svrIdx < (int)tcpServerList_.size(); ++svrIdx)
    {
        TcpEventLoopList& eventLoopList = tcpServerList_[svrIdx]->eventLoopList_;
        EventLoop *eventLoop = eventLoopList.findEventLoop(loopThreadId);
        if (eventLoop != NULL)
        {
            result = eventLoop;
            break;
        }
    }

    if (!result)
        result = tcpClientEventLoopList_->findEventLoop(loopThreadId);

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 创建TCP服务器
//-----------------------------------------------------------------------------
void MainTcpServer::createTcpServerList()
{
    int serverCount = iseApp().iseOptions().getTcpServerCount();
    ISE_ASSERT(serverCount >= 0);

    tcpServerList_.resize(serverCount);
    for (int i = 0; i < serverCount; i++)
    {
        TcpServer *tcpServer = new TcpServer(iseApp().iseOptions().getTcpServerEventLoopCount(i));
        tcpServer->setContext(i);
        tcpServer->setLocalPort(static_cast<WORD>(iseApp().iseOptions().getTcpServerPort(i)));

        tcpServerList_[i] = tcpServer;
    }
}

//-----------------------------------------------------------------------------
// 描述: 销毁TCP服务器
//-----------------------------------------------------------------------------
void MainTcpServer::destroyTcpServerList()
{
    for (int i = 0; i < (int)tcpServerList_.size(); i++)
        delete tcpServerList_[i];
    tcpServerList_.clear();
}

//-----------------------------------------------------------------------------
// 描述: 开启服务器
//-----------------------------------------------------------------------------
void MainTcpServer::doOpen()
{
    for (int i = 0; i < (int)tcpServerList_.size(); i++)
        tcpServerList_[i]->open();

    if (tcpClientEventLoopList_)
        tcpClientEventLoopList_->start();
}

//-----------------------------------------------------------------------------
// 描述: 关闭服务器
//-----------------------------------------------------------------------------
void MainTcpServer::doClose()
{
    if (tcpClientEventLoopList_)
        tcpClientEventLoopList_->stop();

    for (int i = 0; i < (int)tcpServerList_.size(); i++)
        tcpServerList_[i]->close();
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
