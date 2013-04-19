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

IseBusiness* createIseBusinessObject();

///////////////////////////////////////////////////////////////////////////////
// 主函数

#ifdef ISE_WINDOWS

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    try
    {
        if (iseApp().parseArguments(__argc, __argv))
        {
            AutoFinalizer finalizer(boost::bind(&IseApplication::finalize, &iseApp()));

            iseApp().initialize();
            iseApp().run();
        }
    }
    catch (Exception& e)
    {
        logger().writeException(e);
    }

    return 0;
}

#endif
#ifdef ISE_LINUX

int main(int argc, char *argv[])
{
    try
    {
        if (iseApp().parseArguments(argc, argv))
        {
            AutoFinalizer finalizer(boost::bind(&IseApplication::finalize, &iseApp()));

            iseApp().initialize();
            iseApp().run();
        }
    }
    catch (Exception& e)
    {
        cout << e.makeLogStr() << endl << endl;
        logger().writeException(e);
    }

    return 0;
}

#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 全局变量定义

#ifdef ISE_LINUX
// 用于进程退出时长跳转
static sigjmp_buf procExitJmpBuf;
#endif

// 用于内存不足的情况下程序退出
static char *reservedMemoryForExit;

///////////////////////////////////////////////////////////////////////////////
// 信号处理器

#ifdef ISE_LINUX
//-----------------------------------------------------------------------------
// 描述: 正常退出程序 信号处理器
//-----------------------------------------------------------------------------
void exitProgramSignalHandler(int sigNo)
{
    static bool isInHandler = false;
    if (isInHandler) return;
    isInHandler = true;

    // 停止主线程循环
    iseApp().setTerminated(true);

    logger().writeFmt(SEM_SIG_TERM, sigNo);

    siglongjmp(procExitJmpBuf, 1);
}

//-----------------------------------------------------------------------------
// 描述: 致命非法操作 信号处理器
//-----------------------------------------------------------------------------
void fatalErrorSignalHandler(int sigNo)
{
    static bool isInHandler = false;
    if (isInHandler) return;
    isInHandler = true;

    // 停止主线程循环
    iseApp().setTerminated(true);

    logger().writeFmt(SEM_SIG_FATAL_ERROR, sigNo);
    abort();
}
#endif

//-----------------------------------------------------------------------------
// 描述: 用户信号处理器
//-----------------------------------------------------------------------------
void userSignalHandler(int sigNo)
{
    const CallbackList<UserSignalHandlerCallback>& callbackList = iseApp().onUserSignal_;

    for (int i = 0; i < callbackList.getCount(); i++)
    {
        const UserSignalHandlerCallback& callback = callbackList.getItem(i);
        if (callback)
            callback(sigNo);
    }
}

//-----------------------------------------------------------------------------
// 描述: 内存不足错误处理器
// 备注:
//   若未安装错误处理器(set_new_handler)，则当 new 操作失败时抛出 bad_alloc 异常；
//   而安装错误处理器后，new 操作将不再抛出异常，而是调用处理器函数。
//-----------------------------------------------------------------------------
void outOfMemoryHandler()
{
    static bool isInHandler = false;
    if (isInHandler) return;
    isInHandler = true;

    // 释放保留内存，以免程序退出过程中再次出现内存不足
    delete[] reservedMemoryForExit;
    reservedMemoryForExit = NULL;

    logger().writeStr(SEM_OUT_OF_MEMORY);
    abort();
}

///////////////////////////////////////////////////////////////////////////////
// class IseOptions

IseOptions::IseOptions()
{
    logFileName_ = "";
    logNewFileDaily_ = false;
    isDaemon_ = false;
    allowMultiInstance_ = false;

    setServerType(DEF_SERVER_TYPE);
    setAdjustThreadInterval(DEF_ADJUST_THREAD_INTERVAL);
    setAssistorThreadCount(DEF_ASSISTOR_THREAD_COUNT);

    setUdpServerPort(DEF_UDP_SERVER_PORT);
    setUdpListenerThreadCount(DEF_UDP_LISTENER_THD_COUNT);
    setUdpRequestGroupCount(DEF_UDP_REQ_GROUP_COUNT);
    for (int i = 0; i < DEF_UDP_REQ_GROUP_COUNT; i++)
    {
        setUdpRequestQueueCapacity(i, DEF_UDP_REQ_QUEUE_CAPACITY);
        setUdpWorkerThreadCount(i, DEF_UDP_WORKER_THREADS_MIN, DEF_UDP_WORKER_THREADS_MAX);
    }
    setUdpRequestEffWaitTime(DEF_UDP_REQ_EFF_WAIT_TIME);
    setUdpWorkerThreadTimeOut(DEF_UDP_WORKER_THD_TIMEOUT);
    SetUdpRequestQueueAlertLine(DEF_UDP_QUEUE_ALERT_LINE);

    setTcpServerCount(DEF_TCP_REQ_GROUP_COUNT);
    for (int i = 0; i < DEF_TCP_REQ_GROUP_COUNT; i++)
        setTcpServerPort(i, DEF_TCP_SERVER_PORT);
    setTcpEventLoopCount(DEF_TCP_EVENT_LOOP_COUNT);
}

//-----------------------------------------------------------------------------
// 描述: 设置服务器类型(UDP|TCP)
// 参数:
//   serverType - 服务器器类型(可多选或不选)
// 示例:
//   SetServerType(ST_UDP | ST_TCP);
//-----------------------------------------------------------------------------
void IseOptions::setServerType(UINT serverType)
{
    serverType_ = serverType;
}

//-----------------------------------------------------------------------------
// 描述: 设置后台维护工作者线程数量的时间间隔(秒)
//-----------------------------------------------------------------------------
void IseOptions::setAdjustThreadInterval(int seconds)
{
    if (seconds <= 0) seconds = DEF_ADJUST_THREAD_INTERVAL;
    adjustThreadInterval_ = seconds;
}

//-----------------------------------------------------------------------------
// 描述: 设置辅助线程的数量
//-----------------------------------------------------------------------------
void IseOptions::setAssistorThreadCount(int count)
{
    if (count < 0) count = 0;
    assistorThreadCount_ = count;
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP服务端口号
//-----------------------------------------------------------------------------
void IseOptions::setUdpServerPort(int port)
{
    udpServerPort_ = port;
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP监听线程的数量
//-----------------------------------------------------------------------------
void IseOptions::setUdpListenerThreadCount(int count)
{
    if (count < 1) count = 1;

    udpListenerThreadCount_ = count;
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP数据包的组别总数
//-----------------------------------------------------------------------------
void IseOptions::setUdpRequestGroupCount(int count)
{
    if (count <= 0) count = DEF_UDP_REQ_GROUP_COUNT;

    udpRequestGroupCount_ = count;
    udpRequestGroupOpts_.resize(count);
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP请求队列的最大容量 (即可容纳多少个数据包)
// 参数:
//   groupIndex - 组别号 (0-based)
//   capacity   - 容量
//-----------------------------------------------------------------------------
void IseOptions::setUdpRequestQueueCapacity(int groupIndex, int capacity)
{
    if (groupIndex < 0 || groupIndex >= udpRequestGroupCount_) return;

    if (capacity <= 0) capacity = DEF_UDP_REQ_QUEUE_CAPACITY;

    udpRequestGroupOpts_[groupIndex].requestQueueCapacity = capacity;
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP工作者线程个数的上下限
// 参数:
//   groupIndex - 组别号 (0-based)
//   minThreads - 线程个数的下限
//   maxThreads - 线程个数的上限
//-----------------------------------------------------------------------------
void IseOptions::setUdpWorkerThreadCount(int groupIndex, int minThreads, int maxThreads)
{
    if (groupIndex < 0 || groupIndex >= udpRequestGroupCount_) return;

    if (minThreads < 1) minThreads = 1;
    if (maxThreads < minThreads) maxThreads = minThreads;

    udpRequestGroupOpts_[groupIndex].minWorkerThreads = minThreads;
    udpRequestGroupOpts_[groupIndex].maxWorkerThreads = maxThreads;
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP请求在队列中的有效等待时间，超时则不予处理
// 参数:
//   nMSecs - 等待秒数
//-----------------------------------------------------------------------------
void IseOptions::setUdpRequestEffWaitTime(int seconds)
{
    if (seconds <= 0) seconds = DEF_UDP_REQ_EFF_WAIT_TIME;
    udpRequestEffWaitTime_ = seconds;
}

//-----------------------------------------------------------------------------
// 描述: 设置UDP工作者线程的工作超时时间(秒)，若为0表示不进行超时检测
//-----------------------------------------------------------------------------
void IseOptions::setUdpWorkerThreadTimeOut(int seconds)
{
    if (seconds < 0) seconds = 0;
    udpWorkerThreadTimeOut_ = seconds;
}

//-----------------------------------------------------------------------------
// 描述: 设置请求队列中数据包数量警戒线
//-----------------------------------------------------------------------------
void IseOptions::SetUdpRequestQueueAlertLine(int count)
{
    if (count < 1) count = 1;
    udpRequestQueueAlertLine_ = count;
}

//-----------------------------------------------------------------------------
// 描述: 设置TCP数据包的组别总数
//-----------------------------------------------------------------------------
void IseOptions::setTcpServerCount(int count)
{
    if (count < 0) count = 0;

    tcpServerCount_ = count;
    tcpServerOpts_.resize(count);
}

//-----------------------------------------------------------------------------
// 描述: 设置TCP服务端口号
// 参数:
//   serverIndex - TCP服务器序号 (0-based)
//   port        - 端口号
//-----------------------------------------------------------------------------
void IseOptions::setTcpServerPort(int serverIndex, int port)
{
    if (serverIndex < 0 || serverIndex >= tcpServerCount_) return;

    tcpServerOpts_[serverIndex].tcpServerPort = port;
}

//-----------------------------------------------------------------------------
// 描述: 设置TCP事件循环的个数
//-----------------------------------------------------------------------------
void IseOptions::setTcpEventLoopCount(int count)
{
    if (count < 1) count = 1;
    tcpEventLoopCount_ = count;
}

//-----------------------------------------------------------------------------
// 描述: 取得UDP请求队列的最大容量 (即可容纳多少个数据包)
// 参数:
//   groupIndex - 组别号 (0-based)
//-----------------------------------------------------------------------------
int IseOptions::getUdpRequestQueueCapacity(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= udpRequestGroupCount_) return -1;

    return udpRequestGroupOpts_[groupIndex].requestQueueCapacity;
}

//-----------------------------------------------------------------------------
// 描述: 取得UDP工作者线程个数的上下限
// 参数:
//   groupIndex - 组别号 (0-based)
//   minThreads - 存放线程个数的下限
//   maxThreads - 存放线程个数的上限
//-----------------------------------------------------------------------------
void IseOptions::getUdpWorkerThreadCount(int groupIndex, int& minThreads, int& maxThreads)
{
    if (groupIndex < 0 || groupIndex >= udpRequestGroupCount_) return;

    minThreads = udpRequestGroupOpts_[groupIndex].minWorkerThreads;
    maxThreads = udpRequestGroupOpts_[groupIndex].maxWorkerThreads;
}

//-----------------------------------------------------------------------------
// 描述: 取得TCP服务端口号
// 参数:
//   serverIndex - TCP服务器的序号 (0-based)
//-----------------------------------------------------------------------------
int IseOptions::getTcpServerPort(int serverIndex)
{
    if (serverIndex < 0 || serverIndex >= tcpServerCount_) return -1;

    return tcpServerOpts_[serverIndex].tcpServerPort;
}

///////////////////////////////////////////////////////////////////////////////
// class IseMainServer

IseMainServer::IseMainServer() :
    udpServer_(NULL),
    tcpServer_(NULL),
    assistorServer_(NULL),
    sysThreadMgr_(NULL)
{
    // nothing
}

IseMainServer::~IseMainServer()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 服务器初始化 (若初始化失败则抛出异常)
// 备注: 由 iseApp().initialize() 调用
//-----------------------------------------------------------------------------
void IseMainServer::initialize()
{
    // 初始化 UDP 服务器
    if (iseApp().getIseOptions().getServerType() & ST_UDP)
    {
        udpServer_ = new MainUdpServer();
        udpServer_->setLocalPort(iseApp().getIseOptions().getUdpServerPort());
        udpServer_->setListenerThreadCount(iseApp().getIseOptions().getUdpListenerThreadCount());
        udpServer_->open();
    }

    // 初始化 TCP 服务器
    if (iseApp().getIseOptions().getServerType() & ST_TCP)
    {
        tcpServer_ = new MainTcpServer();
        tcpServer_->open();
    }

    // 初始化辅助服务器
    assistorServer_ = new AssistorServer();
    assistorServer_->open();

    // 初始化系统线程管理器
    sysThreadMgr_ = new SysThreadMgr();
    sysThreadMgr_->initialize();
}

//-----------------------------------------------------------------------------
// 描述: 服务器结束化
// 备注: 由 iseApp().finalize() 调用，在 IseMainServer 的析构函数中不必调用
//-----------------------------------------------------------------------------
void IseMainServer::finalize()
{
    if (assistorServer_)
    {
        assistorServer_->close();
        delete assistorServer_;
        assistorServer_ = NULL;
    }

    if (udpServer_)
    {
        udpServer_->close();
        delete udpServer_;
        udpServer_ = NULL;
    }

    if (tcpServer_)
    {
        tcpServer_->close();
        delete tcpServer_;
        tcpServer_ = NULL;
    }

    if (sysThreadMgr_)
    {
        sysThreadMgr_->finalize();
        delete sysThreadMgr_;
        sysThreadMgr_ = NULL;
    }
}

//-----------------------------------------------------------------------------
// 描述: 开始运行服务器
// 备注: 由 iseApp().run() 调用
//-----------------------------------------------------------------------------
void IseMainServer::run()
{
    runBackground();
}

//-----------------------------------------------------------------------------
// 描述: 服务器开始运行后，主线程进行后台守护工作
//-----------------------------------------------------------------------------
void IseMainServer::runBackground()
{
    int adjustThreadInterval = iseApp().getIseOptions().getAdjustThreadInterval();
    int secondCount = 0;

    while (!iseApp().isTerminated())
    try
    {
        try
        {
            // 每隔 adjustThreadInterval 秒执行一次
            if ((secondCount % adjustThreadInterval) == 0)
            {
#ifdef ISE_LINUX
                // 暂时屏蔽退出信号
                SignalMasker sigMasker(true);
                sigMasker.setSignals(1, SIGTERM);
                sigMasker.block();
#endif

                // 维护工作者线程的数量
                if (udpServer_) udpServer_->adjustWorkerThreadCount();
            }
        }
        catch (...)
        {}

        secondCount++;
        sleepSec(1, true);  // 1秒
    }
    catch (...)
    {}
}

///////////////////////////////////////////////////////////////////////////////
// class IseApplication

IseApplication::IseApplication() :
    iseBusiness_(NULL),
    mainServer_(NULL),
    appStartTime_(time(NULL)),
    initialized_(false),
    terminated_(false)
{
    createIseBusiness();
}

IseApplication::~IseApplication()
{
    finalize();
}

//-----------------------------------------------------------------------------

IseApplication& IseApplication::instance()
{
    static IseApplication obj;
    return obj;
}

//-----------------------------------------------------------------------------
// 描述: 解释命令行参数
// 返回:
//   true  - 允许程序继执行
//   false - 程序应退出 (比如命令行参数不正确)
//-----------------------------------------------------------------------------
bool IseApplication::parseArguments(int argc, char *argv[])
{
    // 先记录命令行参数
    argList_.clear();
    for (int i = 0; i < argc; i++)
        argList_.add(argv[i]);

    // 处理标准命令行参数
    if (processStandardArgs()) return false;

    // 交给 IseBusiness 对象解释
    return iseBusiness_->parseArguments(argc, argv);
}

//-----------------------------------------------------------------------------
// 描述: 应用程序初始化 (若初始化失败则抛出异常)
//-----------------------------------------------------------------------------
void IseApplication::initialize()
{
    try
    {
#ifdef ISE_LINUX
        // 在初始化阶段要屏蔽退出信号
        SignalMasker sigMasker(true);
        sigMasker.setSignals(1, SIGTERM);
        sigMasker.block();
#endif

        networkInitialize();
        initExeName();
        iseBusiness_->doStartupState(SS_BEFORE_START);
        iseBusiness_->initIseOptions(iseOptions_);
        checkMultiInstance();
        if (iseOptions_.getIsDaemon()) initDaemon();
        initSignals();
        initNewOperHandler();
        applyIseOptions();
        createMainServer();
        iseBusiness_->initialize();
        mainServer_->initialize();
        iseBusiness_->doStartupState(SS_AFTER_START);
        if (iseOptions_.getIsDaemon()) closeTerminal();
        initialized_ = true;
    }
    catch (Exception&)
    {
        iseBusiness_->doStartupState(SS_START_FAIL);
        doFinalize();
        throw;
    }
}

//-----------------------------------------------------------------------------
// 描述: 应用程序结束化
//-----------------------------------------------------------------------------
void IseApplication::finalize()
{
    if (initialized_)
    {
        doFinalize();
        initialized_ = false;
    }
}

//-----------------------------------------------------------------------------
// 描述: 开始运行应用程序
//-----------------------------------------------------------------------------
void IseApplication::run()
{
#ifdef ISE_LINUX
    // 进程被终止时长跳转到此处并立即返回
    if (sigsetjmp(procExitJmpBuf, 0)) return;
#endif

    if (mainServer_)
        mainServer_->run();
}

//-----------------------------------------------------------------------------
// 描述: 取得可执行文件所在的路径
//-----------------------------------------------------------------------------
string IseApplication::getExePath()
{
    return extractFilePath(exeName_);
}

//-----------------------------------------------------------------------------
// 描述: 取得命令行参数字符串 (index: 0-based)
//-----------------------------------------------------------------------------
string IseApplication::getArgString(int index)
{
    if (index >= 0 && index < (int)argList_.getCount())
        return argList_[index];
    else
        return "";
}

//-----------------------------------------------------------------------------
// 描述: 注册用户信号处理器
//-----------------------------------------------------------------------------
void IseApplication::registerUserSignalHandler(const UserSignalHandlerCallback& callback)
{
    onUserSignal_.registerCallback(callback);
}

//-----------------------------------------------------------------------------
// 描述: 处理标准命令行参数
// 返回:
//   true  - 当前命令行参数是标准参数
//   false - 与上相反
//-----------------------------------------------------------------------------
bool IseApplication::processStandardArgs()
{
    if (getArgCount() == 2)
    {
        string arg = getArgString(1);
        if (arg == "--version")
        {
            string version = iseBusiness_->getAppVersion();
            printf("%s\n", version.c_str());
            return true;
        }
        if (arg == "--help")
        {
            string help = iseBusiness_->getAppHelp();
            printf("%s\n", help.c_str());
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
// 描述: 检查是否运行了多个程序实体
//-----------------------------------------------------------------------------
void IseApplication::checkMultiInstance()
{
    if (iseOptions_.getAllowMultiInstance()) return;

#ifdef ISE_WINDOWS
    CreateMutexA(NULL, false, getExeName().c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        iseThrowException(SEM_ALREADY_RUNNING);
#endif
#ifdef ISE_LINUX
    umask(0);
    int fd = open(getExeName().c_str(), O_RDONLY, 0666);
    if (fd >= 0 && flock(fd, LOCK_EX | LOCK_NB) != 0)
        iseThrowException(SEM_ALREADY_RUNNING);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 应用 ISE 配置
//-----------------------------------------------------------------------------
void IseApplication::applyIseOptions()
{
    logger().setFileName(iseOptions_.getLogFileName(), iseOptions_.getLogNewFileDaily());
}

//-----------------------------------------------------------------------------
// 描述: 创建主服务器
//-----------------------------------------------------------------------------
void IseApplication::createMainServer()
{
    if (!mainServer_)
        mainServer_ = new IseMainServer;
}

//-----------------------------------------------------------------------------
// 描述: 释放主服务器
//-----------------------------------------------------------------------------
void IseApplication::freeMainServer()
{
    delete mainServer_;
    mainServer_ = NULL;
}

//-----------------------------------------------------------------------------
// 描述: 创建 IseBusiness 对象
//-----------------------------------------------------------------------------
void IseApplication::createIseBusiness()
{
    if (!iseBusiness_)
        iseBusiness_ = createIseBusinessObject();

    if (!iseBusiness_)
        iseThrowException(SEM_BUSINESS_OBJ_EXPECTED);
}

//-----------------------------------------------------------------------------
// 描述: 释放 IseBusiness 对象
//-----------------------------------------------------------------------------
void IseApplication::freeIseBusiness()
{
    delete iseBusiness_;
    iseBusiness_ = NULL;
}

//-----------------------------------------------------------------------------
// 描述: 取得自身文件的全名，并初始化 exeName_
//-----------------------------------------------------------------------------
void IseApplication::initExeName()
{
    exeName_ = GetAppExeName();
}

//-----------------------------------------------------------------------------
// 描述: 守护模式初始化
//-----------------------------------------------------------------------------
void IseApplication::initDaemon()
{
#ifdef ISE_WINDOWS
#endif
#ifdef ISE_LINUX
    int r;

    r = fork();
    if (r < 0)
        iseThrowException(SEM_INIT_DAEMON_ERROR);
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
void IseApplication::initSignals()
{
#ifdef ISE_WINDOWS
#endif
#ifdef ISE_LINUX
    int i;

    // 忽略某些信号
    int ignoreSignals[] = {SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGTSTP, SIGTTIN,
        SIGTTOU, SIGXCPU, SIGCHLD, SIGPWR, SIGALRM, SIGVTALRM, SIGIO};
    for (i = 0; i < sizeof(ignoreSignals)/sizeof(int); i++)
        signal(ignoreSignals[i], SIG_IGN);

    // 安装致命非法操作信号处理器
    int fatalSignals[] = {SIGILL, SIGBUS, SIGFPE, SIGSEGV, SIGSTKFLT, SIGPROF, SIGSYS};
    for (i = 0; i < sizeof(fatalSignals)/sizeof(int); i++)
        signal(fatalSignals[i], fatalErrorSignalHandler);

    // 安装正常退出信号处理器
    int exitSignals[] = {SIGTERM/*, SIGABRT*/};
    for (i = 0; i < sizeof(exitSignals)/sizeof(int); i++)
        signal(exitSignals[i], exitProgramSignalHandler);

    // 安装用户信号处理器
    int userSignals[] = {SIGUSR1, SIGUSR2};
    for (i = 0; i < sizeof(userSignals)/sizeof(int); i++)
        signal(userSignals[i], userSignalHandler);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 初始化 new 操作符的错误处理函数
//-----------------------------------------------------------------------------
void IseApplication::initNewOperHandler()
{
    const int RESERVED_MEM_SIZE = 1024*1024*2;     // 2M

    set_new_handler(outOfMemoryHandler);

    // 用于内存不足的情况下程序退出
    reservedMemoryForExit = new char[RESERVED_MEM_SIZE];
}

//-----------------------------------------------------------------------------
// 描述: 关闭终端
//-----------------------------------------------------------------------------
void IseApplication::closeTerminal()
{
#ifdef ISE_WINDOWS
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
// 描述: 应用程序结束化 (不检查 initialized_ 标志)
//-----------------------------------------------------------------------------
void IseApplication::doFinalize()
{
    try { if (mainServer_) mainServer_->finalize(); } catch (...) {}
    try { iseBusiness_->finalize(); } catch (...) {}
    try { freeMainServer(); } catch (...) {}
    try { freeIseBusiness(); } catch (...) {}
    try { networkFinalize(); } catch (...) {}
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
