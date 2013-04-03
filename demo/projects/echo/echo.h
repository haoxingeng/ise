///////////////////////////////////////////////////////////////////////////////

#ifndef _ECHO_H_
#define _ECHO_H_

#include "sse.h"

using namespace sse;

///////////////////////////////////////////////////////////////////////////////

class CAppBusiness : public CSseBusiness 
{
public:
    CAppBusiness() {}
    virtual ~CAppBusiness() {}

	virtual void Initialize();
	virtual void Finalize();

	virtual void DoStartupState(STARTUP_STATE nState);
    virtual void InitSseOptions(CSseOptions& SseOpt);

	virtual void OnTcpConnection(CTcpConnection *pConnection);
	virtual void OnTcpError(CTcpConnection *pConnection);
	virtual void OnTcpRecvComplete(CTcpConnection *pConnection, void *pPacketBuffer,
		int nPacketSize, const CCustomParams& Params);
	virtual void OnTcpSendComplete(CTcpConnection *pConnection, const CCustomParams& Params);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _ECHO_H_ 
