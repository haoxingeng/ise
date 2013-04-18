///////////////////////////////////////////////////////////////////////////////

#include "echo.h"

IseBusiness* createIseBusinessObject()
{
    return new AppBusiness();
}

//-----------------------------------------------------------------------------
// 描述: 初始化 (失败则抛出异常)
//-----------------------------------------------------------------------------
void AppBusiness::initialize()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 结束化 (无论初始化是否有异常，结束时都会执行)
//-----------------------------------------------------------------------------
void AppBusiness::finalize()
{
    const char *pMsg = "Echo server stoped.";
    cout << pMsg << endl;
    logger().writeStr(pMsg);
}

//-----------------------------------------------------------------------------
// 描述: 处理启动状态
//-----------------------------------------------------------------------------
void AppBusiness::doStartupState(STARTUP_STATE state)
{
    switch (state)
    {
    case SS_AFTER_START:
        {
            const char *pMsg = "Echo server started.";
            cout << endl << pMsg << endl;
            logger().writeStr(pMsg);
        }
        break;

    case SS_START_FAIL:
        {
            const char *pMsg = "Fail to start echo server.";
            cout << endl << pMsg << endl;
            logger().writeStr(pMsg);
        }
        break;
    }
}

//-----------------------------------------------------------------------------
// 描述: 初始化SSE配置信息
//-----------------------------------------------------------------------------
void AppBusiness::initIseOptions(IseOptions& options)
{
    options.setLogFileName(getAppSubPath("log") + "echo-log.txt", true);
    options.setIsDaemon(true);
    options.setAllowMultiInstance(false);

    // 设置服务器类型
    options.setServerType(ST_TCP);
    // 设置TCP服务器的总数
    options.setTcpServerCount(1);
    // 设置TCP服务端口号
    options.setTcpServerPort(0, 12345);
    // 设置TCP事件循环的个数
    options.setTcpEventLoopCount(3);
}

//-----------------------------------------------------------------------------
// 描述: 接受了一个新的TCP连接
//-----------------------------------------------------------------------------
void AppBusiness::onTcpConnect(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpConnect (%s)", connection->getPeerAddr().getDisplayStr().c_str());

    connection->recv(LINE_PACKET_SPLITTER);
}

//-----------------------------------------------------------------------------
// 描述: 断开了一个TCP连接 (ISE将随之删除此连接对象)
//-----------------------------------------------------------------------------
void AppBusiness::onTcpDisconnect(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpDisconnect (%s)", connection->getPeerAddr().getDisplayStr().c_str());
}

//-----------------------------------------------------------------------------
// 描述: TCP连接上的一个接收任务已完成
//-----------------------------------------------------------------------------
void AppBusiness::onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
    int packetSize, const Context& context)
{
    logger().writeStr("onTcpRecvComplete");

    string strMsg((char*)packetBuffer, packetSize);
    strMsg = trimString(strMsg);
    if (strMsg == "bye")
        connection->disconnect();
    else
        connection->send((char*)packetBuffer, packetSize);

    logger().writeFmt("Received message: %s", strMsg.c_str());
}

//-----------------------------------------------------------------------------
// 描述: TCP连接上的一个发送任务已完成
//-----------------------------------------------------------------------------
void AppBusiness::onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context)
{
    logger().writeStr("onTcpSendComplete");
    connection->recv(LINE_PACKET_SPLITTER);
}
