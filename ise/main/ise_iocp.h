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
// ise_iocp.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_IOCP_H_
#define _ISE_IOCP_H_

#include "ise/main/ise_options.h"
#include "ise/main/ise_classes.h"
#include "ise/main/ise_sys_utils.h"
#include "ise/main/ise_socket.h"
#include "ise/main/ise_exceptions.h"

#ifdef ISE_WINDOWS
#include <windows.h>
#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// classes

#ifdef ISE_WINDOWS
class IocpTaskData;
class IocpBufferAllocator;
class IocpPendingCounter;
class IocpObject;
#endif

// 提前声明
class EventLoop;

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
    Mutex mutex_;
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
    Mutex mutex_;
};

///////////////////////////////////////////////////////////////////////////////
// class IocpObject

class IocpObject : boost::noncopyable
{
public:
    IocpObject(EventLoop *eventLoop);
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

    EventLoop *eventLoop_;
    HANDLE iocpHandle_;

    friend class AutoFinalizer;
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_WINDOWS */

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_IOCP_H_
