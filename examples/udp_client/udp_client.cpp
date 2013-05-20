///////////////////////////////////////////////////////////////////////////////

#include "udp_client.h"
#include "../udp_server/packet_defs.h"

//-----------------------------------------------------------------------------

IseBusiness* createIseBusinessObject()
{
    return new AppBusiness();
}

///////////////////////////////////////////////////////////////////////////////

void AppBusiness::initialize()
{
    serverAddr_ = InetAddress("127.0.0.1", 8000);
    udpClient_.reset(new BaseUdpClient());
}

//-----------------------------------------------------------------------------

void AppBusiness::initIseOptions(IseOptions& options)
{
    options.setLogFileName(getAppSubPath("log") + changeFileExt(extractFileName(getAppExeName()), ".log"), true);
    options.setIsDaemon(false);
    options.setAllowMultiInstance(true);
    options.setAssistorThreadCount(1);
}

//-----------------------------------------------------------------------------

void AppBusiness::assistorThreadExecute(AssistorThread& assistorThread, int assistorIndex)
{
    switch (assistorIndex)
    {
    case 0:
        {
            // send request packet
            HelloWorldPacket reqPacket;
            reqPacket.initPacket("Hello World!");
            udpClient_->sendBuffer(reqPacket.getBuffer(), reqPacket.getSize(), serverAddr_);

            // receive ack packet
            Buffer recvBuf(1024);
            int recvBytes = udpClient_->recvBuffer(recvBuf.data(), recvBuf.getSize());
            if (recvBytes > 0)
            {
                AckPacket ackPacket;
                if (ackPacket.unpack(recvBuf.data(), recvBytes))
                {
                    string log = formatString("Received ack packet. (ackCode: %d)", ackPacket.ackCode);
                    logger().writeStr(log);
                    std::cout << log << std::endl;
                }
            }

            iseApp().setTerminated(true);
            break;
        }

    default:
        break;
    }
}
