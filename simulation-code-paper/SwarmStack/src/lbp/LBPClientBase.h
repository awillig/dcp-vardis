/*
 * LBPClientBase.h
 *
 *  Created on: Sep 6, 2020
 *      Author: awillig
 */

#ifndef LBP_LBPCLIENTBASE_H_
#define LBP_LBPCLIENTBASE_H_


#include <omnetpp.h>
#include <inet/common/InitStages.h>
#include <inet/common/Protocol.h>
#include <inet/common/packet/Packet.h>
#include <base/SwarmStackBase.h>

using namespace omnetpp;
using namespace inet;

/**
 * @brief
 * Skeleton of a LBP client
 *
 */



class LBPClientBase : public cSimpleModule {

public:

  virtual void initialize(int stage);
  virtual void handleMessage (cMessage* msg);
  virtual int  numInitStages () const override { return inet::NUM_INIT_STAGES; };


protected:

  inet::Protocol *pProtocolLBP   = nullptr;
  int             gidToLBP       = -1;
  int             gidFromLBP     = -1;
  NodeIdentifier  ownIdentifier  = nullIdentifier;
  std::string     debugMsgPrefix = "LBPClient";

protected:

  // debugging functions
  virtual void debugMsg (const std::string text);
  virtual void enter (const std::string methodname);
  virtual void leave (const std::string methodname);

  // this returns a protocol object for the client protocol, to be
  // registered with the message dispatcher. getProtocol will be
  // called from initialize() during the last stage (INITSTAGE_LAST)
  virtual const inet::Protocol& getProtocol (void) const = 0;

  virtual void sendViaLBP (Packet* packet);

  virtual void handleSelfMessage(cMessage* msg) = 0;
  virtual void handleOtherMessage(cMessage* msg) = 0;

  // this where a derived class deals with its own packets it receives,
  // it is called from handleLowerMessage
  virtual void handleReceivedBroadcast(Packet* packet) = 0;
};





#endif /* LBP_LBPCLIENTBASE_H_ */
