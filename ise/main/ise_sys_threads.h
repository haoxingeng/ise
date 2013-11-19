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

#include "ise/main/ise_options.h"
#include "ise/main/ise_global_defs.h"
#include "ise/main/ise_classes.h"
#include "ise/main/ise_thread.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// classes

class SysThread;
class SysDaemonThread;
class SysSchedulerThread;
class SysThreadMgr;

///////////////////////////////////////////////////////////////////////////////
// class SysThread

class SysThread : public Thread
{
public:
    SysThread(SysThreadMgr& threadMgr);
    virtual ~SysThread();
protected:
    SysThreadMgr& threadMgr_;
};

///////////////////////////////////////////////////////////////////////////////
// class SysDaemonThread

class SysDaemonThread : public SysThread
{
public:
    SysDaemonThread(SysThreadMgr& threadMgr) : SysThread(threadMgr) {}
protected:
    virtual void execute();
};

///////////////////////////////////////////////////////////////////////////////
// class SysThreadMgr

class SysThreadMgr : boost::noncopyable
{
public:
    SysThreadMgr() {}
    ~SysThreadMgr() {}

    void initialize();
    void finalize();

private:
    void registerThread(SysThread *thread);
    void unregisterThread(SysThread *thread);

private:
    ThreadList threadList_;
    friend class SysThread;
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SYS_THREADS_H_
