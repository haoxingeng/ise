///////////////////////////////////////////////////////////////////////////////

#ifndef _CHARGEN_CLIENT_H_
#define _CHARGEN_CLIENT_H_

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

    virtual bool parseArguments(int argc, char *argv[]);
    virtual string getAppHelp();
    virtual void doStartupState(STARTUP_STATE state);
    virtual void initIseOptions(IseOptions& options);

    virtual void onTcpConnected(const TcpConnectionPtr& connection);
    virtual void onTcpDisconnected(const TcpConnectionPtr& connection);
    virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
        int packetSize, const Context& context);

private:
    void onConnectComplete(bool success, TcpConnection *connection,
        const InetAddress& peerAddr, const Context& context);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _CHARGEN_CLIENT_H_
