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
        cout << getAppHelp() << endl;
        return false;
    }
    else
        return true;
}

//-----------------------------------------------------------------------------

string AppBusiness::getAppHelp()
{
    return formatString("Usage: %s ip\n",
        extractFileName(GetAppExeName()).c_str());
}

//-----------------------------------------------------------------------------

void AppBusiness::doStartupState(STARTUP_STATE state)
{
    switch (state)
    {
    case SS_AFTER_START:
        {
            string ip = iseApp().getArgString(1);
            int port = 10003;

            TcpConnector::instance().connect(InetAddress(ip, port),
                boost::bind(&AppBusiness::onConnectComplete, this, _1, _2, _3, _4));
        }
        break;
    }
}

//-----------------------------------------------------------------------------

void AppBusiness::initIseOptions(IseOptions& options)
{
    options.setLogFileName(getAppSubPath("log") + "chargen-client-log.txt", true);
    options.setIsDaemon(false);
    options.setAllowMultiInstance(true);
    options.setServerType(ST_TCP);
    options.setTcpEventLoopCount(1);
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
    logger().writeFmt("[%s] Discarded %u bytes.",
        connection->getConnectionName().c_str(), packetSize);

    connection->recv();
}

//-----------------------------------------------------------------------------

void AppBusiness::onConnectComplete(bool success,  TcpConnection *connection,
    const InetAddress& peerAddr, const Context& context)
{
    logger().writeFmt("connect to %s %s.", peerAddr.getDisplayStr().c_str(), success? "successfully" : "failed");
}
