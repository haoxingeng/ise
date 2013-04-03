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
#include "ise_sysutils.h"
#include "ise_socket.h"
#include "ise_exceptions.h"

#ifdef ISE_WIN32
#include <windows.h>
#endif

#ifdef ISE_LINUX
#include <sys/epoll.h>
#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class CPacketMeasurer;
class CIoBuffer;
class CTcpConnection;
class CTcpEventLoopThread;
class CBaseTcpEventLoop;
class CTcpEventLoop;
class CTcpEventLoopList;

#ifdef ISE_WIN32
class CIocpTaskData;
class CIocpBufferAllocator;
class CIocpPendingCounter;
class CIocpObject;
#endif

#ifdef ISE_LINUX
class CEpollObject;
#endif

class CMainTcpServer;

///////////////////////////////////////////////////////////////////////////////
// class CPacketMeasurer - 数据包定界器

class CPacketMeasurer
{
public:
	virtual ~CPacketMeasurer() {}
	virtual bool IsCompletePacket(const char *pData, int nBytes, int& nPacketSize) = 0;
};

class CDelimiterPacketMeasurer : public CPacketMeasurer
{
private:
	char m_chDelimiter;
public:
	CDelimiterPacketMeasurer(char chDelimiter) : m_chDelimiter(chDelimiter) {}
	virtual bool IsCompletePacket(const char *pData, int nBytes, int& nPacketSize);
};

class CLinePacketMeasurer : public CDelimiterPacketMeasurer
{
private:
	CLinePacketMeasurer() : CDelimiterPacketMeasurer('\n') {}
public:
	static CLinePacketMeasurer& Instance()
	{
		static CLinePacketMeasurer obj;
		return obj;
	}
};

class CNullTerminatedPacketMeasurer : public CDelimiterPacketMeasurer
{
private:
	CNullTerminatedPacketMeasurer() : CDelimiterPacketMeasurer('\0') {}
public:
	static CNullTerminatedPacketMeasurer& Instance()
	{
		static CNullTerminatedPacketMeasurer obj;
		return obj;
	}
};

///////////////////////////////////////////////////////////////////////////////
// class CIoBuffer - 输入输出缓存
//
// +-----------------+------------------+------------------+
// |  useless bytes  |  readable bytes  |  writable bytes  |
// |                 |     (CONTENT)    |                  |
// +-----------------+------------------+------------------+
// |                 |                  |                  |
// 0     <=    nReaderIndex   <=   nWriterIndex    <=    size

class CIoBuffer
{
public:
	enum { INITIAL_SIZE = 1024 };
private:
	vector<char> m_Buffer;
	int m_nReaderIndex;
	int m_nWriterIndex;
private:
	char* GetBufferPtr() const { return (char*)&*m_Buffer.begin(); }
	char* GetWriterPtr() const { return GetBufferPtr() + m_nWriterIndex; }
	void MakeSpace(int nMoreBytes);
public:
	CIoBuffer();
	~CIoBuffer();

	int GetReadableBytes() const { return m_nWriterIndex - m_nReaderIndex; }
	int GetWritableBytes() const { return (int)m_Buffer.size() - m_nWriterIndex; }
	int GetUselessBytes() const { return m_nReaderIndex; }

	void Append(const string& str);
	void Append(const char *pData, int nBytes);
	void Append(int nBytes);

	void Retrieve(int nBytes);
	void RetrieveAll(string& str);
	void RetrieveAll();

	const char* Peek() const { return GetBufferPtr() + m_nReaderIndex; }
};

///////////////////////////////////////////////////////////////////////////////
// class CTcpEventLoopThread - 事件循环执行线程

class CTcpEventLoopThread : public CThread
{
private:
	CBaseTcpEventLoop& m_EventLoop;
protected:
	virtual void Execute();
public:
	CTcpEventLoopThread(CBaseTcpEventLoop& EventLoop);
};

///////////////////////////////////////////////////////////////////////////////
// class CBaseTcpEventLoop - 事件循环基类

class CBaseTcpEventLoop
{
public:
	friend class CTcpEventLoopThread;
private:
	CTcpEventLoopThread *m_pThread;
	CObjectList<CTcpConnection> m_AcceptedConnList;  // 由TcpServer新产生的TCP连接
protected:
	virtual void ExecuteLoop(CThread *pThread) = 0;
	virtual void WakeupLoop() {}
	virtual void RegisterConnection(CTcpConnection *pConnection) = 0;
	virtual void UnregisterConnection(CTcpConnection *pConnection) = 0;
protected:
	void ProcessAcceptedConnList();
public:
	CBaseTcpEventLoop();
	virtual ~CBaseTcpEventLoop();

	void Start();
	void Stop(bool bForce, bool bWaitFor);
	bool IsRunning();

	void AddConnection(CTcpConnection *pConnection);
	void RemoveConnection(CTcpConnection *pConnection);
};

///////////////////////////////////////////////////////////////////////////////
// class CTcpEventLoopList - 事件循环列表

class CTcpEventLoopList
{
public:
	enum { MAX_LOOP_COUNT = 64 };
private:
	CObjectList<CBaseTcpEventLoop> m_Items;
private:
	void SetCount(int nCount);
public:
	CTcpEventLoopList(int nLoopCount);
	virtual ~CTcpEventLoopList();

	void Start();
	void Stop();

	int GetCount() { return m_Items.GetCount(); }
	CBaseTcpEventLoop* GetItem(int nIndex) { return m_Items[nIndex]; }
	CBaseTcpEventLoop* operator[] (int nIndex) { return GetItem(nIndex); }
};

///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_WIN32

///////////////////////////////////////////////////////////////////////////////
// 类型定义

enum IOCP_TASK_TYPE
{
	ITT_SEND = 1,
	ITT_RECV = 2,
};

typedef void (*IOCP_CALLBACK_PROC)(const CIocpTaskData& TaskData, PVOID pParam);
typedef CCallBackDef<IOCP_CALLBACK_PROC> IOCP_CALLBACK_DEF;

///////////////////////////////////////////////////////////////////////////////
// class CTcpConnection - Proactor模型下的TCP连接

class CTcpConnection : public CBaseTcpConnection
{
public:
	friend class CMainTcpServer;
public:
	struct SEND_TASK
	{
		int nBytes;
		CCustomParams Params;
	};

	struct RECV_TASK
	{
		CPacketMeasurer *pPacketMeasurer;
		CCustomParams Params;
	};

	typedef deque<SEND_TASK> SEND_TASK_QUEUE;
	typedef deque<RECV_TASK> RECV_TASK_QUEUE;

private:
	CTcpServer *m_pTcpServer;          // 所属 CTcpServer
	CTcpEventLoop *m_pEventLoop;       // 所属 CTcpEventLoop
	CIoBuffer m_SendBuffer;            // 数据发送缓存
	CIoBuffer m_RecvBuffer;            // 数据接收缓存
	SEND_TASK_QUEUE m_SendTaskQueue;   // 发送任务队列
	RECV_TASK_QUEUE m_RecvTaskQueue;   // 接收任务队列
	bool m_bSending;                   // 是否已向IOCP提交发送任务但尚未收到回调通知
	bool m_bRecving;                   // 是否已向IOCP提交接收任务但尚未收到回调通知
	int m_nBytesSent;                  // 自从上次发送任务完成回调以来共发送了多少字节
	int m_nBytesRecved;                // 自从上次接收任务完成回调以来共接收了多少字节
private:
	void TrySend();
	void TryRecv();
	void ErrorOccurred();

	void SetEventLoop(CTcpEventLoop *pEventLoop);
	CTcpEventLoop* GetEventLoop() { return m_pEventLoop; }

	static void OnIocpCallBack(const CIocpTaskData& TaskData, PVOID pParam);
	void OnSendCallBack(const CIocpTaskData& TaskData);
	void OnRecvCallBack(const CIocpTaskData& TaskData);
protected:
	virtual void DoDisconnect();
public:
	CTcpConnection(CTcpServer *pTcpServer, SOCKET nSocketHandle, const CPeerAddress& PeerAddr);
	virtual ~CTcpConnection();

	void PostSendTask(char *pBuffer, int nSize, const CCustomParams& Params = EMPTY_PARAMS);
	void PostRecvTask(CPacketMeasurer *pPacketMeasurer, const CCustomParams& Params = EMPTY_PARAMS);

	int GetServerIndex() const { return (int)m_pTcpServer->CustomData(); }
	int GetServerPort() const { return m_pTcpServer->GetLocalPort(); }
};

///////////////////////////////////////////////////////////////////////////////
// class CIocpTaskData

class CIocpTaskData
{
public:
	friend class CIocpObject;
private:
	HANDLE m_hIocpHandle;
	HANDLE m_hFileHandle;
	IOCP_TASK_TYPE m_nTaskType;
	UINT m_nTaskSeqNum;
	CCallBackDef<IOCP_CALLBACK_PROC> m_CallBack;
	PVOID m_pCaller;
	CCustomParams m_Params;
	PVOID m_pEntireDataBuf;
	int m_nEntireDataSize;
	WSABUF m_WSABuffer;
	int m_nBytesTrans;
	int m_nErrorCode;

public:
	CIocpTaskData();

	HANDLE GetIocpHandle() const { return m_hIocpHandle; }
	HANDLE GetFileHandle() const { return m_hFileHandle; }
	IOCP_TASK_TYPE GetTaskType() const { return m_nTaskType; }
	UINT GetTaskSeqNum() const { return m_nTaskSeqNum; }
	const IOCP_CALLBACK_DEF& GetCallBack() const { return m_CallBack; }
	PVOID GetCaller() const { return m_pCaller; }
	const CCustomParams& GetParams() const { return m_Params; }
	char* GetEntireDataBuf() const { return (char*)m_pEntireDataBuf; }
	int GetEntireDataSize() const { return m_nEntireDataSize; }
	char* GetDataBuf() const { return (char*)m_WSABuffer.buf; }
	int GetDataSize() const { return m_WSABuffer.len; }
	int GetBytesTrans() const { return m_nBytesTrans; }
	int GetErrorCode() const { return m_nErrorCode; }
};

#pragma pack(1)
struct CIocpOverlappedData
{
	OVERLAPPED Overlapped;
	CIocpTaskData TaskData;
};
#pragma pack()

///////////////////////////////////////////////////////////////////////////////
// class CIocpBufferAllocator

class CIocpBufferAllocator
{
private:
	int m_nBufferSize;
	CList m_Items;
	int m_nUsedCount;
	CCriticalSection m_Lock;
private:
	void Clear();
public:
	CIocpBufferAllocator(int nBufferSize);
	~CIocpBufferAllocator();

	PVOID AllocBuffer();
	void ReturnBuffer(PVOID pBuffer);

	int GetUsedCount() const { return m_nUsedCount; }
};

///////////////////////////////////////////////////////////////////////////////
// class CIocpPendingCounter

class CIocpPendingCounter
{
private:
	struct COUNT_DATA
	{
		int nSendCount;
		int nRecvCount;
	};

	typedef std::map<PVOID, COUNT_DATA> ITEMS;   // <pCaller, COUNT_DATA>

	ITEMS m_Items;
	CCriticalSection m_Lock;
public:
	CIocpPendingCounter() {}
	virtual ~CIocpPendingCounter() {}

	void Inc(PVOID pCaller, IOCP_TASK_TYPE nTaskType);
	void Dec(PVOID pCaller, IOCP_TASK_TYPE nTaskType);
	int Get(PVOID pCaller);
	int Get(IOCP_TASK_TYPE nTaskType);
};

///////////////////////////////////////////////////////////////////////////////
// class CIocpObject

class CIocpObject
{
public:
	friend class CAutoFinalizer;

private:
	static CIocpBufferAllocator m_BufferAlloc;
	static CSeqNumberAlloc m_TaskSeqAlloc;
	static CIocpPendingCounter m_PendingCounter;

	HANDLE m_hIocpHandle;

private:
	void Initialize();
	void Finalize();
	void ThrowGeneralError();
	CIocpOverlappedData* CreateOverlappedData(IOCP_TASK_TYPE nTaskType,
		HANDLE hFileHandle, PVOID pBuffer, int nSize, int nOffset,
		const IOCP_CALLBACK_DEF& CallBackDef, PVOID pCaller,
		const CCustomParams& Params);
	void DestroyOverlappedData(CIocpOverlappedData *pOvDataPtr);
	void PostError(int nErrorCode, CIocpOverlappedData *pOvDataPtr);
	void InvokeCallBack(const CIocpTaskData& TaskData);

public:
	CIocpObject();
	virtual ~CIocpObject();

	bool AssociateHandle(SOCKET hSocketHandle);
	bool IsComplete(PVOID pCaller);

	void Work();
	void Wakeup();

	void Send(SOCKET hSocketHandle, PVOID pBuffer, int nSize, int nOffset,
		const IOCP_CALLBACK_DEF& CallBackDef, PVOID pCaller, const CCustomParams& Params);
	void Recv(SOCKET hSocketHandle, PVOID pBuffer, int nSize, int nOffset,
		const IOCP_CALLBACK_DEF& CallBackDef, PVOID pCaller, const CCustomParams& Params);
};

///////////////////////////////////////////////////////////////////////////////
// class CTcpEventLoop - 事件循环

class CTcpEventLoop : public CBaseTcpEventLoop
{
private:
	CIocpObject *m_pIocpObject;
protected:
	virtual void ExecuteLoop(CThread *pThread);
	virtual void WakeupLoop();
	virtual void RegisterConnection(CTcpConnection *pConnection);
	virtual void UnregisterConnection(CTcpConnection *pConnection);
public:
	CTcpEventLoop();
	virtual ~CTcpEventLoop();

	CIocpObject* GetIocpObject() { return m_pIocpObject; }
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_WIN32 */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_LINUX

///////////////////////////////////////////////////////////////////////////////
// class CTcpConnection - Proactor模型下的TCP连接

class CTcpConnection : public CBaseTcpConnection
{
public:
	friend class CMainTcpServer;
	friend class CTcpEventLoop;
public:
	struct SEND_TASK
	{
		int nBytes;
		CCustomParams Params;
	};

	struct RECV_TASK
	{
		CPacketMeasurer *pPacketMeasurer;
		CCustomParams Params;
	};

	typedef deque<SEND_TASK> SEND_TASK_QUEUE;
	typedef deque<RECV_TASK> RECV_TASK_QUEUE;

private:
	CTcpServer *m_pTcpServer;          // 所属 CTcpServer
	CTcpEventLoop *m_pEventLoop;       // 所属 CTcpEventLoop
	CIoBuffer m_SendBuffer;            // 数据发送缓存
	CIoBuffer m_RecvBuffer;            // 数据接收缓存
	SEND_TASK_QUEUE m_SendTaskQueue;   // 发送任务队列
	RECV_TASK_QUEUE m_RecvTaskQueue;   // 接收任务队列
	int m_nBytesSent;                  // 自从上次发送任务完成回调以来共发送了多少字节
	bool m_bEnableSend;                // 是否监视可发送事件
	bool m_bEnableRecv;                // 是否监视可接收事件
private:
	void SetSendEnabled(bool bEnabled);
	void SetRecvEnabled(bool bEnabled);

	void TrySend();
	void TryRecv();
	void ErrorOccurred();

	void SetEventLoop(CTcpEventLoop *pEventLoop);
	CTcpEventLoop* GetEventLoop() { return m_pEventLoop; }
protected:
	virtual void DoDisconnect();
public:
	CTcpConnection(CTcpServer *pTcpServer, SOCKET nSocketHandle, const CPeerAddress& PeerAddr);
	virtual ~CTcpConnection();

	void PostSendTask(char *pBuffer, int nSize, const CCustomParams& Params = EMPTY_PARAMS);
	void PostRecvTask(CPacketMeasurer *pPacketMeasurer, const CCustomParams& Params = EMPTY_PARAMS);

	int GetServerIndex() const { return (long)(m_pTcpServer->CustomData()); }
	int GetServerPort() const { return m_pTcpServer->GetLocalPort(); }
};

///////////////////////////////////////////////////////////////////////////////
// class CEpollObject - Linux EPoll 功能封装

class CEpollObject
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

	typedef vector<struct epoll_event> EVENT_LIST;
	typedef int EVENT_PIPE[2];

	typedef void (*NOTIFY_EVENT_PROC)(void *pParam, CTcpConnection *pConnection, EVENT_TYPE nEventType);

private:
	int m_nEpollFd;                    // EPoll 的文件描述符
	EVENT_LIST m_Events;               // 存放 epoll_wait() 返回的事件
	EVENT_PIPE m_PipeFds;              // 用于唤醒 epoll_wait() 的管道
	CCallBackDef<NOTIFY_EVENT_PROC> m_OnNotifyEvent;
private:
	void CreateEpoll();
	void DestroyEpoll();
	void CreatePipe();
	void DestroyPipe();

	void EpollControl(int nOperation, void *pParam, int nHandle, bool bEnableSend, bool bEnableRecv);

	void ProcessPipeEvent();
	void ProcessEvents(int nEventCount);
public:
	CEpollObject();
	~CEpollObject();

	void Poll();
	void Wakeup();

	void AddConnection(CTcpConnection *pConnection, bool bEnableSend, bool bEnableRecv);
	void UpdateConnection(CTcpConnection *pConnection, bool bEnableSend, bool bEnableRecv);
	void RemoveConnection(CTcpConnection *pConnection);

	void SetOnNotifyEventCallBack(NOTIFY_EVENT_PROC pProc, void *pParam = NULL);
};

///////////////////////////////////////////////////////////////////////////////
// class CTcpEventLoop - 事件循环

class CTcpEventLoop : public CBaseTcpEventLoop
{
private:
	CEpollObject *m_pEpollObject;
private:
	static void OnEpollNotifyEvent(void *pParam, CTcpConnection *pConnection,
		CEpollObject::EVENT_TYPE nEventType);
protected:
	virtual void ExecuteLoop(CThread *pThread);
	virtual void WakeupLoop();
	virtual void RegisterConnection(CTcpConnection *pConnection);
	virtual void UnregisterConnection(CTcpConnection *pConnection);
public:
	CTcpEventLoop();
	virtual ~CTcpEventLoop();

	void UpdateConnection(CTcpConnection *pConnection, bool bEnableSend, bool bEnableRecv);
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_LINUX */

///////////////////////////////////////////////////////////////////////////////
// class CMainTcpServer - TCP主服务器类

class CMainTcpServer
{
private:
	typedef vector<CTcpServer*> TCP_SERVER_LIST;
private:
	bool m_bActive;
	TCP_SERVER_LIST m_TcpServerList;
	CTcpEventLoopList m_EventLoopList;

private:
	void CreateTcpServerList();
	void DestroyTcpServerList();
	void DoOpen();
	void DoClose();
private:
	static void OnCreateConnection(void *pParam, CTcpServer *pTcpServer,
		SOCKET nSocketHandle, const CPeerAddress& PeerAddr, CBaseTcpConnection*& pConnection);
	static void OnAcceptConnection(void *pParam, CTcpServer *pTcpServer,
		CBaseTcpConnection *pConnection);
public:
	explicit CMainTcpServer();
	virtual ~CMainTcpServer();

	void Open();
	void Close();
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SERVER_TCP_H_
