///////////////////////////////////////////////////////////////////////////////

#ifndef _SVR_MOD_ECHO_H_
#define _SVR_MOD_ECHO_H_

#include "ise/main/ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class ServerModule_Echo: public IseServerModule
{
public:
    ServerModule_Echo() {}
    virtual ~ServerModule_Echo() {}

    virtual int getTcpServerCount() { return 1; }
    virtual void getTcpServerOptions(int serverIndex, TcpServerOptions& options) { options.serverPort = 10000; }

    virtual void onTcpConnected(const TcpConnectionPtr& connection);
    virtual void onTcpDisconnected(const TcpConnectionPtr& connection);
    virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
        int packetSize, const Context& context);
    virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _SVR_MOD_ECHO_H_
