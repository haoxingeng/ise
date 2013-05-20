///////////////////////////////////////////////////////////////////////////////

#include "svr_mod_echo.h"

///////////////////////////////////////////////////////////////////////////////

void ServerModule_Echo::onTcpConnected(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpConnected (%s) (ConnCount: %d)",
        connection->getPeerAddr().getDisplayStr().c_str(),
        connection->getServerConnCount());

    string msg = "Welcome to the simple echo server, type 'quit' to exit.\r\n";
    connection->send(msg.c_str(), msg.length());
}

//-----------------------------------------------------------------------------

void ServerModule_Echo::onTcpDisconnected(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpDisconnected (%s)", connection->getConnectionName().c_str());
}

//-----------------------------------------------------------------------------

void ServerModule_Echo::onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
    int packetSize, const Context& context)
{
    logger().writeStr("onTcpRecvComplete");

    string msg((char*)packetBuffer, packetSize);
    msg = trimString(msg);
    if (msg == "quit")
        connection->disconnect();
    else
        connection->send((char*)packetBuffer, packetSize);

    logger().writeFmt("Received message: %s", msg.c_str());
}

//-----------------------------------------------------------------------------

void ServerModule_Echo::onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context)
{
    logger().writeStr("onTcpSendComplete");

    connection->recv(LINE_PACKET_SPLITTER, EMPTY_CONTEXT);
}
