///////////////////////////////////////////////////////////////////////////////

#ifndef _SVR_MOD_DAYTIME_H_
#define _SVR_MOD_DAYTIME_H_

#include "ise/main/ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class ServerModule_Daytime: public IseServerModule
{
public:
    ServerModule_Daytime() {}
    virtual ~ServerModule_Daytime() {}

    virtual int getTcpServerCount() { return 1; }
    virtual void getTcpServerOptions(int serverIndex, TcpServerOptions& options) { options.port = 10002; }

    virtual void onTcpConnected(const TcpConnectionPtr& connection);
    virtual void onTcpDisconnected(const TcpConnectionPtr& connection);
    virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _SVR_MOD_DAYTIME_H_
