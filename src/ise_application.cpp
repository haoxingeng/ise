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
// 文件名称: ise_application.cpp
// 功能描述: 服务器应用主单元
///////////////////////////////////////////////////////////////////////////////

#include "ise_application.h"
#include "ise_errmsgs.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////
// 外部函数声明

CIseBusiness* CreateIseBusinessObject();

///////////////////////////////////////////////////////////////////////////////
// 主函数

#ifdef ISE_WIN32

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	try
	{
		if (IseApplication.ParseArguments(__argc, __argv))
		{
			struct CAppFinalizer {
				~CAppFinalizer() { IseApplication.Finalize(); }
			} AppFinalizer;

			IseApplication.Initialize();
			IseApplication.Run();
		}
	}
	catch (CException& e)
	{
		Logger().WriteException(e);
	}

	return 0;
}

#endif
#ifdef ISE_LINUX

int main(int argc, char *argv[])
{
	try
	{
		if (IseApplication.ParseArguments(argc, argv))
		{
			struct CAppFinalizer {
				~CAppFinalizer() { IseApplication.Finalize(); }
			} AppFinalizer;

			IseApplication.Initialize();
			IseApplication.Run();
		}
	}
	catch (CException& e)
	{
		cout << e.MakeLogStr() << endl << endl;
		Logger().WriteException(e);
	}

	return 0;
}

#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 全局变量定义

// 应用程序对象
CIseApplication IseApplication;
// ISE业务对象指针
CIseBusiness *pIseBusiness;

#ifdef ISE_LINUX
// 用于进程退出时长跳转
static sigjmp_buf ProcExitJmpBuf;
#endif

// 用于内存不足的情况下程序退出
static char *pReservedMemoryForExit;

///////////////////////////////////////////////////////////////////////////////
// 信号处理器

#ifdef ISE_LINUX
//-----------------------------------------------------------------------------
// 描述: 正常退出程序 信号处理器
//-----------------------------------------------------------------------------
void ExitProgramSignalHandler(int nSigNo)
{
	static bool bInHandler = false;
	if (bInHandler) return;
	bInHandler = true;

	// 停止主线程循环
	IseApplication.SetTerminated(true);

	Logger().WriteFmt(SEM_SIG_TERM, nSigNo);

	siglongjmp(ProcExitJmpBuf, 1);
}

//-----------------------------------------------------------------------------
// 描述: 致命非法操作 信号处理器
//-----------------------------------------------------------------------------
void FatalErrorSignalHandler(int nSigNo)
{
	static bool bInHandler = false;
	if (bInHandler) return;
	bInHandler = true;

	// 停止主线程循环
	IseApplication.SetTerminated(true);

	Logger().WriteFmt(SEM_SIG_FATAL_ERROR, nSigNo);
	abort();
}
#endif

//-----------------------------------------------------------------------------
// 描述: 用户信号处理器
//-----------------------------------------------------------------------------
void UserSignalHandler(int nSigNo)
{
	const CCallBackList<USER_SIGNAL_HANDLER_PROC>& ProcList = IseApplication.m_OnUserSignal;

	for (int i = 0; i < ProcList.GetCount(); i++)
	{
		const CCallBackDef<USER_SIGNAL_HANDLER_PROC>& ProcItem = ProcList.GetItem(i);
		ProcItem.pProc(ProcItem.pParam, nSigNo);
	}
}

//-----------------------------------------------------------------------------
// 描述: 内存不足错误处理器
// 备注:
//   若未安装错误处理器(set_new_handler)，则当 new 操作失败时抛出 bad_alloc 异常；
//   而安装错误处理器后，new 操作将不再抛出异常，而是调用处理器函数。
//-----------------------------------------------------------------------------
void OutOfMemoryHandler()
{
	static bool bInHandler = false;
	if (bInHandler) return;
	bInHandler = true;

	// 释放保留内存，以免程序退出过程中再次出现内存不足
	delete[] pReservedMemoryForExit;
	pReservedMemoryForExit = NULL;

	Logger().WriteStr(SEM_OUT_OF_MEMORY);
	abort();
}

///////////////////////////////////////////////////////////////////////////////
// class CIseOptions

CIseOptions::CIseOptions()
{
	m_strLogFileName = "";
	m_bLogNewFileDaily = false;
	m_bIsDaemon = false;
	m_bAllowMultiInstance = false;

	SetServerType(DEF_SERVER_TYPE);
	SetAdjustThreadInterval(DEF_ADJUST_THREAD_INTERVAL);
	SetAssistorThreadCount(DEF_ASSISTOR_THREAD_COUNT);

	SetUdpServerPort(DEF_UDP_SERVER_PORT);
	SetUdpListenerThreadCount(DEF_UDP_LISTENER_THD_COUNT);
	SetUdpRequestGroupCount(DEF_UDP_REQ_GROUP_COUNT);
	for (int i = 0; i < DEF_UDP_REQ_GROUP_COUNT; i++)
	{
		SetUdpRequestQueueCapacity(i, DEF_UDP_REQ_QUEUE_CAPACITY);
		SetUdpWorkerThreadCount(i, DEF_UDP_WORKER_THREADS_MIN, DEF_UDP_WORKER_THREADS_MAX);
	}
	SetUdpRequestEffWaitTime(DEF_UDP_REQ_EFF_WAIT_TIME);
	SetUdpWorkerThreadTimeOut(DEF_UDP_WORKER_THD_TIMEOUT);
	SetUdpRequestQueueAlertLine(DEF_UDP_QUEUE_ALERT_LINE);

	SetTcpServerCount(DEF_TCP_REQ_GROUP_COUNT);
	for (int i = 0; i < DEF_TCP_REQ_GROUP_COUNT; i++)
		SetTcpServerPort(i, DEF_TCP_SERVER_PORT);
	SetTcpEventLoopCount(DEF_TCP_EVENT_LOOP_COUNT);
}

//-----------------------------------------------------------------------------
// 描述: 设置服务器类型(UDP|TCP)
// 参数:
//   nSvrType - 服务器器类型(可多选或不选)
// 示例:
//   SetServerType(ST_UDP | ST_TCP);
//-----------------------------------------------------------------------------
void CIseOptions::SetServerType(UINT nSvrType)
{
	m_nServerType = nSvrType;
}

//-----------------------------------------------------------------------------
// 描述: 设置后台维护工作者线程数量的时间间隔(秒)
//-----------------------------------------------------------------------------
void CIseOptions::SetAdjustThreadInterval(int nSecs)
{
	if (nSecs <= 0) nSecs = DEF_ADJUST_THREAD_INTERVAL;
	m_nAdjustThreadInterval = nSecs;
}

//-----------------------------------------------------------------------------
// 描述: 设置辅助线程的数量
//-----------------------------------------------------------------------------
void CIseOptions::SetAssistorThreadCount(int nCount)
{
	if (nCount < 0) nCount = 0;
	m_nAssistorThreadCount = nCount;
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP服务端口号
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpServerPort(int nPort)
{
	m_nUdpServerPort = nPort;
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP监听线程的数量
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpListenerThreadCount(int nCount)
{
	if (nCount < 1) nCount = 1;

	m_nUdpListenerThreadCount = nCount;
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP数据包的组别总数
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpRequestGroupCount(int nCount)
{
	if (nCount <= 0) nCount = DEF_UDP_REQ_GROUP_COUNT;

	m_nUdpRequestGroupCount = nCount;
	m_UdpRequestGroupOpts.resize(nCount);
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP请求队列的最大容量 (即可容纳多少个数据包)
// 参数:
//   nGroupIndex - 组别号 (0-based)
//   nCapacity   - 容量
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpRequestQueueCapacity(int nGroupIndex, int nCapacity)
{
	if (nGroupIndex < 0 || nGroupIndex >= m_nUdpRequestGroupCount) return;

	if (nCapacity <= 0) nCapacity = DEF_UDP_REQ_QUEUE_CAPACITY;

	m_UdpRequestGroupOpts[nGroupIndex].nRequestQueueCapacity = nCapacity;
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP工作者线程个数的上下限
// 参数:
//   nGroupIndex - 组别号 (0-based)
//   nMinThreads - 线程个数的下限
//   nMaxThreads - 线程个数的上限
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpWorkerThreadCount(int nGroupIndex, int nMinThreads, int nMaxThreads)
{
	if (nGroupIndex < 0 || nGroupIndex >= m_nUdpRequestGroupCount) return;

	if (nMinThreads < 1) nMinThreads = 1;
	if (nMaxThreads < nMinThreads) nMaxThreads = nMinThreads;

	m_UdpRequestGroupOpts[nGroupIndex].nMinWorkerThreads = nMinThreads;
	m_UdpRequestGroupOpts[nGroupIndex].nMaxWorkerThreads = nMaxThreads;
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP请求在队列中的有效等待时间，超时则不予处理
// 参数:
//   nMSecs - 等待秒数
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpRequestEffWaitTime(int nSecs)
{
	if (nSecs <= 0) nSecs = DEF_UDP_REQ_EFF_WAIT_TIME;
	m_nUdpRequestEffWaitTime = nSecs;
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP工作者线程的工作超时时间(秒)，若为0表示不进行超时检测
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpWorkerThreadTimeOut(int nSecs)
{
	if (nSecs < 0) nSecs = 0;
	m_nUdpWorkerThreadTimeOut = nSecs;
}

//-----------------------------------------------------------------------------
// 描述: 设置请求队列中数据包数量警戒线
//-----------------------------------------------------------------------------
void CIseOptions::SetUdpRequestQueueAlertLine(int nCount)
{
	if (nCount < 1) nCount = 1;
	m_nUdpRequestQueueAlertLine = nCount;
}

//-----------------------------------------------------------------------------
// 描述: 设置TCP数据包的组别总数
//-----------------------------------------------------------------------------
void CIseOptions::SetTcpServerCount(int nCount)
{
	if (nCount < 0) nCount = 0;

	m_nTcpServerCount = nCount;
	m_TcpServerOpts.resize(nCount);
}

//-----------------------------------------------------------------------------
// 描述: 设置TCP服务端口号
// 参数:
//   nServerIndex - TCP服务器序号 (0-based)
//   nPort        - 端口号
//-----------------------------------------------------------------------------
void CIseOptions::SetTcpServerPort(int nServerIndex, int nPort)
{
	if (nServerIndex < 0 || nServerIndex >= m_nTcpServerCount) return;

	m_TcpServerOpts[nServerIndex].nTcpServerPort = nPort;
}

//-----------------------------------------------------------------------------
// 描述: 设置TCP事件循环的个数
//-----------------------------------------------------------------------------
void CIseOptions::SetTcpEventLoopCount(int nCount)
{
	if (nCount < 1) nCount = 1;
	m_nTcpEventLoopCount = nCount;
}

//-----------------------------------------------------------------------------
// 描述: 取得UDP请求队列的最大容量 (即可容纳多少个数据包)
// 参数:
//   nGroupIndex - 组别号 (0-based)
//-----------------------------------------------------------------------------
int CIseOptions::GetUdpRequestQueueCapacity(int nGroupIndex)
{
	if (nGroupIndex < 0 || nGroupIndex >= m_nUdpRequestGroupCount) return -1;

	return m_UdpRequestGroupOpts[nGroupIndex].nRequestQueueCapacity;
}

//-----------------------------------------------------------------------------
// 描述: 取得UDP工作者线程个数的上下限
// 参数:
//   nGroupIndex - 组别号 (0-based)
//   nMinThreads - 存放线程个数的下限
//   nMaxThreads - 存放线程个数的上限
//-----------------------------------------------------------------------------
void CIseOptions::GetUdpWorkerThreadCount(int nGroupIndex, int& nMinThreads, int& nMaxThreads)
{
	if (nGroupIndex < 0 || nGroupIndex >= m_nUdpRequestGroupCount) return;

	nMinThreads = m_UdpRequestGroupOpts[nGroupIndex].nMinWorkerThreads;
	nMaxThreads = m_UdpRequestGroupOpts[nGroupIndex].nMaxWorkerThreads;
}

//-----------------------------------------------------------------------------
// 描述: 取得TCP服务端口号
// 参数:
//   nServerIndex - TCP服务器的序号 (0-based)
//-----------------------------------------------------------------------------
int CIseOptions::GetTcpServerPort(int nServerIndex)
{
	if (nServerIndex < 0 || nServerIndex >= m_nTcpServerCount) return -1;

	return m_TcpServerOpts[nServerIndex].nTcpServerPort;
}

///////////////////////////////////////////////////////////////////////////////
// class CIseMainServer

CIseMainServer::CIseMainServer() :
	m_pUdpServer(NULL),
	m_pTcpServer(NULL),
	m_pAssistorServer(NULL),
	m_pSysThreadMgr(NULL)
{
	// nothing
}

CIseMainServer::~CIseMainServer()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 服务器开始运行后，主线程进行后台守护工作
//-----------------------------------------------------------------------------
void CIseMainServer::RunBackground()
{
	int nAdjustThreadInterval = IseApplication.GetIseOptions().GetAdjustThreadInterval();
	int nSecondCount = 0;

	while (!IseApplication.GetTerminated())
	try
	{
		try
		{
			// 每隔 nAdjustThreadInterval 秒执行一次
			if ((nSecondCount % nAdjustThreadInterval) == 0)
			{
#ifdef ISE_LINUX
				// 暂时屏蔽退出信号
				CSignalMasker SigMasker(true);
				SigMasker.SetSignals(1, SIGTERM);
				SigMasker.Block();
#endif

				// 维护工作者线程的数量
				if (m_pUdpServer) m_pUdpServer->AdjustWorkerThreadCount();
			}
		}
		catch (...)
		{}

		nSecondCount++;
		SleepSec(1, true);  // 1秒
	}
	catch (...)
	{}
}

//-----------------------------------------------------------------------------
// 描述: 服务器初始化 (若初始化失败则抛出异常)
// 备注: 由 IseApplication.Initialize() 调用
//-----------------------------------------------------------------------------
void CIseMainServer::Initialize()
{
	// 初始化 UDP 服务器
	if (IseApplication.GetIseOptions().GetServerType() & ST_UDP)
	{
		m_pUdpServer = new CMainUdpServer();
		m_pUdpServer->SetLocalPort(IseApplication.GetIseOptions().GetUdpServerPort());
		m_pUdpServer->SetListenerThreadCount(IseApplication.GetIseOptions().GetUdpListenerThreadCount());
		m_pUdpServer->Open();
	}

	// 初始化 TCP 服务器
	if (IseApplication.GetIseOptions().GetServerType() & ST_TCP)
	{
		m_pTcpServer = new CMainTcpServer();
		m_pTcpServer->Open();
	}

	// 初始化辅助服务器
	m_pAssistorServer = new CAssistorServer();
	m_pAssistorServer->Open();

	// 初始化系统线程管理器
	m_pSysThreadMgr = new CSysThreadMgr();
	m_pSysThreadMgr->Initialize();
}

//-----------------------------------------------------------------------------
// 描述: 服务器结束化
// 备注: 由 IseApplication.Finalize() 调用，在 CIseMainServer 的析构函数中不必调用
//-----------------------------------------------------------------------------
void CIseMainServer::Finalize()
{
	if (m_pAssistorServer)
	{
		m_pAssistorServer->Close();
		delete m_pAssistorServer;
		m_pAssistorServer = NULL;
	}

	if (m_pUdpServer)
	{
		m_pUdpServer->Close();
		delete m_pUdpServer;
		m_pUdpServer = NULL;
	}

	if (m_pTcpServer)
	{
		m_pTcpServer->Close();
		delete m_pTcpServer;
		m_pTcpServer = NULL;
	}

	if (m_pSysThreadMgr)
	{
		m_pSysThreadMgr->Finalize();
		delete m_pSysThreadMgr;
		m_pSysThreadMgr = NULL;
	}
}

//-----------------------------------------------------------------------------
// 描述: 开始运行服务器
// 备注: 由 IseApplication.Run() 调用
//-----------------------------------------------------------------------------
void CIseMainServer::Run()
{
	RunBackground();
}

///////////////////////////////////////////////////////////////////////////////
// class CIseApplication

CIseApplication::CIseApplication() :
	m_pMainServer(NULL),
	m_nAppStartTime(time(NULL)),
	m_bInitialized(false),
	m_bTerminated(false)
{
	CreateIseBusiness();
}

CIseApplication::~CIseApplication()
{
	Finalize();
}

//-----------------------------------------------------------------------------
// 描述: 处理标准命令行参数
// 返回:
//   true  - 当前命令行参数是标准参数
//   false - 与上相反
//-----------------------------------------------------------------------------
bool CIseApplication::ProcessStandardArgs()
{
	if (GetArgCount() == 2)
	{
		string strArg = GetArgString(1);
		if (strArg == "--version")
		{
			string strVersion = pIseBusiness->GetAppVersion();
			printf("%s\n", strVersion.c_str());
			return true;
		}
		if (strArg == "--help")
		{
			string strHelp = pIseBusiness->GetAppHelp();
			printf("%s\n", strHelp.c_str());
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// 描述: 检查是否运行了多个程序实体
//-----------------------------------------------------------------------------
void CIseApplication::CheckMultiInstance()
{
	if (m_IseOptions.GetAllowMultiInstance()) return;

#ifdef ISE_WIN32
	CreateMutexA(NULL, false, GetExeName().c_str());
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		IseThrowException(SEM_ALREADY_RUNNING);
#endif
#ifdef ISE_LINUX
	umask(0);
	int fd = open(GetExeName().c_str(), O_RDONLY, 0666);
	if (fd >= 0 && flock(fd, LOCK_EX | LOCK_NB) != 0)
		IseThrowException(SEM_ALREADY_RUNNING);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 应用 ISE 配置
//-----------------------------------------------------------------------------
void CIseApplication::ApplyIseOptions()
{
	Logger().SetFileName(m_IseOptions.GetLogFileName(), m_IseOptions.GetLogNewFileDaily());
}

//-----------------------------------------------------------------------------
// 描述: 创建主服务器
//-----------------------------------------------------------------------------
void CIseApplication::CreateMainServer()
{
	if (!m_pMainServer)
		m_pMainServer = new CIseMainServer;
}

//-----------------------------------------------------------------------------
// 描述: 释放主服务器
//-----------------------------------------------------------------------------
void CIseApplication::FreeMainServer()
{
	delete m_pMainServer;
	m_pMainServer = NULL;
}

//-----------------------------------------------------------------------------
// 描述: 创建 CIseBusiness 对象
//-----------------------------------------------------------------------------
void CIseApplication::CreateIseBusiness()
{
	if (!pIseBusiness)
		pIseBusiness = CreateIseBusinessObject();
}

//-----------------------------------------------------------------------------
// 描述: 释放 CIseBusiness 对象
//-----------------------------------------------------------------------------
void CIseApplication::FreeIseBusiness()
{
	delete pIseBusiness;
	pIseBusiness = NULL;
}

//-----------------------------------------------------------------------------
// 描述: 取得自身文件的全名，并初始化 m_strExeName
//-----------------------------------------------------------------------------
void CIseApplication::InitExeName()
{
	m_strExeName = GetAppExeName();
}

//-----------------------------------------------------------------------------
// 描述: 守护模式初始化
//-----------------------------------------------------------------------------
void CIseApplication::InitDaemon()
{
#ifdef ISE_WIN32
#endif
#ifdef ISE_LINUX
	int r;

	r = fork();
	if (r < 0)
		IseThrowException(SEM_INIT_DAEMON_ERROR);
	else if (r != 0)
		exit(0);

	// 第一子进程后台继续执行

	// 第一子进程成为新的会话组长和进程组长
	r = setsid();
	if (r < 0) exit(1);

	// 忽略 SIGHUP 信号 (注: 程序与终端断开时发生，比如启动程序后关闭telnet)
	signal(SIGHUP, SIG_IGN);

	// 第一子进程退出
	r = fork();
	if (r < 0) exit(1);
	else if (r != 0) exit(0);

	// 第二子进程继续执行，它不再是会话组长

	// 改变当前工作目录 (core dump 会输出到该目录下)
	// chdir("/");

	// 重设文件创建掩模
	umask(0);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 初始化信号 (信号的安装、忽略等)
//
//  信号名称    值                         信号说明
// ---------  ----  -----------------------------------------------------------
// # SIGHUP    1    本信号在用户终端连接(正常或非正常)结束时发出，通常是在终端的控制进程
//                  结束时，通知同一 session 内的各个作业，这时它们与控制终端不再关联。
// # SIGINT    2    程序终止(interrupt)信号，在用户键入INTR字符(通常是Ctrl-C)时发出。
// # SIGQUIT   3    和 SIGINT 类似，但由QUIT字符 (通常是 Ctrl-\) 来控制。进程在因收到
//                  此信号退出时会产生core文件，在这个意义上类似于一个程序错误信号。
// # SIGILL    4    执行了非法指令。通常是因为可执行文件本身出现错误，或者试图执行数据段。
//                  堆栈溢出时也有可能产生这个信号。
// # SIGTRAP   5    由断点指令或其它 trap 指令产生。由 debugger 使用。
// # SIGABRT   6    程序自己发现错误并调用 abort 时产生。
// # SIGIOT    6    在PDP-11上由iot指令产生。在其它机器上和 SIGABRT 一样。
// # SIGBUS    7    非法地址，包括内存地址对齐(alignment)出错。eg: 访问一个四个字长的
//                  整数，但其地址不是 4 的倍数。
// # SIGFPE    8    在发生致命的算术运算错误时发出。不仅包括浮点运算错误，还包括溢出及除
//                  数为 0 等其它所有的算术的错误。
// # SIGKILL   9    用来立即结束程序的运行。本信号不能被阻塞、处理和忽略。
// # SIGUSR1   10   留给用户使用。
// # SIGSEGV   11   试图访问未分配给自己的内存，或试图往没有写权限的内存地址写数据。
// # SIGUSR2   12   留给用户使用。
// # SIGPIPE   13   管道破裂(broken pipe)，写一个没有读端口的管道。
// # SIGALRM   14   时钟定时信号，计算的是实际的时间或时钟时间。alarm 函数使用该信号。
// # SIGTERM   15   程序结束(terminate)信号，与 SIGKILL 不同的是该信号可以被阻塞和处理。
//                  通常用来要求程序自己正常退出。shell 命令 kill 缺省产生这个信号。
// # SIGSTKFLT 16   协处理器堆栈错误(stack fault)。
// # SIGCHLD   17   子进程结束时，父进程会收到这个信号。
// # SIGCONT   18   让一个停止(stopped)的进程继续执行。本信号不能被阻塞。可以用一个
//                  handler 来让程序在由 stopped 状态变为继续执行时完成特定的工作。例如
//                  重新显示提示符。
// # SIGSTOP   19   停止(stopped)进程的执行。注意它和terminate以及interrupt的区别:
//                  该进程还未结束，只是暂停执行。本信号不能被阻塞、处理或忽略。
// # SIGTSTP   20   停止进程的运行，但该信号可以被处理和忽略。用户键入SUSP字符时(通常是^Z)
//                  发出这个信号。
// # SIGTTIN   21   当后台作业要从用户终端读数据时，该作业中的所有进程会收到此信号。缺省时
//                  这些进程会停止执行。
// # SIGTTOU   22   类似于SIGTTIN，但在写终端(或修改终端模式)时收到。
// # SIGURG    23   有 "紧急" 数据或带外(out-of-band) 数据到达 socket 时产生。
// # SIGXCPU   24   超过CPU时间资源限制。这个限制可以由getrlimit/setrlimit来读取和改变。
// # SIGXFSZ   25   超过文件大小资源限制。
// # SIGVTALRM 26   虚拟时钟信号。类似于 SIGALRM，但是计算的是该进程占用的CPU时间。
// # SIGPROF   27   类似于SIGALRM/SIGVTALRM，但包括该进程用的CPU时间以及系统调用的时间。
// # SIGWINCH  28   终端视窗的改变时发出。
// # SIGIO     29   文件描述符准备就绪，可以开始进行输入/输出操作。
// # SIGPWR    30   Power failure.
// # SIGSYS    31   非法的系统调用。
//-----------------------------------------------------------------------------
void CIseApplication::InitSignals()
{
#ifdef ISE_WIN32
#endif
#ifdef ISE_LINUX
	int i;

	// 忽略某些信号
	int nIgnoreSignals[] = {SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGTSTP, SIGTTIN,
		SIGTTOU, SIGXCPU, SIGCHLD, SIGPWR, SIGALRM, SIGVTALRM, SIGIO};
	for (i = 0; i < sizeof(nIgnoreSignals)/sizeof(int); i++)
		signal(nIgnoreSignals[i], SIG_IGN);

	// 安装致命非法操作信号处理器
	int nFatalSignals[] = {SIGILL, SIGBUS, SIGFPE, SIGSEGV, SIGSTKFLT, SIGPROF, SIGSYS};
	for (i = 0; i < sizeof(nFatalSignals)/sizeof(int); i++)
		signal(nFatalSignals[i], FatalErrorSignalHandler);

	// 安装正常退出信号处理器
	int nExitSignals[] = {SIGTERM/*, SIGABRT*/};
	for (i = 0; i < sizeof(nExitSignals)/sizeof(int); i++)
		signal(nExitSignals[i], ExitProgramSignalHandler);

	// 安装用户信号处理器
	int nUserSignals[] = {SIGUSR1, SIGUSR2};
	for (i = 0; i < sizeof(nUserSignals)/sizeof(int); i++)
		signal(nUserSignals[i], UserSignalHandler);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 初始化 new 操作符的错误处理函数
//-----------------------------------------------------------------------------
void CIseApplication::InitNewOperHandler()
{
	const int RESERVED_MEM_SIZE = 1024*1024*2;     // 2M

	set_new_handler(OutOfMemoryHandler);

	// 用于内存不足的情况下程序退出
	pReservedMemoryForExit = new char[RESERVED_MEM_SIZE];
}

//-----------------------------------------------------------------------------
// 描述: 关闭终端
//-----------------------------------------------------------------------------
void CIseApplication::CloseTerminal()
{
#ifdef ISE_WIN32
#endif
#ifdef ISE_LINUX
	close(0);  // 关闭标准输入(stdin)
	/*
	close(1);  // 关闭标准输出(stdout)
	close(2);  // 关闭标准错误输出(stderr)
	*/
#endif
}

//-----------------------------------------------------------------------------
// 描述: 应用程序结束化 (不检查 m_bInitialized 标志)
//-----------------------------------------------------------------------------
void CIseApplication::DoFinalize()
{
	try { if (m_pMainServer) m_pMainServer->Finalize(); } catch (...) {}
	try { pIseBusiness->Finalize(); } catch (...) {}
	try { FreeMainServer(); } catch (...) {}
	try { FreeIseBusiness(); } catch (...) {}
	try { NetworkFinalize(); } catch (...) {}
}

//-----------------------------------------------------------------------------
// 描述: 解释命令行参数
// 返回:
//   true  - 允许程序继执行
//   false - 程序应退出 (比如命令行参数不正确)
//-----------------------------------------------------------------------------
bool CIseApplication::ParseArguments(int nArgc, char *sArgv[])
{
	// 先记录命令行参数
	m_ArgList.clear();
	for (int i = 0; i < nArgc; i++)
		m_ArgList.push_back(sArgv[i]);

	// 处理标准命令行参数
	if (ProcessStandardArgs()) return false;

	// 交给 pIseBusiness 解释
	return pIseBusiness->ParseArguments(nArgc, sArgv);
}

//-----------------------------------------------------------------------------
// 描述: 应用程序初始化 (若初始化失败则抛出异常)
//-----------------------------------------------------------------------------
void CIseApplication::Initialize()
{
	try
	{
#ifdef ISE_LINUX
		// 在初始化阶段要屏蔽退出信号
		CSignalMasker SigMasker(true);
		SigMasker.SetSignals(1, SIGTERM);
		SigMasker.Block();
#endif

		NetworkInitialize();
		InitExeName();
		pIseBusiness->DoStartupState(SS_BEFORE_START);
		pIseBusiness->InitIseOptions(m_IseOptions);
		CheckMultiInstance();
		if (m_IseOptions.GetIsDaemon()) InitDaemon();
		InitSignals();
		InitNewOperHandler();
		ApplyIseOptions();
		CreateMainServer();
		pIseBusiness->Initialize();
		m_pMainServer->Initialize();
		pIseBusiness->DoStartupState(SS_AFTER_START);
		if (m_IseOptions.GetIsDaemon()) CloseTerminal();
		m_bInitialized = true;
	}
	catch (CException&)
	{
		pIseBusiness->DoStartupState(SS_START_FAIL);
		DoFinalize();
		throw;
	}
}

//-----------------------------------------------------------------------------
// 描述: 应用程序结束化
//-----------------------------------------------------------------------------
void CIseApplication::Finalize()
{
	if (m_bInitialized)
	{
		DoFinalize();
		m_bInitialized = false;
	}
}

//-----------------------------------------------------------------------------
// 描述: 开始运行应用程序
//-----------------------------------------------------------------------------
void CIseApplication::Run()
{
#ifdef ISE_LINUX
	// 进程被终止时长跳转到此处并立即返回
	if (sigsetjmp(ProcExitJmpBuf, 0)) return;
#endif

	if (m_pMainServer)
		m_pMainServer->Run();
}

//-----------------------------------------------------------------------------
// 描述: 取得可执行文件所在的路径
//-----------------------------------------------------------------------------
string CIseApplication::GetExePath()
{
	return ExtractFilePath(m_strExeName);
}

//-----------------------------------------------------------------------------
// 描述: 取得命令行参数字符串 (nIndex: 0-based)
//-----------------------------------------------------------------------------
string CIseApplication::GetArgString(int nIndex)
{
	if (nIndex >= 0 && nIndex < (int)m_ArgList.size())
		return m_ArgList[nIndex];
	else
		return "";
}

//-----------------------------------------------------------------------------
// 描述: 注册用户信号处理器
//-----------------------------------------------------------------------------
void CIseApplication::RegisterUserSignalHandler(USER_SIGNAL_HANDLER_PROC pProc, void *pParam)
{
	m_OnUserSignal.Register(pProc, pParam);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
