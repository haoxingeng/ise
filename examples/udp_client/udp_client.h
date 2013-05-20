///////////////////////////////////////////////////////////////////////////////

#ifndef _UDP_CLIENT_H_
#define _UDP_CLIENT_H_

#include "ise/main/ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class AppBusiness : public IseBusiness
{
public:
    AppBusiness() {}
    virtual ~AppBusiness() {}

    virtual void initialize();
    virtual void initIseOptions(IseOptions& options);
    virtual void assistorThreadExecute(AssistorThread& assistorThread, int assistorIndex);

private:
    InetAddress serverAddr_;
    boost::scoped_ptr<BaseUdpClient> udpClient_;
};

///////////////////////////////////////////////////////////////////////////////

#endif // _UDP_CLIENT_H_
