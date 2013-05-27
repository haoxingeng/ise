///////////////////////////////////////////////////////////////////////////////

#ifndef _HELLO_WORLD_H_
#define _HELLO_WORLD_H_

#include "ise/main/ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class AppBusiness : public IseBusiness
{
public:
    virtual void initIseOptions(IseOptions& options);
    virtual void onTcpConnected(const TcpConnectionPtr& connection);
    virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _HELLO_WORLD_H_
