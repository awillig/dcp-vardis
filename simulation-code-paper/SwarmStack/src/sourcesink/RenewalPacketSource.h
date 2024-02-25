/*
 * RenewalPacketSource.h
 *
 *  Created on: Oct 2, 2020
 *      Author: awillig
 */

#ifndef SOURCESINK_RENEWALPACKETSOURCE_H_
#define SOURCESINK_RENEWALPACKETSOURCE_H_


#include <omnetpp.h>
using namespace omnetpp;

class RenewalPacketSource : public cSimpleModule {
public:
    void initialize();
    void handleMessage(cMessage* msg);
    ~RenewalPacketSource();
private:
    int64_t         ovhdSize;
    cMessage*       wakeup;
    int             gidToLower;
    int             sequenceNumber;
};


#endif /* SOURCESINK_RENEWALPACKETSOURCE_H_ */
