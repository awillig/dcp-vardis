/*
 * LocalBroadcastProtocol.cc
 *
 *  Created on: Sep 5, 2020
 *      Author: awillig
 */



#include <omnetpp.h>
#include <lbp/LocalBroadcastProtocol.h>
#include <inet/common/ModuleAccess.h>
#include <inet/common/InitStages.h>
#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/common/ProtocolGroup.h>
#include <inet/linklayer/common/MacAddress.h>
#include <inet/networklayer/contract/IInterfaceTable.h>
#include <inet/networklayer/common/InterfaceTable.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include "inet/linklayer/common/MacAddressTag_m.h"


using namespace inet;

Define_Module (LocalBroadcastProtocol);

// =======================================================================
// =======================================================================

static const uint16_t   LBP_MAGICNO    = 0x497E;
static const uint16_t   LBP_VERSION    = 1;

const inet::Protocol protocolLBP("LocalBroadcastProtocol","Local Broadcast Protocol (LBP)");

simsignal_t sigBroadcastsSent = cComponent::registerSignal("lbpBroadcastsSent");
simsignal_t sigBroadcastsRcvd = cComponent::registerSignal("lbpBroadcastsRcvd");
simsignal_t sigBroadcastLengthsSent = cComponent::registerSignal("lbpBroadcastLengthsSent");
simsignal_t sigBroadcastLengthsRcvd = cComponent::registerSignal("lbpBroadcastLengthsRcvd");


// =======================================================================
// =======================================================================

void lbpDebugMsg (const std::string preamble, const std::string text)
{
  EV << "LBP [ " << preamble << " ]: " << text << std::endl;
  EV.flush();
}

void LocalBroadcastProtocol::debugMsg (const std::string text)
{
  std::ostringstream os;
  os << xsprintf("time=%.4lf", simTime().dbl())
     << " , id=" << ownIdentifier;
  lbpDebugMsg (os.str(), text);
}

void LocalBroadcastProtocol::enter (const std::string methodname)
{
  std::string et = "Entering ";
  debugMsg (et.append(methodname));
}

void LocalBroadcastProtocol::leave (const std::string methodname)
{
  std::string lt = "Leaving ";
  debugMsg (lt.append(methodname));
}


// =======================================================================
// =======================================================================

// -------------------------------------------------

NodeIdentifier LocalBroadcastProtocol::getOwnMacAddress()
{
    if (ownIdentifier != nullIdentifier)
    {
        return ownIdentifier;
    }

    readParameters();
    findModulePointers();

    // find the right interface to link the LBP to
    for (int i = 0; i<interfaces->getNumInterfaces(); i++)
    {
        NetworkInterface*  iface = interfaces->getInterface(i);
        assert(iface);
        std::string  s    = iface->str();
        std::string  sfst = s.substr(0, s.find(' '));

        if (sfst.compare(interfaceName) == 0)
        {
            ownIdentifier = iface->getMacAddress();
            wlanInterface = iface;
            return ownIdentifier;
        }
    }

    error("LocalBroadcastProtocol::initialize: interface not found");
    return nullIdentifier;
}

// -------------------------------------------------

void LocalBroadcastProtocol::initialize(int stage)
{
    enter("initialize");

    if (stage == INITSTAGE_LAST)
    {
        sequenceNumber = 0;

        lowerLayerIn    = findGate("lowerLayerIn");
        higherLayerIn   = findGate("higherLayerIn");
        higherLayerOut  = findGate("higherLayerOut");
        lowerLayerOut   = findGate("lowerLayerOut");

        registerProtocol(protocolLBP, nullptr, gate("lowerLayerOut"));
        registerService(protocolLBP, gate("lowerLayerIn"), nullptr);
        registerProtocol(protocolLBP, gate("lowerLayerOut"), nullptr);
        registerService(protocolLBP, nullptr, gate("lowerLayerIn"));
        ProtocolGroup::ethertype.addProtocol(0x8999, &protocolLBP);

        registerProtocol(protocolLBP, gate("higherLayerOut"), nullptr);
        registerService(protocolLBP, gate("higherLayerOut"), nullptr);
        registerProtocol(protocolLBP, nullptr, gate("higherLayerIn"));
        registerService(protocolLBP, nullptr, gate("higherLayerIn"));

        getOwnMacAddress ();
    }
}

// -------------------------------------------------

void LocalBroadcastProtocol::readParameters()
{
    enter("readParameters");
    localBroadcastHeaderLength  =  par("localBroadcastHeaderLength");
    interfaceName               =  par("interfaceName").stdstringValue();
}


// -------------------------------------------------

void LocalBroadcastProtocol::findModulePointers()
{
    enter("findModulePointers");
    cModule *host = getContainingNode (this);
    assert(host);
    interfaces    = check_and_cast<InterfaceTable *>(host->getSubmodule("interfaceTable"));
    assert(interfaces);
}


// -------------------------------------------------

void LocalBroadcastProtocol::handleMessage (cMessage* msg)
{
    enter("handleMessage");
    debugMsg (xsprintf("handleMessage: arrivedOn(lower) = %d, arrivedOn(higher) = %d, name of sender module = %s, name of arrival gate = %s",
                       msg->arrivedOn(lowerLayerIn),
                       msg->arrivedOn(higherLayerIn),
                       (msg->getSenderModule())->getName(),
                       (msg->getArrivalGate())->getName()));

    if (msg->arrivedOn(lowerLayerIn))
    {
        handleLowerMessage(msg);
        return;
    }

    if (msg->arrivedOn(higherLayerIn))
    {
        handleHigherMessage(msg);
        return;
    }

    error("LBP::handleMessage: cannot handle message");
}

// -------------------------------------------------

bool LocalBroadcastProtocol::headerWellFormed (const LocalBroadcastHeader* header)
{
    enter("headerWellFormed");
    assert(header);

    if (header->getMagicNo() != LBP_MAGICNO)
      {
        EV << "LBP::headerWellFormed: magicno is wrong, header = " << header << std::endl;
        return false;
      }

    if (header->getVersion() != LBP_VERSION)
      {
        EV << "LBP::headerWellFormed: wrong version number, header = " << header << std::endl;
        return false;
      }

    return true;
}

// -------------------------------------------------

void LocalBroadcastProtocol::handleLowerMessage (cMessage* msg)
{
    enter("handleLowerMessage");

    auto packet         = check_and_cast<Packet *>(msg);
    const auto& header  = packet->popAtFront<LocalBroadcastHeader>();

    if (headerWellFormed(&(*header)))
    {
        emit (sigBroadcastsRcvd, true);
        emit (sigBroadcastLengthsRcvd, packet->getByteLength());

        send(packet, higherLayerOut);
    }
    else
    {
        error("LBP::handleLowerMessage: malformed header");
    }
}

// -------------------------------------------------

void LocalBroadcastProtocol::handleHigherMessage (cMessage* msg)
{
    enter("handleHigherMessage");
    auto packet = check_and_cast<Packet*>(msg);
    sendPacket(packet);
}


// -------------------------------------------------

Ptr<LocalBroadcastHeader> LocalBroadcastProtocol::formatHeader ()
{
    enter("formatHeader");

    auto header = makeShared<LocalBroadcastHeader>();
    assert(header);
    header->setMagicNo(LBP_MAGICNO);
    header->setVersion(LBP_VERSION);
    header->setSenderId(ownIdentifier);
    header->setSeqno(sequenceNumber++);
    header->setChunkLength(B(localBroadcastHeaderLength));

    return header;
}


// -------------------------------------------------

void LocalBroadcastProtocol::sendPacket (Packet* packet)
{
    enter("sendPacket");
    assert(packet);

    EV << "LBP::SendPacket: length before is " << packet->getByteLength() << " bytes" << endl;

    Ptr<LocalBroadcastHeader> header  = formatHeader();
    assert(header);

    packet->insertAtFront(header);

    packet->removeTag<DispatchProtocolReq>();
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&protocolLBP);
    packet->addTagIfAbsent<InterfaceReq>()->setInterfaceId(wlanInterface->getInterfaceId());
    packet->addTagIfAbsent<MacAddressReq>()->setDestAddress(MacAddress::BROADCAST_ADDRESS);

    EV << "LBP::sendPacket: the composed packet is " << packet << endl;
    EV << "LBP::SendPacket: length after is " << packet->getByteLength() << " bytes" << endl;

    send(packet, lowerLayerOut);

    emit(sigBroadcastsSent, true);
    emit(sigBroadcastLengthsSent, packet->getByteLength());
}

// -------------------------------------------------

void LocalBroadcastProtocol::finish ()
{

}


