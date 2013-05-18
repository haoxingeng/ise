///////////////////////////////////////////////////////////////////////////////

#include "svr_mod_chargen.h"

///////////////////////////////////////////////////////////////////////////////

ServerModule_Chargen::ServerModule_Chargen()
{
    initMessage();
    transferredBytes_ = 0;
}

//-----------------------------------------------------------------------------

void ServerModule_Chargen::initMessage()
{
    std::string line;
    for (int i = 33; i < 127; ++i)
        line.push_back(char(i));
    line += line;

    message_.clear();
    for (size_t i = 0; i < 127-33; ++i)
        message_ += line.substr(i, 72) + '\n';
}

//-----------------------------------------------------------------------------

void ServerModule_Chargen::onTcpConnected(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpConnected (%s) (ConnCount: %d)",
        connection->getPeerAddr().getDisplayStr().c_str(),
        connection->getServerConnCount());

    connection->setNoDelay(true);

    connection->recv();
    connection->send(message_.c_str(), message_.length());
}

//-----------------------------------------------------------------------------

void ServerModule_Chargen::onTcpDisconnected(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpDisconnected (%s)", connection->getConnectionName().c_str());
}

//-----------------------------------------------------------------------------

void ServerModule_Chargen::onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
    int packetSize, const Context& context)
{
    logger().writeFmt("[%s] Discarded %u bytes.",
        connection->getConnectionName().c_str(), packetSize);

    connection->recv();
}

//-----------------------------------------------------------------------------

void ServerModule_Chargen::onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context)
{
    transferredBytes_ += message_.length();
    connection->send(message_.c_str(), message_.length());
}
