/*
 * RenewalPacketSource.cc
 *
 *  Created on: Oct 2, 2020
 *      Author: awillig
 */


#include <sourcesink/RenewalPacketSource.h>
#include <inet/common/packet/Packet.h>
#include <inet/common/packet/chunk/ByteCountChunk.h>
#include <sstream>

Define_Module(RenewalPacketSource);

using namespace inet;


void RenewalPacketSource::initialize()
{
   ovhdSize       = par("overheadSize");
   gidToLower     = findGate("toLower");
   wakeup         = new cMessage ("RenewalPacketSource::wakeup");
   sequenceNumber = 0;

   scheduleAt(simTime() + par("interArrivalTime").doubleValue(), wakeup);
}


void RenewalPacketSource::handleMessage(cMessage* msg)
{
    if (msg == wakeup)
    {
        int64_t  dataSize  = par("packetSize");
        int64_t  totalSize = dataSize + ovhdSize;

        std::stringstream ss;
        ss << "RenewalPacketSource-seqno=" << sequenceNumber << "-tsize=" << totalSize;

        auto data    = makeShared<ByteCountChunk>(B(totalSize));
        auto packet  = new Packet(ss.str().c_str(), data);

        sequenceNumber++;

        EV << "RenewalPacketSource::handleMessage: generating packet with name " << (packet->getName())<< endl;

        scheduleAt(simTime() + par("interArrivalTime").doubleValue(), wakeup);
        send(packet,gidToLower);
    }
    else
    {
        error("RenewalPacketSource::handleMessage: received unforeseen message");
    }
}


RenewalPacketSource::~RenewalPacketSource()
{
    cancelAndDelete(wakeup);
}


