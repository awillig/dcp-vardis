/*
 * GenericPacketSink.h
 *
 *  Created on: Oct 2, 2020
 *      Author: awillig
 */

#ifndef SOURCESINK_GENERICPACKETSINK_H_
#define SOURCESINK_GENERICPACKETSINK_H_


#include <omnetpp.h>
using namespace omnetpp;

class GenericPacketSink : public cSimpleModule {
public:
    void initialize();
    void handleMessage(cMessage* msg);
private:
    int gidFromLower;
};


#endif /* SOURCESINK_GENERICPACKETSINK_H_ */
