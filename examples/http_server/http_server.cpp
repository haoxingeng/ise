///////////////////////////////////////////////////////////////////////////////

#include "http_server.h"

IseBusiness* createIseBusinessObject()
{
    return new AppBusiness();
}

///////////////////////////////////////////////////////////////////////////////

void AppBusiness::initialize()
{
    httpServer_.setHttpSessionCallback(boost::bind(&AppBusiness::onHttpSession, this, _1, _2));
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
    options.setLogFileName(getAppSubPath("log") + changeFileExt(extractFileName(getAppExeName()), ".log"), true);
    options.setIsDaemon(true);
    options.setAllowMultiInstance(false);

    options.setServerType(ST_TCP);
    options.setTcpServerCount(1);
    options.setTcpServerPort(8080);
    options.setTcpServerEventLoopCount(1);
}

//-----------------------------------------------------------------------------

void AppBusiness::onTcpConnected(const TcpConnectionPtr& connection)
{
    httpServer_.onTcpConnected(connection);
}

//-----------------------------------------------------------------------------

void AppBusiness::onTcpDisconnected(const TcpConnectionPtr& connection)
{
    httpServer_.onTcpDisconnected(connection);
}

//-----------------------------------------------------------------------------

void AppBusiness::onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
    int packetSize, const Context& context)
{
    httpServer_.onTcpRecvComplete(connection, packetBuffer, packetSize, context);
}

//-----------------------------------------------------------------------------

void AppBusiness::onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context)
{
    httpServer_.onTcpSendComplete(connection, context);
}

//-----------------------------------------------------------------------------

void AppBusiness::onHttpSession(const HttpRequest& request, HttpResponse& response)
{
    string content = "this is a simple http server.";

    response.setStatusCode(200);
    response.getContentStream()->write(content.c_str(), content.length());
}
