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
// ise_server_tcp.h
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// 说明:
//
// * 收到连接后(onTcpConnected)，即使用户不调用 connection->recv()，ISE也会在后台
//   自动接收数据。接收到的数据暂存于缓存中。
//
// * 即使用户在连接上无任何动作(既不 send 也不 recv)，当对方断开连接 (close/shutdown) 时，
//   我方也能够感知，并通过 onTcpDisconnected() 通知用户。
//
// * 关于连接的断开:
//   connection->disconnect() 会立即关闭 (shutdown) 发送通道，而不管该连接的
//   ISE缓存中有没有未发送完的数据。由于没有关闭接收通道，所以此时程序仍可以接
//   收对方的数据。正常情况下，对方在检测到我方关闭发送 (read 返回 0) 后，应断
//   开连接 (close)，这样我方才能引发接收错误，进入 errorOccurred()，既而销毁连接。
//
//   如果希望把缓存中的数据发送完毕后再 disconnect()，可在 onTcpSendComplete()
//   中进行断开操作。
//
//   TcpConnection 提供了更灵活的 shutdown(bool closeSend, bool closeRecv) 方法。
//   用户如果希望断开连接时双向关闭，可直接调用 connection->shutdown() 方法，
//   而不是 connection->disconnect()。
//
//   以下情况ISE会立即双向关闭 (shutdown(true, true)) 连接:
//   1. 连接上有错误发生 (errorOccurred())；
//   2. 发送或接收超时 (checkTimeout())；
//   3. 程序退出时关闭现存连接 (clearConnections())。
//
// * 连接对象 (TcpConnection) 采用 boost::shared_ptr 管理，由以下几个角色持有:
//   1. TcpEventLoop.
//      由 TcpEventLoop::tcpConnMap_ 持有，TcpEventLoop::removeConnection() 时释放。
//      当调用 TcpConnection::setEventLoop(NULL) 时，将引发 removeConnection()。
//      程序正常退出 (kill 或 iseApp().setTerminated(true)) 时，将清理全部连接
//      (TcpEventLoop::clearConnections())，从而使 TcpEventLoop 释放它持有的全部
//      boost::shared_ptr。
//   2. ISE_WINDOWS::IOCP.
//      由 IocpTaskData::callback_ 持有。callback_ 作为一个 boost::function，保存
//      了由 TcpConnection::shared_from_this() 传递过来的 shared_ptr。
//      当一次IOCP轮循完毕后，会调用 IocpObject::destroyOverlappedData()，此处会
//      析构 callback_，从而释放 shared_ptr。
//      这说明要想销毁一个连接对象，至少应保证在该连接上投递给IOCP的请求在轮循中
//      得到了处理。只有处理完请求，IOCP 才会释放 shared_ptr。
//   3. ISE 的用户.
//      业务接口中 IseBusiness::onTcpXXX() 系列函数将连接对象以 shared_ptr 形式
//      传递给用户。大部分情况下，ISE对连接对象的自动管理能满足实际需要，但用户
//      仍然可以持有 shared_ptr，以免连接被自动销毁。
//
// * 连接对象 (TcpConnection) 的几个销毁场景:
//   1. IseBusiness::onTcpConnected() 之后，用户调用了 connection->disconnect()，
//      引发 IOCP/EPoll 错误，调用 errorOccurred()，执行 onTcpDisconnected() 和
//      TcpConnecton::setEventLoop(NULL)。
//   2. IseBusiness::onTcpConnected() 之后，用户没有任何动作，但对方断开了连接，
//      我方检测到后引发 IOCP/EPoll 错误，之后同1。
//   3. 程序在 Linux 下被 kill，或程序中执行了 iseApp().setTerminated(true)，
//      当前剩余连接全部在 TcpEventLoop::clearConnections() 中 被 disconnect()，
//      在接下来的轮循中引发了 IOCP/EPoll 错误，之后同1。


#ifndef _ISE_SERVER_TCP_H_
#define _ISE_SERVER_TCP_H_

#include "ise_options.h"
#include "ise_classes.h"
#include "ise_thread.h"
#include "ise_sys_utils.h"
#include "ise_socket.h"
#include "ise_exceptions.h"

#ifdef ISE_WINDOWS
#include <windows.h>
#endif

#ifdef ISE_LINUX
#include <sys/epoll.h>
#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class IoBuffer;
class TcpEventLoopThread;
class TcpEventLoop;
class TcpEventLoopList;
class TcpConnection;
class TcpClient;
class TcpServer;
class TcpConnector;

#ifdef ISE_WINDOWS
class WinTcpConnection;
class WinTcpEventLoop;
class IocpTaskData;
class IocpBufferAllocator;
class IocpPendingCounter;
class IocpObject;
#endif

#ifdef ISE_LINUX
class LinuxTcpConnection;
class LinuxTcpEventLoop;
class EpollObject;
#endif

class MainTcpServer;

///////////////////////////////////////////////////////////////////////////////
// 类型定义

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

// 数据包分界器
typedef boost::function<void (
    const char *data,   // 缓存中可用数据的首字节指针
    int bytes,          // 缓存中可用数据的字节数
    int& retrieveBytes  // 返回分离出来的数据包大小，返回0表示现存数据中尚不足以分离出一个完整数据包
)> PacketSplitter;

///////////////////////////////////////////////////////////////////////////////
// 预定义数据包分界器

void bytePacketSplitter(const char *data, int bytes, int& retrieveBytes);
void linePacketSplitter(const char *data, int bytes, int& retrieveBytes);
void nullTerminatedPacketSplitter(const char *data, int bytes, int& retrieveBytes);
void anyPacketSplitter(const char *data, int bytes, int& retrieveBytes);

// 每次接收一个字节的数据包分界器
const PacketSplitter BYTE_PACKET_SPLITTER = &ise::bytePacketSplitter;
// 以 '\r'或'\n' 或其组合为分界字符的数据包分界器
const PacketSplitter LINE_PACKET_SPLITTER = &ise::linePacketSplitter;
// 以 '\0' 为分界字符的数据包分界器
const PacketSplitter NULL_TERMINATED_PACKET_SPLITTER = &ise::nullTerminatedPacketSplitter;
// 无论收到多少字节都立即获取的数据包分界器
const PacketSplitter ANY_PACKET_SPLITTER = &ise::anyPacketSplitter;

///////////////////////////////////////////////////////////////////////////////
// class TcpInspectInfo

class TcpInspectInfo : boost::noncopyable
{
public:
    AtomicInt tcpConnCreateCount;    // TcpConnection 对象的创建次数
    AtomicInt tcpConnDestroyCount;   // TcpConnection 对象的销毁次数
    AtomicInt errorOccurredCount;    // TcpConnection::errorOccurred() 的调用次数
    AtomicInt addConnCount;          // TcpEventLoop::addConnection() 的调用次数
    AtomicInt removeConnCount;       // TcpEventLoop::removeConnection() 的调用次数
public:
    static TcpInspectInfo& instance()
    {
        static TcpInspectInfo obj;
        return obj;
    }
};

///////////////////////////////////////////////////////////////////////////////
// class IoBuffer - 输入输出缓存
//
// +-----------------+------------------+------------------+
// |  useless bytes  |  readable bytes  |  writable bytes  |
// |                 |     (CONTENT)    |                  |
// +-----------------+------------------+------------------+
// |                 |                  |                  |
// 0     <=     readerIndex   <=   writerIndex    <=    size

class IoBuffer
{
public:
    enum { INITIAL_SIZE = 1024 };

public:
    IoBuffer();
    ~IoBuffer();

    int getReadableBytes() const { return writerIndex_ - readerIndex_; }
    int getWritableBytes() const { return (int)buffer_.size() - writerIndex_; }
    int getUselessBytes() const { return readerIndex_; }

    void append(const string& str);
    void append(const void *data, int bytes);
    void append(int bytes);

    void retrieve(int bytes);
    void retrieveAll(string& str);
    void retrieveAll();

    void swap(IoBuffer& rhs);
    const char* peek() const { return getBufferPtr() + readerIndex_; }

private:
    char* getBufferPtr() const { return (char*)&*buffer_.begin(); }
    char* getWriterPtr() const { return getBufferPtr() + writerIndex_; }
    void makeSpace(int moreBytes);

private:
    vector<char> buffer_;
    int readerIndex_;
    int writerIndex_;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopThread - 事件循环执行线程

class TcpEventLoopThread : public Thread
{
public:
    TcpEventLoopThread(TcpEventLoop& eventLoop);
protected:
    virtual void execute();
    virtual void afterExecute();
private:
    TcpEventLoop& eventLoop_;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoop - 事件循环基类

class TcpEventLoop : boost::noncopyable
{
public:
    typedef vector<Functor> Functors;
    typedef map<string, TcpConnectionPtr> TcpConnectionMap;  // <connectionName, TcpConnectionPtr>

    struct FunctorList
    {
        Functors items;
        CriticalSection lock;
    };

public:
    TcpEventLoop();
    virtual ~TcpEventLoop();

    void start();
    void stop(bool force, bool waitFor);

    bool isRunning();
    bool isInLoopThread();
    void assertInLoopThread();
    void executeInLoop(const Functor& functor);
    void delegateToLoop(const Functor& functor);
    void addFinalizer(const Functor& finalizer);

    void addConnection(TcpConnection *connection);
    void removeConnection(TcpConnection *connection);
    void clearConnections();

protected:
    virtual void doLoopWork(Thread *thread) = 0;
    virtual void wakeupLoop() {}
    virtual void registerConnection(TcpConnection *connection) = 0;
    virtual void unregisterConnection(TcpConnection *connection) = 0;

protected:
    void runLoop(Thread *thread);
    void executeDelegatedFunctors();
    void executeFinalizer();

private:
    void checkTimeout();

private:
    TcpEventLoopThread *thread_;
    THREAD_ID loopThreadId_;
    TcpConnectionMap tcpConnMap_;
    FunctorList delegatedFunctors_;
    FunctorList finalizers_;
    UINT lastCheckTimeoutTicks_;

    friend class TcpEventLoopThread;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopList - 事件循环列表

class TcpEventLoopList : boost::noncopyable
{
public:
    enum { MAX_LOOP_COUNT = 64 };

public:
    TcpEventLoopList(int loopCount);
    virtual ~TcpEventLoopList();

    void start();
    void stop();

    int getCount() { return items_.getCount(); }
    TcpEventLoop* getItem(int index) { return items_[index]; }
    TcpEventLoop* operator[] (int index) { return getItem(index); }

private:
    void setCount(int count);

private:
    ObjectList<TcpEventLoop> items_;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpConnection - Proactor模型下的TCP连接

class TcpConnection :
    public BaseTcpConnection,
    public boost::enable_shared_from_this<TcpConnection>
{
public:
    struct SendTask
    {
    public:
        int bytes;
        Context context;
        int timeout;
        UINT startTicks;
    public:
        SendTask()
        {
            bytes = 0;
            timeout = 0;
            startTicks = 0;
        }
    };

    struct RecvTask
    {
    public:
        PacketSplitter packetSplitter;
        Context context;
        int timeout;
        UINT startTicks;
    public:
        RecvTask()
        {
            timeout = 0;
            startTicks = 0;
        }
    };

    typedef deque<SendTask> SendTaskQueue;
    typedef deque<RecvTask> RecvTaskQueue;

public:
    TcpConnection();
    TcpConnection(TcpServer *tcpServer, SOCKET socketHandle);
    virtual ~TcpConnection();

    void send(
        const void *buffer,
        int size,
        const Context& context = EMPTY_CONTEXT,
        int timeout = TIMEOUT_INFINITE
        );

    void recv(
        const PacketSplitter& packetSplitter = ANY_PACKET_SPLITTER,
        const Context& context = EMPTY_CONTEXT,
        int timeout = TIMEOUT_INFINITE
        );

    const string& getConnectionName() const;
    int getServerIndex() const;
    int getServerPort() const;
    int getServerConnCount() const;

protected:
    virtual void doDisconnect();
    virtual void eventLoopChanged() {}
    virtual void postSendTask(const void *buffer, int size, const Context& context, int timeout) = 0;
    virtual void postRecvTask(const PacketSplitter& packetSplitter, const Context& context, int timeout) = 0;

protected:
    void errorOccurred();
    void checkTimeout(UINT curTicks);
    void postSendTask(const string& data, const Context& context, int timeout);

    void setEventLoop(TcpEventLoop *eventLoop);
    TcpEventLoop* getEventLoop() { return eventLoop_; }

private:
    void init();

protected:
    TcpServer *tcpServer_;           // 所属 TcpServer
    TcpEventLoop *eventLoop_;        // 所属 TcpEventLoop
    mutable string connectionName_;  // 连接名称
    IoBuffer sendBuffer_;            // 数据发送缓存
    IoBuffer recvBuffer_;            // 数据接收缓存
    SendTaskQueue sendTaskQueue_;    // 发送任务队列
    RecvTaskQueue recvTaskQueue_;    // 接收任务队列
    bool isErrorOccurred_;           // 连接上是否发生了错误

    friend class MainTcpServer;
    friend class TcpEventLoop;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpClient

class TcpClient : public BaseTcpClient
{
public:
    TcpConnection& getConnection() { return *static_cast<TcpConnection*>(connection_); }
protected:
    virtual BaseTcpConnection* createConnection();
private:
    bool registerToEventLoop(int index = -1);
private:
    friend class TcpConnector;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpServer

class TcpServer : public BaseTcpServer
{
public:
    int getConnectionCount() const { return connCount_.get(); }

protected:
    virtual BaseTcpConnection* createConnection(SOCKET socketHandle);

private:
    void incConnCount() { connCount_.increment(); }
    void decConnCount() { connCount_.decrement(); }

private:
    mutable AtomicInt connCount_;
    friend class TcpConnection;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpConnector - TCP连接器类

class TcpConnector : boost::noncopyable
{
public:
    typedef boost::function<void (bool success, TcpConnection *connection,
        const InetAddress& peerAddr, const Context& context)> CompleteCallback;

private:
    typedef vector<SOCKET> FdList;

    struct TaskItem
    {
        TcpClient tcpClient;
        InetAddress peerAddr;
        CompleteCallback completeCallback;
        ASYNC_CONNECT_STATE state;
        Context context;
    };

    typedef ObjectList<TaskItem> TaskList;

    class WorkerThread : public Thread
    {
    public:
        WorkerThread(TcpConnector& owner) : owner_(owner) {}
    protected:
        virtual void execute() { owner_.work(*this); }
        virtual void afterExecute() { owner_.thread_ = NULL; }
    private:
        TcpConnector& owner_;
    };

    friend class WorkerThread;

public:
    ~TcpConnector();
    static TcpConnector& instance();

    void connect(const InetAddress& peerAddr,
        const CompleteCallback& completeCallback,
        const Context& context = EMPTY_CONTEXT);
    void clear();

private:
    TcpConnector();

private:
    void start();
    void stop();
    void work(WorkerThread& thread);

    void tryConnect();
    void getPendingFdsFromTaskList(int& fromIndex, FdList& fds);
    void checkAsyncConnectState(const FdList& fds, FdList& connectedFds, FdList& failedFds);
    TaskItem* findTask(SOCKET fd);
    void invokeCompleteCallback();

private:
    TaskList taskList_;
    CriticalSection lock_;
    WorkerThread *thread_;
};

///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_WINDOWS

///////////////////////////////////////////////////////////////////////////////
// 类型定义

enum IOCP_TASK_TYPE
{
    ITT_SEND = 1,
    ITT_RECV = 2,
};

typedef boost::function<void (const IocpTaskData& taskData)> IocpCallback;

///////////////////////////////////////////////////////////////////////////////
// class WinTcpConnection

class WinTcpConnection : public TcpConnection
{
public:
    WinTcpConnection();
    WinTcpConnection(TcpServer *tcpServer, SOCKET socketHandle);

protected:
    virtual void eventLoopChanged();
    virtual void postSendTask(const void *buffer, int size, const Context& context, int timeout);
    virtual void postRecvTask(const PacketSplitter& packetSplitter, const Context& context, int timeout);

private:
    void init();

    WinTcpEventLoop* getEventLoop() { return (WinTcpEventLoop*)eventLoop_; }

    void trySend();
    void tryRecv();

    static void onIocpCallback(const TcpConnectionPtr& thisObj, const IocpTaskData& taskData);
    void onSendCallback(const IocpTaskData& taskData);
    void onRecvCallback(const IocpTaskData& taskData);

private:
    bool isSending_;       // 是否已向IOCP提交发送任务但尚未收到回调通知
    bool isRecving_;       // 是否已向IOCP提交接收任务但尚未收到回调通知
    int bytesSent_;        // 自从上次发送任务完成回调以来共发送了多少字节
    int bytesRecved_;      // 自从上次接收任务完成回调以来共接收了多少字节
};

///////////////////////////////////////////////////////////////////////////////
// class IocpTaskData

class IocpTaskData
{
public:
    IocpTaskData();

    HANDLE getIocpHandle() const { return iocpHandle_; }
    HANDLE getFileHandle() const { return fileHandle_; }
    IOCP_TASK_TYPE getTaskType() const { return taskType_; }
    UINT getTaskSeqNum() const { return taskSeqNum_; }
    PVOID getCaller() const { return caller_; }
    const Context& getContext() const { return context_; }
    char* getEntireDataBuf() const { return (char*)entireDataBuf_; }
    int getEntireDataSize() const { return entireDataSize_; }
    char* getDataBuf() const { return (char*)wsaBuffer_.buf; }
    int getDataSize() const { return wsaBuffer_.len; }
    int getBytesTrans() const { return bytesTrans_; }
    int getErrorCode() const { return errorCode_; }
    const IocpCallback& getCallback() const { return callback_; }

private:
    HANDLE iocpHandle_;
    HANDLE fileHandle_;
    IOCP_TASK_TYPE taskType_;
    UINT taskSeqNum_;
    PVOID caller_;
    Context context_;
    PVOID entireDataBuf_;
    int entireDataSize_;
    WSABUF wsaBuffer_;
    int bytesTrans_;
    int errorCode_;
    IocpCallback callback_;

    friend class IocpObject;
};

#pragma pack(1)
struct IocpOverlappedData
{
    OVERLAPPED overlapped;
    IocpTaskData taskData;
};
#pragma pack()

///////////////////////////////////////////////////////////////////////////////
// class IocpBufferAllocator

class IocpBufferAllocator : boost::noncopyable
{
public:
    IocpBufferAllocator(int bufferSize);
    ~IocpBufferAllocator();

    PVOID allocBuffer();
    void returnBuffer(PVOID buffer);

    int getUsedCount() const { return usedCount_; }

private:
    void clear();

private:
    int bufferSize_;
    PointerList items_;
    int usedCount_;
    CriticalSection lock_;
};

///////////////////////////////////////////////////////////////////////////////
// class IocpPendingCounter

class IocpPendingCounter : boost::noncopyable
{
public:
    IocpPendingCounter() {}
    virtual ~IocpPendingCounter() {}

    void inc(PVOID caller, IOCP_TASK_TYPE taskType);
    void dec(PVOID caller, IOCP_TASK_TYPE taskType);
    int get(PVOID caller);
    int get(IOCP_TASK_TYPE taskType);

private:
    struct CountData
    {
        int sendCount;
        int recvCount;
    };

    typedef std::map<PVOID, CountData> Items;   // <caller, CountData>

    Items items_;
    CriticalSection lock_;
};

///////////////////////////////////////////////////////////////////////////////
// class IocpObject

class IocpObject : boost::noncopyable
{
public:
    IocpObject();
    virtual ~IocpObject();

    bool associateHandle(SOCKET socketHandle);
    bool isComplete(PVOID caller);

    void work();
    void wakeup();

    void send(SOCKET socketHandle, PVOID buffer, int size, int offset,
        const IocpCallback& callback, PVOID caller, const Context& context);
    void recv(SOCKET socketHandle, PVOID buffer, int size, int offset,
        const IocpCallback& callback, PVOID caller, const Context& context);

private:
    void initialize();
    void finalize();
    void throwGeneralError();
    IocpOverlappedData* createOverlappedData(IOCP_TASK_TYPE taskType,
        HANDLE fileHandle, PVOID buffer, int size, int offset,
        const IocpCallback& callback, PVOID caller, const Context& context);
    void destroyOverlappedData(IocpOverlappedData *ovDataPtr);
    void postError(int errorCode, IocpOverlappedData *ovDataPtr);
    void invokeCallback(const IocpTaskData& taskData);

private:
    static IocpBufferAllocator bufferAlloc_;
    static SeqNumberAlloc taskSeqAlloc_;
    static IocpPendingCounter pendingCounter_;

    HANDLE iocpHandle_;

    friend class AutoFinalizer;
};

///////////////////////////////////////////////////////////////////////////////
// class WinTcpEventLoop

class WinTcpEventLoop : public TcpEventLoop
{
public:
    WinTcpEventLoop();
    virtual ~WinTcpEventLoop();

    IocpObject* getIocpObject() { return iocpObject_; }

protected:
    virtual void doLoopWork(Thread *thread);
    virtual void wakeupLoop();
    virtual void registerConnection(TcpConnection *connection);
    virtual void unregisterConnection(TcpConnection *connection);

private:
    IocpObject *iocpObject_;
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_WINDOWS */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_LINUX

///////////////////////////////////////////////////////////////////////////////
// class LinuxTcpConnection

class LinuxTcpConnection : public TcpConnection
{
public:
    LinuxTcpConnection();
    LinuxTcpConnection(TcpServer *tcpServer, SOCKET socketHandle);

protected:
    virtual void eventLoopChanged();
    virtual void postSendTask(const void *buffer, int size, const Context& context, int timeout);
    virtual void postRecvTask(const PacketSplitter& packetSplitter, const Context& context, int timeout);

private:
    void init();

    LinuxTcpEventLoop* getEventLoop() { return (LinuxTcpEventLoop*)eventLoop_; }

    void setSendEnabled(bool enabled);
    void setRecvEnabled(bool enabled);

    void trySend();
    void tryRecv();

    bool tryRetrievePacket();

private:
    int bytesSent_;                  // 自从上次发送任务完成回调以来共发送了多少字节
    bool enableSend_;                // 是否监视可发送事件
    bool enableRecv_;                // 是否监视可接收事件

    friend class LinuxTcpEventLoop;
};

///////////////////////////////////////////////////////////////////////////////
// class EpollObject - Linux EPoll 功能封装

class EpollObject
{
public:
    enum { INITIAL_EVENT_SIZE = 32 };

    enum EVENT_TYPE
    {
        ET_NONE        = 0,
        ET_ERROR       = 1,
        ET_ALLOW_SEND  = 2,
        ET_ALLOW_RECV  = 3,
    };

    typedef vector<struct epoll_event> EventList;
    typedef int EventPipe[2];

    typedef boost::function<void (TcpConnection *connection, EVENT_TYPE eventType)> NotifyEventCallback;

public:
    EpollObject();
    ~EpollObject();

    void poll();
    void wakeup();

    void addConnection(TcpConnection *connection, bool enableSend, bool enableRecv);
    void updateConnection(TcpConnection *connection, bool enableSend, bool enableRecv);
    void removeConnection(TcpConnection *connection);

    void setNotifyEventCallback(const NotifyEventCallback& callback);

private:
    void createEpoll();
    void destroyEpoll();
    void createPipe();
    void destroyPipe();

    void epollControl(int operation, void *param, int handle, bool enableSend, bool enableRecv);

    void processPipeEvent();
    void processEvents(int eventCount);

private:
    int epollFd_;                    // EPoll 的文件描述符
    EventList events_;               // 存放 epoll_wait() 返回的事件
    EventPipe pipeFds_;              // 用于唤醒 epoll_wait() 的管道
    NotifyEventCallback onNotifyEvent_;
};

///////////////////////////////////////////////////////////////////////////////
// class LinuxTcpEventLoop

class LinuxTcpEventLoop : public TcpEventLoop
{
public:
    LinuxTcpEventLoop();
    virtual ~LinuxTcpEventLoop();

    void updateConnection(TcpConnection *connection, bool enableSend, bool enableRecv);

protected:
    virtual void doLoopWork(Thread *thread);
    virtual void wakeupLoop();
    virtual void registerConnection(TcpConnection *connection);
    virtual void unregisterConnection(TcpConnection *connection);

private:
    void onEpollNotifyEvent(TcpConnection *connection, EpollObject::EVENT_TYPE eventType);

private:
    EpollObject *epollObject_;
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_LINUX */

///////////////////////////////////////////////////////////////////////////////
// class MainTcpServer - TCP主服务器类

class MainTcpServer
{
public:
    explicit MainTcpServer();
    virtual ~MainTcpServer();

    void open();
    void close();

    bool registerToEventLoop(BaseTcpConnection *connection, int eventLoopIndex = -1);

private:
    void createTcpServerList();
    void destroyTcpServerList();
    void doOpen();
    void doClose();

    void onAcceptConnection(BaseTcpServer *tcpServer, BaseTcpConnection *connection);

private:
    typedef vector<TcpServer*> TcpServerList;

    bool isActive_;
    TcpServerList tcpServerList_;
    TcpEventLoopList eventLoopList_;
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SERVER_TCP_H_
