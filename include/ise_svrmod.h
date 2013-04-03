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
// ise_svrmod.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SVRMOD_H_
#define _ISE_SVRMOD_H_

#include "ise.h"
#include "ise_application.h"
#include "ise_svrmod_msgs.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class CIseServerModule;
class CIseServerModuleMgr;
class CIseSvrModBusiness;

///////////////////////////////////////////////////////////////////////////////
// 类型定义

// 动作代码数组
typedef vector<UINT> ACTION_CODE_ARRAY;

// UDP组别的配置
struct UDP_GROUP_OPTIONS
{
	int nQueueCapacity;
	int nMinThreads;
	int nMaxThreads;
};

// TCP服务器的配置
struct TCP_SERVER_OPTIONS
{
	int nPort;
};

///////////////////////////////////////////////////////////////////////////////
// class CIseServerModule - 服务器模块基类

class CIseServerModule
{
public:
	friend class CIseServerModuleMgr;
private:
	int m_nSvrModIndex;   // (0-based)
public:
	CIseServerModule() : m_nSvrModIndex(0) {}
	virtual ~CIseServerModule() {}

	// 取得该服务模块中的UDP组别数量
	virtual int GetUdpGroupCount() { return 0; }
	// 取得该模块中某UDP组别所接管的动作代码
	virtual void GetUdpGroupActionCodes(int nGroupIndex, ACTION_CODE_ARRAY& List) {}
	// 取得该模块中某UDP组别的配置
	virtual void GetUdpGroupOptions(int nGroupIndex, UDP_GROUP_OPTIONS& Options) {}
	// 取得该服务模块中的TCP服务器数量
	virtual int GetTcpServerCount() { return 0; }
	// 取得该模块中某TCP服务器的配置
	virtual void GetTcpServerOptions(int nServerIndex, TCP_SERVER_OPTIONS& Options) {}

	// UDP数据包分派
	virtual void DispatchUdpPacket(CUdpWorkerThread& WorkerThread, CUdpPacket& ) {}

	// 接受了一个新的TCP连接
	virtual void OnTcpConnection(CTcpConnection *pConnection) {}
	// TCP连接传输过程发生了错误 (ISE将随之删除此连接对象)
	virtual void OnTcpError(CTcpConnection *pConnection) {}
	// TCP连接上的一个接收任务已完成
	virtual void OnTcpRecvComplete(CTcpConnection *pConnection, void *pPacketBuffer,
		int nPacketSize, const CCustomParams& Params) {}
	// TCP连接上的一个发送任务已完成
	virtual void OnTcpSendComplete(CTcpConnection *pConnection, const CCustomParams& Params) {}

	// 返回此模块所需辅助服务线程的数量
	virtual int GetAssistorThreadCount() { return 0; }
	// 辅助服务线程执行(nAssistorIndex: 0-based)
	virtual void AssistorThreadExecute(CAssistorThread& AssistorThread, int nAssistorIndex) {}
	// 系统守护线程执行 (nSecondCount: 0-based)
	virtual void DaemonThreadExecute(CThread& Thread, int nSecondCount) {}

	// 消息分派
	virtual void DispatchMessage(CBaseSvrModMessage& Message) {}

	// 返回服务模块序号
	int GetSvrModIndex() const { return m_nSvrModIndex; }
};

///////////////////////////////////////////////////////////////////////////////
// class CIseServerModuleMgr - 服务模块管理器

class CIseServerModuleMgr
{
private:
	CList m_Items;        // 服务模块列表( (CIseServerModule*)[] )
public:
	CIseServerModuleMgr();
	virtual ~CIseServerModuleMgr();

	void InitServerModuleList(const CList& List);
	void ClearServerModuleList();

	inline int GetCount() const { return m_Items.GetCount(); }
	inline CIseServerModule& GetItems(int nIndex) { return *(CIseServerModule*)m_Items[nIndex]; }
};

///////////////////////////////////////////////////////////////////////////////
// class CIseSvrModBusiness - 支持服务模块的ISE业务类

class CIseSvrModBusiness : public CIseBusiness
{
public:
	friend class CIseServerModule;
protected:
	typedef hash_map<UINT, int> ACTION_CODE_MAP;      // <动作代码, UDP组别号>
	typedef hash_map<UINT, int> UDP_GROUP_INDEX_MAP;  // <全局UDP组别号, 服务模块号>
	typedef hash_map<UINT, int> TCP_SERVER_INDEX_MAP; // <全局TCP服务器序号, 服务模块号>

	CIseServerModuleMgr m_ServerModuleMgr;            // 服务模块管理器
	ACTION_CODE_MAP m_ActionCodeMap;                  // <动作代码, UDP组别号> 映射表
	UDP_GROUP_INDEX_MAP m_UdpGroupIndexMap;           // <全局UDP组别号, 服务模块号> 映射表
	TCP_SERVER_INDEX_MAP m_TcpServerIndexMap;         // <全局TCP服务器序号, 服务模块号> 映射表
private:
	int GetUdpGroupCount();
	int GetTcpServerCount();
	void InitActionCodeMap();
	void InitUdpGroupIndexMap();
	void InitTcpServerIndexMap();
	void UpdateIseOptions();
protected:
	// UDP数据包过滤函数 (返回: true-有效包, false-无效包)
	virtual bool FilterUdpPacket(void *pPacketBuffer, int nPacketSize) { return true; }
	// 取得UDP数据包中的动作代码
	virtual UINT GetUdpPacketActionCode(void *pPacketBuffer, int nPacketSize) { return 0; }
	// 创建所有服务模块
	virtual void CreateServerModules(CList& SvrModList) {}
public:
	CIseSvrModBusiness() {}
	virtual ~CIseSvrModBusiness() {}
public:
	virtual void Initialize();
	virtual void Finalize();

	virtual void ClassifyUdpPacket(void *pPacketBuffer, int nPacketSize, int& nGroupIndex);
	virtual void DispatchUdpPacket(CUdpWorkerThread& WorkerThread, int nGroupIndex, CUdpPacket& Packet);

	virtual void OnTcpConnection(CTcpConnection *pConnection);
	virtual void OnTcpError(CTcpConnection *pConnection);
	virtual void OnTcpRecvComplete(CTcpConnection *pConnection, void *pPacketBuffer,
		int nPacketSize, const CCustomParams& Params);
	virtual void OnTcpSendComplete(CTcpConnection *pConnection, const CCustomParams& Params);

	virtual void AssistorThreadExecute(CAssistorThread& AssistorThread, int nAssistorIndex);
	virtual void DaemonThreadExecute(CThread& Thread, int nSecondCount);
public:
	int GetAssistorIndex(int nServerModuleIndex, int nLocalAssistorIndex);
	void DispatchMessage(CBaseSvrModMessage& Message);
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SVRMOD_H_
