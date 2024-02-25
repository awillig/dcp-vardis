/*
 * NullPacketSource.cc
 *
 *  Created on: Oct 2, 2020
 *      Author: awillig
 */


#include <sourcesink/NullPacketSource.h>

Define_Module(NullPacketSource);

void NullPacketSource::handleMessage(cMessage *msg)
{
    delete msg;
}

