///////////////////////////////////////////////////////////////////////////////

#ifndef _SVR_MOD_DISCARD_H_
#define _SVR_MOD_DISCARD_H_

#include "ise/main/ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class ServerModule_Discard: public IseServerModule
{
public:
    ServerModule_Discard() {}
    virtual ~ServerModule_Discard() {}

    virtual int getTcpServerCount() { return 1; }
    virtual void getTcpServerOptions(int serverIndex, TcpServerOptions& options) { options.serverPort = 10001; }

    virtual void onTcpConnected(const TcpConnectionPtr& connection);
    virtual void onTcpDisconnected(const TcpConnectionPtr& connection);
    virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
        int packetSize, const Context& context);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _SVR_MOD_DISCARD_H_
