///////////////////////////////////////////////////////////////////////////////

#include "svr_mod_1.h"

///////////////////////////////////////////////////////////////////////////////
// class ServerModule_1

void ServerModule_1::assistorThreadExecute(AssistorThread& assistorThread, int assistorIndex)
{
    switch (assistorIndex)
    {
    case 0:
        {
            MyMessage msg;
            msg.init(SMMC_MY_MESSAGE, "hello world!");
            broadcastMessage(msg);
            break;
        }

    default:
        break;
    }
}
