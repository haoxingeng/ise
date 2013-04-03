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
// ise_application.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_APPLICATION_H_
#define _ISE_APPLICATION_H_

#include "ise_options.h"
#include "ise_classes.h"
#include "ise_thread.h"
#include "ise_sysutils.h"
#include "ise_socket.h"
#include "ise_exceptions.h"
#include "ise_server_udp.h"
#include "ise_server_tcp.h"
#include "ise_server_assistor.h"
#include "ise_sys_threads.h"
#include "ise_scheduler.h"

#ifdef ISE_WIN32
#include <stdarg.h>
#include <windows.h>
#endif

#ifdef ISE_LINUX
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class CIseBusiness;
class CIseOptions;
class CIseMainServer;
class CIseApplication;

///////////////////////////////////////////////////////////////////////////////
// 类型定义

// 启动状态
enum STARTUP_STATE
{
	SS_BEFORE_START,     // 启动之前
	SS_AFTER_START,      // 启动之后
	SS_START_FAIL        // 启动失败
};

// 服务器类型(可多选或不选)
enum SERVER_TYPE
{
	ST_UDP = 0x0001,     // UDP服务器
	ST_TCP = 0x0002      // TCP服务器
};

// 用户信号处理回调
typedef void (*USER_SIGNAL_HANDLER_PROC)(void *pParam, int nSignalNumber);

///////////////////////////////////////////////////////////////////////////////
// class CIseBusiness - ISE业务基类

class CIseBusiness
{
public:
	CIseBusiness() {}
	virtual ~CIseBusiness() {}

	// 初始化 (失败则抛出异常)
	virtual void Initialize() {}
	// 结束化 (无论初始化是否有异常，结束时都会执行)
	virtual void Finalize() {}

	// 解释命令行参数，参数不正确则返回 false
	virtual bool ParseArguments(int nArgc, char *sArgv[]) { return true; }
	// 返回程序的当前版本号
	virtual string GetAppVersion() { return "1.0.0.0"; }
	// 返回程序的帮助信息
	virtual string GetAppHelp() { return ""; }
	// 处理启动状态
	virtual void DoStartupState(STARTUP_STATE nState) {}
	// 初始化ISE配置信息
	virtual void InitIseOptions(CIseOptions& IseOpt) {}

	// UDP数据包分类
	virtual void ClassifyUdpPacket(void *pPacketBuffer, int nPacketSize, int& nGroupIndex) { nGroupIndex = 0; }
	// UDP数据包分派
	virtual void DispatchUdpPacket(CUdpWorkerThread& WorkerThread, int nGroupIndex, CUdpPacket& Packet) {}

	// 接受了一个新的TCP连接
	virtual void OnTcpConnection(CTcpConnection *pConnection) {}
	// TCP连接传输过程发生了错误 (ISE将随之删除此连接对象)
	virtual void OnTcpError(CTcpConnection *pConnection) {}
	// TCP连接上的一个接收任务已完成
	virtual void OnTcpRecvComplete(CTcpConnection *pConnection, void *pPacketBuffer,
		int nPacketSize, const CCustomParams& Params) {}
	// TCP连接上的一个发送任务已完成
	virtual void OnTcpSendComplete(CTcpConnection *pConnection, const CCustomParams& Params) {}

	// 辅助服务线程执行(nAssistorIndex: 0-based)
	virtual void AssistorThreadExecute(CAssistorThread& AssistorThread, int nAssistorIndex) {}
	// 系统守护线程执行 (nSecondCount: 0-based)
	virtual void DaemonThreadExecute(CThread& Thread, int nSecondCount) {}
};

///////////////////////////////////////////////////////////////////////////////
// class CIseOptions - ISE配置类

class CIseOptions
{
public:
	// 服务器常规配置缺省值
	enum
	{
		DEF_SERVER_TYPE             = ST_UDP,    // 服务器默认类型
		DEF_ADJUST_THREAD_INTERVAL  = 5,         // 后台调整 "工作者线程数量" 的时间间隔缺省值(秒)
		DEF_ASSISTOR_THREAD_COUNT   = 0,         // 辅助线程的个数
	};

	// UDP服务器配置缺省值
	enum
	{
		DEF_UDP_SERVER_PORT         = 9000,      // UDP服务默认端口
		DEF_UDP_LISTENER_THD_COUNT  = 1,         // 监听线程的数量
		DEF_UDP_REQ_GROUP_COUNT     = 1,         // 请求组别总数的缺省值
		DEF_UDP_REQ_QUEUE_CAPACITY  = 5000,      // 请求队列的缺省容量(即能放下多少数据包)
		DEF_UDP_WORKER_THREADS_MIN  = 1,         // 每个组别中工作者线程的缺省最少个数
		DEF_UDP_WORKER_THREADS_MAX  = 8,         // 每个组别中工作者线程的缺省最多个数
		DEF_UDP_REQ_EFF_WAIT_TIME   = 10,        // 请求在队列中的有效等待时间缺省值(秒)
		DEF_UDP_WORKER_THD_TIMEOUT  = 60,        // 工作者线程的工作超时时间缺省值(秒)
		DEF_UDP_QUEUE_ALERT_LINE    = 500,       // 队列中数据包数量警戒线缺省值，若超过警戒线则尝试增加线程
	};

	// TCP服务器配置缺省值
	enum
	{
		DEF_TCP_SERVER_PORT         = 9000,      // TCP服务默认端口
		DEF_TCP_REQ_GROUP_COUNT     = 1,         // 请求组别总数的缺省值
		DEF_TCP_EVENT_LOOP_COUNT    = 8,         // TCP事件循环的个数
	};

private:
	/* ------------ 系统配置: ------------------ */

	string m_strLogFileName;            // 日志文件名 (含路径)
	bool m_bLogNewFileDaily;            // 是否每天用一个单独的文件存储日志
	bool m_bIsDaemon;                   // 是否后台守护程序(daemon)
	bool m_bAllowMultiInstance;         // 是否允许多个程序实体同时运行

	/* ------------ 服务器常规配置: ------------ */

	// 服务器类型 (ST_UDP | ST_TCP)
	UINT m_nServerType;
	// 后台调整工作者线程数量的时间间隔(秒)
	int m_nAdjustThreadInterval;
	// 辅助线程的个数
	int m_nAssistorThreadCount;

	/* ------------ UDP服务器配置: ------------ */

	struct CUdpRequestGroupOpt
	{
		int nRequestQueueCapacity;      // 请求队列的容量(即可容纳多少个数据包)
		int nMinWorkerThreads;          // 工作者线程的最少个数
		int nMaxWorkerThreads;          // 工作者线程的最多个数

		CUdpRequestGroupOpt()
		{
			nRequestQueueCapacity = DEF_UDP_REQ_QUEUE_CAPACITY;
			nMinWorkerThreads = DEF_UDP_WORKER_THREADS_MIN;
			nMaxWorkerThreads = DEF_UDP_WORKER_THREADS_MAX;
		}
	};
	typedef vector<CUdpRequestGroupOpt> CUdpRequestGroupOpts;

	// UDP服务端口
	int m_nUdpServerPort;
	// 监听线程的数量
	int m_nUdpListenerThreadCount;
	// 请求组别的数量
	int m_nUdpRequestGroupCount;
	// 每个组别内的配置
	CUdpRequestGroupOpts m_UdpRequestGroupOpts;
	// 数据包在队列中的有效等待时间，超时则不予处理(秒)
	int m_nUdpRequestEffWaitTime;
	// 工作者线程的工作超时时间(秒)，若为0表示不进行超时检测
	int m_nUdpWorkerThreadTimeOut;
	// 请求队列中数据包数量警戒线，若超过警戒线则尝试增加线程
	int m_nUdpRequestQueueAlertLine;

	/* ------------ TCP服务器配置: ------------ */

	struct CTcpServerOpt
	{
		int nTcpServerPort;             // TCP服务端口号

		CTcpServerOpt()
		{
			nTcpServerPort = 0;
		}
	};
	typedef vector<CTcpServerOpt> CTcpServerOpts;

	// TCP服务器的数量 (一个TCP服务器占用一个TCP端口)
	int m_nTcpServerCount;
	// 每个TCP服务器的配置
	CTcpServerOpts m_TcpServerOpts;
	// TCP事件循环的个数
	int m_nTcpEventLoopCount;

public:
	CIseOptions();

	// 系统配置设置/获取-------------------------------------------------------

	void SetLogFileName(const string& strValue, bool bLogNewFileDaily = false)
		{ m_strLogFileName = strValue; m_bLogNewFileDaily = bLogNewFileDaily; }
	void SetIsDaemon(bool bValue) { m_bIsDaemon = bValue; }
	void SetAllowMultiInstance(bool bValue) { m_bAllowMultiInstance = bValue; }

	string GetLogFileName() { return m_strLogFileName; }
	bool GetLogNewFileDaily() { return m_bLogNewFileDaily; }
	bool GetIsDaemon() { return m_bIsDaemon; }
	bool GetAllowMultiInstance() { return m_bAllowMultiInstance; }

	// 服务器配置设置----------------------------------------------------------

	// 设置服务器类型(ST_UDP | ST_TCP)
	void SetServerType(UINT nSvrType);
	// 设置后台调整工作者线程数量的时间间隔(秒)
	void SetAdjustThreadInterval(int nSecs);
	// 设置辅助线程的数量
	void SetAssistorThreadCount(int nCount);

	// 设置UDP服务端口号
	void SetUdpServerPort(int nPort);
	// 设置UDP监听线程的数量
	void SetUdpListenerThreadCount(int nCount);
	// 设置UDP请求的组别总数
	void SetUdpRequestGroupCount(int nCount);
	// 设置UDP请求队列的最大容量 (即可容纳多少个数据包)
	void SetUdpRequestQueueCapacity(int nGroupIndex, int nCapacity);
	// 设置UDP工作者线程个数的上下限
	void SetUdpWorkerThreadCount(int nGroupIndex, int nMinThreads, int nMaxThreads);
	// 设置UDP请求在队列中的有效等待时间，超时则不予处理(秒)
	void SetUdpRequestEffWaitTime(int nSecs);
	// 设置UDP工作者线程的工作超时时间(秒)，若为0表示不进行超时检测
	void SetUdpWorkerThreadTimeOut(int nSecs);
	// 设置UDP请求队列中数据包数量警戒线
	void SetUdpRequestQueueAlertLine(int nCount);

	// 设置TCP服务器的总数
	void SetTcpServerCount(int nCount);
	// 设置TCP服务端口号
	void SetTcpServerPort(int nServerIndex, int nPort);
	// 设置TCP事件循环的个数
	void SetTcpEventLoopCount(int nCount);

	// 服务器配置获取----------------------------------------------------------

	UINT GetServerType() { return m_nServerType; }
	int GetAdjustThreadInterval() { return m_nAdjustThreadInterval; }
	int GetAssistorThreadCount() { return m_nAssistorThreadCount; }

	int GetUdpServerPort() { return m_nUdpServerPort; }
	int GetUdpListenerThreadCount() { return m_nUdpListenerThreadCount; }
	int GetUdpRequestGroupCount() { return m_nUdpRequestGroupCount; }
	int GetUdpRequestQueueCapacity(int nGroupIndex);
	void GetUdpWorkerThreadCount(int nGroupIndex, int& nMinThreads, int& nMaxThreads);
	int GetUdpRequestEffWaitTime() { return m_nUdpRequestEffWaitTime; }
	int GetUdpWorkerThreadTimeOut() { return m_nUdpWorkerThreadTimeOut; }
	int GetUdpRequestQueueAlertLine() { return m_nUdpRequestQueueAlertLine; }

	int GetTcpServerCount() { return m_nTcpServerCount; }
	int GetTcpServerPort(int nServerIndex);
	int GetTcpEventLoopCount() { return m_nTcpEventLoopCount; }
};

///////////////////////////////////////////////////////////////////////////////
// class CIseMainServer - 主服务器类

class CIseMainServer
{
private:
	CMainUdpServer *m_pUdpServer;        // UDP服务器
	CMainTcpServer *m_pTcpServer;        // TCP服务器
	CAssistorServer *m_pAssistorServer;  // 辅助服务器
	CSysThreadMgr *m_pSysThreadMgr;      // 系统线程管理器
private:
	void RunBackground();
public:
	CIseMainServer();
	virtual ~CIseMainServer();

	void Initialize();
	void Finalize();
	void Run();

	CMainUdpServer& GetMainUdpServer() { return *m_pUdpServer; }
	CMainTcpServer& GetMainTcpServer() { return *m_pTcpServer; }
	CAssistorServer& GetAssistorServer() { return *m_pAssistorServer; }
};

///////////////////////////////////////////////////////////////////////////////
// class CIseApplication - ISE应用程序类
//
// 说明:
// 1. 此类是整个服务程序的主框架，全局单例对象(Application)在程序启动时即被创建；
// 2. 一般来说，服务程序是由外部发出命令(kill)而退出的。在ISE中，程序收到kill退出命令后
//    (此时当前执行点一定在 IseApplication.Run() 中)，会触发 ExitProgramSignalHandler
//    信号处理器，进而利用 longjmp 方法使执行点模拟从 Run() 中退出，继而执行 Finalize。
// 3. 若程序发生致命的非法操作错误，会先触发 FatalErrorSignalHandler 信号处理器，
//    然后同样按照正常的析构顺序退出。
// 4. 若程序内部想正常退出，不推荐使用exit函数。而是 IseApplication.SetTeraminted(true).
//    这样才能让程序按照正常的析构顺序退出。

class CIseApplication
{
private:
	friend void UserSignalHandler(int nSigNo);

private:
	CIseOptions m_IseOptions;                       // ISE配置
	CIseMainServer *m_pMainServer;                  // 主服务器
	StringArray m_ArgList;                          // 命令行参数
	string m_strExeName;                            // 可执行文件的全名(含绝对路径)
	time_t m_nAppStartTime;                         // 程序启动时的时间
	bool m_bInitialized;                            // 是否成功初始化
	bool m_bTerminated;                             // 是否应退出的标志
	CCallBackList<USER_SIGNAL_HANDLER_PROC> m_OnUserSignal;  // 用户信号处理回调

private:
	bool ProcessStandardArgs();
	void CheckMultiInstance();
	void ApplyIseOptions();
	void CreateMainServer();
	void FreeMainServer();
	void CreateIseBusiness();
	void FreeIseBusiness();
	void InitExeName();
	void InitDaemon();
	void InitSignals();
	void InitNewOperHandler();
	void CloseTerminal();
	void DoFinalize();

public:
	CIseApplication();
	virtual ~CIseApplication();

	bool ParseArguments(int nArgc, char *sArgv[]);
	void Initialize();
	void Finalize();
	void Run();

	inline CIseOptions& GetIseOptions() { return m_IseOptions; }
	inline CIseMainServer& GetMainServer() { return *m_pMainServer; }
	inline CIseScheduleTaskMgr& GetScheduleTaskMgr() { return CIseScheduleTaskMgr::Instance(); }

	inline void SetTerminated(bool bValue) { m_bTerminated = bValue; }
	inline bool GetTerminated() { return m_bTerminated; }

	// 取得可执行文件的全名(含绝对路径)
	string GetExeName() { return m_strExeName; }
	// 取得可执行文件所在的路径
	string GetExePath();
	// 取得命令行参数个数(首个参数为程序路径文件名)
	int GetArgCount() { return (int)m_ArgList.size(); }
	// 取得命令行参数字符串 (nIndex: 0-based)
	string GetArgString(int nIndex);
	// 取得程序启动时的时间
	time_t GetAppStartTime() { return m_nAppStartTime; }

	// 注册用户信号处理器
	void RegisterUserSignalHandler(USER_SIGNAL_HANDLER_PROC pProc, void *pParam = NULL);
};

///////////////////////////////////////////////////////////////////////////////
// 全局变量声明

// 应用程序对象
extern CIseApplication IseApplication;
// ISE业务对象指针
extern CIseBusiness *pIseBusiness;

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_APPLICATION_H_
