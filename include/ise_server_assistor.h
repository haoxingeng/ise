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
#include "ise_sysutils.h"
#include "ise_socket.h"
#include "ise_exceptions.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class CAssistorThread;
class CAssistorThreadPool;
class CAssistorServer;

///////////////////////////////////////////////////////////////////////////////
// class CAssistorThread - 辅助线程类
//
// 说明:
// 1. 对于服务端程序，除了网络服务之外，一般还需要若干后台守护线程，用于后台维护工作，
//    比如垃圾数据回收、数据库过期数据清理等等。这类服务线程统称为 assistor thread.

class CAssistorThread : public CThread
{
private:
	CAssistorThreadPool *m_pOwnPool;        // 所属线程池
	int m_nAssistorIndex;                   // 辅助服务序号(0-based)
protected:
	virtual void Execute();
	virtual void DoKill();
public:
	CAssistorThread(CAssistorThreadPool *pThreadPool, int nAssistorIndex);
	virtual ~CAssistorThread();

	int GetIndex() const { return m_nAssistorIndex; }
};

///////////////////////////////////////////////////////////////////////////////
// class CAssistorThreadPool - 辅助线程池类

class CAssistorThreadPool
{
private:
	CAssistorServer *m_pOwnAssistorSvr;     // 所属辅助服务器
	CThreadList m_ThreadList;               // 线程列表

public:
	explicit CAssistorThreadPool(CAssistorServer *pOwnAssistorServer);
	virtual ~CAssistorThreadPool();

	void RegisterThread(CAssistorThread *pThread);
	void UnregisterThread(CAssistorThread *pThread);

	// 通知所有线程退出
	void TerminateAllThreads();
	// 等待所有线程退出
	void WaitForAllThreads();
	// 打断指定线程的睡眠
	void InterruptThreadSleep(int nAssistorIndex);

	// 取得当前线程数量
	int GetThreadCount() { return m_ThreadList.GetCount(); }
	// 取得所属辅助服务器
	CAssistorServer& GetAssistorServer() { return *m_pOwnAssistorSvr; }
};

///////////////////////////////////////////////////////////////////////////////
// class CAssistorServer - 辅助服务类

class CAssistorServer
{
private:
	bool m_bActive;                         // 服务器是否启动
	CAssistorThreadPool m_ThreadPool;       // 辅助线程池

public:
	explicit CAssistorServer();
	virtual ~CAssistorServer();

	// 启动服务器
	void Open();
	// 关闭服务器
	void Close();

	// 辅助服务线程执行函数
	void OnAssistorThreadExecute(CAssistorThread& AssistorThread, int nAssistorIndex);

	// 通知所有辅助线程退出
	void TerminateAllAssistorThreads();
	// 等待所有辅助线程退出
	void WaitForAllAssistorThreads();
	// 打断指定辅助线程的睡眠
	void InterruptAssistorThreadSleep(int nAssistorIndex);
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SERVER_ASSISTOR_H_
