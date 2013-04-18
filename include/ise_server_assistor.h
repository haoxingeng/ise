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
// ise_server_assistor.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SERVER_ASSISTOR_H_
#define _ISE_SERVER_ASSISTOR_H_

#include "ise_options.h"
#include "ise_classes.h"
#include "ise_thread.h"
#include "ise_sys_utils.h"
#include "ise_socket.h"
#include "ise_exceptions.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class AssistorThread;
class AssistorThreadPool;
class AssistorServer;

///////////////////////////////////////////////////////////////////////////////
// class AssistorThread - 辅助线程类
//
// 说明:
// 1. 对于服务端程序，除了网络服务之外，一般还需要若干后台守护线程，用于后台维护工作，
//    比如垃圾数据回收、数据库过期数据清理等等。这类服务线程统称为 assistor thread.

class AssistorThread : public Thread
{
public:
    AssistorThread(AssistorThreadPool *threadPool, int assistorIndex);
    virtual ~AssistorThread();

    int getIndex() const { return assistorIndex_; }

protected:
    virtual void execute();
    virtual void doKill();

private:
    AssistorThreadPool *ownPool_;        // 所属线程池
    int assistorIndex_;                  // 辅助服务序号(0-based)
};

///////////////////////////////////////////////////////////////////////////////
// class AssistorThreadPool - 辅助线程池类

class AssistorThreadPool : boost::noncopyable
{
public:
    explicit AssistorThreadPool(AssistorServer *ownAssistorServer);
    virtual ~AssistorThreadPool();

    void registerThread(AssistorThread *thread);
    void unregisterThread(AssistorThread *thread);

    // 通知所有线程退出
    void terminateAllThreads();
    // 等待所有线程退出
    void waitForAllThreads();
    // 打断指定线程的睡眠
    void interruptThreadSleep(int assistorIndex);

    // 取得当前线程数量
    int getThreadCount() { return threadList_.getCount(); }
    // 取得所属辅助服务器
    AssistorServer& getAssistorServer() { return *ownAssistorSvr_; }

private:
    AssistorServer *ownAssistorSvr_;     // 所属辅助服务器
    ThreadList threadList_;               // 线程列表
};

///////////////////////////////////////////////////////////////////////////////
// class AssistorServer - 辅助服务类

class AssistorServer : boost::noncopyable
{
public:
    explicit AssistorServer();
    virtual ~AssistorServer();

    // 启动服务器
    void open();
    // 关闭服务器
    void close();

    // 辅助服务线程执行函数
    void onAssistorThreadExecute(AssistorThread& assistorThread, int assistorIndex);

    // 通知所有辅助线程退出
    void terminateAllAssistorThreads();
    // 等待所有辅助线程退出
    void waitForAllAssistorThreads();
    // 打断指定辅助线程的睡眠
    void interruptAssistorThreadSleep(int assistorIndex);

private:
    bool isActive_;                       // 服务器是否启动
    AssistorThreadPool threadPool_;       // 辅助线程池
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SERVER_ASSISTOR_H_
