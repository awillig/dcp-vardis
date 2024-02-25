/*
 * GlenorchyPacketSink.h
 *
 *  Created on: Oct 8, 2020
 *      Author: awillig
 */

#ifndef GLENORCHY_GLENORCHYPACKETSINK_H_
#define GLENORCHY_GLENORCHYPACKETSINK_H_

#include <omnetpp.h>
using namespace omnetpp;

class GlenorchyPacketSink : public cSimpleModule {
public:
    void initialize();
    void handleMessage(cMessage* msg);
private:
    int gidFromLower;
};




#endif /* GLENORCHY_GLENORCHYPACKETSINK_H_ */
