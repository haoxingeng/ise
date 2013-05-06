///////////////////////////////////////////////////////////////////////////////

#include "chargen_server.h"

//-----------------------------------------------------------------------------

IseBusiness* createIseBusinessObject()
{
    return new AppBusiness();
}

///////////////////////////////////////////////////////////////////////////////

void AppBusiness::initMessage()
{
    string line;
    for (int i = 33; i < 127; ++i)
        line.push_back(char(i));
    line += line;

    message_.clear();
    for (size_t i = 0; i < 127-33; ++i)
        message_ += line.substr(i, 72) + '\n';
}

//-----------------------------------------------------------------------------

void AppBusiness::initialize()
{
    initMessage();
    transferredBytes_ = 0;
}

//-----------------------------------------------------------------------------

void AppBusiness::finalize()
{
    const char *msg = "Chargen server stoped.";
    cout << msg << endl;
    logger().writeStr(msg);
}

//-----------------------------------------------------------------------------

void AppBusiness::doStartupState(STARTUP_STATE state)
{
    switch (state)
    {
    case SS_AFTER_START:
        {
            const char *msg = "Chargen server started.";
            cout << endl << msg << endl;
            logger().writeStr(msg);
        }
        break;

    case SS_START_FAIL:
        {
            const char *msg = "Fail to start chargen server.";
            cout << endl << msg << endl;
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
    options.setLogFileName(getAppSubPath("log") + "chargen-server-log.txt", true);
    options.setIsDaemon(true);
    options.setAllowMultiInstance(false);

    options.setServerType(ST_TCP);
    options.setTcpServerCount(1);
    options.setTcpServerPort(10003);
    options.setTcpEventLoopCount(1);
}

//-----------------------------------------------------------------------------

void AppBusiness::onTcpConnected(const TcpConnectionPtr& connection)
{
    logger().writeFmt("onTcpConnected (%s) (ConnCount: %d)",
        connection->getPeerAddr().getDisplayStr().c_str(),
        connection->getServerConnCount());

    connection->setNoDelay(true);

    connection->recv();
    connection->send(message_.c_str(), message_.length());
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

void AppBusiness::onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context)
{
    transferredBytes_ += message_.length();
    connection->send(message_.c_str(), message_.length());
}
