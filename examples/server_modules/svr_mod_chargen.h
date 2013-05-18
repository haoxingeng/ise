///////////////////////////////////////////////////////////////////////////////

#ifndef _SVR_MOD_CHARGEN_H_
#define _SVR_MOD_CHARGEN_H_

#include "ise/main/ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class ServerModule_Chargen: public IseServerModule
{
public:
    ServerModule_Chargen();
    virtual ~ServerModule_Chargen() {}

    virtual int getTcpServerCount() { return 1; }
    virtual void getTcpServerOptions(int serverIndex, TcpServerOptions& options) { options.serverPort = 10003; }

    virtual void onTcpConnected(const TcpConnectionPtr& connection);
    virtual void onTcpDisconnected(const TcpConnectionPtr& connection);
    virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
        int packetSize, const Context& context);
    virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context);

private:
    void initMessage();
private:
    std::string message_;
    UINT64 transferredBytes_;
};

///////////////////////////////////////////////////////////////////////////////

#endif // _SVR_MOD_CHARGEN_H_
