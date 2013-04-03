///////////////////////////////////////////////////////////////////////////////

#include "echo.h"

CSseBusiness* CreateSseBusinessObject()
{
	return new CAppBusiness();
}

//-----------------------------------------------------------------------------
// 描述: 初始化 (失败则抛出异常)
//-----------------------------------------------------------------------------
void CAppBusiness::Initialize()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 结束化 (无论初始化是否有异常，结束时都会执行)
//-----------------------------------------------------------------------------
void CAppBusiness::Finalize()
{
	const char *pMsg = "Echo server stoped.";
	cout << pMsg << endl;
	Logger().WriteStr(pMsg);
}

//-----------------------------------------------------------------------------
// 描述: 处理启动状态
//-----------------------------------------------------------------------------
void CAppBusiness::DoStartupState(STARTUP_STATE nState)
{
	switch (nState)
	{
	case SS_AFTER_START:
		{
			const char *pMsg = "Echo server started.";
			cout << endl << pMsg << endl;
			Logger().WriteStr(pMsg);
		}
		break;

	case SS_START_FAIL:
		{
			const char *pMsg = "Fail to start echo server.";
			cout << endl << pMsg << endl;
			Logger().WriteStr(pMsg);
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// 描述: 初始化SSE配置信息
//-----------------------------------------------------------------------------
void CAppBusiness::InitSseOptions(CSseOptions& SseOpt)
{
	SseOpt.SetLogFileName(GetAppSubPath("log") + "echo-log.txt", true);
	SseOpt.SetIsDaemon(true);
	SseOpt.SetAllowMultiInstance(false);

	// 设置服务器类型
	SseOpt.SetServerType(ST_TCP);
	// 设置TCP服务器的总数
	SseOpt.SetTcpServerCount(1);
	// 设置TCP服务端口号
	SseOpt.SetTcpServerPort(0, 12345);
	// 设置TCP事件循环的个数
	SseOpt.SetTcpEventLoopCount(3);
}

//-----------------------------------------------------------------------------
// 描述: 接受了一个新的TCP连接
//-----------------------------------------------------------------------------
void CAppBusiness::OnTcpConnection(CTcpConnection *pConnection)
{
	Logger().WriteStr("OnTcpConnection");
	pConnection->PostRecvTask(&CLinePacketMeasurer::Instance());
}

//-----------------------------------------------------------------------------
// 描述: TCP连接传输过程发生了错误 (SSE将随之删除此连接对象)
//-----------------------------------------------------------------------------
void CAppBusiness::OnTcpError(CTcpConnection *pConnection)
{
	Logger().WriteStr("OnTcpError");
}

//-----------------------------------------------------------------------------
// 描述: TCP连接上的一个接收任务已完成
//-----------------------------------------------------------------------------
void CAppBusiness::OnTcpRecvComplete(CTcpConnection *pConnection, void *pPacketBuffer,
	int nPacketSize, const CCustomParams& Params)
{
	Logger().WriteStr("OnTcpRecvComplete");

	string strMsg((char*)pPacketBuffer, nPacketSize);
	strMsg = TrimString(strMsg);
	if (strMsg == "bye")
		pConnection->Disconnect();
	else
		pConnection->PostSendTask((char*)pPacketBuffer, nPacketSize);

	Logger().WriteFmt("Received message: %s", strMsg.c_str());
}

//-----------------------------------------------------------------------------
// 描述: TCP连接上的一个发送任务已完成
//-----------------------------------------------------------------------------
void CAppBusiness::OnTcpSendComplete(CTcpConnection *pConnection, const CCustomParams& Params)
{
	Logger().WriteStr("OnTcpSendComplete");
	pConnection->PostRecvTask(&CLinePacketMeasurer::Instance());
}
