///////////////////////////////////////////////////////////////////////////////

#ifndef _PACKET_DEFS_H_
#define _PACKET_DEFS_H_

#include "ise/main/ise.h"

using namespace ise;

///////////////////////////////////////////////////////////////////////////////
// Action Codes:

const UINT AC_HELLO_WORLD = 100;
const UINT AC_ACK         = 200;

///////////////////////////////////////////////////////////////////////////////
// UDP Packet Header

#pragma pack(1)

class UdpPacketHeader
{
public:
    UINT actionCode;       // action code
    UINT seqNumber;        // auto increment
public:
    void init(UINT actionCode, UINT seqNumber = 0)
    {
        static SeqNumberAlloc seqNumAlloc;
        this->actionCode = actionCode;
        this->seqNumber = (seqNumber == 0 ? seqNumAlloc.allocId() : seqNumber);
    }
};

#pragma pack()

///////////////////////////////////////////////////////////////////////////////
// class BaseUdpPacket

class BaseUdpPacket : public Packet
{
public:
    UdpPacketHeader header;
protected:
    virtual void doPack() { writeBuffer(&header, sizeof(header)); }
    virtual void doUnpack() { readBuffer(&header, sizeof(header)); }
};

///////////////////////////////////////////////////////////////////////////////
// HelloWorldPacket

class HelloWorldPacket : public BaseUdpPacket
{
public:
    string message;
protected:
    virtual void doPack()
    {
        BaseUdpPacket::doPack();
        writeString(message);
    }

    virtual void doUnpack()
    {
        BaseUdpPacket::doUnpack();
        readString(message);
    }

public:
    void initPacket(const string& message)
    {
        header.init(AC_HELLO_WORLD);
        this->message = message;
        pack();
    }
};

///////////////////////////////////////////////////////////////////////////////
// AckPacket

class AckPacket : public BaseUdpPacket
{
public:
    INT32 ackCode;
protected:
    virtual void doPack()
    {
        BaseUdpPacket::doPack();
        writeINT32(ackCode);
    }

    virtual void doUnpack()
    {
        BaseUdpPacket::doUnpack();
        ackCode = readINT32();
    }

public:
    void initPacket(UINT seqNumber, INT32 ackCode)
    {
        header.init(AC_ACK, seqNumber);
        this->ackCode = ackCode;
        pack();
    }
};

///////////////////////////////////////////////////////////////////////////////

#endif // _PACKET_DEFS_H_
