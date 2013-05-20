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

#include "ise/main/ise_options.h"
#include "ise/main/ise_classes.h"
#include "ise/main/ise_thread.h"
#include "ise/main/ise_sys_utils.h"
#include "ise/main/ise_socket.h"
#include "ise/main/ise_exceptions.h"
#include "ise/main/ise_server_udp.h"
#include "ise/main/ise_server_tcp.h"
#include "ise/main/ise_server_assistor.h"
#include "ise/main/ise_sys_threads.h"
#include "ise/main/ise_scheduler.h"

#ifdef ISE_WINDOWS
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
typedef boost::function<void (int signalNumber)> UserSignalHandlerCallback;

///////////////////////////////////////////////////////////////////////////////
// interfaces

class UdpCallbacks
{
public:
    virtual ~UdpCallbacks() {}

    // UDP数据包分类
    virtual void classifyUdpPacket(void *packetBuffer, int packetSize, int& groupIndex) = 0;
    // 收到了UDP数据包
    virtual void onRecvedUdpPacket(UdpWorkerThread& workerThread, int groupIndex, UdpPacket& packet) = 0;
};

class TcpCallbacks
{
public:
    virtual ~TcpCallbacks() {}

    // 接受了一个新的TCP连接
    virtual void onTcpConnected(const TcpConnectionPtr& connection) = 0;
    // 断开了一个TCP连接
    virtual void onTcpDisconnected(const TcpConnectionPtr& connection) = 0;
    // TCP连接上的一个接收任务已完成
    virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
        int packetSize, const Context& context) = 0;
    // TCP连接上的一个发送任务已完成
    virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// class IseBusiness - ISE业务基类

class IseBusiness :
    boost::noncopyable,
    public UdpCallbacks,
    public TcpCallbacks
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
    virtual string getAppVersion() { return "0.0.0.0"; }
    // 返回程序的帮助信息
    virtual string getAppHelp() { return ""; }
    // 处理启动状态
    virtual void doStartupState(STARTUP_STATE state) {}
    // 初始化ISE配置信息
    virtual void initIseOptions(IseOptions& options) {}

public:  /* interface UdpCallbacks */
    // UDP数据包分类
    virtual void classifyUdpPacket(void *packetBuffer, int packetSize, int& groupIndex) { groupIndex = 0; }
    // 收到了UDP数据包
    virtual void onRecvedUdpPacket(UdpWorkerThread& workerThread, int groupIndex, UdpPacket& packet) {}

public:  /* interface TcpCallbacks */
    // 接受了一个新的TCP连接
    virtual void onTcpConnected(const TcpConnectionPtr& connection) {}
    // 断开了一个TCP连接
    virtual void onTcpDisconnected(const TcpConnectionPtr& connection) {}
    // TCP连接上的一个接收任务已完成
    virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
        int packetSize, const Context& context) {}
    // TCP连接上的一个发送任务已完成
    virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context) {}

public:
    // 辅助服务线程执行(assistorIndex: 0-based)
    virtual void assistorThreadExecute(AssistorThread& assistorThread, int assistorIndex) {}
    // 系统守护线程执行 (secondCount: 0-based)
    virtual void daemonThreadExecute(Thread& thread, int secondCount) {}
};

///////////////////////////////////////////////////////////////////////////////
// class IseOptions - ISE配置类

class IseOptions : boost::noncopyable
{
public:
    // 服务器常规配置缺省值
    enum
    {
        DEF_SERVER_TYPE                 = 0,             // 服务器默认类型
        DEF_ADJUST_THREAD_INTERVAL      = 5,             // 后台调整 "工作者线程数量" 的时间间隔缺省值(秒)
        DEF_ASSISTOR_THREAD_COUNT       = 0,             // 辅助线程的个数
    };

    // UDP服务器配置缺省值
    enum
    {
        DEF_UDP_SERVER_PORT             = 8000,          // UDP服务默认端口
        DEF_UDP_LISTENER_THD_COUNT      = 1,             // 监听线程的数量
        DEF_UDP_REQ_GROUP_COUNT         = 1,             // 请求组别总数的缺省值
        DEF_UDP_REQ_QUEUE_CAPACITY      = 5000,          // 请求队列的缺省容量(即能放下多少数据包)
        DEF_UDP_WORKER_THREADS_MIN      = 1,             // 每个组别中工作者线程的缺省最少个数
        DEF_UDP_WORKER_THREADS_MAX      = 8,             // 每个组别中工作者线程的缺省最多个数
        DEF_UDP_REQ_EFF_WAIT_TIME       = 10,            // 请求在队列中的有效等待时间缺省值(秒)
        DEF_UDP_WORKER_THD_TIMEOUT      = 60,            // 工作者线程的工作超时时间缺省值(秒)
        DEF_UDP_QUEUE_ALERT_LINE        = 500,           // 队列中数据包数量警戒线缺省值，若超过警戒线则尝试增加线程
    };

    // TCP服务器配置缺省值
    enum
    {
        DEF_TCP_SERVER_PORT             = 8000,          // TCP服务默认端口
        DEF_TCP_SERVER_COUNT            = 1,             // TCP服务器总数的缺省值
        DEF_TCP_SERVER_EVENT_LOOP_COUNT = 1,             // TCP服务器中事件循环个数的缺省值
        DEF_TCP_CLIENT_EVENT_LOOP_COUNT = 1,             // 用于全部TCP客户端的事件循环的个数的缺省值
        DEF_TCP_MAX_RECV_BUFFER_SIZE    = 1024*1024*1,   // TCP接收缓存的最大字节数
    };

    // UDP请求组别的配置
    struct UdpRequestGroupOption
    {
        int requestQueueCapacity;      // 请求队列的容量(即可容纳多少个数据包)
        int minWorkerThreads;          // 工作者线程的最少个数
        int maxWorkerThreads;          // 工作者线程的最多个数

        UdpRequestGroupOption()
        {
            requestQueueCapacity = DEF_UDP_REQ_QUEUE_CAPACITY;
            minWorkerThreads = DEF_UDP_WORKER_THREADS_MIN;
            maxWorkerThreads = DEF_UDP_WORKER_THREADS_MAX;
        }
    };
    typedef std::vector<UdpRequestGroupOption> UdpRequestGroupOptions;

    // TCP服务器的配置
    struct TcpServerOption
    {
        int serverPort;                // TCP服务端口号
        int eventLoopCount;            // 事件循环个数

        TcpServerOption()
        {
            serverPort = DEF_TCP_SERVER_PORT;
            eventLoopCount = DEF_TCP_SERVER_EVENT_LOOP_COUNT;
        }
    };
    typedef std::vector<TcpServerOption> TcpServerOptions;

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
    void setUdpWorkerThreadTimeout(int seconds);
    // 设置UDP请求队列中数据包数量警戒线
    void setUdpRequestQueueAlertLine(int count);

    // 设置TCP服务器的总数
    void setTcpServerCount(int count);
    // 设置TCP服务端口号
    void setTcpServerPort(int serverIndex, int port);
    void setTcpServerPort(int port) { setTcpServerPort(0, port); }
    // 设置每个TCP服务器中事件循环的个数
    void setTcpServerEventLoopCount(int serverIndex, int eventLoopCount);
    void setTcpServerEventLoopCount(int eventLoopCount) { setTcpServerEventLoopCount(0, eventLoopCount); }
    // 设置用于全部TCP客户端的事件循环的个数
    void setTcpClientEventLoopCount(int eventLoopCount);
    // 设置TCP接收缓存在无接收任务时的最大字节数
    void setTcpMaxRecvBufferSize(int bytes);

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
    int getUdpWorkerThreadTimeout() { return udpWorkerThreadTimeout_; }
    int getUdpRequestQueueAlertLine() { return udpRequestQueueAlertLine_; }

    int getTcpServerCount() { return tcpServerCount_; }
    int getTcpServerPort(int serverIndex);
    int getTcpServerEventLoopCount(int serverIndex);
    int getTcpClientEventLoopCount() { return tcpClientEventLoopCount_; }
    int getTcpMaxRecvBufferSize() { return tcpMaxRecvBufferSize_; }

private:
    /* ------------ 系统配置: ------------------ */

    string logFileName_;         // 日志文件名 (含路径)
    bool logNewFileDaily_;       // 是否每天用一个单独的文件存储日志
    bool isDaemon_;              // 是否后台守护程序(daemon)
    bool allowMultiInstance_;    // 是否允许多个程序实体同时运行

    /* ------------ 服务器常规配置: ------------ */

    // 服务器类型 (ST_UDP | ST_TCP)
    UINT serverType_;
    // 后台调整工作者线程数量的时间间隔(秒)
    int adjustThreadInterval_;
    // 辅助线程的个数
    int assistorThreadCount_;

    /* ------------ UDP服务器配置: ------------ */

    // UDP服务端口
    int udpServerPort_;
    // 监听线程的数量
    int udpListenerThreadCount_;
    // 请求组别的数量
    int udpRequestGroupCount_;
    // 每个组别内的配置
    UdpRequestGroupOptions udpRequestGroupOpts_;
    // 数据包在队列中的有效等待时间，超时则不予处理(秒)
    int udpRequestEffWaitTime_;
    // 工作者线程的工作超时时间(秒)，若为0表示不进行超时检测
    int udpWorkerThreadTimeout_;
    // 请求队列中数据包数量警戒线，若超过警戒线则尝试增加线程
    int udpRequestQueueAlertLine_;

    /* ------------ TCP服务器配置: ------------ */

    // TCP服务器的数量 (一个TCP服务器占用一个TCP端口)
    int tcpServerCount_;
    // 每个TCP服务器的配置
    TcpServerOptions tcpServerOpts_;
    // 用于全部TCP客户端的事件循环的个数
    int tcpClientEventLoopCount_;
    // TCP接收缓存在无接收任务时的最大字节数
    int tcpMaxRecvBufferSize_;
};

///////////////////////////////////////////////////////////////////////////////
// class IseMainServer - 主服务器类

class IseMainServer : boost::noncopyable
{
public:
    IseMainServer();
    virtual ~IseMainServer();

    void initialize();
    void finalize();
    void run();

    MainUdpServer& getMainUdpServer();
    MainTcpServer& getMainTcpServer();
    AssistorServer& getAssistorServer() { return *assistorServer_; }
private:
    void runBackground();
private:
    MainUdpServer *udpServer_;        // UDP服务器
    MainTcpServer *tcpServer_;        // TCP服务器
    AssistorServer *assistorServer_;  // 辅助服务器
    SysThreadMgr *sysThreadMgr_;      // 系统线程管理器
};

///////////////////////////////////////////////////////////////////////////////
// class IseApplication - ISE应用程序类
//
// 说明:
// 1. 此类是整个服务程序的主框架，全局单例对象(Application)在程序启动时即被创建；
// 2. 一般来说，服务程序是由外部发出命令(kill)而退出的。在ISE中，程序收到kill退出命令后
//    (此时当前执行点一定在 iseApp().run() 中)，会触发 exitProgramSignalHandler
//    信号处理器，进而利用 longjmp 方法使执行点模拟从 run() 中退出，继而执行 finalize。
// 3. 若程序发生致命的非法操作错误，会先触发 fatalErrorSignalHandler 信号处理器，
//    然后同样按照正常的析构顺序退出。
// 4. 若程序内部想正常退出，不推荐使用exit函数。而是 iseApp().setTeraminted(true).
//    这样才能让程序按照正常的析构顺序退出。

class IseApplication : boost::noncopyable
{
public:
    friend void userSignalHandler(int sigNo);

    virtual ~IseApplication();
    static IseApplication& instance();

    bool parseArguments(int argc, char *argv[]);
    void initialize();
    void finalize();
    void run();

    IseOptions& getIseOptions() { return iseOptions_; }
    IseBusiness& getIseBusiness() { return *iseBusiness_; }
    IseScheduleTaskMgr& getScheduleTaskMgr() { return IseScheduleTaskMgr::instance(); }
    IseMainServer& getMainServer() { return *mainServer_; }
    BaseUdpServer& getUdpServer() { return getMainServer().getMainUdpServer().getUdpServer(); }
    BaseTcpServer& getTcpServer(int index) { return getMainServer().getMainTcpServer().getTcpServer(index); }

    void setTerminated(bool value) { terminated_ = value; }
    bool isTerminated() { return terminated_; }

    // 取得可执行文件的全名(含绝对路径)
    string getExeName() { return exeName_; }
    // 取得可执行文件所在的路径
    string getExePath();
    // 取得命令行参数个数(首个参数为程序路径文件名)
    int getArgCount() { return argList_.getCount(); }
    // 取得命令行参数字符串 (index: 0-based)
    string getArgString(int index);
    // 取得程序启动时的时间
    time_t getAppStartTime() { return appStartTime_; }

    // 注册用户信号处理器
    void registerUserSignalHandler(const UserSignalHandlerCallback& callback);

private:
    IseApplication();

private:
    bool processStandardArgs(bool isInitializing);
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
    void openTerminal();
    void closeTerminal();
    void doFinalize();

private:
    IseOptions iseOptions_;              // ISE配置
    IseBusiness *iseBusiness_;           // 业务对象
    IseMainServer *mainServer_;          // 主服务器
    StrList argList_;                    // 命令行参数 (不包括程序名 argv[0])
    string exeName_;                     // 可执行文件的全名(含绝对路径)
    time_t appStartTime_;                // 程序启动时的时间
    bool initialized_;                   // 是否成功初始化
    bool terminated_;                    // 是否应退出的标志
    CallbackList<UserSignalHandlerCallback> onUserSignal_;  // 用户信号处理回调
};

///////////////////////////////////////////////////////////////////////////////
// 全局函数

inline IseApplication& iseApp() { return IseApplication::instance(); }

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_APPLICATION_H_
