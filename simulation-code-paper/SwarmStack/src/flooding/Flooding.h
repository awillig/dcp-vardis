/*
 * Flooding.h
 *
 *  Created on: Oct 1, 2020
 *      Author: awillig
 */

#ifndef FLOODING_FLOODING_H_
#define FLOODING_FLOODING_H_

#include <omnetpp.h>
#include <lbp/LBPClientBase.h>
#include <inet/common/packet/Packet.h>
#include <inet/common/Protocol.h>
#include <flooding/FloodingHeader_m.h>
#include <map>


using namespace omnetpp;
using namespace inet;


class Flooding : public LBPClientBase {

public:

    Flooding (void);
    virtual void initialize (int stage);

protected:

    // module parameters
    int         numRepetitions;
    int         floodingHeaderSize;

    // internal state variables
    cQueue      packetQueue;
    int         repetitionsToGo;
    uint32_t    sequenceNumber = 0;
    Packet*     currentPacket = nullptr;
    std::map<MacAddress,uint32_t> lastSeen;
    cMessage*   nextRepetition;

    int         gidFromHigher;
    int         gidToHigher;

protected:

    // helper methods
    virtual void readParameters (void);
    virtual bool headerWellFormed (const FloodingHeader* header);
    virtual inet::Ptr<FloodingHeader> composeHeader (void);
    virtual void workOnPacketQueue(void);

    // methods that any LBPClient has to implement
    virtual const inet::Protocol& getProtocol (void) const;
    virtual void handleSelfMessage(cMessage* msg);
    virtual void handleOtherMessage(cMessage* msg);
    virtual void handleReceivedBroadcast(Packet* packet);

    virtual ~Flooding();
};





#endif /* FLOODING_FLOODING_H_ */
