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
// ise_server_udp.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SERVER_UDP_H_
#define _ISE_SERVER_UDP_H_

#include "ise_options.h"
#include "ise_classes.h"
#include "ise_thread.h"
#include "ise_sysutils.h"
#include "ise_socket.h"
#include "ise_exceptions.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class CThreadTimeOutChecker;
class CUdpPacket;
class CUdpRequestQueue;
class CUdpWorkerThread;
class CUdpWorkerThreadPool;
class CUdpRequestGroup;
class CMainUdpServer;

///////////////////////////////////////////////////////////////////////////////
// class CThreadTimeOutChecker - 线程超时检测类
//
// 说明:
// 此类用于配合 CUdpWorkerThread/CTcpWorkerThread，进行工作者线程的工作时间超时检测。
// 当工作者线程收到一个请求后，马上进入工作状态。一般而言，工作者线程为单个请求持续工作的
// 时间不宜太长，若太长则会导致服务器空闲工作者线程短缺，使得应付并发请求的能力下降。尤其
// 对于UDP服务来说情况更是如此。通常情况下，线程工作超时，很少是因为程序的流程和逻辑，而
// 是由于外部原因，比如数据库繁忙、资源死锁、网络拥堵等等。当线程工作超时后，应通知其退出，
// 若被通知退出后若干时间内仍未退出，则强行杀死。工作者线程调度中心再适时创建新的线程。

class CThreadTimeOutChecker : public CAutoInvokable
{
private:
	CThread *m_pThread;         // 被检测的线程
	time_t m_tStartTime;        // 开始计时时的时间戳
	bool m_bStarted;            // 是否已开始计时
	UINT m_nTimeOutSecs;        // 超过多少秒认为超时 (为0表示不进行超时检测)
	CCriticalSection m_Lock;

private:
	void Start();
	void Stop();

protected:
	virtual void InvokeInitialize() { Start(); }
	virtual void InvokeFinalize() { Stop(); }

public:
	explicit CThreadTimeOutChecker(CThread *pThread);
	virtual ~CThreadTimeOutChecker() {}

	// 检测线程是否已超时，若超时则通知其退出
	bool Check();

	// 设置超时时间，若为0则表示不进行超时检测
	void SetTimeOutSecs(UINT nValue) { m_nTimeOutSecs = nValue; }
	// 返回是否已开始计时
	bool GetStarted();
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpPacket - UDP数据包类

class CUdpPacket
{
private:
	void *m_pPacketBuffer;

public:
	UINT m_nRecvTimeStamp;
	CPeerAddress m_PeerAddr;
	int m_nPacketSize;

public:
	CUdpPacket() :
		m_pPacketBuffer(NULL),
		m_nRecvTimeStamp(0),
		m_PeerAddr(0, 0),
		m_nPacketSize(0)
	{}
	virtual ~CUdpPacket()
	{ if (m_pPacketBuffer) free(m_pPacketBuffer); }

	void SetPacketBuffer(void *pPakcetBuffer, int nPacketSize);
	inline void* GetPacketBuffer() const { return m_pPacketBuffer; }
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpRequestQueue - UDP请求队列类

class CUdpRequestQueue
{
private:
	// 各容器优缺点:
	// deque  - 支持头尾快速增删，但增删中间元素很慢，支持下标访问。
	// vector - 支持尾部快速增删，头部和中间元素增删很慢，支持下标访问。
	// list   - 支持任何元素的快速增删，但不支持下标访问，不支持快速取当前长度(size())；
	typedef deque<CUdpPacket*> PacketList;

	CUdpRequestGroup *m_pOwnGroup;   // 所属组别
	PacketList m_PacketList;         // 数据包列表
	int m_nPacketCount;              // 队列中数据包的个数(为了快速访问)
	int m_nCapacity;                 // 队列的最大容量
	int m_nEffWaitTime;              // 数据包有效等待时间(秒)
	CCriticalSection m_Lock;
	CSemaphore m_Semaphore;

public:
	explicit CUdpRequestQueue(CUdpRequestGroup *pOwnGroup);
	virtual ~CUdpRequestQueue() { Clear(); }

	void AddPacket(CUdpPacket *pPacket);
	CUdpPacket* ExtractPacket();
	void Clear();
	void BreakWaiting(int nSemCount);

	int GetCount() { return m_nPacketCount; }
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpWorkerThread - UDP工作者线程类
//
// 说明:
// 1. 缺省情况下，UDP工作者线程允许进行超时检测，若某些情况下需禁用超时检测，可以:
//    CUdpWorkerThread::GetTimeOutChecker().SetTimeOutSecs(0);
//
// 名词解释:
// 1. 超时线程: 因某一请求进入工作状态但长久未完成的线程。
// 2. 僵死线程: 已被通知退出但长久不退出的线程。

class CUdpWorkerThread : public CThread
{
private:
	CUdpWorkerThreadPool *m_pOwnPool;        // 所属线程池
	CThreadTimeOutChecker m_TimeOutChecker;  // 超时检测器
protected:
	virtual void Execute();
	virtual void DoTerminate();
	virtual void DoKill();
public:
	explicit CUdpWorkerThread(CUdpWorkerThreadPool *pThreadPool);
	virtual ~CUdpWorkerThread();

	// 返回超时检测器
	CThreadTimeOutChecker& GetTimeOutChecker() { return m_TimeOutChecker; }
	// 返回该线程是否空闲状态(即在等待请求)
	bool IsIdle() { return !m_TimeOutChecker.GetStarted(); }
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpWorkerThreadPool - UDP工作者线程池类

class CUdpWorkerThreadPool
{
public:
	enum
	{
		MAX_THREAD_TERM_SECS     = 60*3,    // 线程被通知退出后的最长寿命(秒)
		MAX_THREAD_WAIT_FOR_SECS = 2        // 线程池清空时最多等待时间(秒)
	};

private:
	CUdpRequestGroup *m_pOwnGroup;          // 所属组别
	CThreadList m_ThreadList;               // 线程列表
private:
	void CreateThreads(int nCount);
	void TerminateThreads(int nCount);
	void CheckThreadTimeOut();
	void KillZombieThreads();
public:
	explicit CUdpWorkerThreadPool(CUdpRequestGroup *pOwnGroup);
	virtual ~CUdpWorkerThreadPool();

	void RegisterThread(CUdpWorkerThread *pThread);
	void UnregisterThread(CUdpWorkerThread *pThread);

	// 根据负载情况动态调整线程数量
	void AdjustThreadCount();
	// 通知所有线程退出
	void TerminateAllThreads();
	// 等待所有线程退出
	void WaitForAllThreads();

	// 取得当前线程数量
	int GetThreadCount() { return m_ThreadList.GetCount(); }
	// 取得所属组别
	CUdpRequestGroup& GetRequestGroup() { return *m_pOwnGroup; }
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpRequestGroup - UDP请求组别类

class CUdpRequestGroup
{
private:
	CMainUdpServer *m_pOwnMainUdpSvr;       // 所属UDP服务器
	int m_nGroupIndex;                      // 组别号(0-based)
	CUdpRequestQueue m_RequestQueue;        // 请求队列
	CUdpWorkerThreadPool m_ThreadPool;      // 工作者线程池

public:
	CUdpRequestGroup(CMainUdpServer *pOwnMainUdpSvr, int nGroupIndex);
	virtual ~CUdpRequestGroup() {}

	int GetGroupIndex() { return m_nGroupIndex; }
	CUdpRequestQueue& GetRequestQueue() { return m_RequestQueue; }
	CUdpWorkerThreadPool& GetThreadPool() { return m_ThreadPool; }

	// 取得所属UDP服务器
	CMainUdpServer& GetMainUdpServer() { return *m_pOwnMainUdpSvr; }
};

///////////////////////////////////////////////////////////////////////////////
// class CMainUdpServer - UDP主服务器类

class CMainUdpServer
{
private:
	CUdpServer m_UdpServer;
	vector<CUdpRequestGroup*> m_RequestGroupList;   // 请求组别列表
	int m_nRequestGroupCount;                       // 请求组别总数
private:
	void InitUdpServer();
	void InitRequestGroupList();
	void ClearRequestGroupList();
private:
	static void OnRecvData(void *pParam, void *pPacketBuffer, int nPacketSize,
		const CPeerAddress& PeerAddr);
public:
	explicit CMainUdpServer();
	virtual ~CMainUdpServer();

	void Open();
	void Close();

	void SetLocalPort(int nValue) { m_UdpServer.SetLocalPort(nValue); }
	void SetListenerThreadCount(int nValue) { m_UdpServer.SetListenerThreadCount(nValue); }

	// 根据负载情况动态调整工作者线程数量
	void AdjustWorkerThreadCount();
	// 通知所有工作者线程退出
	void TerminateAllWorkerThreads();
	// 等待所有工作者线程退出
	void WaitForAllWorkerThreads();

	CUdpServer& GetUdpServer() { return m_UdpServer; }
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SERVER_UDP_H_
