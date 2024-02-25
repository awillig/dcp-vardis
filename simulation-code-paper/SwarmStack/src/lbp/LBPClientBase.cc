/*
 * LBPClientBase.cc
 *
 *  Created on: Sep 6, 2020
 *      Author: awillig
 */

#include <omnetpp.h>
#include <lbp/LBPClientBase.h>
#include <lbp/LocalBroadcastProtocol.h>
#include <base/SwarmStackBase.h>
#include <inet/common/ModuleAccess.h>
#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/networklayer/common/InterfaceTable.h>
#include <inet/common/ProtocolGroup.h>
#include <cassert>
#include <sstream>

using namespace inet;

// =======================================================================
// =======================================================================

void LBPClientBase::debugMsg (const std::string text)
{
  std::ostringstream os;
  os << xsprintf("time=%.4lf", simTime().dbl())
     << " , id=" << ownIdentifier;

  EV << debugMsgPrefix << " [ " << os.str() << " ]: " << text << std::endl;
  EV.flush();
}

void LBPClientBase::enter (const std::string methodname)
{
  std::string et = "Entering ";
  debugMsg (et.append(methodname));
}

void LBPClientBase::leave (const std::string methodname)
{
  std::string lt = "Leaving ";
  debugMsg (lt.append(methodname));
}

// -------------------------------------------

void LBPClientBase::initialize (int stage)
{
  enter("LBPClientBase::initialize");
  debugMsg(xsprintf("LBPClientBase::initialize: stage is %d", stage));

  if (stage == INITSTAGE_LAST)
    {
      gidFromLBP  = findGate("fromLBP");
      gidToLBP    = findGate("toLBP");

      registerProtocol(getProtocol(), gate("toLBP"), nullptr);
      registerService(getProtocol(), nullptr, gate("fromLBP"));

      pProtocolLBP = (inet::Protocol*) Protocol::findProtocol("LocalBroadcastProtocol");
      assert(pProtocolLBP);

      // lots of INET voodoo to find the own MAC address
      cModule *host = getContainingNode (this);
      assert(host);

      // test code
      LocalBroadcastProtocol *lbp    = check_and_cast<LocalBroadcastProtocol*>(host->getSubmodule("lbp"));
      assert(lbp);
      ownIdentifier  =  lbp->getOwnMacAddress();
    }

  leave("LBPClientBase::initialize");
}

// -------------------------------------------

void LBPClientBase::handleMessage (cMessage *msg)
{
  enter("LBPClientBase::handleMessage");

  if (msg->isSelfMessage ())
    {
      debugMsg("LBPClientBase::handleMessage: message is self-message");
      handleSelfMessage(msg);
      return;
    }

  if (msg->arrivedOn(gidFromLBP))
    {
      debugMsg("LBPClientBase::handleMessage: message came from LBP");
      auto packet = check_and_cast<Packet *>(msg);
      handleReceivedBroadcast(packet);
      return;
    }

  handleOtherMessage(msg);
}

// -------------------------------------------

void LBPClientBase::sendViaLBP (Packet* packet)
{
  enter("LBPClientBase::sendViaLBP");

  assert(packet);

  packet->removeTagIfPresent<DispatchProtocolReq>();
  auto req = packet->addTag<DispatchProtocolReq>();
  req->setProtocol(pProtocolLBP);

  send(packet, gidToLBP);

  leave("LBPClientBase::sendViaLBP");
}
