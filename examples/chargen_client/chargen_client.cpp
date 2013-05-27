///////////////////////////////////////////////////////////////////////////////

#include "chargen_client.h"

//-----------------------------------------------------------------------------

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
    // nothing
}

//-----------------------------------------------------------------------------

bool AppBusiness::parseArguments(int argc, char *argv[])
{
    if (argc <= 1)
    {
        std::cout << getAppHelp() << std::endl;
        return false;
    }
    else
        return true;
}

//-----------------------------------------------------------------------------

string AppBusiness::getAppHelp()
{
    return formatString("Usage: %s ip\n",
        extractFileName(getAppExeName()).c_str());
}

//-----------------------------------------------------------------------------

void AppBusiness::afterInit()
{
    string ip = iseApp().getArgString(0);
    int port = 10003;

    TcpConnector::instance().connect(InetAddress(ip, port),
        boost::bind(&AppBusiness::onConnectComplete, this, _1, _2, _3, _4));
}

//-----------------------------------------------------------------------------

void AppBusiness::initIseOptions(IseOptions& options)
{
    options.setIsDaemon(false);
    options.setServerType(ST_TCP);
    options.setTcpServerEventLoopCount(1);
}

//-----------------------------------------------------------------------------

void AppBusiness::onTcpConnected(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpConnected (%s) (ConnCount: %d)",
        connection->getPeerAddr().getDisplayStr().c_str(),
        connection->getServerConnCount());

    connection->recv();
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
    string msg((const char*)packetBuffer, packetSize);

    logger().writeFmt("[%s] Discarded %u bytes.",
        connection->getConnectionName().c_str(), packetSize);

    std::cout << msg;

    connection->recv();
}

//-----------------------------------------------------------------------------

void AppBusiness::onConnectComplete(bool success,  TcpConnection *connection,
    const InetAddress& peerAddr, const Context& context)
{
    logger().writeFmt("connect to %s %s.", peerAddr.getDisplayStr().c_str(), success? "successfully" : "failed");
}
