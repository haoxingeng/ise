///////////////////////////////////////////////////////////////////////////////

#include "svr_mod_2.h"

///////////////////////////////////////////////////////////////////////////////
// class ServerModule_2

void ServerModule_2::onSvrModMessage(BaseSvrModMessage& message)
{
    switch (message.messageCode)
    {
    case SMMC_MY_MESSAGE:
        {
            MyMessage *msg = static_cast<MyMessage*>(&message);
            msg->isHandled = true;
            logger().writeFmt("recved server module message: %s", msg->msg.c_str());
            break;
        }

    default:
        break;
    }
}
