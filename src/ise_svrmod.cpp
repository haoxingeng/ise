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
// 文件名称: ise_svrmod.cpp
// 功能描述: 服务模块支持
///////////////////////////////////////////////////////////////////////////////

#include "ise_svrmod.h"
#include "ise_scheduler.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// classs CIseServerModuleMgr

CIseServerModuleMgr::CIseServerModuleMgr()
{
	// nothing
}

CIseServerModuleMgr::~CIseServerModuleMgr()
{
	ClearServerModuleList();
}

//-----------------------------------------------------------------------------
// 描述: 初始化服务模块列表
//-----------------------------------------------------------------------------
void CIseServerModuleMgr::InitServerModuleList(const CList& List)
{
	m_Items.Clear();
	for (int i = 0; i < List.GetCount(); i++)
	{
		CIseServerModule *pModule = (CIseServerModule*)List[i];
		pModule->m_nSvrModIndex = i;
		m_Items.Add(pModule);
	}
}

//-----------------------------------------------------------------------------
// 描述: 清空所有服务模块
//-----------------------------------------------------------------------------
void CIseServerModuleMgr::ClearServerModuleList()
{
	for (int i = 0; i < m_Items.GetCount(); i++)
		delete (CIseServerModule*)m_Items[i];
	m_Items.Clear();
}

///////////////////////////////////////////////////////////////////////////////
// class CIseSvrModBusiness

//-----------------------------------------------------------------------------
// 描述: 取得全局UDP组别数量
//-----------------------------------------------------------------------------
int CIseSvrModBusiness::GetUdpGroupCount()
{
	int nResult = 0;

	for (int i = 0; i < m_ServerModuleMgr.GetCount(); i++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(i);
		nResult += ServerModule.GetUdpGroupCount();
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// 描述: 取得全局TCP服务器数量
//-----------------------------------------------------------------------------
int CIseSvrModBusiness::GetTcpServerCount()
{
	int nResult = 0;

	for (int i = 0; i < m_ServerModuleMgr.GetCount(); i++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(i);
		nResult += ServerModule.GetTcpServerCount();
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// 描述: 初始化 m_ActionCodeMap
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::InitActionCodeMap()
{
	int nGlobalGroupIndex = 0;
	m_ActionCodeMap.clear();

	for (int nModIndex = 0; nModIndex < m_ServerModuleMgr.GetCount(); nModIndex++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(nModIndex);
		for (int nGroupIndex = 0; nGroupIndex < ServerModule.GetUdpGroupCount(); nGroupIndex++)
		{
			ACTION_CODE_ARRAY ActionCodes;
			ServerModule.GetUdpGroupActionCodes(nGroupIndex, ActionCodes);

			for (UINT i = 0; i < ActionCodes.size(); i++)
				m_ActionCodeMap[ActionCodes[i]] = nGlobalGroupIndex;

			nGlobalGroupIndex++;
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 初始化 m_UdpGroupIndexMap
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::InitUdpGroupIndexMap()
{
	int nGlobalGroupIndex = 0;
	m_UdpGroupIndexMap.clear();

	for (int nModIndex = 0; nModIndex < m_ServerModuleMgr.GetCount(); nModIndex++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(nModIndex);
		for (int nGroupIndex = 0; nGroupIndex < ServerModule.GetUdpGroupCount(); nGroupIndex++)
		{
			m_UdpGroupIndexMap[nGlobalGroupIndex] = nModIndex;
			nGlobalGroupIndex++;
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 初始化 m_TcpServerIndexMap
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::InitTcpServerIndexMap()
{
	int nGlobalServerIndex = 0;
	m_TcpServerIndexMap.clear();

	for (int nModIndex = 0; nModIndex < m_ServerModuleMgr.GetCount(); nModIndex++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(nModIndex);
		for (int nServerIndex = 0; nServerIndex < ServerModule.GetTcpServerCount(); nServerIndex++)
		{
			m_TcpServerIndexMap[nGlobalServerIndex] = nModIndex;
			nGlobalServerIndex++;
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 更新ISE配置
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::UpdateIseOptions()
{
	CIseOptions& IseOpt = IseApplication.GetIseOptions();

	// 设置UDP请求组别的总数量
	IseOpt.SetUdpRequestGroupCount(GetUdpGroupCount());
	// 设置TCP服务器的总数量
	IseOpt.SetTcpServerCount(GetTcpServerCount());

	// UDP请求组别相关设置
	int nGlobalGroupIndex = 0;
	for (int nModIndex = 0; nModIndex < m_ServerModuleMgr.GetCount(); nModIndex++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(nModIndex);
		for (int nGroupIndex = 0; nGroupIndex < ServerModule.GetUdpGroupCount(); nGroupIndex++)
		{
			UDP_GROUP_OPTIONS GrpOpt;
			memset(&GrpOpt, 0, sizeof(GrpOpt));
			ServerModule.GetUdpGroupOptions(nGroupIndex, GrpOpt);
			IseOpt.SetUdpRequestQueueCapacity(nGlobalGroupIndex, GrpOpt.nQueueCapacity);
			IseOpt.SetUdpWorkerThreadCount(nGlobalGroupIndex, GrpOpt.nMinThreads, GrpOpt.nMaxThreads);
			nGlobalGroupIndex++;
		}
	}

	// TCP服务器相关设置
	int nGlobalServerIndex = 0;
	for (int nModIndex = 0; nModIndex < m_ServerModuleMgr.GetCount(); nModIndex++)
	{
		CIseServerModule& ServerModule = m_ServerModuleMgr.GetItems(nModIndex);
		for (int nServerIndex = 0; nServerIndex < ServerModule.GetTcpServerCount(); nServerIndex++)
		{
			TCP_SERVER_OPTIONS SvrOpt;
			memset(&SvrOpt, 0, sizeof(SvrOpt));
			ServerModule.GetTcpServerOptions(nServerIndex, SvrOpt);
			IseOpt.SetTcpServerPort(nGlobalServerIndex, SvrOpt.nPort);
			nGlobalServerIndex++;
		}
	}

	// 设置辅助服务线程的数量
	int nAssistorCount = 0;
	for (int i = 0; i < m_ServerModuleMgr.GetCount(); i++)
		nAssistorCount += m_ServerModuleMgr.GetItems(i).GetAssistorThreadCount();
	IseOpt.SetAssistorThreadCount(nAssistorCount);
}

//-----------------------------------------------------------------------------
// 描述: 初始化 (失败则抛出异常)
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::Initialize()
{
	CIseBusiness::Initialize();

	// 创建所有服务模块
	CList SvrModList;
	CreateServerModules(SvrModList);
	m_ServerModuleMgr.InitServerModuleList(SvrModList);

	// 初始化各个映射表
	InitActionCodeMap();
	InitUdpGroupIndexMap();
	InitTcpServerIndexMap();

	// 更新ISE配置
	UpdateIseOptions();
}

//-----------------------------------------------------------------------------
// 描述: 结束化 (无论初始化是否有异常，结束时都会执行)
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::Finalize()
{
	try { m_ServerModuleMgr.ClearServerModuleList(); } catch (...) {}

	CIseBusiness::Finalize();
}

//-----------------------------------------------------------------------------
// 描述: UDP数据包分类
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::ClassifyUdpPacket(void *pPacketBuffer, int nPacketSize, int& nGroupIndex)
{
	nGroupIndex = -1;
	if (nPacketSize <= 0 || !FilterUdpPacket(pPacketBuffer, nPacketSize)) return;

	UINT nActionCode = GetUdpPacketActionCode(pPacketBuffer, nPacketSize);
	ACTION_CODE_MAP::iterator iter = m_ActionCodeMap.find(nActionCode);
	if (iter != m_ActionCodeMap.end())
	{
		nGroupIndex = iter->second;
	}
}

//-----------------------------------------------------------------------------
// 描述: UDP数据包分派
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::DispatchUdpPacket(CUdpWorkerThread& WorkerThread,
	int nGroupIndex, CUdpPacket& Packet)
{
	UDP_GROUP_INDEX_MAP::iterator iter = m_UdpGroupIndexMap.find(nGroupIndex);
	if (iter != m_UdpGroupIndexMap.end())
	{
		int nModIndex = iter->second;
		m_ServerModuleMgr.GetItems(nModIndex).DispatchUdpPacket(WorkerThread, Packet);
	}
}

//-----------------------------------------------------------------------------
// 描述: 接受了一个新的TCP连接
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::OnTcpConnection(CTcpConnection *pConnection)
{
	TCP_SERVER_INDEX_MAP::iterator iter = m_TcpServerIndexMap.find(pConnection->GetServerIndex());
	if (iter != m_TcpServerIndexMap.end())
	{
		int nModIndex = iter->second;
		m_ServerModuleMgr.GetItems(nModIndex).OnTcpConnection(pConnection);
	}
}

//-----------------------------------------------------------------------------
// 描述: TCP连接传输过程发生了错误 (ISE将随之删除此连接对象)
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::OnTcpError(CTcpConnection *pConnection)
{
	TCP_SERVER_INDEX_MAP::iterator iter = m_TcpServerIndexMap.find(pConnection->GetServerIndex());
	if (iter != m_TcpServerIndexMap.end())
	{
		int nModIndex = iter->second;
		m_ServerModuleMgr.GetItems(nModIndex).OnTcpError(pConnection);
	}
}

//-----------------------------------------------------------------------------
// 描述: TCP连接上的一个接收任务已完成
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::OnTcpRecvComplete(CTcpConnection *pConnection, void *pPacketBuffer,
	int nPacketSize, const CCustomParams& Params)
{
	TCP_SERVER_INDEX_MAP::iterator iter = m_TcpServerIndexMap.find(pConnection->GetServerIndex());
	if (iter != m_TcpServerIndexMap.end())
	{
		int nModIndex = iter->second;
		m_ServerModuleMgr.GetItems(nModIndex).OnTcpRecvComplete(pConnection,
			pPacketBuffer, nPacketSize, Params);
	}
}

//-----------------------------------------------------------------------------
// 描述: TCP连接上的一个发送任务已完成
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::OnTcpSendComplete(CTcpConnection *pConnection, const CCustomParams& Params)
{
	TCP_SERVER_INDEX_MAP::iterator iter = m_TcpServerIndexMap.find(pConnection->GetServerIndex());
	if (iter != m_TcpServerIndexMap.end())
	{
		int nModIndex = iter->second;
		m_ServerModuleMgr.GetItems(nModIndex).OnTcpSendComplete(pConnection, Params);
	}
}

//-----------------------------------------------------------------------------
// 描述: 辅助服务线程执行(nAssistorIndex: 0-based)
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::AssistorThreadExecute(CAssistorThread& AssistorThread, int nAssistorIndex)
{
	int nIndex1, nIndex2 = 0;

	for (int i = 0; i < m_ServerModuleMgr.GetCount(); i++)
	{
		nIndex1 = nIndex2;
		nIndex2 += m_ServerModuleMgr.GetItems(i).GetAssistorThreadCount();

		if (nAssistorIndex >= nIndex1 && nAssistorIndex < nIndex2)
		{
			m_ServerModuleMgr.GetItems(i).AssistorThreadExecute(AssistorThread, nAssistorIndex - nIndex1);
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 系统守护线程执行 (nSecondCount: 0-based)
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::DaemonThreadExecute(CThread& Thread, int nSecondCount)
{
	for (int i = 0; i < m_ServerModuleMgr.GetCount(); i++)
		m_ServerModuleMgr.GetItems(i).DaemonThreadExecute(Thread, nSecondCount);
}

//-----------------------------------------------------------------------------
// 描述: 根据服务模块序号和模块内的局部辅助线程序号，取得此辅助线程的全局序号
//-----------------------------------------------------------------------------
int CIseSvrModBusiness::GetAssistorIndex(int nServerModuleIndex, int nLocalAssistorIndex)
{
	int nResult = -1;

	if (nServerModuleIndex >= 0 && nServerModuleIndex < m_ServerModuleMgr.GetCount())
	{
		nResult = 0;
		for (int i = 0; i < nServerModuleIndex; i++)
			nResult += m_ServerModuleMgr.GetItems(i).GetAssistorThreadCount();
		nResult += nLocalAssistorIndex;
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// 描述: 分派消息给服务模块
//-----------------------------------------------------------------------------
void CIseSvrModBusiness::DispatchMessage(CBaseSvrModMessage& Message)
{
	for (int i = 0; i < m_ServerModuleMgr.GetCount(); i++)
	{
		if (Message.bHandled) break;
		m_ServerModuleMgr.GetItems(i).DispatchMessage(Message);
	}
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
