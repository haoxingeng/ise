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

	virtual void onTcpConnection(TcpConnection *connection);
	virtual void onTcpError(TcpConnection *connection);
	virtual void onTcpRecvComplete(TcpConnection *connection, void *packetBuffer,
		int packetSize, const CustomParams& params);
	virtual void onTcpSendComplete(TcpConnection *connection, const CustomParams& params);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _ECHO_H_ 
