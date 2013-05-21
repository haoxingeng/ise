///////////////////////////////////////////////////////////////////////////////

#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include "ise/main/ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class AppBusiness : public IseBusiness
{
public:
    AppBusiness() {}
    virtual ~AppBusiness() {}

    virtual void initialize();
    virtual void finalize();

    virtual void afterInit();
    virtual void onInitFailed(Exception& e);
    virtual void initIseOptions(IseOptions& options);

    virtual void onTcpConnected(const TcpConnectionPtr& connection);
    virtual void onTcpDisconnected(const TcpConnectionPtr& connection);
    virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
        int packetSize, const Context& context);
    virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context);

    void onHttpSession(const HttpRequest& request, HttpResponse& response);

private:
    HttpServer httpServer_;
};

///////////////////////////////////////////////////////////////////////////////

#endif // _HTTP_SERVER_H_
