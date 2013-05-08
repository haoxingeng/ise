///////////////////////////////////////////////////////////////////////////////

#include "daytime_server.h"

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
    const char *msg = "Daytime server stoped.";
    std::cout << msg << std::endl;
    logger().writeStr(msg);
}

//-----------------------------------------------------------------------------

void AppBusiness::doStartupState(STARTUP_STATE state)
{
    switch (state)
    {
    case SS_AFTER_START:
        {
            const char *msg = "Daytime server started.";
            std::cout << std::endl << msg << std::endl;
            logger().writeStr(msg);
        }
        break;

    case SS_START_FAIL:
        {
            const char *msg = "Fail to start daytime server.";
            std::cout << std::endl << msg << std::endl;
            logger().writeStr(msg);
        }
        break;

    default:
        break;
    }
}

//-----------------------------------------------------------------------------

void AppBusiness::initIseOptions(IseOptions& options)
{
    options.setLogFileName(getAppSubPath("log") + "daytime-log.txt", true);
    options.setIsDaemon(true);
    options.setAllowMultiInstance(false);

    options.setServerType(ST_TCP);
    options.setTcpServerCount(1);
    options.setTcpServerPort(10002);
    options.setTcpEventLoopCount(1);
}

//-----------------------------------------------------------------------------

void AppBusiness::onTcpConnected(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpConnected (%s) (ConnCount: %d)",
        connection->getPeerAddr().getDisplayStr().c_str(),
        connection->getServerConnCount());

    string msg = DateTime::currentDateTime().dateTimeString() + "\n";
    connection->send(msg.c_str(), msg.length());
}

//-----------------------------------------------------------------------------

void AppBusiness::onTcpDisconnected(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpDisconnected (%s)", connection->getConnectionName().c_str());
}

//-----------------------------------------------------------------------------

void AppBusiness::onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context)
{
    logger().writeStr("onTcpSendComplete");

    connection->disconnect();
}
