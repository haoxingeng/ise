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

#include "ise/main/ise_sys_threads.h"
#include "ise/main/ise_application.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class SysThread

SysThread::SysThread(SysThreadMgr& threadMgr) :
    threadMgr_(threadMgr)
{
    setAutoDelete(true);
    threadMgr_.registerThread(this);
}

SysThread::~SysThread()
{
    threadMgr_.unregisterThread(this);
}

///////////////////////////////////////////////////////////////////////////////
// class SysDaemonThread

void SysDaemonThread::execute()
{
    int secondCount = 0;

    while (!isTerminated())
    {
        try
        {
            iseApp().getIseBusiness().daemonThreadExecute(*this, secondCount);
        }
        catch (Exception& e)
        {
            logger().writeException(e);
        }

        secondCount++;
        this->sleep(1);
    }
}

///////////////////////////////////////////////////////////////////////////////
// class SysSchedulerThread

void SysSchedulerThread::execute()
{
    iseApp().getScheduleTaskMgr().execute(*this);
}

///////////////////////////////////////////////////////////////////////////////
// class SysThreadMgr

void SysThreadMgr::initialize()
{
    threadList_.clear();
    Thread *thread;

    thread = new SysDaemonThread(*this);
    threadList_.add(thread);
    thread->run();

    thread = new SysSchedulerThread(*this);
    threadList_.add(thread);
    thread->run();
}

//-----------------------------------------------------------------------------

void SysThreadMgr::finalize()
{
    const int MAX_WAIT_FOR_SECS = 5;
    int killedCount = 0;

    threadList_.waitForAllThreads(MAX_WAIT_FOR_SECS, &killedCount);

    if (killedCount > 0)
        logger().writeFmt(SEM_THREAD_KILLED, killedCount, "system");
}

//-----------------------------------------------------------------------------

void SysThreadMgr::registerThread(SysThread *thread)
{
    threadList_.add(thread);
}

//-----------------------------------------------------------------------------

void SysThreadMgr::unregisterThread(SysThread *thread)
{
    threadList_.remove(thread);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
