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
// ise_sys_threads.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SYS_THREADS_H_
#define _ISE_SYS_THREADS_H_

#include "ise_options.h"
#include "ise_global_defs.h"
#include "ise_classes.h"
#include "ise_thread.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// classes

class CSysThread;
class CSysDaemonThread;
class CSysSchedulerThread;
class CSysThreadMgr;

///////////////////////////////////////////////////////////////////////////////
// class CSysThread

class CSysThread : public CThread
{
protected:
	CSysThreadMgr& m_ThreadMgr;
public:
	CSysThread(CSysThreadMgr& ThreadMgr);
	virtual ~CSysThread();
};

///////////////////////////////////////////////////////////////////////////////
// class CSysDaemonThread

class CSysDaemonThread : public CSysThread
{
protected:
	virtual void Execute();
public:
	CSysDaemonThread(CSysThreadMgr& ThreadMgr) : CSysThread(ThreadMgr) {}
};

///////////////////////////////////////////////////////////////////////////////
// class CSysSchedulerThread

class CSysSchedulerThread : public CSysThread
{
protected:
	virtual void Execute();
public:
	CSysSchedulerThread(CSysThreadMgr& ThreadMgr) : CSysThread(ThreadMgr) {}
};

///////////////////////////////////////////////////////////////////////////////
// class CSysThreadMgr

class CSysThreadMgr
{
private:
	friend class CSysThread;
private:
	CThreadList m_ThreadList;
private:
	void RegisterThread(CSysThread *pThread);
	void UnregisterThread(CSysThread *pThread);
public:
	CSysThreadMgr() {}
	~CSysThreadMgr() {}

	void Initialize();
	void Finalize();
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SYS_THREADS_H_
