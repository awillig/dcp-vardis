/*
 * PeriodicBeaconing.h
 *
 *  Created on: Sep 6, 2020
 *      Author: awillig
 */

#ifndef BEACONING_RENEWALBEACONING_RENEWALBEACONING_H_
#define BEACONING_RENEWALBEACONING_RENEWALBEACONING_H_

#include <omnetpp.h>
#include <inet/common/Ptr.h>
#include <inet/common/InitStages.h>
#include <inet/common/Protocol.h>
#include <inet/common/geometry/common/Coord.h>
#include <inet/mobility/contract/IMobility.h>
#include <inet/linklayer/common/MacAddress.h>
#include <inet/networklayer/common/InterfaceTable.h>
#include <lbp/LBPClientBase.h>
#include <beaconing/base/Beacon_m.h>
#include <beaconing/base/BeaconingBase.h>

using namespace omnetpp;


/**
 * @brief
 * Skeleton of a beaconing protocol
 *
 */


class RenewalBeaconing : public LBPClientBase {

public:

    RenewalBeaconing (void);
    virtual void initialize(int stage);
    virtual void finish();

protected:

    // configuration data
    double   initialWaitTime;
    double   positionSamplingPeriod;
    int      beaconLength;

    // pointers to important other (sub-)modules
    inet::IMobility   *mobility       = nullptr;

    // pointers to self-messages
    cMessage  *pMsgGenerate       = nullptr; // generate the next packet
    cMessage  *pMsgSamplePosition = nullptr; // sample mobility model for current position, speed, etc

    // state information
    Coord    currPosition;
    Coord    currVelocity;

    // gate index for beacon reports
    cGate   *reportingGate = nullptr;

    // other variables
    uint32_t        sequenceNumber;

protected:

    virtual void readParameters (void);
    virtual void findModulePointers (void);
    virtual void startSelfMessages(void);
    virtual void samplePosition (void);
    virtual inet::Ptr<Beacon> composeBeacon (void);
    virtual void sendBeacon (void);
    virtual void processReceivedBeacon(const Beacon& beacon);
    virtual bool beaconWellFormed(const Beacon& beacon);
    virtual void sendBeaconReport(const Beacon& beacon);

    virtual const inet::Protocol& getProtocol(void) const;
    virtual void handleSelfMessage (cMessage* msg);
    virtual void handleOtherMessage (cMessage* msg);
    virtual void handleReceivedBroadcast (Packet* packet);

    virtual ~RenewalBeaconing();
};





#endif /* BEACONING_RENEWALBEACONING_RENEWALBEACONING_H_ */
