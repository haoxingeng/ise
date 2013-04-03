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
// ise_thread.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_THREAD_H_
#define _ISE_THREAD_H_

#include "ise_options.h"

#ifdef ISE_WIN32
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <process.h>
#endif

#ifdef ISE_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#endif

#include "ise_global_defs.h"
#include "ise_errmsgs.h"
#include "ise_classes.h"

using namespace std;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
/* 说明

一、Win32平台下和Linux平台下线程的主要区别:

	1. Win32线程拥有Handle和ThreadId，而Linux线程只有ThreadId。
	2. Win32线程只有ThreadPriority，而Linux线程有ThreadPolicy和ThreadPriority。

*/
///////////////////////////////////////////////////////////////////////////////

class CThread;

///////////////////////////////////////////////////////////////////////////////
// 类型定义

#ifdef ISE_WIN32
// 线程优先级
enum
{
	THREAD_PRI_IDLE         = 0,
	THREAD_PRI_LOWEST       = 1,
	THREAD_PRI_NORMAL       = 2,
	THREAD_PRI_HIGHER       = 3,
	THREAD_PRI_HIGHEST      = 4,
	THREAD_PRI_TIMECRITICAL = 5
};
#endif

#ifdef ISE_LINUX
// 线程调度策略
enum
{
	THREAD_POL_DEFAULT      = SCHED_OTHER,
	THREAD_POL_RR           = SCHED_RR,
	THREAD_POL_FIFO         = SCHED_FIFO
};

// 线程优先级
enum
{
	THREAD_PRI_DEFAULT      = 0,
	THREAD_PRI_MIN          = 0,
	THREAD_PRI_MAX          = 99,
	THREAD_PRI_HIGH         = 80
};
#endif

#ifdef ISE_WIN32
typedef DWORD THREAD_ID;
#endif
#ifdef ISE_LINUX
typedef pthread_t THREAD_ID;
#endif

///////////////////////////////////////////////////////////////////////////////
// class CThreadImpl - 平台线程实现基类

class CThreadImpl
{
public:
	friend class CThread;
protected:
	CThread& m_Thread;              // 相关联的 CThread 对象
	THREAD_ID m_nThreadId;          // 线程ID
	bool m_bExecuting;              // 线程是否正在执行线程函数
	bool m_bRunCalled;              // Run() 函数是否已被调用过
	int m_nTermTime;                // 调用 Terminate() 时的时间戳
	bool m_bFreeOnTerminate;        // 线程退出时是否同时释放类对象
	bool m_bTerminated;             // 是否应退出的标志
	bool m_bSleepInterrupted;       // 睡眠是否被中断
	int m_nReturnValue;             // 线程返回值 (可在 Execute 函数中修改此值，函数 WaitFor 返回此值)

protected:
	void Execute();
	void BeforeTerminate();
	void BeforeKill();

	void CheckNotRunning();
public:
	CThreadImpl(CThread *pThread);
	virtual ~CThreadImpl() {}

	virtual void Run() = 0;
	virtual void Terminate();
	virtual void Kill() = 0;
	virtual int WaitFor() = 0;

	void Sleep(double fSeconds);
	void InterruptSleep() { m_bSleepInterrupted = true; }
	bool IsRunning() { return m_bExecuting; }

	// 属性 (getter)
	CThread* GetThread() { return (CThread*)&m_Thread; }
	THREAD_ID GetThreadId() const { return m_nThreadId; }
	int GetTerminated() const { return m_bTerminated; }
	int GetReturnValue() const { return m_nReturnValue; }
	bool GetFreeOnTerminate() const { return m_bFreeOnTerminate; }
	int GetTermElapsedSecs() const;
	// 属性 (setter)
	void SetThreadId(THREAD_ID nValue) { m_nThreadId = nValue; }
	void SetExecuting(bool bValue) { m_bExecuting = bValue; }
	void SetTerminated(bool bValue);
	void SetReturnValue(int nValue) { m_nReturnValue = nValue; }
	void SetFreeOnTerminate(bool bValue) { m_bFreeOnTerminate = bValue; }
};

///////////////////////////////////////////////////////////////////////////////
// class CWin32ThreadImpl - Win32平台线程实现类

#ifdef ISE_WIN32
class CWin32ThreadImpl : public CThreadImpl
{
public:
	friend UINT __stdcall ThreadExecProc(void *pParam);

protected:
	HANDLE m_nHandle;               // 线程句柄
	int m_nPriority;                // 线程优先级

private:
	void CheckThreadError(bool bSuccess);

public:
	CWin32ThreadImpl(CThread *pThread);
	virtual ~CWin32ThreadImpl();

	virtual void Run();
	virtual void Terminate();
	virtual void Kill();
	virtual int WaitFor();

	int GetPriority() const { return m_nPriority; }
	void SetPriority(int nValue);
};
#endif

///////////////////////////////////////////////////////////////////////////////
// class CLinuxThreadImpl - Linux平台线程实现类

#ifdef ISE_LINUX
class CLinuxThreadImpl : public CThreadImpl
{
public:
	friend void ThreadFinalProc(void *pParam);
	friend void* ThreadExecProc(void *pParam);

protected:
	int m_nPolicy;                  // 线程调度策略 (THREAD_POLICY_XXX)
	int m_nPriority;                // 线程优先级 (0..99)
	CSemaphore *m_pExecSem;         // 用于启动线程函数时暂时阻塞

private:
	void CheckThreadError(int nErrorCode);

public:
	CLinuxThreadImpl(CThread *pThread);
	virtual ~CLinuxThreadImpl();

	virtual void Run();
	virtual void Terminate();
	virtual void Kill();
	virtual int WaitFor();

	int GetPolicy() const { return m_nPolicy; }
	int GetPriority() const { return m_nPriority; }
	void SetPolicy(int nValue);
	void SetPriority(int nValue);
};
#endif

///////////////////////////////////////////////////////////////////////////////
// class CThread - 线程类

typedef void (*THREAD_EXEC_PROC)(void *pParam);

class CThread
{
public:
	friend class CThreadImpl;

private:
#ifdef ISE_WIN32
	CWin32ThreadImpl m_ThreadImpl;
#endif
#ifdef ISE_LINUX
	CLinuxThreadImpl m_ThreadImpl;
#endif

	THREAD_EXEC_PROC m_pExecProc;
	void *m_pThreadParam;

protected:
	// 线程的执行函数，子类必须重写。
	virtual void Execute() { if (m_pExecProc != NULL) (*m_pExecProc)(m_pThreadParam); }

	// 执行 Terminate() 前的附加操作。
	// 注: 由于 Terminate() 属于自愿退出机制，为了能让线程能尽快退出，除了
	// m_bTerminated 标志被设为 true 之外，有时还应当补充一些附加的操作以
	// 便能让线程尽快从阻塞操作中解脱出来。
	virtual void BeforeTerminate() {}

	// 执行 Kill() 前的附加操作。
	// 注: 线程被杀死后，用户所管理的某些重要资源可能未能得到释放，比如锁资源
	// (还未来得及解锁便被杀了)，所以重要资源的释放工作必须在 BeforeKill 中进行。
	virtual void BeforeKill() {}
public:
	CThread() : m_ThreadImpl(this), m_pExecProc(NULL), m_pThreadParam(NULL) {}
	virtual ~CThread() {}

	// 创建一个线程并马上执行
	static void Create(THREAD_EXEC_PROC pExecProc, void *pParam = NULL);

	// 创建并执行线程。
	// 注: 此成员方法在对象声明周期中只可调用一次。
	void Run() { m_ThreadImpl.Run(); }

	// 通知线程退出 (自愿退出机制)
	// 注: 若线程由于某些阻塞式操作迟迟不退出，可调用 Kill() 强行退出。
	void Terminate() { m_ThreadImpl.Terminate(); }

	// 强行杀死线程 (强行退出机制)
	void Kill() { m_ThreadImpl.Kill(); }

	// 等待线程退出
	int WaitFor() { return m_ThreadImpl.WaitFor(); }

	// 进入睡眠状态 (睡眠过程中会检测 m_bTerminated 的状态)
	// 注: 此函数必须由线程自己调用方可生效。
	void Sleep(double fSeconds) { m_ThreadImpl.Sleep(fSeconds); }
	// 打断睡眠
	void InterruptSleep() { m_ThreadImpl.InterruptSleep(); }

	// 判断线程是否正在运行
	bool IsRunning() { return m_ThreadImpl.IsRunning(); }

	// 属性 (getter)
	THREAD_ID GetThreadId() const { return m_ThreadImpl.GetThreadId(); }
	int GetTerminated() const { return m_ThreadImpl.GetTerminated(); }
	int GetReturnValue() const { return m_ThreadImpl.GetReturnValue(); }
	bool GetFreeOnTerminate() const { return m_ThreadImpl.GetFreeOnTerminate(); }
	int GetTermElapsedSecs() const { return m_ThreadImpl.GetTermElapsedSecs(); }
#ifdef ISE_WIN32
	int GetPriority() const { return m_ThreadImpl.GetPriority(); }
#endif
#ifdef ISE_LINUX
	int GetPolicy() const { return m_ThreadImpl.GetPolicy(); }
	int GetPriority() const { return m_ThreadImpl.GetPriority(); }
#endif
	// 属性 (setter)
	void SetTerminated(bool bValue) { m_ThreadImpl.SetTerminated(bValue); }
	void SetReturnValue(int nValue) { m_ThreadImpl.SetReturnValue(nValue); }
	void SetFreeOnTerminate(bool bValue) { m_ThreadImpl.SetFreeOnTerminate(bValue); }
#ifdef ISE_WIN32
	void SetPriority(int nValue) { m_ThreadImpl.SetPriority(nValue); }
#endif
#ifdef ISE_LINUX
	void SetPolicy(int nValue) { m_ThreadImpl.SetPolicy(nValue); }
	void SetPriority(int nValue) { m_ThreadImpl.SetPriority(nValue); }
#endif
};

///////////////////////////////////////////////////////////////////////////////
// class CThreadList - 线程列表类

class CThreadList
{
protected:
	CObjectList<CThread> m_Items;
	mutable CCriticalSection m_Lock;
public:
	CThreadList();
	virtual ~CThreadList();

	void Add(CThread *pThread);
	void Remove(CThread *pThread);
	bool Exists(CThread *pThread);
	void Clear();

	void TerminateAllThreads();
	void WaitForAllThreads(int nMaxWaitForSecs = 5, int *pKilledCount = NULL);

	int GetCount() const { return m_Items.GetCount(); }
	CThread* GetItem(int nIndex) const { return m_Items[nIndex]; }
	CThread* operator[] (int nIndex) const { return GetItem(nIndex); }

	CCriticalSection& GetLock() const { return m_Lock; }
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_THREAD_H_
