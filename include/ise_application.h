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

class IseBusiness;
class IseOptions;
class IseMainServer;
class IseApplication;

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
typedef void (*USER_SIGNAL_HANDLER_PROC)(void *param, int signalNumber);

///////////////////////////////////////////////////////////////////////////////
// class IseBusiness - ISE业务基类

class IseBusiness
{
public:
	IseBusiness() {}
	virtual ~IseBusiness() {}

	// 初始化 (失败则抛出异常)
	virtual void initialize() {}
	// 结束化 (无论初始化是否有异常，结束时都会执行)
	virtual void finalize() {}

	// 解释命令行参数，参数不正确则返回 false
	virtual bool parseArguments(int argc, char *argv[]) { return true; }
	// 返回程序的当前版本号
	virtual string getAppVersion() { return "1.0.0.0"; }
	// 返回程序的帮助信息
	virtual string getAppHelp() { return ""; }
	// 处理启动状态
	virtual void doStartupState(STARTUP_STATE state) {}
	// 初始化ISE配置信息
	virtual void initIseOptions(IseOptions& options) {}

	// UDP数据包分类
	virtual void classifyUdpPacket(void *packetBuffer, int packetSize, int& groupIndex) { groupIndex = 0; }
	// UDP数据包分派
	virtual void dispatchUdpPacket(UdpWorkerThread& workerThread, int groupIndex, UdpPacket& packet) {}

	// 接受了一个新的TCP连接
	virtual void onTcpConnection(TcpConnection *connection) {}
	// TCP连接传输过程发生了错误 (ISE将随之删除此连接对象)
	virtual void onTcpError(TcpConnection *connection) {}
	// TCP连接上的一个接收任务已完成
	virtual void onTcpRecvComplete(TcpConnection *connection, void *packetBuffer,
		int packetSize, const CustomParams& params) {}
	// TCP连接上的一个发送任务已完成
	virtual void onTcpSendComplete(TcpConnection *connection, const CustomParams& params) {}

	// 辅助服务线程执行(assistorIndex: 0-based)
	virtual void assistorThreadExecute(AssistorThread& assistorThread, int assistorIndex) {}
	// 系统守护线程执行 (secondCount: 0-based)
	virtual void daemonThreadExecute(Thread& thread, int secondCount) {}
};

///////////////////////////////////////////////////////////////////////////////
// class IseOptions - ISE配置类

class IseOptions
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

	string logFileName_;              // 日志文件名 (含路径)
	bool logNewFileDaily_;            // 是否每天用一个单独的文件存储日志
	bool isDaemon_;                   // 是否后台守护程序(daemon)
	bool allowMultiInstance_;         // 是否允许多个程序实体同时运行

	/* ------------ 服务器常规配置: ------------ */

	// 服务器类型 (ST_UDP | ST_TCP)
	UINT serverType_;
	// 后台调整工作者线程数量的时间间隔(秒)
	int adjustThreadInterval_;
	// 辅助线程的个数
	int assistorThreadCount_;

	/* ------------ UDP服务器配置: ------------ */

	struct CUdpRequestGroupOpt
	{
		int requestQueueCapacity;      // 请求队列的容量(即可容纳多少个数据包)
		int minWorkerThreads;          // 工作者线程的最少个数
		int maxWorkerThreads;          // 工作者线程的最多个数

		CUdpRequestGroupOpt()
		{
			requestQueueCapacity = DEF_UDP_REQ_QUEUE_CAPACITY;
			minWorkerThreads = DEF_UDP_WORKER_THREADS_MIN;
			maxWorkerThreads = DEF_UDP_WORKER_THREADS_MAX;
		}
	};
	typedef vector<CUdpRequestGroupOpt> CUdpRequestGroupOpts;

	// UDP服务端口
	int udpServerPort_;
	// 监听线程的数量
	int udpListenerThreadCount_;
	// 请求组别的数量
	int udpRequestGroupCount_;
	// 每个组别内的配置
	CUdpRequestGroupOpts udpRequestGroupOpts_;
	// 数据包在队列中的有效等待时间，超时则不予处理(秒)
	int udpRequestEffWaitTime_;
	// 工作者线程的工作超时时间(秒)，若为0表示不进行超时检测
	int udpWorkerThreadTimeOut_;
	// 请求队列中数据包数量警戒线，若超过警戒线则尝试增加线程
	int udpRequestQueueAlertLine_;

	/* ------------ TCP服务器配置: ------------ */

	struct CTcpServerOpt
	{
		int tcpServerPort;             // TCP服务端口号

		CTcpServerOpt()
		{
			tcpServerPort = 0;
		}
	};
	typedef vector<CTcpServerOpt> CTcpServerOpts;

	// TCP服务器的数量 (一个TCP服务器占用一个TCP端口)
	int tcpServerCount_;
	// 每个TCP服务器的配置
	CTcpServerOpts tcpServerOpts_;
	// TCP事件循环的个数
	int tcpEventLoopCount_;

public:
	IseOptions();

	// 系统配置设置/获取-------------------------------------------------------

	void setLogFileName(const string& value, bool logNewFileDaily = false)
		{ logFileName_ = value; logNewFileDaily_ = logNewFileDaily; }
	void setIsDaemon(bool value) { isDaemon_ = value; }
	void setAllowMultiInstance(bool value) { allowMultiInstance_ = value; }

	string getLogFileName() { return logFileName_; }
	bool getLogNewFileDaily() { return logNewFileDaily_; }
	bool getIsDaemon() { return isDaemon_; }
	bool getAllowMultiInstance() { return allowMultiInstance_; }

	// 服务器配置设置----------------------------------------------------------

	// 设置服务器类型(ST_UDP | ST_TCP)
	void setServerType(UINT serverType);
	// 设置后台调整工作者线程数量的时间间隔(秒)
	void setAdjustThreadInterval(int seconds);
	// 设置辅助线程的数量
	void setAssistorThreadCount(int count);

	// 设置UDP服务端口号
	void setUdpServerPort(int port);
	// 设置UDP监听线程的数量
	void setUdpListenerThreadCount(int count);
	// 设置UDP请求的组别总数
	void setUdpRequestGroupCount(int count);
	// 设置UDP请求队列的最大容量 (即可容纳多少个数据包)
	void setUdpRequestQueueCapacity(int groupIndex, int capacity);
	// 设置UDP工作者线程个数的上下限
	void setUdpWorkerThreadCount(int groupIndex, int minThreads, int maxThreads);
	// 设置UDP请求在队列中的有效等待时间，超时则不予处理(秒)
	void setUdpRequestEffWaitTime(int seconds);
	// 设置UDP工作者线程的工作超时时间(秒)，若为0表示不进行超时检测
	void setUdpWorkerThreadTimeOut(int seconds);
	// 设置UDP请求队列中数据包数量警戒线
	void SetUdpRequestQueueAlertLine(int count);

	// 设置TCP服务器的总数
	void setTcpServerCount(int count);
	// 设置TCP服务端口号
	void setTcpServerPort(int serverIndex, int port);
	// 设置TCP事件循环的个数
	void setTcpEventLoopCount(int count);

	// 服务器配置获取----------------------------------------------------------

	UINT getServerType() { return serverType_; }
	int getAdjustThreadInterval() { return adjustThreadInterval_; }
	int getAssistorThreadCount() { return assistorThreadCount_; }

	int getUdpServerPort() { return udpServerPort_; }
	int getUdpListenerThreadCount() { return udpListenerThreadCount_; }
	int getUdpRequestGroupCount() { return udpRequestGroupCount_; }
	int getUdpRequestQueueCapacity(int groupIndex);
	void getUdpWorkerThreadCount(int groupIndex, int& minThreads, int& maxThreads);
	int getUdpRequestEffWaitTime() { return udpRequestEffWaitTime_; }
	int getUdpWorkerThreadTimeOut() { return udpWorkerThreadTimeOut_; }
	int getUdpRequestQueueAlertLine() { return udpRequestQueueAlertLine_; }

	int getTcpServerCount() { return tcpServerCount_; }
	int getTcpServerPort(int serverIndex);
	int getTcpEventLoopCount() { return tcpEventLoopCount_; }
};

///////////////////////////////////////////////////////////////////////////////
// class IseMainServer - 主服务器类

class IseMainServer
{
private:
	MainUdpServer *udpServer_;        // UDP服务器
	MainTcpServer *tcpServer_;        // TCP服务器
	AssistorServer *assistorServer_;  // 辅助服务器
	SysThreadMgr *sysThreadMgr_;      // 系统线程管理器
private:
	void runBackground();
public:
	IseMainServer();
	virtual ~IseMainServer();

	void initialize();
	void finalize();
	void run();

	MainUdpServer& getMainUdpServer() { return *udpServer_; }
	MainTcpServer& getMainTcpServer() { return *tcpServer_; }
	AssistorServer& getAssistorServer() { return *assistorServer_; }
};

///////////////////////////////////////////////////////////////////////////////
// class IseApplication - ISE应用程序类
//
// 说明:
// 1. 此类是整个服务程序的主框架，全局单例对象(Application)在程序启动时即被创建；
// 2. 一般来说，服务程序是由外部发出命令(kill)而退出的。在ISE中，程序收到kill退出命令后
//    (此时当前执行点一定在 iseApplication.run() 中)，会触发 ExitProgramSignalHandler
//    信号处理器，进而利用 longjmp 方法使执行点模拟从 run() 中退出，继而执行 finalize。
// 3. 若程序发生致命的非法操作错误，会先触发 FatalErrorSignalHandler 信号处理器，
//    然后同样按照正常的析构顺序退出。
// 4. 若程序内部想正常退出，不推荐使用exit函数。而是 iseApplication.SetTeraminted(true).
//    这样才能让程序按照正常的析构顺序退出。

class IseApplication
{
private:
	friend void userSignalHandler(int sigNo);

private:
	IseOptions iseOptions_;                       // ISE配置
	IseMainServer *mainServer_;                   // 主服务器
	StringArray argList_;                         // 命令行参数
	string exeName_;                              // 可执行文件的全名(含绝对路径)
	time_t appStartTime_;                         // 程序启动时的时间
	bool initialized_;                            // 是否成功初始化
	bool terminated_;                             // 是否应退出的标志
	CallbackList<USER_SIGNAL_HANDLER_PROC> onUserSignal_;  // 用户信号处理回调

private:
	bool processStandardArgs();
	void checkMultiInstance();
	void applyIseOptions();
	void createMainServer();
	void freeMainServer();
	void createIseBusiness();
	void freeIseBusiness();
	void initExeName();
	void initDaemon();
	void initSignals();
	void initNewOperHandler();
	void closeTerminal();
	void doFinalize();

public:
	IseApplication();
	virtual ~IseApplication();

	bool parseArguments(int argc, char *argv[]);
	void initialize();
	void finalize();
	void run();

	inline IseOptions& getIseOptions() { return iseOptions_; }
	inline IseMainServer& getMainServer() { return *mainServer_; }
	inline IseScheduleTaskMgr& getScheduleTaskMgr() { return IseScheduleTaskMgr::instance(); }

	inline void setTerminated(bool value) { terminated_ = value; }
	inline bool getTerminated() { return terminated_; }

	// 取得可执行文件的全名(含绝对路径)
	string getExeName() { return exeName_; }
	// 取得可执行文件所在的路径
	string getExePath();
	// 取得命令行参数个数(首个参数为程序路径文件名)
	int getArgCount() { return (int)argList_.size(); }
	// 取得命令行参数字符串 (index: 0-based)
	string getArgString(int index);
	// 取得程序启动时的时间
	time_t getAppStartTime() { return appStartTime_; }

	// 注册用户信号处理器
	void registerUserSignalHandler(USER_SIGNAL_HANDLER_PROC proc, void *param = NULL);
};

///////////////////////////////////////////////////////////////////////////////
// 全局变量声明

// 应用程序对象
extern IseApplication iseApplication;
// ISE业务对象指针
extern IseBusiness *iseBusiness;

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_APPLICATION_H_
