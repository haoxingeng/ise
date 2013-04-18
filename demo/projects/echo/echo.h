///////////////////////////////////////////////////////////////////////////////

#ifndef _ECHO_H_
#define _ECHO_H_

#include "ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class AppBusiness : public IseBusiness
{
public:
    AppBusiness() {}
    virtual ~AppBusiness() {}

    virtual void initialize();
    virtual void finalize();

    virtual void doStartupState(STARTUP_STATE state);
    virtual void initIseOptions(IseOptions& options);

    virtual void onTcpConnect(const TcpConnectionPtr& connection);
    virtual void onTcpDisconnect(const TcpConnectionPtr& connection);
    virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
        int packetSize, const Context& context);
    virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _ECHO_H_
