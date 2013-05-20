///////////////////////////////////////////////////////////////////////////////

#include "svr_mod_daytime.h"

///////////////////////////////////////////////////////////////////////////////

void ServerModule_Daytime::onTcpConnected(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpConnected (%s) (ConnCount: %d)",
        connection->getPeerAddr().getDisplayStr().c_str(),
        connection->getServerConnCount());

    string msg = DateTime::currentDateTime().dateTimeString() + "\n";
    connection->send(msg.c_str(), msg.length());
}

//-----------------------------------------------------------------------------

void ServerModule_Daytime::onTcpDisconnected(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpDisconnected (%s)", connection->getConnectionName().c_str());
}

//-----------------------------------------------------------------------------

void ServerModule_Daytime::onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context)
{
    logger().writeStr("onTcpSendComplete");

    connection->disconnect();
}
