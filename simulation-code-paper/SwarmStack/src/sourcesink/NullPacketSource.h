/*
 * NullPacketSource.h
 *
 *  Created on: Oct 2, 2020
 *      Author: awillig
 */

#ifndef SOURCESINK_NULLPACKETSOURCE_H_
#define SOURCESINK_NULLPACKETSOURCE_H_

#include <omnetpp.h>
using namespace omnetpp;

class NullPacketSource : public cSimpleModule {
public:
    void handleMessage(cMessage* msg);
};



#endif /* SOURCESINK_NULLPACKETSOURCE_H_ */
