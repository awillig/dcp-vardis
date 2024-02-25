/*
 * LocalBroadcastProtocol.h
 *
 *  Created on: Sep 5, 2020
 *      Author: awillig
 */

#ifndef BASE_LOCALBROADCASTPROTOCOL_H_
#define BASE_LOCALBROADCASTPROTOCOL_H_


#include <omnetpp.h>
#include <inet/common/Ptr.h>
#include <inet/common/Protocol.h>
#include <inet/common/InitStages.h>
#include <inet/common/packet/Packet.h>
#include <inet/linklayer/common/MacAddress.h>
#include <inet/networklayer/common/InterfaceTable.h>
#include <lbp/LocalBroadcastHeader_m.h>
#include <base/SwarmStackBase.h>


using namespace omnetpp;
using namespace inet;

// -------------------------------------------

class LocalBroadcastProtocol : public cSimpleModule {

public:
    virtual void initialize (int stage);
    virtual void finish ();
    virtual void handleMessage (cMessage* msg);
    virtual int  numInitStages () const override { return inet::NUM_INIT_STAGES; };

public:
    NodeIdentifier getOwnMacAddress (void);

protected:

    // module parameters
    int           localBroadcastHeaderLength;
    std::string   interfaceName;

    // pointers to important other (sub-)modules
    inet::InterfaceTable   *interfaces     = nullptr;
    inet::NetworkInterface *wlanInterface  = nullptr;

    // state information
    uint32_t        sequenceNumber = 0;
    NodeIdentifier  ownIdentifier  = nullIdentifier;

    // Gate indices and pointers
    int     lowerLayerOut  = -1;
    int     higherLayerOut = -1;
    int     lowerLayerIn   = -1;
    int     higherLayerIn  = -1;


protected:

    // debug helpers
    void    enter (const std::string text);
    void    leave (const std::string text);
    void    debugMsg (const std::string text);


    void                        readParameters();
    void                        findModulePointers();
    Ptr<LocalBroadcastHeader>   formatHeader ();
    void                        sendPacket (Packet* packet);
    bool                        headerWellFormed (const LocalBroadcastHeader* header);

    void    handleSelfMessage (cMessage* msg);
    void    handleLowerMessage (cMessage* msg);
    void    handleHigherMessage (cMessage* msg);
};



#endif /* BASE_LOCALBROADCASTPROTOCOL_H_ */
