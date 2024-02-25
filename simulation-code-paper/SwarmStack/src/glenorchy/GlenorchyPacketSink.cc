/*
 * GlenorchyPacketSink.cc
 *
 *  Created on: Oct 8, 2020
 *      Author: awillig
 */


#include <glenorchy/GlenorchyPacketSink.h>
#include <inet/common/packet/Packet.h>

Define_Module(GlenorchyPacketSink);


simsignal_t  delaySignalId  =  cComponent::registerSignal("glenorchysinkdelay");
simsignal_t  countSignalId  =  cComponent::registerSignal("glenorchysinkcount");

using namespace omnetpp;
using namespace inet;

void GlenorchyPacketSink::initialize()
{
    gidFromLower = findGate("fromLower");
}


void GlenorchyPacketSink::handleMessage(cMessage* msg)
{
    if (dynamic_cast<Packet*>(msg) && msg->arrivedOn(gidFromLower))
    {
        Packet*    packet = (Packet*) msg;
        simtime_t  delay  = simTime() - (packet->getCreationTime());

        emit(delaySignalId, delay);
        emit(countSignalId, true);

        EV << "GlenorchyPacketSink: Received packet at time " << simTime()
           << ", of length " << packet->getByteLength()
           << ", of name " << packet->getName()
           << ", with a delay since creation of " << delay
           << endl;

        delete msg;
    }
    else
    {
        error("GlenorchyPacketSink::handleMessage: arriving message is not a Packet or did not arrive at the right gate");
    }
}


