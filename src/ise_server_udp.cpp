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
// class CThreadTimeOutChecker

CThreadTimeOutChecker::CThreadTimeOutChecker(CThread *pThread) :
	m_pThread(pThread),
	m_tStartTime(0),
	m_bStarted(false),
	m_nTimeOutSecs(0)
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 开始计时
//-----------------------------------------------------------------------------
void CThreadTimeOutChecker::Start()
{
	CAutoLocker Locker(m_Lock);

	m_tStartTime = time(NULL);
	m_bStarted = true;
}

//-----------------------------------------------------------------------------
// 描述: 停止计时
//-----------------------------------------------------------------------------
void CThreadTimeOutChecker::Stop()
{
	CAutoLocker Locker(m_Lock);

	m_bStarted = false;
}

//-----------------------------------------------------------------------------
// 描述: 检测线程是否已超时，若超时则通知其退出
//-----------------------------------------------------------------------------
bool CThreadTimeOutChecker::Check()
{
	bool bResult = false;

	if (m_bStarted && m_nTimeOutSecs > 0)
	{
		if ((UINT)time(NULL) - m_tStartTime >= m_nTimeOutSecs)
		{
			if (!m_pThread->GetTerminated()) m_pThread->Terminate();
			bResult = true;
		}
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// 描述: 返回是否已开始计时
//-----------------------------------------------------------------------------
bool CThreadTimeOutChecker::GetStarted()
{
	CAutoLocker Locker(m_Lock);

	return m_bStarted;
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpPacket

void CUdpPacket::SetPacketBuffer(void *pPakcetBuffer, int nPacketSize)
{
	if (m_pPacketBuffer)
	{
		free(m_pPacketBuffer);
		m_pPacketBuffer = NULL;
	}

	m_pPacketBuffer = malloc(nPacketSize);
	if (!m_pPacketBuffer)
		IseThrowMemoryException();

	memcpy(m_pPacketBuffer, pPakcetBuffer, nPacketSize);
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpRequestQueue

//-----------------------------------------------------------------------------
// 描述: 构造函数
// 参数:
//   pOwnGroup - 指定所属组别
//-----------------------------------------------------------------------------
CUdpRequestQueue::CUdpRequestQueue(CUdpRequestGroup *pOwnGroup)
{
	int nGroupIndex;

	m_pOwnGroup = pOwnGroup;
	nGroupIndex = pOwnGroup->GetGroupIndex();
	m_nCapacity = IseApplication.GetIseOptions().GetUdpRequestQueueCapacity(nGroupIndex);
	m_nEffWaitTime = IseApplication.GetIseOptions().GetUdpRequestEffWaitTime();
	m_nPacketCount = 0;
}

//-----------------------------------------------------------------------------
// 描述: 向队列中添加数据包
//-----------------------------------------------------------------------------
void CUdpRequestQueue::AddPacket(CUdpPacket *pPacket)
{
	if (m_nCapacity <= 0) return;
	bool bRemoved = false;

	{
		CAutoLocker Locker(m_Lock);

		if (m_nPacketCount >= m_nCapacity)
		{
			CUdpPacket *p;
			p = m_PacketList.front();
			delete p;
			m_PacketList.pop_front();
			m_nPacketCount--;
			bRemoved = true;
		}

		m_PacketList.push_back(pPacket);
		m_nPacketCount++;
	}

	if (!bRemoved) m_Semaphore.Increase();
}

//-----------------------------------------------------------------------------
// 描述: 从队列中取出数据包 (取出后应自行释放，若失败则返回 NULL)
// 备注: 若队列中尚没有数据包，则一直等待。
//-----------------------------------------------------------------------------
CUdpPacket* CUdpRequestQueue::ExtractPacket()
{
	m_Semaphore.Wait();

	{
		CAutoLocker Locker(m_Lock);
		CUdpPacket *p, *pResult = NULL;

		while (m_nPacketCount > 0)
		{
			p = m_PacketList.front();
			m_PacketList.pop_front();
			m_nPacketCount--;

			if (time(NULL) - (UINT)p->m_nRecvTimeStamp <= (UINT)m_nEffWaitTime)
			{
				pResult = p;
				break;
			}
			else
			{
				delete p;
			}
		}

		return pResult;
	}
}

//-----------------------------------------------------------------------------
// 描述: 清空队列
//-----------------------------------------------------------------------------
void CUdpRequestQueue::Clear()
{
	CAutoLocker Locker(m_Lock);
	CUdpPacket *p;

	for (UINT i = 0; i < m_PacketList.size(); i++)
	{
		p = m_PacketList[i];
		delete p;
	}

	m_PacketList.clear();
	m_nPacketCount = 0;
	m_Semaphore.Reset();
}

//-----------------------------------------------------------------------------
// 描述: 增加信号量的值，使等待数据的线程中断等待
//-----------------------------------------------------------------------------
void CUdpRequestQueue::BreakWaiting(int nSemCount)
{
	for (int i = 0; i < nSemCount; i++)
		m_Semaphore.Increase();
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpWorkerThread

CUdpWorkerThread::CUdpWorkerThread(CUdpWorkerThreadPool *pThreadPool) :
	m_pOwnPool(pThreadPool),
	m_TimeOutChecker(this)
{
	SetFreeOnTerminate(true);
	// 启用超时检测
	m_TimeOutChecker.SetTimeOutSecs(IseApplication.GetIseOptions().GetUdpWorkerThreadTimeOut());

	m_pOwnPool->RegisterThread(this);
}

CUdpWorkerThread::~CUdpWorkerThread()
{
	m_pOwnPool->UnregisterThread(this);
}

//-----------------------------------------------------------------------------
// 描述: 线程的执行函数
//-----------------------------------------------------------------------------
void CUdpWorkerThread::Execute()
{
	int nGroupIndex;
	CUdpRequestQueue *pRequestQueue;
	CUdpPacket *pPacket;

	nGroupIndex = m_pOwnPool->GetRequestGroup().GetGroupIndex();
	pRequestQueue = &(m_pOwnPool->GetRequestGroup().GetRequestQueue());

	while (!GetTerminated())
	try
	{
		pPacket = pRequestQueue->ExtractPacket();
		if (pPacket)
		{
			std::auto_ptr<CUdpPacket> AutoPtr(pPacket);
			CAutoInvoker AutoInvoker(m_TimeOutChecker);

			// 分派数据包
			if (!GetTerminated())
				pIseBusiness->DispatchUdpPacket(*this, nGroupIndex, *pPacket);
		}
	}
	catch (CException&)
	{}
}

//-----------------------------------------------------------------------------
// 描述: 执行 Terminate() 前的附加操作
//-----------------------------------------------------------------------------
void CUdpWorkerThread::DoTerminate()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 执行 Kill() 前的附加操作
//-----------------------------------------------------------------------------
void CUdpWorkerThread::DoKill()
{
	// nothing
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpWorkerThreadPool

CUdpWorkerThreadPool::CUdpWorkerThreadPool(CUdpRequestGroup *pOwnGroup) :
	m_pOwnGroup(pOwnGroup)
{
	// nothing
}

CUdpWorkerThreadPool::~CUdpWorkerThreadPool()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 创建 nCount 个线程
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::CreateThreads(int nCount)
{
	for (int i = 0; i < nCount; i++)
	{
		CUdpWorkerThread *pThread;
		pThread = new CUdpWorkerThread(this);
		pThread->Run();
	}
}

//-----------------------------------------------------------------------------
// 描述: 终止 nCount 个线程
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::TerminateThreads(int nCount)
{
	CAutoLocker Locker(m_ThreadList.GetLock());

	int nTermCount = 0;
	if (nCount > m_ThreadList.GetCount())
		nCount = m_ThreadList.GetCount();

	for (int i = m_ThreadList.GetCount() - 1; i >= 0; i--)
	{
		CUdpWorkerThread *pThread;
		pThread = (CUdpWorkerThread*)m_ThreadList[i];
		if (pThread->IsIdle())
		{
			pThread->Terminate();
			nTermCount++;
			if (nTermCount >= nCount) break;
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 检测线程是否工作超时 (超时线程: 因某一请求进入工作状态但长久未完成的线程)
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::CheckThreadTimeOut()
{
	CAutoLocker Locker(m_ThreadList.GetLock());

	for (int i = 0; i < m_ThreadList.GetCount(); i++)
	{
		CUdpWorkerThread *pThread;
		pThread = (CUdpWorkerThread*)m_ThreadList[i];
		pThread->GetTimeOutChecker().Check();
	}
}

//-----------------------------------------------------------------------------
// 描述: 强行杀死僵死的线程 (僵死线程: 已被通知退出但长久不退出的线程)
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::KillZombieThreads()
{
	CAutoLocker Locker(m_ThreadList.GetLock());

	for (int i = m_ThreadList.GetCount() - 1; i >= 0; i--)
	{
		CUdpWorkerThread *pThread;
		pThread = (CUdpWorkerThread*)m_ThreadList[i];
		if (pThread->GetTermElapsedSecs() >= MAX_THREAD_TERM_SECS)
		{
			pThread->Kill();
			m_ThreadList.Remove(pThread);
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 注册线程
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::RegisterThread(CUdpWorkerThread *pThread)
{
	m_ThreadList.Add(pThread);
}

//-----------------------------------------------------------------------------
// 描述: 注销线程
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::UnregisterThread(CUdpWorkerThread *pThread)
{
	m_ThreadList.Remove(pThread);
}

//-----------------------------------------------------------------------------
// 描述: 根据负载情况动态调整线程数量
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::AdjustThreadCount()
{
	int nPacketCount, nThreadCount;
	int nMinThreads, nMaxThreads, nDeltaThreads;
	int nPacketAlertLine;

	// 取得线程数量上下限
	IseApplication.GetIseOptions().GetUdpWorkerThreadCount(
		m_pOwnGroup->GetGroupIndex(), nMinThreads, nMaxThreads);
	// 取得请求队列中数据包数量警戒线
	nPacketAlertLine = IseApplication.GetIseOptions().GetUdpRequestQueueAlertLine();

	// 检测线程是否工作超时
	CheckThreadTimeOut();
	// 强行杀死僵死的线程
	KillZombieThreads();

	// 取得数据包数量和线程数量
	nPacketCount = m_pOwnGroup->GetRequestQueue().GetCount();
	nThreadCount = m_ThreadList.GetCount();

	// 保证线程数量在上下限范围之内
	if (nThreadCount < nMinThreads)
	{
		CreateThreads(nMinThreads - nThreadCount);
		nThreadCount = nMinThreads;
	}
	if (nThreadCount > nMaxThreads)
	{
		TerminateThreads(nThreadCount - nMaxThreads);
		nThreadCount = nMaxThreads;
	}

	// 如果请求队列中的数量超过警戒线，则尝试增加线程数量
	if (nThreadCount < nMaxThreads && nPacketCount >= nPacketAlertLine)
	{
		nDeltaThreads = Min(nMaxThreads - nThreadCount, 3);
		CreateThreads(nDeltaThreads);
	}

	// 如果请求队列为空，则尝试减少线程数量
	if (nThreadCount > nMinThreads && nPacketCount == 0)
	{
		nDeltaThreads = 1;
		TerminateThreads(nDeltaThreads);
	}
}

//-----------------------------------------------------------------------------
// 描述: 通知所有线程退出
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::TerminateAllThreads()
{
	m_ThreadList.TerminateAllThreads();

	CAutoLocker Locker(m_ThreadList.GetLock());

	// 使线程从等待中解脱，尽快退出
	GetRequestGroup().GetRequestQueue().BreakWaiting(m_ThreadList.GetCount());
}

//-----------------------------------------------------------------------------
// 描述: 等待所有线程退出
//-----------------------------------------------------------------------------
void CUdpWorkerThreadPool::WaitForAllThreads()
{
	TerminateAllThreads();

	int nKilledCount = 0;
	m_ThreadList.WaitForAllThreads(MAX_THREAD_WAIT_FOR_SECS, &nKilledCount);

	if (nKilledCount)
		Logger().WriteFmt(SEM_THREAD_KILLED, nKilledCount, "udp worker");
}

///////////////////////////////////////////////////////////////////////////////
// class CUdpRequestGroup

CUdpRequestGroup::CUdpRequestGroup(CMainUdpServer *pOwnMainUdpSvr, int nGroupIndex) :
	m_pOwnMainUdpSvr(pOwnMainUdpSvr),
	m_nGroupIndex(nGroupIndex),
	m_RequestQueue(this),
	m_ThreadPool(this)
{
	// nothing
}

///////////////////////////////////////////////////////////////////////////////
// class CMainUdpServer

CMainUdpServer::CMainUdpServer()
{
	InitUdpServer();
	InitRequestGroupList();
}

CMainUdpServer::~CMainUdpServer()
{
	ClearRequestGroupList();
}

//-----------------------------------------------------------------------------
// 描述: 初始化 m_UdpServer
//-----------------------------------------------------------------------------
void CMainUdpServer::InitUdpServer()
{
	m_UdpServer.SetOnRecvDataCallBack(OnRecvData, this);
}

//-----------------------------------------------------------------------------
// 描述: 初始化请求组别列表
//-----------------------------------------------------------------------------
void CMainUdpServer::InitRequestGroupList()
{
	ClearRequestGroupList();

	m_nRequestGroupCount = IseApplication.GetIseOptions().GetUdpRequestGroupCount();
	for (int nGroupIndex = 0; nGroupIndex < m_nRequestGroupCount; nGroupIndex++)
	{
		CUdpRequestGroup *p;
		p = new CUdpRequestGroup(this, nGroupIndex);
		m_RequestGroupList.push_back(p);
	}
}

//-----------------------------------------------------------------------------
// 描述: 清空请求组别列表
//-----------------------------------------------------------------------------
void CMainUdpServer::ClearRequestGroupList()
{
	for (UINT i = 0; i < m_RequestGroupList.size(); i++)
		delete m_RequestGroupList[i];
	m_RequestGroupList.clear();
}

//-----------------------------------------------------------------------------
// 描述: 收到数据包
//-----------------------------------------------------------------------------
void CMainUdpServer::OnRecvData(void *pParam, void *pPacketBuffer, int nPacketSize,
	const CPeerAddress& PeerAddr)
{
	CMainUdpServer *pThis = (CMainUdpServer*)pParam;
	int nGroupIndex;

	// 先进行数据包分类，得到组别号
	pIseBusiness->ClassifyUdpPacket(pPacketBuffer, nPacketSize, nGroupIndex);

	// 如果组别号合法
	if (nGroupIndex >= 0 && nGroupIndex < pThis->m_nRequestGroupCount)
	{
		CUdpPacket *p = new CUdpPacket();
		if (p)
		{
			p->m_nRecvTimeStamp = (UINT)time(NULL);
			p->m_PeerAddr = PeerAddr;
			p->m_nPacketSize = nPacketSize;
			p->SetPacketBuffer(pPacketBuffer, nPacketSize);

			// 添加到请求队列中
			pThis->m_RequestGroupList[nGroupIndex]->GetRequestQueue().AddPacket(p);
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 开启服务器
//-----------------------------------------------------------------------------
void CMainUdpServer::Open()
{
	m_UdpServer.Open();
}

//-----------------------------------------------------------------------------
// 描述: 关闭服务器
//-----------------------------------------------------------------------------
void CMainUdpServer::Close()
{
	TerminateAllWorkerThreads();
	WaitForAllWorkerThreads();

	m_UdpServer.Close();
}

//-----------------------------------------------------------------------------
// 描述: 根据负载情况动态调整工作者线程数量
//-----------------------------------------------------------------------------
void CMainUdpServer::AdjustWorkerThreadCount()
{
	for (UINT i = 0; i < m_RequestGroupList.size(); i++)
		m_RequestGroupList[i]->GetThreadPool().AdjustThreadCount();
}

//-----------------------------------------------------------------------------
// 描述: 通知所有工作者线程退出
//-----------------------------------------------------------------------------
void CMainUdpServer::TerminateAllWorkerThreads()
{
	for (UINT i = 0; i < m_RequestGroupList.size(); i++)
		m_RequestGroupList[i]->GetThreadPool().TerminateAllThreads();
}

//-----------------------------------------------------------------------------
// 描述: 等待所有工作者线程退出
//-----------------------------------------------------------------------------
void CMainUdpServer::WaitForAllWorkerThreads()
{
	for (UINT i = 0; i < m_RequestGroupList.size(); i++)
		m_RequestGroupList[i]->GetThreadPool().WaitForAllThreads();
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
