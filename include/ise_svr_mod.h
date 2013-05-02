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
// ise_svr_mod.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SVR_MOD_H_
#define _ISE_SVR_MOD_H_

#include "ise.h"
#include "ise_application.h"
#include "ise_svr_mod_msgs.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class IseServerModule;
class IseServerModuleMgr;
class IseSvrModBusiness;

///////////////////////////////////////////////////////////////////////////////
// 类型定义

// 动作代码数组
typedef vector<UINT> ActionCodeArray;

// UDP组别的配置
struct UdpGroupOptions
{
    int queueCapacity;
    int minThreads;
    int maxThreads;
};

// TCP服务器的配置
struct TcpServerOptions
{
    int port;
};

///////////////////////////////////////////////////////////////////////////////
// class IseServerModule - 服务器模块基类

class IseServerModule : boost::noncopyable
{
public:
    friend class IseServerModuleMgr;
public:
    IseServerModule() : svrModIndex_(0) {}
    virtual ~IseServerModule() {}

    // 取得该服务模块中的UDP组别数量
    virtual int getUdpGroupCount() { return 0; }
    // 取得该模块中某UDP组别所接管的动作代码
    virtual void getUdpGroupActionCodes(int groupIndex, ActionCodeArray& list) {}
    // 取得该模块中某UDP组别的配置
    virtual void getUdpGroupOptions(int groupIndex, UdpGroupOptions& options) {}
    // 取得该服务模块中的TCP服务器数量
    virtual int getTcpServerCount() { return 0; }
    // 取得该模块中某TCP服务器的配置
    virtual void getTcpServerOptions(int serverIndex, TcpServerOptions& options) {}

    // UDP数据包分派
    virtual void dispatchUdpPacket(UdpWorkerThread& workerThread, UdpPacket& ) {}

    // 接受了一个新的TCP连接
    virtual void onTcpConnected(const TcpConnectionPtr& connection) {}
    // 断开了一个TCP连接
    virtual void onTcpDisconnected(const TcpConnectionPtr& connection) {}
    // TCP连接上的一个接收任务已完成
    virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
        int packetSize, const Context& context) {}
    // TCP连接上的一个发送任务已完成
    virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context) {}

    // 返回此模块所需辅助服务线程的数量
    virtual int getAssistorThreadCount() { return 0; }
    // 辅助服务线程执行(assistorIndex: 0-based)
    virtual void assistorThreadExecute(AssistorThread& assistorThread, int assistorIndex) {}
    // 系统守护线程执行 (secondCount: 0-based)
    virtual void daemonThreadExecute(Thread& thread, int secondCount) {}

    // 消息分派
    virtual void dispatchMessage(BaseSvrModMessage& Message) {}

    // 返回服务模块序号
    int getSvrModIndex() const { return svrModIndex_; }

private:
    int svrModIndex_;   // (0-based)
};

///////////////////////////////////////////////////////////////////////////////
// class IseServerModuleMgr - 服务模块管理器

class IseServerModuleMgr : boost::noncopyable
{
public:
    IseServerModuleMgr();
    virtual ~IseServerModuleMgr();

    void initServerModuleList(const PointerList& list);
    void clearServerModuleList();

    inline int getCount() const { return items_.getCount(); }
    inline IseServerModule& getItems(int index) { return *(IseServerModule*)items_[index]; }

private:
    PointerList items_;        // 服务模块列表( (IseServerModule*)[] )
};

///////////////////////////////////////////////////////////////////////////////
// class IseSvrModBusiness - 支持服务模块的ISE业务类

class IseSvrModBusiness : public IseBusiness
{
public:
    friend class IseServerModule;

public:
    IseSvrModBusiness() {}
    virtual ~IseSvrModBusiness() {}
public:
    virtual void initialize();
    virtual void finalize();

    virtual void classifyUdpPacket(void *packetBuffer, int packetSize, int& groupIndex);
    virtual void dispatchUdpPacket(UdpWorkerThread& workerThread, int groupIndex, UdpPacket& packet);

    virtual void onTcpConnected(const TcpConnectionPtr& connection);
    virtual void onTcpDisconnected(const TcpConnectionPtr& connection);
    virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
        int packetSize, const Context& context);
    virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context);

    virtual void assistorThreadExecute(AssistorThread& assistorThread, int assistorIndex);
    virtual void daemonThreadExecute(Thread& thread, int secondCount);

public:
    int getAssistorIndex(int serverModuleIndex, int localAssistorIndex);
    void dispatchMessage(BaseSvrModMessage& message);

protected:
    // UDP数据包过滤函数 (返回: true-有效包, false-无效包)
    virtual bool filterUdpPacket(void *packetBuffer, int packetSize) { return true; }
    // 取得UDP数据包中的动作代码
    virtual UINT getUdpPacketActionCode(void *packetBuffer, int packetSize) { return 0; }
    // 创建所有服务模块
    virtual void createServerModules(PointerList& svrModList) {}

private:
    int getUdpGroupCount();
    int getTcpServerCount();
    void initActionCodeMap();
    void initUdpGroupIndexMap();
    void initTcpServerIndexMap();
    void updateIseOptions();

protected:
    typedef map<UINT, int> ActionCodeMap;            // <动作代码, UDP组别号>
    typedef map<UINT, int> UdpGroupIndexMap;         // <全局UDP组别号, 服务模块号>
    typedef map<UINT, int> TcpServerIndexMap;        // <全局TCP服务器序号, 服务模块号>

    IseServerModuleMgr serverModuleMgr_;             // 服务模块管理器
    ActionCodeMap actionCodeMap_;                    // <动作代码, UDP组别号> 映射表
    UdpGroupIndexMap udpGroupIndexMap_;              // <全局UDP组别号, 服务模块号> 映射表
    TcpServerIndexMap tcpServerIndexMap_;            // <全局TCP服务器序号, 服务模块号> 映射表
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SVR_MOD_H_
