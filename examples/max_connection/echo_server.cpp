///////////////////////////////////////////////////////////////////////////////

#include "echo_server.h"

IseBusiness* createIseBusinessObject()
{
    return new AppBusiness();
}

///////////////////////////////////////////////////////////////////////////////

void AppBusiness::initialize()
{
    // nothing
}

//-----------------------------------------------------------------------------

void AppBusiness::finalize()
{
    string msg = formatString("%s stoped.", getAppExeName(false).c_str());
    std::cout << msg << std::endl;
    logger().writeStr(msg);
}

//-----------------------------------------------------------------------------

void AppBusiness::afterInit()
{
    string msg = formatString("%s started.", getAppExeName(false).c_str());
    std::cout << std::endl << msg << std::endl;
    logger().writeStr(msg);
}

//-----------------------------------------------------------------------------

void AppBusiness::onInitFailed(Exception& e)
{
    string msg = formatString("fail to start %s.", getAppExeName(false).c_str());
    std::cout << std::endl << msg << std::endl;
    logger().writeStr(msg);
}

//-----------------------------------------------------------------------------

void AppBusiness::initIseOptions(IseOptions& options)
{
    options.setServerType(ST_TCP);
    options.setTcpServerCount(1);
    options.setTcpServerPort(10000);
    options.setTcpServerEventLoopCount(1);
}

//-----------------------------------------------------------------------------

void AppBusiness::onTcpConnected(const TcpConnectionPtr& connection)
{
    const int MAX_CONN_COUNT = 3;

    logger().writeFmt("onTcpConnected (%s) (ConnCount: %d)",
        connection->getPeerAddr().getDisplayStr().c_str(),
        connection->getServerConnCount());

    int connCount = connection->getServerConnCount();
    if (connCount > MAX_CONN_COUNT)
    {
        logger().writeFmt("Too many connections. (ConnCount: %d)", connCount);
        connection->disconnect();
    }
    else
    {
        string msg = "Welcome to the simple echo server, type 'quit' to exit.\r\n";
        connection->send(msg.c_str(), msg.length());
    }
}

//-----------------------------------------------------------------------------

void AppBusiness::onTcpDisconnected(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpDisconnected (%s)", connection->getConnectionName().c_str());
}

//-----------------------------------------------------------------------------

void AppBusiness::onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
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

void AppBusiness::onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context)
{
    logger().writeStr("onTcpSendComplete");

    connection->recv(LINE_PACKET_SPLITTER);
}
