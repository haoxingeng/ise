///////////////////////////////////////////////////////////////////////////////

#ifndef _DAYTIME_SERVER_H_
#define _DAYTIME_SERVER_H_

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

    virtual void onTcpConnected(const TcpConnectionPtr& connection);
    virtual void onTcpDisconnected(const TcpConnectionPtr& connection);
    virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _DAYTIME_SERVER_H_
