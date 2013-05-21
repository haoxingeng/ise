///////////////////////////////////////////////////////////////////////////////

#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_

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

    virtual void onRecvedUdpPacket(UdpWorkerThread& workerThread, int groupIndex, UdpPacket& packet);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _UDP_SERVER_H_
