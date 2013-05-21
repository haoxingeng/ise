///////////////////////////////////////////////////////////////////////////////

#include "udp_server.h"
#include "packet_defs.h"

//-----------------------------------------------------------------------------

IseBusiness* createIseBusinessObject()
{
    return new AppBusiness();
}

///////////////////////////////////////////////////////////////////////////////

void AppBusiness::initialize()
{
    // nothing
}

//-----------------------------------------------------------------------------

void AppBusiness::finalize()
{
    string msg = formatString("%s stoped.", getAppExeName(false).c_str());
    std::cout << msg << std::endl;
    logger().writeStr(msg);
}

//-----------------------------------------------------------------------------

void AppBusiness::afterInit()
{
    string msg = formatString("%s started.", getAppExeName(false).c_str());
    std::cout << std::endl << msg << std::endl;
    logger().writeStr(msg);
}

//-----------------------------------------------------------------------------

void AppBusiness::onInitFailed(Exception& e)
{
    string msg = formatString("fail to start %s.", getAppExeName(false).c_str());
    std::cout << std::endl << msg << std::endl;
    logger().writeStr(msg);
}

//-----------------------------------------------------------------------------

void AppBusiness::initIseOptions(IseOptions& options)
{
    options.setLogFileName(getAppSubPath("log") + changeFileExt(extractFileName(getAppExeName()), ".log"), true);
    options.setIsDaemon(true);
    options.setAllowMultiInstance(false);

    options.setServerType(ST_UDP);
    options.setUdpServerPort(8000);
}

//-----------------------------------------------------------------------------

void AppBusiness::onRecvedUdpPacket(UdpWorkerThread& workerThread, int groupIndex, UdpPacket& packet)
{
    void* packetBuffer = packet.getPacketBuffer();
    int packetSize = packet.getPacketSize();

    if (packetSize < (int)sizeof(UdpPacketHeader))
    {
        logger().writeStr("Invalid packet.");
        return;
    }

    UINT actionCode = ((UdpPacketHeader*)packetBuffer)->actionCode;

    switch (actionCode)
    {
    case AC_HELLO_WORLD:
        {
            HelloWorldPacket reqPacket;
            if (reqPacket.unpack(packetBuffer, packetSize))
            {
                logger().writeFmt("Received packet from <%s>: %s.",
                    packet.getPeerAddr().getDisplayStr().c_str(),
                    reqPacket.message.c_str());

                AckPacket ackPacket;
                ackPacket.initPacket(reqPacket.header.seqNumber, 12345);
                iseApp().getUdpServer().sendBuffer(ackPacket.getBuffer(), ackPacket.getSize(), packet.getPeerAddr());
            }
            break;
        }

    default:
        {
            logger().writeStr("Unknown packet.");
            break;
        }
    }
}
