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
// 文件名称: ise_sys_threads.cpp
// 功能描述: ISE系统线程
///////////////////////////////////////////////////////////////////////////////

#include "ise_sys_threads.h"
#include "ise_application.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class CSysThread

CSysThread::CSysThread(CSysThreadMgr& ThreadMgr) :
	m_ThreadMgr(ThreadMgr)
{
	SetFreeOnTerminate(true);
	m_ThreadMgr.RegisterThread(this);
}

CSysThread::~CSysThread()
{
	m_ThreadMgr.UnregisterThread(this);
}

///////////////////////////////////////////////////////////////////////////////
// class CSysDaemonThread

void CSysDaemonThread::Execute()
{
	int nSecondCount = 0;

	while (!GetTerminated())
	{
		try
		{
			pIseBusiness->DaemonThreadExecute(*this, nSecondCount);
		}
		catch (CException& e)
		{
			Logger().WriteException(e);
		}

		nSecondCount++;
		this->Sleep(1);
	}
}

///////////////////////////////////////////////////////////////////////////////
// class CSysSchedulerThread

void CSysSchedulerThread::Execute()
{
	IseApplication.GetScheduleTaskMgr().Execute(*this);
}

///////////////////////////////////////////////////////////////////////////////
// class CSysThreadMgr

void CSysThreadMgr::RegisterThread(CSysThread *pThread)
{
	m_ThreadList.Add(pThread);
}

//-----------------------------------------------------------------------------

void CSysThreadMgr::UnregisterThread(CSysThread *pThread)
{
	m_ThreadList.Remove(pThread);
}

//-----------------------------------------------------------------------------

void CSysThreadMgr::Initialize()
{
	m_ThreadList.Clear();
	CThread *pThread;

	pThread = new CSysDaemonThread(*this);
	m_ThreadList.Add(pThread);
	pThread->Run();

	pThread = new CSysSchedulerThread(*this);
	m_ThreadList.Add(pThread);
	pThread->Run();
}

//-----------------------------------------------------------------------------

void CSysThreadMgr::Finalize()
{
	const int MAX_WAIT_FOR_SECS = 5;
	int nKilledCount = 0;

	m_ThreadList.WaitForAllThreads(MAX_WAIT_FOR_SECS, &nKilledCount);

	if (nKilledCount > 0)
		Logger().WriteFmt(SEM_THREAD_KILLED, nKilledCount, "system");
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
