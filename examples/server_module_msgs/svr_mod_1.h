///////////////////////////////////////////////////////////////////////////////

#ifndef _SVR_MOD_1_H_
#define _SVR_MOD_1_H_

#include "ise/main/ise.h"
#include "svr_mod_msgs.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////

class ServerModule_1: public IseServerModule
{
public:
    virtual int getAssistorThreadCount() { return 1; }
    virtual void assistorThreadExecute(AssistorThread& assistorThread, int assistorIndex);
};

///////////////////////////////////////////////////////////////////////////////

#endif // _SVR_MOD_1_H_
