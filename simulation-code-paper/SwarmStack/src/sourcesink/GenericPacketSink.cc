/*
 * GenericPacketSink.cc
 *
 *  Created on: Oct 2, 2020
 *      Author: awillig
 */


#include <sourcesink/GenericPacketSink.h>
#include <inet/common/packet/Packet.h>

Define_Module(GenericPacketSink);

using namespace omnetpp;
using namespace inet;

void GenericPacketSink::initialize()
{
    gidFromLower = findGate("fromLower");
}


void GenericPacketSink::handleMessage(cMessage* msg)
{
    if (dynamic_cast<Packet*>(msg) && msg->arrivedOn(gidFromLower))
    {
        Packet* packet = (Packet*) msg;

        EV << "GenericPacketSink: Received packet at time " << simTime()
           << ", of length " << packet->getByteLength()
           << ", and name " << packet->getName()
           << endl;

        delete msg;
    }
    else
    {
        error("GenericPacketSink::handleMessage: arriving message is not a Packet or did not arrive at the right gate");
    }
}
