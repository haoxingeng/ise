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
// 文件名称: ise_thread.cpp
// 功能描述: 线程类
///////////////////////////////////////////////////////////////////////////////

#include "ise_thread.h"
#include "ise_sysutils.h"
#include "ise_classes.h"
#include "ise_exceptions.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class CThreadImpl

CThreadImpl::CThreadImpl(CThread *pThread) :
	m_Thread(*pThread),
	m_nThreadId(0),
	m_bExecuting(false),
	m_bRunCalled(false),
	m_nTermTime(0),
	m_bFreeOnTerminate(false),
	m_bTerminated(false),
	m_bSleepInterrupted(false),
	m_nReturnValue(0)
{
	// nothing
}

//-----------------------------------------------------------------------------

void CThreadImpl::Execute()
{
	m_Thread.Execute();
}

//-----------------------------------------------------------------------------

void CThreadImpl::BeforeTerminate()
{
	m_Thread.BeforeTerminate();
}

//-----------------------------------------------------------------------------

void CThreadImpl::BeforeKill()
{
	m_Thread.BeforeKill();
}

//-----------------------------------------------------------------------------
// 描述: 如果线程已运行，则抛出异常
//-----------------------------------------------------------------------------
void CThreadImpl::CheckNotRunning()
{
	if (m_bRunCalled)
		IseThrowThreadException(SEM_THREAD_RUN_ONCE);
}

//-----------------------------------------------------------------------------
// 描述: 通知线程退出
//-----------------------------------------------------------------------------
void CThreadImpl::Terminate()
{
	if (!m_bTerminated)
	{
		BeforeTerminate();
		m_nTermTime = (int)time(NULL);
		m_bTerminated = true;
	}
}

//-----------------------------------------------------------------------------
// 描述: 取得从调用 Terminate 到当前共经过多少时间(秒)
//-----------------------------------------------------------------------------
int CThreadImpl::GetTermElapsedSecs() const
{
	int nResult = 0;

	// 如果已经通知退出，但线程还活着
	if (m_bTerminated && m_nThreadId != 0)
	{
		nResult = (int)(time(NULL) - m_nTermTime);
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// 描述: 设置是否 Terminate
//-----------------------------------------------------------------------------
void CThreadImpl::SetTerminated(bool bValue)
{
	if (bValue != m_bTerminated)
	{
		if (bValue)
			Terminate();
		else
		{
			m_bTerminated = false;
			m_nTermTime = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 进入睡眠状态 (睡眠过程中会检测 m_bTerminated 的状态)
// 参数:
//   fSeconds - 睡眠的秒数，可为小数，可精确到毫秒
// 注意:
//   由于将睡眠时间分成了若干份，每次睡眠时间的小误差累加起来将扩大总误差。
//-----------------------------------------------------------------------------
void CThreadImpl::Sleep(double fSeconds)
{
	const double SLEEP_INTERVAL = 0.5;      // 每次睡眠的时间(秒)
	double fOnceSecs;

	m_bSleepInterrupted = false;

	while (!GetTerminated() && fSeconds > 0 && !m_bSleepInterrupted)
	{
		fOnceSecs = (fSeconds >= SLEEP_INTERVAL ? SLEEP_INTERVAL : fSeconds);
		fSeconds -= fOnceSecs;

		SleepSec(fOnceSecs, true);
	}
}

///////////////////////////////////////////////////////////////////////////////
// class CWin32ThreadImpl

#ifdef ISE_WIN32

//-----------------------------------------------------------------------------
// 描述: 线程执行函数
// 参数:
//   pParam - 线程参数，此处指向 CWin32ThreadImpl 对象
//-----------------------------------------------------------------------------
UINT __stdcall ThreadExecProc(void *pParam)
{
	CWin32ThreadImpl *pThreadImpl = (CWin32ThreadImpl*)pParam;
	int nReturnValue = 0;

	{
		pThreadImpl->SetExecuting(true);

		// 对象 AutoFinalizer 进行自动化善后工作
		struct CAutoFinalizer
		{
			CWin32ThreadImpl *m_pThreadImpl;
			CAutoFinalizer(CWin32ThreadImpl *pThreadImpl) { m_pThreadImpl = pThreadImpl; }
			~CAutoFinalizer()
			{
				m_pThreadImpl->SetExecuting(false);
				if (m_pThreadImpl->GetFreeOnTerminate())
					delete m_pThreadImpl->GetThread();
			}
		} AutoFinalizer(pThreadImpl);

		if (!pThreadImpl->m_bTerminated)
		{
			try { pThreadImpl->Execute(); } catch (CException&) {}

			// 记下线程返回值
			nReturnValue = pThreadImpl->m_nReturnValue;
		}
	}

	// 注意: 请查阅 _endthreadex 和 ExitThread 的区别
	_endthreadex(nReturnValue);
	//ExitThread(nReturnValue);

	return nReturnValue;
}

//-----------------------------------------------------------------------------
// 描述: 构造函数
//-----------------------------------------------------------------------------
CWin32ThreadImpl::CWin32ThreadImpl(CThread *pThread) :
	CThreadImpl(pThread),
	m_nHandle(0),
	m_nPriority(THREAD_PRI_NORMAL)
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 析构函数
//-----------------------------------------------------------------------------
CWin32ThreadImpl::~CWin32ThreadImpl()
{
	if (m_nThreadId != 0)
	{
		if (m_bExecuting)
			Terminate();
		if (!m_bFreeOnTerminate)
			WaitFor();
	}

	if (m_nThreadId != 0)
	{
		CloseHandle(m_nHandle);
	}
}

//-----------------------------------------------------------------------------
// 描述: 线程错误处理
//-----------------------------------------------------------------------------
void CWin32ThreadImpl::CheckThreadError(bool bSuccess)
{
	if (!bSuccess)
	{
		string strErrMsg = SysErrorMessage(GetLastError());
		Logger().WriteStr(strErrMsg.c_str());
		IseThrowThreadException(strErrMsg.c_str());
	}
}

//-----------------------------------------------------------------------------
// 描述: 创建线程并执行
// 注意: 此成员方法在对象声明周期中只可调用一次。
//-----------------------------------------------------------------------------
void CWin32ThreadImpl::Run()
{
	CheckNotRunning();
	m_bRunCalled = true;

	// 注意: 请查阅 CRT::_beginthreadex 和 API::CreateThread 的区别，前者兼容于CRT。
	m_nHandle = (HANDLE)_beginthreadex(NULL, 0, ThreadExecProc, (LPVOID)this,
		CREATE_SUSPENDED, (UINT*)&m_nThreadId);
	//m_nHandle = CreateThread(NULL, 0, ThreadExecProc, (LPVOID)this, CREATE_SUSPENDED, (LPDWORD)&m_nThreadId);

	CheckThreadError(m_nHandle != 0);

	// 设置线程优先级
	if (m_nPriority != THREAD_PRI_NORMAL)
		SetPriority(m_nPriority);

	::ResumeThread(m_nHandle);
}

//-----------------------------------------------------------------------------
// 描述: 通知线程退出
//-----------------------------------------------------------------------------
void CWin32ThreadImpl::Terminate()
{
	CThreadImpl::Terminate();
}

//-----------------------------------------------------------------------------
// 描述: 强行杀死线程
// 注意:
//   1. 调用此函数后，对线程类对象的一切操作皆不可用(Terminate(); WaitFor(); delete pThread; 等)。
//   2. 线程被杀死后，用户所管理的某些重要资源可能未能得到释放，比如锁资源 (还未来得及解锁
//      便被杀了)，所以重要资源的释放工作必须在 BeforeKill 中进行。
//   3. Win32 下强杀线程，线程执行过程中的栈对象不会析构。
//-----------------------------------------------------------------------------
void CWin32ThreadImpl::Kill()
{
	if (m_nThreadId != 0)
	{
		BeforeKill();

		m_nThreadId = 0;
		TerminateThread(m_nHandle, 0);
		::CloseHandle(m_nHandle);
	}

	delete (CThread*)&m_Thread;
}

//-----------------------------------------------------------------------------
// 描述: 等待线程退出
// 返回: 线程返回值
//-----------------------------------------------------------------------------
int CWin32ThreadImpl::WaitFor()
{
	ISE_ASSERT(m_bFreeOnTerminate == false);

	if (m_nThreadId != 0)
	{
		WaitForSingleObject(m_nHandle, INFINITE);
		GetExitCodeThread(m_nHandle, (LPDWORD)&m_nReturnValue);
	}

	return m_nReturnValue;
}

//-----------------------------------------------------------------------------
// 描述: 设置线程的优先级
//-----------------------------------------------------------------------------
void CWin32ThreadImpl::SetPriority(int nValue)
{
	int nPriorities[7] = {
		THREAD_PRIORITY_IDLE,
		THREAD_PRIORITY_LOWEST,
		THREAD_PRIORITY_BELOW_NORMAL,
		THREAD_PRIORITY_NORMAL,
		THREAD_PRIORITY_ABOVE_NORMAL,
		THREAD_PRIORITY_HIGHEST,
		THREAD_PRIORITY_TIME_CRITICAL
	};

	nValue = (int)EnsureRange((int)nValue, 0, 6);
	m_nPriority = nValue;
	if (m_nThreadId != 0)
		SetThreadPriority(m_nHandle, nPriorities[nValue]);
}

#endif

///////////////////////////////////////////////////////////////////////////////
// class CLinuxThreadImpl

#ifdef ISE_LINUX

//-----------------------------------------------------------------------------
// 描述: 线程清理函数
// 参数:
//   pParam - 线程参数，此处指向 CLinuxThreadImpl 对象
//-----------------------------------------------------------------------------
void ThreadFinalProc(void *pParam)
{
	CLinuxThreadImpl *pThreadImpl = (CLinuxThreadImpl*)pParam;

	pThreadImpl->SetExecuting(false);
	if (pThreadImpl->GetFreeOnTerminate())
		delete pThreadImpl->GetThread();
}

//-----------------------------------------------------------------------------
// 描述: 线程执行函数
// 参数:
//   pParam - 线程参数，此处指向 CLinuxThreadImpl 对象
//-----------------------------------------------------------------------------
void* ThreadExecProc(void *pParam)
{
	CLinuxThreadImpl *pThreadImpl = (CLinuxThreadImpl*)pParam;
	int nReturnValue = 0;

	{
		// 等待线程对象准备就绪
		pThreadImpl->m_pExecSem->Wait();
		delete pThreadImpl->m_pExecSem;
		pThreadImpl->m_pExecSem = NULL;

		pThreadImpl->SetExecuting(true);

		// 线程对 cancel 信号的响应方式有三种: (1)不响应 (2)推迟到取消点再响应 (3)尽量立即响应。
		// 此处设置线程为第(3)种方式，即可马上被 cancel 信号终止。
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

		// 注册清理函数
		pthread_cleanup_push(ThreadFinalProc, pThreadImpl);

		if (!pThreadImpl->m_bTerminated)
		{
			// 线程在执行过程中若收到 cancel 信号，会抛出一个异常(究竟是什么类型的异常？)，
			// 此异常千万不可去阻拦它( try{}catch(...){} )，系统能侦测出此异常是否被彻底阻拦，
			// 若是，则会发出 SIGABRT 信号，并在终端输出 "FATAL: exception not rethrown"。
			// 所以此处的策略是只阻拦 CException 异常(在ISE中所有异常皆从 CException 继承)。
			// 在 pThread->Execute() 的执行过程中，用户应该注意如下事项:
			// 1. 请参照此处的做法去拦截异常，而切不可阻拦所有类型的异常( 即catch(...) );
			// 2. 不可抛出 CException 及其子类之外的异常。假如抛出一个整数( 如 throw 5; )，
			//    系统会因为没有此异常的处理程序而调用 abort。(尽管如此，ThreadFinalProc
			//    仍会象 pthread_cleanup_push 所承诺的那样被执行到。)
			try { pThreadImpl->Execute(); } catch (CException& e) {}

			// 记下线程返回值
			nReturnValue = pThreadImpl->m_nReturnValue;
		}

		pthread_cleanup_pop(1);

		// 屏蔽 cancel 信号，出了当前 scope 后将不可被强行终止
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	}

	pthread_exit((void*)nReturnValue);
	return NULL;
}

//-----------------------------------------------------------------------------
// 描述: 构造函数
//-----------------------------------------------------------------------------
CLinuxThreadImpl::CLinuxThreadImpl(CThread *pThread) :
	CThreadImpl(pThread),
	m_nPolicy(THREAD_POL_DEFAULT),
	m_nPriority(THREAD_PRI_DEFAULT),
	m_pExecSem(NULL)
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 析构函数
//-----------------------------------------------------------------------------
CLinuxThreadImpl::~CLinuxThreadImpl()
{
	delete m_pExecSem;
	m_pExecSem = NULL;

	if (m_nThreadId != 0)
	{
		if (m_bExecuting)
			Terminate();
		if (!m_bFreeOnTerminate)
			WaitFor();
	}

	if (m_nThreadId != 0)
	{
		pthread_detach(m_nThreadId);
	}
}

//-----------------------------------------------------------------------------
// 描述: 线程错误处理
//-----------------------------------------------------------------------------
void CLinuxThreadImpl::CheckThreadError(int nErrorCode)
{
	if (nErrorCode != 0)
	{
		string strErrMsg = SysErrorMessage(nErrorCode);
		Logger().WriteStr(strErrMsg.c_str());
		IseThrowThreadException(strErrMsg.c_str());
	}
}

//-----------------------------------------------------------------------------
// 描述: 创建线程并执行
// 注意: 此成员方法在对象声明周期中只可调用一次。
//-----------------------------------------------------------------------------
void CLinuxThreadImpl::Run()
{
	CheckNotRunning();
	m_bRunCalled = true;

	delete m_pExecSem;
	m_pExecSem = new CSemaphore(0);

	// 创建线程
	CheckThreadError(pthread_create((pthread_t*)&m_nThreadId, NULL, ThreadExecProc, (void*)this));

	// 设置线程调度策略
	if (m_nPolicy != THREAD_POL_DEFAULT)
		SetPolicy(m_nPolicy);
	// 设置线程优先级
	if (m_nPriority != THREAD_PRI_DEFAULT)
		SetPriority(m_nPriority);

	// 线程对象已准备就绪，线程函数可以开始运行
	m_pExecSem->Increase();
}

//-----------------------------------------------------------------------------
// 描述: 通知线程退出
//-----------------------------------------------------------------------------
void CLinuxThreadImpl::Terminate()
{
	CThreadImpl::Terminate();
}

//-----------------------------------------------------------------------------
// 描述: 强行杀死线程
// 注意:
//   1. 调用此函数后，线程对象即被销毁，对线程类对象的一切操作皆不可用(Terminate();
//      WaitFor(); delete pThread; 等)。
//   2. 在杀死线程前，m_bFreeOnTerminate 会自动设为 true，以便对象能自动释放。
//   3. 线程被杀死后，用户所管理的某些重要资源可能未能得到释放，比如锁资源 (还未来得及解锁
//      便被杀了)，所以重要资源的释放工作必须在 BeforeKill 中进行。
//   4. pthread 没有规定在线程收到 cancel 信号后是否进行 C++ stack unwinding，也就是说栈对象
//      的析构函数不一定会执行。实验表明，在 RedHat AS (glibc-2.3.4) 下会进行 stack unwinding，
//      而在 Debian 4.0 (glibc-2.3.6) 下则不会。
//-----------------------------------------------------------------------------
void CLinuxThreadImpl::Kill()
{
	if (m_nThreadId != 0)
	{
		BeforeKill();

		if (m_bExecuting)
		{
			SetFreeOnTerminate(true);
			pthread_cancel(m_nThreadId);
			return;
		}
	}

	delete this;
}

//-----------------------------------------------------------------------------
// 描述: 等待线程退出
// 返回: 线程返回值
//-----------------------------------------------------------------------------
int CLinuxThreadImpl::WaitFor()
{
	ISE_ASSERT(m_bFreeOnTerminate == false);

	pthread_t nThreadId = m_nThreadId;

	if (m_nThreadId != 0)
	{
		m_nThreadId = 0;
		CheckThreadError(pthread_join(nThreadId, (void**)&m_nReturnValue));
	}

	return m_nReturnValue;
}

//-----------------------------------------------------------------------------
// 描述: 设置线程的调度策略
//-----------------------------------------------------------------------------
void CLinuxThreadImpl::SetPolicy(int nValue)
{
	if (nValue != THREAD_POL_DEFAULT &&
		nValue != THREAD_POL_RR &&
		nValue != THREAD_POL_FIFO)
	{
		nValue = THREAD_POL_DEFAULT;
	}

	m_nPolicy = nValue;

	if (m_nThreadId != 0)
	{
		struct sched_param param;
		param.sched_priority = m_nPriority;
		pthread_setschedparam(m_nThreadId, m_nPolicy, &param);
	}
}

//-----------------------------------------------------------------------------
// 描述: 设置线程的优先级
//-----------------------------------------------------------------------------
void CLinuxThreadImpl::SetPriority(int nValue)
{
	if (nValue < THREAD_PRI_MIN || nValue > THREAD_PRI_MAX)
		nValue = THREAD_PRI_DEFAULT;

	m_nPriority = nValue;

	if (m_nThreadId != 0)
	{
		struct sched_param param;
		param.sched_priority = m_nPriority;
		pthread_setschedparam(m_nThreadId, m_nPolicy, &param);
	}
}

#endif

///////////////////////////////////////////////////////////////////////////////
// class CThread

//-----------------------------------------------------------------------------
// 描述: 创建一个线程并马上执行
//-----------------------------------------------------------------------------
void CThread::Create(THREAD_EXEC_PROC pExecProc, void *pParam)
{
	CThread *pThread = new CThread();

	pThread->SetFreeOnTerminate(true);
	pThread->m_pExecProc = pExecProc;
	pThread->m_pThreadParam = pParam;

	pThread->Run();
}

///////////////////////////////////////////////////////////////////////////////
// class CThreadList

CThreadList::CThreadList() :
	m_Items(false, false)
{
	// nothing
}

CThreadList::~CThreadList()
{
	// nothing
}

//-----------------------------------------------------------------------------

void CThreadList::Add(CThread *pThread)
{
	CAutoLocker Locker(m_Lock);
	m_Items.Add(pThread, false);
}

//-----------------------------------------------------------------------------

void CThreadList::Remove(CThread *pThread)
{
	CAutoLocker Locker(m_Lock);
	m_Items.Remove(pThread);
}

//-----------------------------------------------------------------------------

bool CThreadList::Exists(CThread *pThread)
{
	CAutoLocker Locker(m_Lock);
	return m_Items.Exists(pThread);
}

//-----------------------------------------------------------------------------

void CThreadList::Clear()
{
	CAutoLocker Locker(m_Lock);
	m_Items.Clear();
}

//-----------------------------------------------------------------------------
// 描述: 通知所有线程退出
//-----------------------------------------------------------------------------
void CThreadList::TerminateAllThreads()
{
	CAutoLocker Locker(m_Lock);

	for (int i = 0; i < m_Items.GetCount(); i++)
	{
		CThread *pThread = m_Items[i];
		pThread->Terminate();
	}
}

//-----------------------------------------------------------------------------
// 描述: 等待所有线程退出
// 参数:
//   nMaxWaitForSecs - 最长等待时间(秒) (为 -1 表示无限等待)
//   pKilledCount    - 传回最终强杀了多少个线程
// 注意:
//   此函数要求列表中各线程在退出时自动销毁自己并从列表中移除。
//-----------------------------------------------------------------------------
void CThreadList::WaitForAllThreads(int nMaxWaitForSecs, int *pKilledCount)
{
	const double SLEEP_INTERVAL = 0.1;  // (秒)
	double nWaitSecs = 0;
	int nKilledCount = 0;

	// 通知所有线程退出
	TerminateAllThreads();

	// 等待线程退出
	while (nWaitSecs < (UINT)nMaxWaitForSecs)
	{
		if (m_Items.GetCount() == 0) break;
		SleepSec(SLEEP_INTERVAL, true);
		nWaitSecs += SLEEP_INTERVAL;
	}

	// 若等待超时，则强行杀死各线程
	if (m_Items.GetCount() > 0)
	{
		CAutoLocker Locker(m_Lock);

		nKilledCount = m_Items.GetCount();
		for (int i = 0; i < m_Items.GetCount(); i++)
			m_Items[i]->Kill();

		m_Items.Clear();
	}

	if (pKilledCount)
		*pKilledCount = nKilledCount;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
