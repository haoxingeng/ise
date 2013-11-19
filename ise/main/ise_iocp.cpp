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
// 文件名称: ise_iocp.cpp
// 功能描述: 完成端口实现
///////////////////////////////////////////////////////////////////////////////

#include "ise/main/ise_iocp.h"
#include "ise/main/ise_err_msgs.h"
#include "ise/main/ise_event_loop.h"

using namespace ise;

namespace ise
{

#ifdef ISE_WINDOWS

///////////////////////////////////////////////////////////////////////////////
// class IocpTaskData

IocpTaskData::IocpTaskData() :
    iocpHandle_(INVALID_HANDLE_VALUE),
    fileHandle_(INVALID_HANDLE_VALUE),
    taskType_((IOCP_TASK_TYPE)0),
    taskSeqNum_(0),
    caller_(0),
    entireDataBuf_(0),
    entireDataSize_(0),
    bytesTrans_(0),
    errorCode_(0)
{
    wsaBuffer_.buf = NULL;
    wsaBuffer_.len = 0;
}

///////////////////////////////////////////////////////////////////////////////
// class IocpBufferAllocator

IocpBufferAllocator::IocpBufferAllocator(int bufferSize) :
    bufferSize_(bufferSize),
    usedCount_(0)
{
    // nothing
}

//-----------------------------------------------------------------------------

IocpBufferAllocator::~IocpBufferAllocator()
{
    clear();
}

//-----------------------------------------------------------------------------

PVOID IocpBufferAllocator::allocBuffer()
{
    AutoLocker locker(mutex_);
    PVOID result;

    if (!items_.isEmpty())
    {
        result = items_.last();
        items_.del(items_.getCount() - 1);
    }
    else
    {
        result = new char[bufferSize_];
        if (result == NULL)
            iseThrowMemoryException();
    }

    usedCount_++;
    return result;
}

//-----------------------------------------------------------------------------

void IocpBufferAllocator::returnBuffer(PVOID buffer)
{
    AutoLocker locker(mutex_);

    if (buffer != NULL && items_.indexOf(buffer) == -1)
    {
        items_.add(buffer);
        usedCount_--;
    }
}

//-----------------------------------------------------------------------------

void IocpBufferAllocator::clear()
{
    AutoLocker locker(mutex_);

    for (int i = 0; i < items_.getCount(); i++)
        delete[] (char*)items_[i];
    items_.clear();
}

///////////////////////////////////////////////////////////////////////////////
// class IocpPendingCounter

void IocpPendingCounter::inc(PVOID caller, IOCP_TASK_TYPE taskType)
{
    AutoLocker locker(mutex_);

    Items::iterator iter = items_.find(caller);
    if (iter == items_.end())
    {
        CountData Data = {0, 0};
        iter = items_.insert(std::make_pair(caller, Data)).first;
    }

    if (taskType == ITT_SEND)
        iter->second.sendCount++;
    else if (taskType == ITT_RECV)
        iter->second.recvCount++;
}

//-----------------------------------------------------------------------------

void IocpPendingCounter::dec(PVOID caller, IOCP_TASK_TYPE taskType)
{
    AutoLocker locker(mutex_);

    Items::iterator iter = items_.find(caller);
    if (iter != items_.end())
    {
        if (taskType == ITT_SEND)
            iter->second.sendCount--;
        else if (taskType == ITT_RECV)
            iter->second.recvCount--;

        if (iter->second.sendCount <= 0 && iter->second.recvCount <= 0)
            items_.erase(iter);
    }
}

//-----------------------------------------------------------------------------

int IocpPendingCounter::get(PVOID caller)
{
    AutoLocker locker(mutex_);

    Items::iterator iter = items_.find(caller);
    if (iter == items_.end())
        return 0;
    else
        return ise::max(0, iter->second.sendCount + iter->second.recvCount);
}

//-----------------------------------------------------------------------------

int IocpPendingCounter::get(IOCP_TASK_TYPE taskType)
{
    AutoLocker locker(mutex_);

    int result = 0;
    if (taskType == ITT_SEND)
    {
        for (Items::iterator iter = items_.begin(); iter != items_.end(); ++iter)
            result += iter->second.sendCount;
    }
    else if (taskType == ITT_RECV)
    {
        for (Items::iterator iter = items_.begin(); iter != items_.end(); ++iter)
            result += iter->second.recvCount;
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class IocpObject

IocpBufferAllocator IocpObject::bufferAlloc_(sizeof(IocpOverlappedData));
SeqNumberAlloc IocpObject::taskSeqAlloc_(0);
IocpPendingCounter IocpObject::pendingCounter_;

//-----------------------------------------------------------------------------

IocpObject::IocpObject(EventLoop *eventLoop) :
    eventLoop_(eventLoop),
    iocpHandle_(0)
{
    initialize();
}

//-----------------------------------------------------------------------------

IocpObject::~IocpObject()
{
    finalize();
}

//-----------------------------------------------------------------------------

bool IocpObject::associateHandle(SOCKET socketHandle)
{
    HANDLE h = ::CreateIoCompletionPort((HANDLE)socketHandle, iocpHandle_, 0, 0);
    return (h != 0);
}

//-----------------------------------------------------------------------------

bool IocpObject::isComplete(PVOID caller)
{
    return (pendingCounter_.get(caller) <= 0);
}

//-----------------------------------------------------------------------------

void IocpObject::work()
{
    /*
    FROM MSDN:

    If the function dequeues a completion packet for a successful I/O operation from the completion port,
    the return value is nonzero. The function stores information in the variables pointed to by the
    lpNumberOfBytes, lpCompletionKey, and lpOverlapped parameters.

    If *lpOverlapped is NULL and the function does not dequeue a completion packet from the completion port,
    the return value is zero. The function does not store information in the variables pointed to by the
    lpNumberOfBytes and lpCompletionKey parameters. To get extended error information, call GetLastError.
    If the function did not dequeue a completion packet because the wait timed out, GetLastError returns
    WAIT_TIMEOUT.

    If *lpOverlapped is not NULL and the function dequeues a completion packet for a failed I/O operation
    from the completion port, the return value is zero. The function stores information in the variables
    pointed to by lpNumberOfBytes, lpCompletionKey, and lpOverlapped. To get extended error information,
    call GetLastError.

    If a socket handle associated with a completion port is closed, GetQueuedCompletionStatus returns
    ERROR_SUCCESS (0), with *lpOverlapped non-NULL and lpNumberOfBytes equal zero.
    */

    while (true)
    {
        IocpOverlappedData *overlappedPtr = NULL;
        DWORD bytesTransferred = 0, nTemp = 0;
        int errorCode = 0;

        struct AutoFinalizer
        {
        private:
            IocpObject& iocpObject_;
            IocpOverlappedData*& ovPtr_;
        public:
            AutoFinalizer(IocpObject& iocpObject, IocpOverlappedData*& ovPtr) :
                iocpObject_(iocpObject), ovPtr_(ovPtr) {}
            ~AutoFinalizer()
            {
                if (ovPtr_)
                {
                    iocpObject_.pendingCounter_.dec(
                        ovPtr_->taskData.getCaller(),
                        ovPtr_->taskData.getTaskType());
                    iocpObject_.destroyOverlappedData(ovPtr_);
                }
            }
        } finalizer(*this, overlappedPtr);

        // 计算等待超时时间 (毫秒)
        int timeout = eventLoop_->calcLoopWaitTimeout();

        // 等待事件
        BOOL ret = ::GetQueuedCompletionStatus(iocpHandle_, &bytesTransferred, &nTemp,
            (LPOVERLAPPED*)&overlappedPtr, timeout);

        // 处理定时器事件
        if (timeout != TIMEOUT_INFINITE)
            eventLoop_->processExpiredTimers();

        // 处理IO事件
        if (ret)
        {
            if (overlappedPtr != NULL && bytesTransferred == 0)
            {
                errorCode = overlappedPtr->taskData.getErrorCode();
                if (errorCode == 0)
                    errorCode = GetLastError();
                if (errorCode == 0)
                    errorCode = SOCKET_ERROR;
            }
        }
        else
        {
            if (overlappedPtr != NULL)
                errorCode = GetLastError();
            else
            {
                int e = GetLastError();
                if (e != 0 && e != WAIT_TIMEOUT)
                    throwGeneralError();
            }
        }

        if (overlappedPtr != NULL)
        {
            IocpTaskData *taskPtr = &overlappedPtr->taskData;
            taskPtr->bytesTrans_ = bytesTransferred;
            if (taskPtr->errorCode_ == 0)
                taskPtr->errorCode_ = errorCode;

            invokeCallback(*taskPtr);
        }
        else
            break;
    }
}

//-----------------------------------------------------------------------------

void IocpObject::wakeup()
{
    ::PostQueuedCompletionStatus(iocpHandle_, 0, 0, NULL);
}

//-----------------------------------------------------------------------------

void IocpObject::send(SOCKET socketHandle, PVOID buffer, int size, int offset,
    const IocpCallback& callback, PVOID caller, const Context& context)
{
    IocpOverlappedData *ovDataPtr;
    IocpTaskData *taskPtr;
    DWORD numberOfBytesSent;

    pendingCounter_.inc(caller, ITT_SEND);

    ovDataPtr = createOverlappedData(ITT_SEND, (HANDLE)socketHandle, buffer, size,
        offset, callback, caller, context);
    taskPtr = &(ovDataPtr->taskData);

    if (::WSASend(socketHandle, &taskPtr->wsaBuffer_, 1, &numberOfBytesSent, 0,
        (LPWSAOVERLAPPED)ovDataPtr, NULL) == SOCKET_ERROR)
    {
        if (GetLastError() != ERROR_IO_PENDING)
            postError(GetLastError(), ovDataPtr);
    }
}

//-----------------------------------------------------------------------------

void IocpObject::recv(SOCKET socketHandle, PVOID buffer, int size, int offset,
    const IocpCallback& callback, PVOID caller, const Context& context)
{
    IocpOverlappedData *ovDataPtr;
    IocpTaskData *taskPtr;
    DWORD nNumberOfBytesRecvd, flags = 0;

    pendingCounter_.inc(caller, ITT_RECV);

    ovDataPtr = createOverlappedData(ITT_RECV, (HANDLE)socketHandle, buffer, size,
        offset, callback, caller, context);
    taskPtr = &(ovDataPtr->taskData);

    if (::WSARecv(socketHandle, &taskPtr->wsaBuffer_, 1, &nNumberOfBytesRecvd, &flags,
        (LPWSAOVERLAPPED)ovDataPtr, NULL) == SOCKET_ERROR)
    {
        if (GetLastError() != ERROR_IO_PENDING)
            postError(GetLastError(), ovDataPtr);
    }
}

//-----------------------------------------------------------------------------

void IocpObject::initialize()
{
    iocpHandle_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1);
    if (iocpHandle_ == 0)
        throwGeneralError();
}

//-----------------------------------------------------------------------------

void IocpObject::finalize()
{
    CloseHandle(iocpHandle_);
    iocpHandle_ = 0;
}

//-----------------------------------------------------------------------------

void IocpObject::throwGeneralError()
{
    iseThrowException(formatString(SEM_IOCP_ERROR, GetLastError()).c_str());
}

//-----------------------------------------------------------------------------

IocpOverlappedData* IocpObject::createOverlappedData(IOCP_TASK_TYPE taskType,
    HANDLE fileHandle, PVOID buffer, int size, int offset,
    const IocpCallback& callback, PVOID caller, const Context& context)
{
    ISE_ASSERT(buffer != NULL);
    ISE_ASSERT(size >= 0);
    ISE_ASSERT(offset >= 0);
    ISE_ASSERT(offset < size);

    IocpOverlappedData *result = (IocpOverlappedData*)bufferAlloc_.allocBuffer();
    memset(result, 0, sizeof(*result));

    result->taskData.iocpHandle_ = iocpHandle_;
    result->taskData.fileHandle_ = fileHandle;
    result->taskData.taskType_ = taskType;
    result->taskData.taskSeqNum_ = taskSeqAlloc_.allocId();
    result->taskData.caller_ = caller;
    result->taskData.context_ = context;
    result->taskData.entireDataBuf_ = buffer;
    result->taskData.entireDataSize_ = size;
    result->taskData.wsaBuffer_.buf = (char*)buffer + offset;
    result->taskData.wsaBuffer_.len = size - offset;
    result->taskData.callback_ = callback;

    return result;
}

//-----------------------------------------------------------------------------

void IocpObject::destroyOverlappedData(IocpOverlappedData *ovDataPtr)
{
    // 很重要。ovDataPtr->taskData 中的对象需要析构。
    // 比如 taskData.callback_ 中持有 TcpConnection 的 shared_ptr。
    ovDataPtr->~IocpOverlappedData();

    bufferAlloc_.returnBuffer(ovDataPtr);
}

//-----------------------------------------------------------------------------

void IocpObject::postError(int errorCode, IocpOverlappedData *ovDataPtr)
{
    ovDataPtr->taskData.errorCode_ = errorCode;
    ::PostQueuedCompletionStatus(iocpHandle_, 0, 0, LPOVERLAPPED(ovDataPtr));
}

//-----------------------------------------------------------------------------

void IocpObject::invokeCallback(const IocpTaskData& taskData)
{
    const IocpCallback& callback = taskData.getCallback();
    if (callback)
        callback(taskData);
}

#endif  /* ifdef ISE_WINDOWS */

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
