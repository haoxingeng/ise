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

#include "ise_server_tcp.h"
#include "ise_errmsgs.h"
#include "ise_application.h"

using namespace ise;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class CDelimiterPacketMeasurer

bool CDelimiterPacketMeasurer::IsCompletePacket(const char *pData, int nBytes, int& nPacketSize)
{
	for (int i = 0; i < nBytes; i++)
	{
		if (pData[i] == m_chDelimiter)
		{
			nPacketSize = i + 1;
			return true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// class CIoBuffer

CIoBuffer::CIoBuffer() :
	m_Buffer(INITIAL_SIZE),
	m_nReaderIndex(0),
	m_nWriterIndex(0)
{
	// nothing
}

CIoBuffer::~CIoBuffer()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 扩展缓存空间以便可再写进 nMoreBytes 个字节
//-----------------------------------------------------------------------------
void CIoBuffer::MakeSpace(int nMoreBytes)
{
	if (GetWritableBytes() + GetUselessBytes() < nMoreBytes)
	{
		m_Buffer.resize(m_nWriterIndex + nMoreBytes);
	}
	else
	{
		// 将全部可读数据移至缓存开始处
		int nReadableBytes = GetReadableBytes();
		char *pBuffer = GetBufferPtr();
		memmove(pBuffer, pBuffer + m_nReaderIndex, nReadableBytes);
		m_nReaderIndex = 0;
		m_nWriterIndex = m_nReaderIndex + nReadableBytes;

		ISE_ASSERT(nReadableBytes == GetReadableBytes());
	}
}

//-----------------------------------------------------------------------------
// 描述: 向缓存追加写入数据
//-----------------------------------------------------------------------------
void CIoBuffer::Append(const string& str)
{
	Append(str.c_str(), (int)str.length());
}

//-----------------------------------------------------------------------------
// 描述: 向缓存追加写入数据
//-----------------------------------------------------------------------------
void CIoBuffer::Append(const char *pData, int nBytes)
{
	if (pData && nBytes > 0)
	{
		if (GetWritableBytes() < nBytes)
			MakeSpace(nBytes);

		ISE_ASSERT(GetWritableBytes() >= nBytes);

		memmove(GetWriterPtr(), pData, nBytes);
		m_nWriterIndex += nBytes;
	}
}

//-----------------------------------------------------------------------------
// 描述: 向缓存追加 nBytes 个字节并填充为'\0'
//-----------------------------------------------------------------------------
void CIoBuffer::Append(int nBytes)
{
	if (nBytes > 0)
	{
		string str;
		str.resize(nBytes, 0);
		Append(str);
	}
}

//-----------------------------------------------------------------------------
// 描述: 从缓存读取 nBytes 个字节数据
//-----------------------------------------------------------------------------
void CIoBuffer::Retrieve(int nBytes)
{
	if (nBytes > 0)
	{
		ISE_ASSERT(nBytes <= GetReadableBytes());
		m_nReaderIndex += nBytes;
	}
}

//-----------------------------------------------------------------------------
// 描述: 从缓存读取全部可读数据并存入 str 中
//-----------------------------------------------------------------------------
void CIoBuffer::RetrieveAll(string& str)
{
	if (GetReadableBytes() > 0)
		str.assign(Peek(), GetReadableBytes());
	else
		str.clear();

	RetrieveAll();
}

//-----------------------------------------------------------------------------
// 描述: 从缓存读取全部可读数据
//-----------------------------------------------------------------------------
void CIoBuffer::RetrieveAll()
{
	m_nReaderIndex = 0;
	m_nWriterIndex = 0;
}

///////////////////////////////////////////////////////////////////////////////
// class CTcpEventLoopThread

CTcpEventLoopThread::CTcpEventLoopThread(CBaseTcpEventLoop& EventLoop) :
	m_EventLoop(EventLoop)
{
	SetFreeOnTerminate(false);
}

//-----------------------------------------------------------------------------

void CTcpEventLoopThread::Execute()
{
	m_EventLoop.ExecuteLoop(this);
}

///////////////////////////////////////////////////////////////////////////////
// class CBaseTcpEventLoop

CBaseTcpEventLoop::CBaseTcpEventLoop() :
	m_pThread(NULL),
	m_AcceptedConnList(true, false)
{
	// nothing
}

CBaseTcpEventLoop::~CBaseTcpEventLoop()
{
	Stop(false, true);
}

//-----------------------------------------------------------------------------
// 描述: 处理由TcpServer新产生的连接
//-----------------------------------------------------------------------------
void CBaseTcpEventLoop::ProcessAcceptedConnList()
{
	CTcpConnection *pConnection;
	while ((pConnection = m_AcceptedConnList.Extract(0)))
		pIseBusiness->OnTcpConnection(pConnection);
}

//-----------------------------------------------------------------------------
// 描述: 启动工作线程
//-----------------------------------------------------------------------------
void CBaseTcpEventLoop::Start()
{
	if (!m_pThread)
	{
		m_pThread = new CTcpEventLoopThread(*this);
		m_pThread->Run();
	}
}

//-----------------------------------------------------------------------------
// 描述: 停止工作线程
//-----------------------------------------------------------------------------
void CBaseTcpEventLoop::Stop(bool bForce, bool bWaitFor)
{
	if (m_pThread && m_pThread->IsRunning())
	{
		if (bForce)
		{
			m_pThread->Kill();
			m_pThread = NULL;
			bWaitFor = false;
		}
		else
		{
			m_pThread->Terminate();
			WakeupLoop();
		}

		if (bWaitFor)
		{
			m_pThread->WaitFor();
			delete m_pThread;
			m_pThread = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 判断工作线程当前是否正在运行
//-----------------------------------------------------------------------------
bool CBaseTcpEventLoop::IsRunning()
{
	return (m_pThread != NULL && m_pThread->IsRunning());
}

//-----------------------------------------------------------------------------
// 描述: 将指定连接注册到此 EventLoop 中
//-----------------------------------------------------------------------------
void CBaseTcpEventLoop::AddConnection(CTcpConnection *pConnection)
{
	RegisterConnection(pConnection);
	m_AcceptedConnList.Add(pConnection);
	WakeupLoop();
}

//-----------------------------------------------------------------------------
// 描述: 将指定连接从此 EventLoop 中注销
//-----------------------------------------------------------------------------
void CBaseTcpEventLoop::RemoveConnection(CTcpConnection *pConnection)
{
	UnregisterConnection(pConnection);
}

///////////////////////////////////////////////////////////////////////////////
// class CTcpEventLoopList

CTcpEventLoopList::CTcpEventLoopList(int nLoopCount) :
	m_Items(false, true)
{
	SetCount(nLoopCount);
}

CTcpEventLoopList::~CTcpEventLoopList()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 设置 EventLoop 的个数
//-----------------------------------------------------------------------------
void CTcpEventLoopList::SetCount(int nCount)
{
	nCount = EnsureRange(nCount, 1, (int)MAX_LOOP_COUNT);

	for (int i = 0; i < nCount; i++)
		m_Items.Add(new CTcpEventLoop());
}

//-----------------------------------------------------------------------------
// 描述: 启动全部 EventLoop 的工作线程
//-----------------------------------------------------------------------------
void CTcpEventLoopList::Start()
{
	for (int i = 0; i < m_Items.GetCount(); i++)
		m_Items[i]->Start();
}

//-----------------------------------------------------------------------------
// 描述: 停止全部 EventLoop 的工作线程
//-----------------------------------------------------------------------------
void CTcpEventLoopList::Stop()
{
	const int MAX_WAIT_FOR_SECS = 5;    // (秒)
	const double SLEEP_INTERVAL = 0.5;  // (秒)

	// 通知停止
	for (int i = 0; i < m_Items.GetCount(); i++)
		m_Items[i]->Stop(false, false);

	// 等待停止
	double nWaitSecs = 0;
	while (nWaitSecs < MAX_WAIT_FOR_SECS)
	{
		int nRunningCount = 0;
		for (int i = 0; i < m_Items.GetCount(); i++)
			if (m_Items[i]->IsRunning()) nRunningCount++;

		if (nRunningCount == 0) break;

		SleepSec(SLEEP_INTERVAL, true);
		nWaitSecs += SLEEP_INTERVAL;
	}

	// 强行停止
	for (int i = 0; i < m_Items.GetCount(); i++)
		m_Items[i]->Stop(true, true);
}

///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_WIN32

///////////////////////////////////////////////////////////////////////////////

CTcpConnection::CTcpConnection(CTcpServer *pTcpServer,
	SOCKET nSocketHandle, const CPeerAddress& PeerAddr) :
		CBaseTcpConnection(nSocketHandle, PeerAddr),
		m_pTcpServer(pTcpServer),
		m_pEventLoop(NULL),
		m_bSending(false),
		m_bRecving(false),
		m_nBytesSent(0),
		m_nBytesRecved(0)
{
	// nothing
}

CTcpConnection::~CTcpConnection()
{
	SetEventLoop(NULL);
}

//-----------------------------------------------------------------------------

void CTcpConnection::TrySend()
{
	if (m_bSending) return;

	int nReadableBytes = m_SendBuffer.GetReadableBytes();
	if (nReadableBytes > 0)
	{
		const int MAX_SEND_SIZE = 1024*32;

		const char *pBuffer = m_SendBuffer.Peek();
		int nSendSize = Min(nReadableBytes, MAX_SEND_SIZE);

		m_bSending = true;
		m_pEventLoop->GetIocpObject()->Send(GetSocket().GetHandle(), 
			(PVOID)pBuffer, nSendSize, 0,
			IOCP_CALLBACK_DEF(OnIocpCallBack, this), this, EMPTY_PARAMS);
	}
}

//-----------------------------------------------------------------------------

void CTcpConnection::TryRecv()
{
	if (m_bRecving) return;

	if (!m_RecvTaskQueue.empty())
	{
		const int MAX_RECV_SIZE = 1024*8;

		m_bRecving = true;
		m_RecvBuffer.Append(MAX_RECV_SIZE);
		const char *pBuffer = m_RecvBuffer.Peek() + m_nBytesRecved;

		m_pEventLoop->GetIocpObject()->Recv(GetSocket().GetHandle(), 
			(PVOID)pBuffer, MAX_RECV_SIZE, 0,
			IOCP_CALLBACK_DEF(OnIocpCallBack, this), this, EMPTY_PARAMS);
	}
}

//-----------------------------------------------------------------------------
// 描述: 发生错误
//-----------------------------------------------------------------------------
void CTcpConnection::ErrorOccurred()
{
	SetEventLoop(NULL);
	pIseBusiness->OnTcpError(this);
	delete this;
}

//-----------------------------------------------------------------------------
// 描述: 设置此连接从属于哪个 EventLoop
//-----------------------------------------------------------------------------
void CTcpConnection::SetEventLoop(CTcpEventLoop *pEventLoop)
{
	if (m_pEventLoop)
	{
		m_pEventLoop->RemoveConnection(this);
		m_pEventLoop = NULL;
	}

	if (pEventLoop)
	{
		m_pEventLoop = pEventLoop;
		pEventLoop->AddConnection(this);
	}
}

//-----------------------------------------------------------------------------

void CTcpConnection::OnIocpCallBack(const CIocpTaskData& TaskData, PVOID pParam)
{
	CTcpConnection *pThis = (CTcpConnection*)pParam;

	if (TaskData.GetErrorCode() == 0)
	{
		switch (TaskData.GetTaskType())
		{
		case ITT_SEND:
			pThis->OnSendCallBack(TaskData);
			break;
		case ITT_RECV:
			pThis->OnRecvCallBack(TaskData);
			break;
		}
	}
	else
	{
		pThis->ErrorOccurred();
	}
}

//-----------------------------------------------------------------------------

void CTcpConnection::OnSendCallBack(const CIocpTaskData& TaskData)
{
	ISE_ASSERT(TaskData.GetErrorCode() == 0);

	if (TaskData.GetBytesTrans() < TaskData.GetDataSize())
	{
		m_pEventLoop->GetIocpObject()->Send(
			(SOCKET)TaskData.GetFileHandle(),
			TaskData.GetEntireDataBuf(),
			TaskData.GetEntireDataSize(),
			TaskData.GetDataBuf() - TaskData.GetEntireDataBuf() + TaskData.GetBytesTrans(),
			TaskData.GetCallBack(), TaskData.GetCaller(), TaskData.GetParams());
	}
	else
	{
		m_bSending = false;
		m_SendBuffer.Retrieve(TaskData.GetEntireDataSize());
	}

	m_nBytesSent += TaskData.GetBytesTrans();

	while (!m_SendTaskQueue.empty())
	{
		const SEND_TASK& Task = m_SendTaskQueue.front();
		if (m_nBytesSent >= Task.nBytes)
		{
			m_nBytesSent -= Task.nBytes;
			pIseBusiness->OnTcpSendComplete(this, Task.Params);
			m_SendTaskQueue.pop_front();
		}
		else
			break;
	}

	if (!m_SendTaskQueue.empty())
		TrySend();
}

//-----------------------------------------------------------------------------

void CTcpConnection::OnRecvCallBack(const CIocpTaskData& TaskData)
{
	ISE_ASSERT(TaskData.GetErrorCode() == 0);

	if (TaskData.GetBytesTrans() < TaskData.GetDataSize())
	{
		m_pEventLoop->GetIocpObject()->Recv(
			(SOCKET)TaskData.GetFileHandle(),
			TaskData.GetEntireDataBuf(),
			TaskData.GetEntireDataSize(),
			TaskData.GetDataBuf() - TaskData.GetEntireDataBuf() + TaskData.GetBytesTrans(),
			TaskData.GetCallBack(), TaskData.GetCaller(), TaskData.GetParams());
	}
	else
	{
		m_bRecving = false;
	}

	m_nBytesRecved += TaskData.GetBytesTrans();

	while (!m_RecvTaskQueue.empty())
	{
		RECV_TASK& Task = m_RecvTaskQueue.front();
		const char *pBuffer = m_RecvBuffer.Peek();
		int nPacketSize = 0;

		if (m_nBytesRecved > 0 &&
			Task.pPacketMeasurer->IsCompletePacket(pBuffer, m_nBytesRecved, nPacketSize) &&
			nPacketSize > 0)
		{
			m_nBytesRecved -= nPacketSize;
			pIseBusiness->OnTcpRecvComplete(this, (void*)pBuffer, nPacketSize, Task.Params);
			m_RecvTaskQueue.pop_front();
			m_RecvBuffer.Retrieve(nPacketSize);
		}
		else
			break;
	}

	if (!m_RecvTaskQueue.empty())
		TryRecv();
}

//-----------------------------------------------------------------------------
// 描述: 断开连接
//-----------------------------------------------------------------------------
void CTcpConnection::DoDisconnect()
{
	SetEventLoop(NULL);
	CBaseTcpConnection::DoDisconnect();
}

//-----------------------------------------------------------------------------
// 描述: 提交一个发送任务
//-----------------------------------------------------------------------------
void CTcpConnection::PostSendTask(char *pBuffer, int nSize, const CCustomParams& Params)
{
	if (!pBuffer || nSize <= 0) return;

	m_SendBuffer.Append(pBuffer, nSize);

	SEND_TASK Task;
	Task.nBytes = nSize;
	Task.Params = Params;

	m_SendTaskQueue.push_back(Task);

	TrySend();
}

//-----------------------------------------------------------------------------
// 描述: 提交一个接收任务
//-----------------------------------------------------------------------------
void CTcpConnection::PostRecvTask(CPacketMeasurer *pPacketMeasurer, const CCustomParams& Params)
{
	if (!pPacketMeasurer) return;

	RECV_TASK Task;
	Task.pPacketMeasurer = pPacketMeasurer;
	Task.Params = Params;

	m_RecvTaskQueue.push_back(Task);

	TryRecv();
}

///////////////////////////////////////////////////////////////////////////////
// class CIocpTaskData

CIocpTaskData::CIocpTaskData() :
	m_hIocpHandle(INVALID_HANDLE_VALUE),
	m_hFileHandle(INVALID_HANDLE_VALUE),
	m_nTaskType((IOCP_TASK_TYPE)0),
	m_nTaskSeqNum(0),
	m_pCaller(0),
	m_pEntireDataBuf(0),
	m_nEntireDataSize(0),
	m_nBytesTrans(0),
	m_nErrorCode(0)
{
	m_WSABuffer.buf = NULL;
	m_WSABuffer.len = 0;
}

///////////////////////////////////////////////////////////////////////////////
// class CIocpBufferAllocator

CIocpBufferAllocator::CIocpBufferAllocator(int nBufferSize) :
	m_nBufferSize(nBufferSize),
	m_nUsedCount(0)
{
	// nothing
}

//-----------------------------------------------------------------------------

CIocpBufferAllocator::~CIocpBufferAllocator()
{
	Clear();
}

//-----------------------------------------------------------------------------

void CIocpBufferAllocator::Clear()
{
	CAutoLocker Locker(m_Lock);

	for (int i = 0; i < m_Items.GetCount(); i++)
		delete[] (char*)m_Items[i];
	m_Items.Clear();
}

//-----------------------------------------------------------------------------

PVOID CIocpBufferAllocator::AllocBuffer()
{
	CAutoLocker Locker(m_Lock);
	PVOID pResult;

	if (!m_Items.IsEmpty())
	{
		pResult = m_Items.Last();
		m_Items.Delete(m_Items.GetCount() - 1);
	}
	else
	{
		pResult = new char[m_nBufferSize];
		if (pResult == NULL)
			IseThrowMemoryException();
	}

	m_nUsedCount++;
	return pResult;
}

//-----------------------------------------------------------------------------

void CIocpBufferAllocator::ReturnBuffer(PVOID pBuffer)
{
	CAutoLocker Locker(m_Lock);

	if (pBuffer != NULL && m_Items.IndexOf(pBuffer) == -1)
	{
		m_Items.Add(pBuffer);
		m_nUsedCount--;
	}
}

///////////////////////////////////////////////////////////////////////////////
// class CIocpPendingCounter

void CIocpPendingCounter::Inc(PVOID pCaller, IOCP_TASK_TYPE nTaskType)
{
	CAutoLocker Locker(m_Lock);

	ITEMS::iterator iter = m_Items.find(pCaller);
	if (iter == m_Items.end())
	{
		COUNT_DATA Data = {0, 0};
		iter = m_Items.insert(std::make_pair(pCaller, Data)).first;
	}

	if (nTaskType == ITT_SEND)
		iter->second.nSendCount++;
	else if (nTaskType == ITT_RECV)
		iter->second.nRecvCount++;
}

//-----------------------------------------------------------------------------

void CIocpPendingCounter::Dec(PVOID pCaller, IOCP_TASK_TYPE nTaskType)
{
	CAutoLocker Locker(m_Lock);

	ITEMS::iterator iter = m_Items.find(pCaller);
	if (iter != m_Items.end())
	{
		if (nTaskType == ITT_SEND)
			iter->second.nSendCount--;
		else if (nTaskType == ITT_RECV)
			iter->second.nRecvCount--;

		if (iter->second.nSendCount <= 0 && iter->second.nRecvCount <= 0)
			m_Items.erase(iter);
	}
}

//-----------------------------------------------------------------------------

int CIocpPendingCounter::Get(PVOID pCaller)
{
	CAutoLocker Locker(m_Lock);

	ITEMS::iterator iter = m_Items.find(pCaller);
	if (iter == m_Items.end())
		return 0;
	else
		return Max(0, iter->second.nSendCount + iter->second.nRecvCount);
}

//-----------------------------------------------------------------------------

int CIocpPendingCounter::Get(IOCP_TASK_TYPE nTaskType)
{
	CAutoLocker Locker(m_Lock);

	int nResult = 0;
	if (nTaskType == ITT_SEND)
	{
		for (ITEMS::iterator iter = m_Items.begin(); iter != m_Items.end(); ++iter)
			nResult += iter->second.nSendCount;
	}
	else if (nTaskType == ITT_RECV)
	{
		for (ITEMS::iterator iter = m_Items.begin(); iter != m_Items.end(); ++iter)
			nResult += iter->second.nRecvCount;
	}

	return nResult;
}

///////////////////////////////////////////////////////////////////////////////
// class CIocpObject

CIocpBufferAllocator CIocpObject::m_BufferAlloc(sizeof(CIocpOverlappedData));
CSeqNumberAlloc CIocpObject::m_TaskSeqAlloc(0);
CIocpPendingCounter CIocpObject::m_PendingCounter;

//-----------------------------------------------------------------------------

CIocpObject::CIocpObject() :
	m_hIocpHandle(0)
{
	Initialize();
}

//-----------------------------------------------------------------------------

CIocpObject::~CIocpObject()
{
	Finalize();
}

//-----------------------------------------------------------------------------

void CIocpObject::Initialize()
{
	m_hIocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1);
	if (m_hIocpHandle == 0)
		ThrowGeneralError();
}

//-----------------------------------------------------------------------------

void CIocpObject::Finalize()
{
	CloseHandle(m_hIocpHandle);
	m_hIocpHandle = 0;
}

//-----------------------------------------------------------------------------

void CIocpObject::ThrowGeneralError()
{
	IseThrowException(FormatString(SEM_IOCP_ERROR, GetLastError()).c_str());
}

//-----------------------------------------------------------------------------

CIocpOverlappedData* CIocpObject::CreateOverlappedData(IOCP_TASK_TYPE nTaskType,
	HANDLE hFileHandle, PVOID pBuffer, int nSize, int nOffset,
	const IOCP_CALLBACK_DEF& CallBackDef, PVOID pCaller, const CCustomParams& Params)
{
	ISE_ASSERT(pBuffer != NULL);
	ISE_ASSERT(nSize >= 0);
	ISE_ASSERT(nOffset >= 0);
	ISE_ASSERT(nOffset < nSize);

	CIocpOverlappedData *pResult = (CIocpOverlappedData*)m_BufferAlloc.AllocBuffer();
	memset(pResult, 0, sizeof(*pResult));

	pResult->TaskData.m_hIocpHandle = m_hIocpHandle;
	pResult->TaskData.m_hFileHandle = hFileHandle;
	pResult->TaskData.m_nTaskType = nTaskType;
	pResult->TaskData.m_nTaskSeqNum = m_TaskSeqAlloc.AllocId();
	pResult->TaskData.m_CallBack = CallBackDef;
	pResult->TaskData.m_pCaller = pCaller;
	pResult->TaskData.m_Params = Params;
	pResult->TaskData.m_pEntireDataBuf = pBuffer;
	pResult->TaskData.m_nEntireDataSize = nSize;
	pResult->TaskData.m_WSABuffer.buf = (char*)pBuffer + nOffset;
	pResult->TaskData.m_WSABuffer.len = nSize - nOffset;

	return pResult;
}

//-----------------------------------------------------------------------------

void CIocpObject::DestroyOverlappedData(CIocpOverlappedData *pOvDataPtr)
{
	m_BufferAlloc.ReturnBuffer(pOvDataPtr);
}

//-----------------------------------------------------------------------------

void CIocpObject::PostError(int nErrorCode, CIocpOverlappedData *pOvDataPtr)
{
	pOvDataPtr->TaskData.m_nErrorCode = nErrorCode;
	::PostQueuedCompletionStatus(m_hIocpHandle, 0, 0, LPOVERLAPPED(pOvDataPtr));
}

//-----------------------------------------------------------------------------

void CIocpObject::InvokeCallBack(const CIocpTaskData& TaskData)
{
	const IOCP_CALLBACK_DEF& CallBackDef = TaskData.GetCallBack();
	if (CallBackDef.pProc != NULL)
		CallBackDef.pProc(TaskData, CallBackDef.pParam);
}

//-----------------------------------------------------------------------------

bool CIocpObject::AssociateHandle(SOCKET hSocketHandle)
{
	HANDLE h = ::CreateIoCompletionPort((HANDLE)hSocketHandle, m_hIocpHandle, 0, 0);
	return (h != 0);
}

//-----------------------------------------------------------------------------

bool CIocpObject::IsComplete(PVOID pCaller)
{
	return (m_PendingCounter.Get(pCaller) <= 0);
}

//-----------------------------------------------------------------------------

void CIocpObject::Work()
{
	const int IOCP_WAIT_TIMEOUT = 1000*1;  // ms

	CIocpOverlappedData *pOverlappedPtr = NULL;
	DWORD nBytesTransferred = 0, nTemp = 0;
	int nErrorCode = 0;

	struct CAutoFinalizer
	{
	private:
		CIocpObject& m_IocpObject;
		CIocpOverlappedData*& m_pOvPtr;
	public:
		CAutoFinalizer(CIocpObject& IocpObject, CIocpOverlappedData*& pOvPtr) :
			m_IocpObject(IocpObject), m_pOvPtr(pOvPtr) {}
		~CAutoFinalizer()
		{
			if (m_pOvPtr)
			{
				m_IocpObject.m_PendingCounter.Dec(
					m_pOvPtr->TaskData.GetCaller(),
					m_pOvPtr->TaskData.GetTaskType());
				m_IocpObject.DestroyOverlappedData(m_pOvPtr);
			}
		}
	} AutoFinalizer(*this, pOverlappedPtr);

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

	if (::GetQueuedCompletionStatus(m_hIocpHandle, &nBytesTransferred, &nTemp,
		(LPOVERLAPPED*)&pOverlappedPtr, IOCP_WAIT_TIMEOUT))
	{
		if (pOverlappedPtr != NULL && nBytesTransferred == 0)
		{
			nErrorCode = pOverlappedPtr->TaskData.GetErrorCode();
			if (nErrorCode == 0)
				nErrorCode = GetLastError();
			if (nErrorCode == 0)
				nErrorCode = SOCKET_ERROR;
		}
	}
	else
	{
		if (pOverlappedPtr != NULL)
			nErrorCode = GetLastError();
		else
		{
			if (GetLastError() != WAIT_TIMEOUT)
				ThrowGeneralError();
		}
	}

	if (pOverlappedPtr != NULL)
	{
		CIocpTaskData *pTaskPtr = &pOverlappedPtr->TaskData;
		pTaskPtr->m_nBytesTrans = nBytesTransferred;
		if (pTaskPtr->m_nErrorCode == 0)
			pTaskPtr->m_nErrorCode = nErrorCode;

		InvokeCallBack(*pTaskPtr);
	}
}

//-----------------------------------------------------------------------------

void CIocpObject::Wakeup()
{
	::PostQueuedCompletionStatus(m_hIocpHandle, 0, 0, NULL);
}

//-----------------------------------------------------------------------------

void CIocpObject::Send(SOCKET hSocketHandle, PVOID pBuffer, int nSize, int nOffset,
	const IOCP_CALLBACK_DEF& CallBackDef, PVOID pCaller, const CCustomParams& Params)
{
	CIocpOverlappedData *pOvDataPtr;
	CIocpTaskData *pTaskPtr;
	DWORD nNumberOfBytesSent;

	m_PendingCounter.Inc(pCaller, ITT_SEND);

	pOvDataPtr = CreateOverlappedData(ITT_SEND, (HANDLE)hSocketHandle, pBuffer, nSize,
		nOffset, CallBackDef, pCaller, Params);
	pTaskPtr = &(pOvDataPtr->TaskData);

	if (::WSASend(hSocketHandle, &pTaskPtr->m_WSABuffer, 1, &nNumberOfBytesSent, 0,
		(LPWSAOVERLAPPED)pOvDataPtr, NULL) == SOCKET_ERROR)
	{
		if (GetLastError() != ERROR_IO_PENDING)
			PostError(GetLastError(), pOvDataPtr);
	}
}

//-----------------------------------------------------------------------------

void CIocpObject::Recv(SOCKET hSocketHandle, PVOID pBuffer, int nSize, int nOffset,
	const IOCP_CALLBACK_DEF& CallBackDef, PVOID pCaller, const CCustomParams& Params)
{
	CIocpOverlappedData *pOvDataPtr;
	CIocpTaskData *pTaskPtr;
	DWORD nNumberOfBytesRecvd, nFlags = 0;

	m_PendingCounter.Inc(pCaller, ITT_RECV);

	pOvDataPtr = CreateOverlappedData(ITT_RECV, (HANDLE)hSocketHandle, pBuffer, nSize,
		nOffset, CallBackDef, pCaller, Params);
	pTaskPtr = &(pOvDataPtr->TaskData);

	if (::WSARecv(hSocketHandle, &pTaskPtr->m_WSABuffer, 1, &nNumberOfBytesRecvd, &nFlags,
		(LPWSAOVERLAPPED)pOvDataPtr, NULL) == SOCKET_ERROR)
	{
		if (GetLastError() != ERROR_IO_PENDING)
			PostError(GetLastError(), pOvDataPtr);
	}
}

///////////////////////////////////////////////////////////////////////////////
// class CTcpEventLoop

CTcpEventLoop::CTcpEventLoop()
{
	m_pIocpObject = new CIocpObject();
}

CTcpEventLoop::~CTcpEventLoop()
{
	Stop(false, true);
	delete m_pIocpObject;
}

//-----------------------------------------------------------------------------
// 描述: 执行事件循环
//-----------------------------------------------------------------------------
void CTcpEventLoop::ExecuteLoop(CThread *pThread)
{
	while (!pThread->GetTerminated())
	{
		m_pIocpObject->Work();
		ProcessAcceptedConnList();
	}
}

//-----------------------------------------------------------------------------
// 描述: 唤醒事件循环中的阻塞操作
//-----------------------------------------------------------------------------
void CTcpEventLoop::WakeupLoop()
{
	m_pIocpObject->Wakeup();
}

//-----------------------------------------------------------------------------
// 描述: 将新连接注册到事件循环中
//-----------------------------------------------------------------------------
void CTcpEventLoop::RegisterConnection(CTcpConnection *pConnection)
{
	m_pIocpObject->AssociateHandle(pConnection->GetSocket().GetHandle());
}

//-----------------------------------------------------------------------------
// 描述: 从事件循环中注销连接
//-----------------------------------------------------------------------------
void CTcpEventLoop::UnregisterConnection(CTcpConnection *pConnection)
{
	// nothing
}

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_WIN32 */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_LINUX

///////////////////////////////////////////////////////////////////////////////
// class CTcpConnection

CTcpConnection::CTcpConnection(CTcpServer *pTcpServer,
	SOCKET nSocketHandle, const CPeerAddress& PeerAddr) :
		CBaseTcpConnection(nSocketHandle, PeerAddr),
		m_pTcpServer(pTcpServer),
		m_pEventLoop(NULL),
		m_nBytesSent(0),
		m_bEnableSend(false),
		m_bEnableRecv(false)
{
	// nothing
}

CTcpConnection::~CTcpConnection()
{
	SetEventLoop(NULL);
}

//-----------------------------------------------------------------------------
// 描述: 设置“是否监视可发送事件”
//-----------------------------------------------------------------------------
void CTcpConnection::SetSendEnabled(bool bEnabled)
{
	ISE_ASSERT(m_pEventLoop != NULL);

	m_bEnableSend = bEnabled;
	m_pEventLoop->UpdateConnection(this, m_bEnableSend, m_bEnableRecv);
}

//-----------------------------------------------------------------------------
// 描述: 设置“是否监视可接收事件”
//-----------------------------------------------------------------------------
void CTcpConnection::SetRecvEnabled(bool bEnabled)
{
	ISE_ASSERT(m_pEventLoop != NULL);

	m_bEnableRecv = bEnabled;
	m_pEventLoop->UpdateConnection(this, m_bEnableSend, m_bEnableRecv);
}

//-----------------------------------------------------------------------------
// 描述: 当“可发送”事件到来时，尝试发送数据
//-----------------------------------------------------------------------------
void CTcpConnection::TrySend()
{
	int nReadableBytes = m_SendBuffer.GetReadableBytes();
	if (nReadableBytes <= 0)
	{
		SetSendEnabled(false);
		return;
	}

	const char *pBuffer = m_SendBuffer.Peek();
	int nBytesSent = SendBuffer((void*)pBuffer, nReadableBytes, false);
	if (nBytesSent < 0)
	{
		ErrorOccurred();
		return;
	}

	if (nBytesSent > 0)
	{
		m_SendBuffer.Retrieve(nBytesSent);
		m_nBytesSent += nBytesSent;

		while (!m_SendTaskQueue.empty())
		{
			const SEND_TASK& Task = m_SendTaskQueue.front();
			if (m_nBytesSent >= Task.nBytes)
			{
				m_nBytesSent -= Task.nBytes;
				pIseBusiness->OnTcpSendComplete(this, Task.Params);
				m_SendTaskQueue.pop_front();
			}
			else
				break;
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 当“可接收”事件到来时，尝试接收数据
//-----------------------------------------------------------------------------
void CTcpConnection::TryRecv()
{
	if (m_RecvTaskQueue.empty())
	{
		SetRecvEnabled(false);
		return;
	}

	const int BUFFER_SIZE = 1024*64;
	char cBuffer[BUFFER_SIZE];

	int nBytesRecved = RecvBuffer(cBuffer, BUFFER_SIZE, false);
	if (nBytesRecved < 0)
	{
		ErrorOccurred();
		return;
	}

	if (nBytesRecved > 0)
		m_RecvBuffer.Append(cBuffer, nBytesRecved);

	while (!m_RecvTaskQueue.empty())
	{
		RECV_TASK& Task = m_RecvTaskQueue.front();
		const char *pBuffer = m_RecvBuffer.Peek();
		int nReadableBytes = m_RecvBuffer.GetReadableBytes();
		int nPacketSize = 0;

		if (nReadableBytes > 0 &&
			Task.pPacketMeasurer->IsCompletePacket(pBuffer, nReadableBytes, nPacketSize) &&
			nPacketSize > 0)
		{
			pIseBusiness->OnTcpRecvComplete(this, (void*)pBuffer, nPacketSize, Task.Params);
			m_RecvTaskQueue.pop_front();
			m_RecvBuffer.Retrieve(nPacketSize);
		}
		else
			break;
	}
}

//-----------------------------------------------------------------------------
// 描述: 发生错误
//-----------------------------------------------------------------------------
void CTcpConnection::ErrorOccurred()
{
	SetEventLoop(NULL);
	pIseBusiness->OnTcpError(this);
	delete this;
}

//-----------------------------------------------------------------------------
// 描述: 设置此连接从属于哪个 EventLoop
//-----------------------------------------------------------------------------
void CTcpConnection::SetEventLoop(CTcpEventLoop *pEventLoop)
{
	if (m_pEventLoop)
	{
		m_pEventLoop->RemoveConnection(this);
		m_pEventLoop = NULL;
	}

	if (pEventLoop)
	{
		m_pEventLoop = pEventLoop;
		pEventLoop->AddConnection(this);
	}
}

//-----------------------------------------------------------------------------
// 描述: 断开连接
//-----------------------------------------------------------------------------
void CTcpConnection::DoDisconnect()
{
	SetEventLoop(NULL);
	CBaseTcpConnection::DoDisconnect();
}

//-----------------------------------------------------------------------------
// 描述: 提交一个发送任务
//-----------------------------------------------------------------------------
void CTcpConnection::PostSendTask(char *pBuffer, int nSize, const CCustomParams& Params)
{
	if (!pBuffer || nSize <= 0) return;

	m_SendBuffer.Append(pBuffer, nSize);

	SEND_TASK Task;
	Task.nBytes = nSize;
	Task.Params = Params;

	m_SendTaskQueue.push_back(Task);

	if (!m_bEnableSend)
		SetSendEnabled(true);
}

//-----------------------------------------------------------------------------
// 描述: 提交一个接收任务
//-----------------------------------------------------------------------------
void CTcpConnection::PostRecvTask(CPacketMeasurer *pPacketMeasurer, const CCustomParams& Params)
{
	if (!pPacketMeasurer) return;

	RECV_TASK Task;
	Task.pPacketMeasurer = pPacketMeasurer;
	Task.Params = Params;

	m_RecvTaskQueue.push_back(Task);

	if (!m_bEnableRecv)
		SetRecvEnabled(true);
}

///////////////////////////////////////////////////////////////////////////////
// class CEpollObject

CEpollObject::CEpollObject()
{
	m_Events.resize(INITIAL_EVENT_SIZE);
	CreateEpoll();
	CreatePipe();
}

CEpollObject::~CEpollObject()
{
	DestroyPipe();
	DestroyEpoll();
}

//-----------------------------------------------------------------------------

void CEpollObject::CreateEpoll()
{
	m_nEpollFd = ::epoll_create(1024);
	if (m_nEpollFd < 0)
		Logger().WriteStr(SEM_CREATE_EPOLL_ERROR);
}

//-----------------------------------------------------------------------------

void CEpollObject::DestroyEpoll()
{
	if (m_nEpollFd > 0)
		::close(m_nEpollFd);
}

//-----------------------------------------------------------------------------

void CEpollObject::CreatePipe()
{
	// m_PipeFds[0] for reading, m_PipeFds[1] for writing.
	memset(m_PipeFds, 0, sizeof(m_PipeFds));
	if (::pipe(m_PipeFds) == 0)
		EpollControl(EPOLL_CTL_ADD, NULL, m_PipeFds[0], false, true);
	else
		Logger().WriteStr(SEM_CREATE_PIPE_ERROR);
}

//-----------------------------------------------------------------------------

void CEpollObject::DestroyPipe()
{
	EpollControl(EPOLL_CTL_DEL, NULL, m_PipeFds[0], false, false);

	if (m_PipeFds[0]) close(m_PipeFds[0]);
	if (m_PipeFds[1]) close(m_PipeFds[1]);

	memset(m_PipeFds, 0, sizeof(m_PipeFds));
}

//-----------------------------------------------------------------------------

void CEpollObject::EpollControl(int nOperation, void *pParam, int nHandle,
	bool bEnableSend, bool bEnableRecv)
{
	// 注: 采用 Level Triggered (LT, 也称 "电平触发") 模式

	struct epoll_event event;
	memset(&event, 0, sizeof(event));
	event.data.ptr = pParam;
	if (bEnableSend)
		event.events |= EPOLLOUT;
	if (bEnableRecv)
		event.events |= (EPOLLIN | EPOLLPRI);

	if (::epoll_ctl(m_nEpollFd, nOperation, nHandle, &event) < 0)
	{
		Logger().WriteFmt(SEM_EPOLL_CTRL_ERROR, nOperation);
	}
}

//-----------------------------------------------------------------------------
// 描述: 处理管道事件
//-----------------------------------------------------------------------------
void CEpollObject::ProcessPipeEvent()
{
	BYTE val;
	::read(m_PipeFds[0], &val, sizeof(BYTE));
}

//-----------------------------------------------------------------------------
// 描述: 处理 EPoll 轮循后的事件
//-----------------------------------------------------------------------------
void CEpollObject::ProcessEvents(int nEventCount)
{
	for (int i = 0; i < nEventCount; i++)
	{
		epoll_event& ev = m_Events[i];
		if (ev.data.ptr == NULL)  // for pipe
		{
			ProcessPipeEvent();
		}
		else
		{
			CTcpConnection *pConnection = (CTcpConnection*)ev.data.ptr;
			EVENT_TYPE nEventType = ET_NONE;

			if (ev.events & (EPOLLERR | EPOLLHUP))
				nEventType = ET_ERROR;
			else if (ev.events & (EPOLLIN | EPOLLPRI))
				nEventType = ET_ALLOW_RECV;
			else if (ev.events & EPOLLOUT)
				nEventType = ET_ALLOW_SEND;

			if (nEventType != ET_NONE && m_OnNotifyEvent.pProc)
				m_OnNotifyEvent.pProc(m_OnNotifyEvent.pParam, pConnection, nEventType);
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 执行一次 EPoll 轮循
//-----------------------------------------------------------------------------
void CEpollObject::Poll()
{
	const int EPOLL_WAIT_TIMEOUT = 1000*1;  // ms

	int nEventCount = ::epoll_wait(m_nEpollFd, &m_Events[0], (int)m_Events.size(), EPOLL_WAIT_TIMEOUT);
	if (nEventCount > 0)
	{
		ProcessEvents(nEventCount);

		if (nEventCount == (int)m_Events.size())
			m_Events.resize(nEventCount * 2);
	}
	else if (nEventCount < 0)
	{
		Logger().WriteStr(SEM_EPOLL_WAIT_ERROR);
	}
}

//-----------------------------------------------------------------------------
// 描述: 唤醒正在阻塞的 Poll() 函数
//-----------------------------------------------------------------------------
void CEpollObject::Wakeup()
{
	BYTE val = 0;
	::write(m_PipeFds[1], &val, sizeof(BYTE));
}

//-----------------------------------------------------------------------------
// 描述: 向 EPoll 中添加一个连接
//-----------------------------------------------------------------------------
void CEpollObject::AddConnection(CTcpConnection *pConnection, bool bEnableSend, bool bEnableRecv)
{
	EpollControl(
		EPOLL_CTL_ADD, pConnection, pConnection->GetSocket().GetHandle(),
		bEnableSend, bEnableRecv);
}

//-----------------------------------------------------------------------------
// 描述: 更新 EPoll 中的一个连接
//-----------------------------------------------------------------------------
void CEpollObject::UpdateConnection(CTcpConnection *pConnection, bool bEnableSend, bool bEnableRecv)
{
	EpollControl(
		EPOLL_CTL_MOD, pConnection, pConnection->GetSocket().GetHandle(),
		bEnableSend, bEnableRecv);
}

//-----------------------------------------------------------------------------
// 描述: 从 EPoll 中删除一个连接
//-----------------------------------------------------------------------------
void CEpollObject::RemoveConnection(CTcpConnection *pConnection)
{
	EpollControl(
		EPOLL_CTL_DEL, pConnection, pConnection->GetSocket().GetHandle(),
		false, false);
}

//-----------------------------------------------------------------------------
// 描述: 设置回调
//-----------------------------------------------------------------------------
void CEpollObject::SetOnNotifyEventCallBack(NOTIFY_EVENT_PROC pProc, void *pParam)
{
	m_OnNotifyEvent.pProc = pProc;
	m_OnNotifyEvent.pParam = pParam;
}

///////////////////////////////////////////////////////////////////////////////
// class CTcpEventLoop

CTcpEventLoop::CTcpEventLoop()
{
	m_pEpollObject = new CEpollObject();
	m_pEpollObject->SetOnNotifyEventCallBack(OnEpollNotifyEvent, this);
}

CTcpEventLoop::~CTcpEventLoop()
{
	Stop(false, true);
	delete m_pEpollObject;
}

//-----------------------------------------------------------------------------
// 描述: EPoll 事件回调
//-----------------------------------------------------------------------------
void CTcpEventLoop::OnEpollNotifyEvent(void *pParam, CTcpConnection *pConnection,
	CEpollObject::EVENT_TYPE nEventType)
{
	if (nEventType == CEpollObject::ET_ALLOW_SEND)
		pConnection->TrySend();
	else if (nEventType == CEpollObject::ET_ALLOW_RECV)
		pConnection->TryRecv();
	else if (nEventType == CEpollObject::ET_ERROR)
		pConnection->ErrorOccurred();
}

//-----------------------------------------------------------------------------
// 描述: 执行事件循环
//-----------------------------------------------------------------------------
void CTcpEventLoop::ExecuteLoop(CThread *pThread)
{
	while (!pThread->GetTerminated())
	{
		m_pEpollObject->Poll();
		ProcessAcceptedConnList();
	}
}

//-----------------------------------------------------------------------------
// 描述: 唤醒事件循环中的阻塞操作
//-----------------------------------------------------------------------------
void CTcpEventLoop::WakeupLoop()
{
	m_pEpollObject->Wakeup();
}

//-----------------------------------------------------------------------------
// 描述: 将新连接注册到事件循环中
//-----------------------------------------------------------------------------
void CTcpEventLoop::RegisterConnection(CTcpConnection *pConnection)
{
	m_pEpollObject->AddConnection(pConnection, false, false);
}

//-----------------------------------------------------------------------------
// 描述: 从事件循环中注销连接
//-----------------------------------------------------------------------------
void CTcpEventLoop::UnregisterConnection(CTcpConnection *pConnection)
{
	m_pEpollObject->RemoveConnection(pConnection);
}

//-----------------------------------------------------------------------------
// 描述: 更新此 EventLoop 中的指定连接的设置
//-----------------------------------------------------------------------------
void CTcpEventLoop::UpdateConnection(CTcpConnection *pConnection, bool bEnableSend, bool bEnableRecv)
{
	m_pEpollObject->UpdateConnection(pConnection, bEnableSend, bEnableRecv);
}

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_LINUX */

///////////////////////////////////////////////////////////////////////////////
// class CMainTcpServer

CMainTcpServer::CMainTcpServer() :
	m_EventLoopList(IseApplication.GetIseOptions().GetTcpEventLoopCount()),
	m_bActive(false)
{
	CreateTcpServerList();
}

CMainTcpServer::~CMainTcpServer()
{
	Close();
	DestroyTcpServerList();
}

//-----------------------------------------------------------------------------
// 描述: 创建TCP服务器
//-----------------------------------------------------------------------------
void CMainTcpServer::CreateTcpServerList()
{
	int nServerCount = IseApplication.GetIseOptions().GetTcpServerCount();
	ISE_ASSERT(nServerCount >= 0);

	m_TcpServerList.resize(nServerCount);
	for (int i = 0; i < nServerCount; i++)
	{
		CTcpServer *pTcpServer = new CTcpServer();
		pTcpServer->CustomData() = (PVOID)i;

		m_TcpServerList[i] = pTcpServer;

		pTcpServer->SetOnCreateConnCallBack(OnCreateConnection, this);
		pTcpServer->SetOnAcceptConnCallBack(OnAcceptConnection, this);

		pTcpServer->SetLocalPort(IseApplication.GetIseOptions().GetTcpServerPort(i));
	}
}

//-----------------------------------------------------------------------------
// 描述: 销毁TCP服务器
//-----------------------------------------------------------------------------
void CMainTcpServer::DestroyTcpServerList()
{
	for (int i = 0; i < (int)m_TcpServerList.size(); i++)
		delete m_TcpServerList[i];
	m_TcpServerList.clear();
}

//-----------------------------------------------------------------------------
// 描述: 开启服务器
//-----------------------------------------------------------------------------
void CMainTcpServer::DoOpen()
{
	for (int i = 0; i < (int)m_TcpServerList.size(); i++)
		m_TcpServerList[i]->Open();

	m_EventLoopList.Start();
}

//-----------------------------------------------------------------------------
// 描述: 关闭服务器
//-----------------------------------------------------------------------------
void CMainTcpServer::DoClose()
{
	m_EventLoopList.Stop();

	for (int i = 0; i < (int)m_TcpServerList.size(); i++)
		m_TcpServerList[i]->Close();
}

//-----------------------------------------------------------------------------
// 描述: 创建连接对象
//-----------------------------------------------------------------------------
void CMainTcpServer::OnCreateConnection(void *pParam, CTcpServer *pTcpServer,
	SOCKET nSocketHandle, const CPeerAddress& PeerAddr, CBaseTcpConnection*& pConnection)
{
	pConnection = new CTcpConnection(pTcpServer, nSocketHandle, PeerAddr);
}

//-----------------------------------------------------------------------------
// 描述: 收到新的连接
// 注意:
//   此回调在TCP服务器监听线程(CTcpListenerThread)中执行。
//   为了避免TCP服务器的监听线程成为系统的瓶颈，故不应在此监听线程中触发
//   pIseBusiness->OnTcpConnection() 回调。正确的做法是，将新产生的连接通知给
//   CTcpEventLoop，再由事件循环线程(CTcpEventLoopThread)触发回调。
//   这样做的另一个好处是，对于同一个TCP连接，pIseBusiness->OnTcpXXX() 系列回调
//   均在同一个线程中执行。
//-----------------------------------------------------------------------------
void CMainTcpServer::OnAcceptConnection(void *pParam, CTcpServer *pTcpServer,
	CBaseTcpConnection *pConnection)
{
	CMainTcpServer *pThis = (CMainTcpServer*)pParam;

	// round-robin
	static int nIndex = 0;
	CTcpEventLoop *pEventLoop = (CTcpEventLoop*)pThis->m_EventLoopList[nIndex];
	nIndex = (nIndex >= pThis->m_EventLoopList.GetCount() - 1 ? 0 : nIndex + 1);

	((CTcpConnection*)pConnection)->SetEventLoop(pEventLoop);
}

//-----------------------------------------------------------------------------
// 描述: 开启服务器
//-----------------------------------------------------------------------------
void CMainTcpServer::Open()
{
	if (!m_bActive)
	{
		try
		{
			DoOpen();
			m_bActive = true;
		}
		catch (...)
		{
			DoClose();
			throw;
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 关闭服务器
//-----------------------------------------------------------------------------
void CMainTcpServer::Close()
{
	if (m_bActive)
	{
		DoClose();
		m_bActive = false;
	}
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
