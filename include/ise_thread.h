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

class Thread;

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
// class ThreadImpl - 平台线程实现基类

class ThreadImpl
{
public:
	friend class Thread;
protected:
	Thread& thread_;              // 相关联的 Thread 对象
	THREAD_ID threadId_;          // 线程ID
	bool isExecuting_;            // 线程是否正在执行线程函数
	bool isRunCalled_;            // run() 函数是否已被调用过
	int termTime_;                // 调用 terminate() 时的时间戳
	bool isFreeOnTerminate_;      // 线程退出时是否同时释放类对象
	bool terminated_;             // 是否应退出的标志
	bool isSleepInterrupted_;     // 睡眠是否被中断
	int returnValue_;             // 线程返回值 (可在 execute 函数中修改此值，函数 waitFor 返回此值)

protected:
	void execute();
	void beforeTerminate();
	void beforeKill();

	void checkNotRunning();
public:
	ThreadImpl(Thread *thread);
	virtual ~ThreadImpl() {}

	virtual void run() = 0;
	virtual void terminate();
	virtual void kill() = 0;
	virtual int waitFor() = 0;

	void sleep(double seconds);
	void interruptSleep() { isSleepInterrupted_ = true; }
	bool isRunning() { return isExecuting_; }

	// 属性 (getter)
	Thread* getThread() { return (Thread*)&thread_; }
	THREAD_ID getThreadId() const { return threadId_; }
	int isTerminated() const { return terminated_; }
	int getReturnValue() const { return returnValue_; }
	bool isFreeOnTerminate() const { return isFreeOnTerminate_; }
	int getTermElapsedSecs() const;
	// 属性 (setter)
	void setThreadId(THREAD_ID value) { threadId_ = value; }
	void setExecuting(bool value) { isExecuting_ = value; }
	void setTerminated(bool value);
	void setReturnValue(int value) { returnValue_ = value; }
	void setFreeOnTerminate(bool value) { isFreeOnTerminate_ = value; }
};

///////////////////////////////////////////////////////////////////////////////
// class Win32ThreadImpl - Win32平台线程实现类

#ifdef ISE_WIN32
class Win32ThreadImpl : public ThreadImpl
{
public:
	friend UINT __stdcall threadExecProc(void *param);

protected:
	HANDLE handle_;               // 线程句柄
	int priority_;                // 线程优先级

private:
	void checkThreadError(bool success);

public:
	Win32ThreadImpl(Thread *thread);
	virtual ~Win32ThreadImpl();

	virtual void run();
	virtual void terminate();
	virtual void kill();
	virtual int waitFor();

	int getPriority() const { return priority_; }
	void setPriority(int value);
};
#endif

///////////////////////////////////////////////////////////////////////////////
// class LinuxThreadImpl - Linux平台线程实现类

#ifdef ISE_LINUX
class LinuxThreadImpl : public ThreadImpl
{
public:
	friend void threadFinalProc(void *param);
	friend void* threadExecProc(void *param);

protected:
	int policy_;                  // 线程调度策略 (THREAD_POLICY_XXX)
	int priority_;                // 线程优先级 (0..99)
	Semaphore *execSem_;          // 用于启动线程函数时暂时阻塞

private:
	void checkThreadError(int errorCode);

public:
	LinuxThreadImpl(Thread *thread);
	virtual ~LinuxThreadImpl();

	virtual void run();
	virtual void terminate();
	virtual void kill();
	virtual int waitFor();

	int getPolicy() const { return policy_; }
	int getPriority() const { return priority_; }
	void setPolicy(int value);
	void setPriority(int value);
};
#endif

///////////////////////////////////////////////////////////////////////////////
// class Thread - 线程类

typedef void (*THREAD_EXEC_PROC)(void *param);

class Thread
{
public:
	friend class ThreadImpl;

private:
#ifdef ISE_WIN32
	Win32ThreadImpl threadImpl_;
#endif
#ifdef ISE_LINUX
	LinuxThreadImpl threadImpl_;
#endif

	THREAD_EXEC_PROC execProc_;
	void *threadParam_;

protected:
	// 线程的执行函数，子类必须重写。
	virtual void execute() { if (execProc_ != NULL) (*execProc_)(threadParam_); }

	// 执行 terminate() 前的附加操作。
	// 注: 由于 terminate() 属于自愿退出机制，为了能让线程能尽快退出，除了
	// terminated_ 标志被设为 true 之外，有时还应当补充一些附加的操作以
	// 便能让线程尽快从阻塞操作中解脱出来。
	virtual void beforeTerminate() {}

	// 执行 kill() 前的附加操作。
	// 注: 线程被杀死后，用户所管理的某些重要资源可能未能得到释放，比如锁资源
	// (还未来得及解锁便被杀了)，所以重要资源的释放工作必须在 beforeKill 中进行。
	virtual void beforeKill() {}
public:
	Thread() : threadImpl_(this), execProc_(NULL), threadParam_(NULL) {}
	virtual ~Thread() {}

	// 创建一个线程并马上执行
	static void create(THREAD_EXEC_PROC execProc, void *param = NULL);

	// 创建并执行线程。
	// 注: 此成员方法在对象声明周期中只可调用一次。
	void run() { threadImpl_.run(); }

	// 通知线程退出 (自愿退出机制)
	// 注: 若线程由于某些阻塞式操作迟迟不退出，可调用 kill() 强行退出。
	void terminate() { threadImpl_.terminate(); }

	// 强行杀死线程 (强行退出机制)
	void kill() { threadImpl_.kill(); }

	// 等待线程退出
	int waitFor() { return threadImpl_.waitFor(); }

	// 进入睡眠状态 (睡眠过程中会检测 terminated_ 的状态)
	// 注: 此函数必须由线程自己调用方可生效。
	void sleep(double seconds) { threadImpl_.sleep(seconds); }
	// 打断睡眠
	void interruptSleep() { threadImpl_.interruptSleep(); }

	// 判断线程是否正在运行
	bool isRunning() { return threadImpl_.isRunning(); }

	// 属性 (getter)
	THREAD_ID getThreadId() const { return threadImpl_.getThreadId(); }
	int isTerminated() const { return threadImpl_.isTerminated(); }
	int getReturnValue() const { return threadImpl_.getReturnValue(); }
	bool isFreeOnTerminate() const { return threadImpl_.isFreeOnTerminate(); }
	int getTermElapsedSecs() const { return threadImpl_.getTermElapsedSecs(); }
#ifdef ISE_WIN32
	int getPriority() const { return threadImpl_.getPriority(); }
#endif
#ifdef ISE_LINUX
	int getPolicy() const { return threadImpl_.getPolicy(); }
	int getPriority() const { return threadImpl_.getPriority(); }
#endif
	// 属性 (setter)
	void setTerminated(bool value) { threadImpl_.setTerminated(value); }
	void setReturnValue(int value) { threadImpl_.setReturnValue(value); }
	void setFreeOnTerminate(bool value) { threadImpl_.setFreeOnTerminate(value); }
#ifdef ISE_WIN32
	void setPriority(int value) { threadImpl_.setPriority(value); }
#endif
#ifdef ISE_LINUX
	void setPolicy(int value) { threadImpl_.setPolicy(value); }
	void setPriority(int value) { threadImpl_.setPriority(value); }
#endif
};

///////////////////////////////////////////////////////////////////////////////
// class ThreadList - 线程列表类

class ThreadList
{
protected:
	ObjectList<Thread> items_;
	mutable CriticalSection lock_;
public:
	ThreadList();
	virtual ~ThreadList();

	void add(Thread *thread);
	void remove(Thread *thread);
	bool exists(Thread *thread);
	void clear();

	void terminateAllThreads();
	void waitForAllThreads(int maxWaitForSecs = 5, int *killedCountPtr = NULL);

	int getCount() const { return items_.getCount(); }
	Thread* getItem(int index) const { return items_[index]; }
	Thread* operator[] (int index) const { return getItem(index); }

	CriticalSection& getLock() const { return lock_; }
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_THREAD_H_
