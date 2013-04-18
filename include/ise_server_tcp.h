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
    int& splitBytes     // 返回分离出来的数据包大小，返回0表示现存数据中尚不足以分离出一个完整数据包
)> PacketSplitter;

///////////////////////////////////////////////////////////////////////////////
// 预定义数据包分界器

void linePacketSplitter(const char *data, int bytes, int& splitBytes);
void nullTerminatedPacketSplitter(const char *data, int bytes, int& splitBytes);

// 以 '\r'或'\n' 或其组合为分界字符的数据包分界器
const PacketSplitter LINE_PACKET_SPLITTER = boost::bind(&ise::linePacketSplitter, _1, _2, _3);
// 以 '\0' 为分界字符的数据包分界器
const PacketSplitter NULL_TERMINATED_PACKET_SPLITTER = boost::bind(&ise::nullTerminatedPacketSplitter, _1, _2, _3);

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
    friend class TcpEventLoopThread;

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
    void executeInLoop(const Functor& functor);
    void delegateToLoop(const Functor& functor);
    void addFinalizer(const Functor& finalizer);

    void addConnection(TcpConnection *connection);
    void removeConnection(TcpConnection *connection);

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
    TcpEventLoopThread *thread_;
    THREAD_ID loopThreadId_;
    TcpConnectionMap tcpConnMap_;
    FunctorList delegatedFunctors_;
    FunctorList finalizers_;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopList - 事件循环列表

class TcpEventLoopList : boost::noncopyable
{
public:
    enum { MAX_LOOP_COUNT = 64 };

public:
    TcpEventLoopList(int nLoopCount);
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
    friend class MainTcpServer;

    struct SendTask
    {
        int bytes;
        Context context;
    };

    struct RecvTask
    {
        PacketSplitter packetSplitter;
        Context context;
    };

    typedef deque<SendTask> SendTaskQueue;
    typedef deque<RecvTask> RecvTaskQueue;

public:
    TcpConnection(TcpServer *tcpServer, SOCKET socketHandle, const string& connectionName);
    virtual ~TcpConnection();

    void send(const void *buffer, int size, const Context& context = EMPTY_CONTEXT);
    void recv(const PacketSplitter& packetSplitter, const Context& context = EMPTY_CONTEXT);

    int getServerIndex() const { return boost::any_cast<int>(tcpServer_->getContext()); }
    int getServerPort() const { return tcpServer_->getLocalPort(); }

protected:
    virtual void doDisconnect();
    virtual void postSendTask(const void *buffer, int size, const Context& context) = 0;
    virtual void postRecvTask(const PacketSplitter& packetSplitter, const Context& context) = 0;

protected:
    void errorOccurred();
    void postSendTask(const string& data, const Context& context);

    void setEventLoop(TcpEventLoop *eventLoop);
    TcpEventLoop* getEventLoop() { return eventLoop_; }

protected:
    TcpServer *tcpServer_;           // 所属 TcpServer
    TcpEventLoop *eventLoop_;        // 所属 TcpEventLoop
    IoBuffer sendBuffer_;            // 数据发送缓存
    IoBuffer recvBuffer_;            // 数据接收缓存
    SendTaskQueue sendTaskQueue_;    // 发送任务队列
    RecvTaskQueue recvTaskQueue_;    // 接收任务队列
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
    WinTcpConnection(TcpServer *tcpServer, SOCKET socketHandle, const string& connectionName);
protected:
    virtual void postSendTask(const void *buffer, int size, const Context& context);
    virtual void postRecvTask(const PacketSplitter& packetSplitter, const Context& context);

private:
    WinTcpEventLoop* getEventLoop() { return (WinTcpEventLoop*)eventLoop_; }

    void trySend();
    void tryRecv();

    void onIocpCallback(const IocpTaskData& taskData);
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
    friend class IocpObject;

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
};

#pragma pack(1)
struct CIocpOverlappedData
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
    friend class AutoFinalizer;

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
    CIocpOverlappedData* createOverlappedData(IOCP_TASK_TYPE taskType,
        HANDLE fileHandle, PVOID buffer, int size, int offset,
        const IocpCallback& callback, PVOID caller, const Context& context);
    void destroyOverlappedData(CIocpOverlappedData *ovDataPtr);
    void postError(int errorCode, CIocpOverlappedData *ovDataPtr);
    void invokeCallback(const IocpTaskData& taskData);

private:
    static IocpBufferAllocator bufferAlloc_;
    static SeqNumberAlloc taskSeqAlloc_;
    static IocpPendingCounter pendingCounter_;

    HANDLE iocpHandle_;
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
    friend class LinuxTcpEventLoop;
public:
    LinuxTcpConnection(TcpServer *tcpServer, SOCKET socketHandle, const string& connectionName);
protected:
    virtual void postSendTask(const void *buffer, int size, const Context& context);
    virtual void postRecvTask(const PacketSplitter& packetSplitter, const Context& context);

private:
    LinuxTcpEventLoop* getEventLoop() { return (LinuxTcpEventLoop*)eventLoop_; }

    void setSendEnabled(bool enabled);
    void setRecvEnabled(bool enabled);

    void trySend();
    void tryRecv();

private:
    int bytesSent_;                  // 自从上次发送任务完成回调以来共发送了多少字节
    bool enableSend_;                // 是否监视可发送事件
    bool enableRecv_;                // 是否监视可接收事件
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

private:
    void createTcpServerList();
    void destroyTcpServerList();
    void doOpen();
    void doClose();

    void onCreateConnection(TcpServer *tcpServer, SOCKET socketHandle,
        const string& connectionName, BaseTcpConnection*& connection);
    void onAcceptConnection(TcpServer *tcpServer, BaseTcpConnection *connection);

private:
    typedef vector<TcpServer*> TcpServerList;

    bool isActive_;
    TcpServerList tcpServerList_;
    TcpEventLoopList eventLoopList_;
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SERVER_TCP_H_
