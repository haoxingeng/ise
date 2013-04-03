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
// 文件名称: ise_server_assistor.cpp
// 功能描述: 辅助服务器的实现
///////////////////////////////////////////////////////////////////////////////

#include "ise_server_assistor.h"
#include "ise_errmsgs.h"
#include "ise_application.h"

using namespace ise;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class CAssistorThread

CAssistorThread::CAssistorThread(CAssistorThreadPool *pThreadPool, int nAssistorIndex) :
	m_pOwnPool(pThreadPool),
	m_nAssistorIndex(nAssistorIndex)
{
	SetFreeOnTerminate(true);
	m_pOwnPool->RegisterThread(this);
}

CAssistorThread::~CAssistorThread()
{
	m_pOwnPool->UnregisterThread(this);
}

//-----------------------------------------------------------------------------
// 描述: 线程执行函数
//-----------------------------------------------------------------------------
void CAssistorThread::Execute()
{
	m_pOwnPool->GetAssistorServer().OnAssistorThreadExecute(*this, m_nAssistorIndex);
}

//-----------------------------------------------------------------------------
// 描述: 执行 Kill() 前的附加操作
//-----------------------------------------------------------------------------
void CAssistorThread::DoKill()
{
	// nothing
}

///////////////////////////////////////////////////////////////////////////////
// class CAssistorThreadPool

CAssistorThreadPool::CAssistorThreadPool(CAssistorServer *pOwnAssistorServer) :
	m_pOwnAssistorSvr(pOwnAssistorServer)
{
	// nothing
}

CAssistorThreadPool::~CAssistorThreadPool()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 注册线程
//-----------------------------------------------------------------------------
void CAssistorThreadPool::RegisterThread(CAssistorThread *pThread)
{
	m_ThreadList.Add(pThread);
}

//-----------------------------------------------------------------------------
// 描述: 注销线程
//-----------------------------------------------------------------------------
void CAssistorThreadPool::UnregisterThread(CAssistorThread *pThread)
{
	m_ThreadList.Remove(pThread);
}

//-----------------------------------------------------------------------------
// 描述: 通知所有线程退出
//-----------------------------------------------------------------------------
void CAssistorThreadPool::TerminateAllThreads()
{
	m_ThreadList.TerminateAllThreads();
}

//-----------------------------------------------------------------------------
// 描述: 等待所有线程退出
//-----------------------------------------------------------------------------
void CAssistorThreadPool::WaitForAllThreads()
{
	const int MAX_WAIT_FOR_SECS = 5;
	int nKilledCount = 0;

	m_ThreadList.WaitForAllThreads(MAX_WAIT_FOR_SECS, &nKilledCount);

	if (nKilledCount > 0)
		Logger().WriteFmt(SEM_THREAD_KILLED, nKilledCount, "assistor");
}

//-----------------------------------------------------------------------------
// 描述: 打断指定线程的睡眠
//-----------------------------------------------------------------------------
void CAssistorThreadPool::InterruptThreadSleep(int nAssistorIndex)
{
	CAutoLocker Locker(m_ThreadList.GetLock());

	for (int i = 0; i < m_ThreadList.GetCount(); i++)
	{
		CAssistorThread *pThread = (CAssistorThread*)m_ThreadList[i];
		if (pThread->GetIndex() == nAssistorIndex)
		{
			pThread->InterruptSleep();
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// class CAssistorServer

CAssistorServer::CAssistorServer() :
	m_bActive(false),
	m_ThreadPool(this)
{
	// nothing
}

CAssistorServer::~CAssistorServer()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 启动服务器
//-----------------------------------------------------------------------------
void CAssistorServer::Open()
{
	if (!m_bActive)
	{
		int nCount = IseApplication.GetIseOptions().GetAssistorThreadCount();

		for (int i = 0; i < nCount; i++)
		{
			CAssistorThread *pThread;
			pThread = new CAssistorThread(&m_ThreadPool, i);
			pThread->Run();
		}

		m_bActive = true;
	}
}

//-----------------------------------------------------------------------------
// 描述: 关闭服务器
//-----------------------------------------------------------------------------
void CAssistorServer::Close()
{
	if (m_bActive)
	{
		WaitForAllAssistorThreads();
		m_bActive = false;
	}
}

//-----------------------------------------------------------------------------
// 描述: 辅助服务线程执行函数
// 参数:
//   nAssistorIndex - 辅助线程序号(0-based)
//-----------------------------------------------------------------------------
void CAssistorServer::OnAssistorThreadExecute(CAssistorThread& AssistorThread, int nAssistorIndex)
{
	pIseBusiness->AssistorThreadExecute(AssistorThread, nAssistorIndex);
}

//-----------------------------------------------------------------------------
// 描述: 通知所有辅助线程退出
//-----------------------------------------------------------------------------
void CAssistorServer::TerminateAllAssistorThreads()
{
	m_ThreadPool.TerminateAllThreads();
}

//-----------------------------------------------------------------------------
// 描述: 等待所有辅助线程退出
//-----------------------------------------------------------------------------
void CAssistorServer::WaitForAllAssistorThreads()
{
	m_ThreadPool.WaitForAllThreads();
}

//-----------------------------------------------------------------------------
// 描述: 打断指定辅助线程的睡眠
//-----------------------------------------------------------------------------
void CAssistorServer::InterruptAssistorThreadSleep(int nAssistorIndex)
{
	m_ThreadPool.InterruptThreadSleep(nAssistorIndex);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
