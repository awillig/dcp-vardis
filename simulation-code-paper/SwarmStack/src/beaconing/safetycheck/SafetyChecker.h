/*
 * SafetyChecker.h
 *
 *  Created on: Oct 26, 2020
 *      Author: awillig
 */

#ifndef BEACONING_SAFETYCHECK_SAFETYCHECKER_H_
#define BEACONING_SAFETYCHECK_SAFETYCHECKER_H_


#include <omnetpp.h>
#include <inet/common/InitStages.h>
#include <inet/common/geometry/common/Coord.h>
#include <inet/mobility/contract/IMobility.h>
#include <beaconing/base/BeaconReport_m.h>
#include <base/SwarmStackBase.h>
#include <map>

using namespace omnetpp;


class SafetyChecker : public cSimpleModule
{
public:
    void initialize(int stage);
    void handleMessage(cMessage* msg);
    virtual int  numInitStages () const override { return inet::NUM_INIT_STAGES; };
    virtual ~SafetyChecker();

protected:

    double                                safetyRadius;
    double                                safetyDeadline;
    int                                   gidBeaconsIn;
    double                                positionSamplingPeriod;
    inet::IMobility                      *mobility            = nullptr;
    cMessage*                             pMsgSamplePosition  = nullptr;
    cMessage*                             pMsgCheckNeighbors  = nullptr;
    inet::Coord                           currPosition;
    NodeIdentifier                        ownIdentifier;
    std::map<NodeIdentifier, simtime_t>   safetyNeighbors;

    void   processBeaconReport(BeaconReport* report);
    void   samplePosition();
    void   checkNeighbors();

};


#endif /* BEACONING_SAFETYCHECK_SAFETYCHECKER_H_ */
