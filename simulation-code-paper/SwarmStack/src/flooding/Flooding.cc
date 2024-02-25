/*
 * Flooding.cc
 *
 *  Created on: Oct 1, 2020
 *      Author: awillig
 */


#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/common/ModuleAccess.h>
#include <flooding/FloodingHeader_m.h>
#include <flooding/Flooding.h>
#include <base/SwarmStackBase.h>
#include <cassert>
#include <sstream>

/*
 * TODO:
 */

// =======================================================================

Define_Module(Flooding);

const inet::Protocol floodingProtocol("flooding","Flooding protocol based on LBP");

const uint16_t  FLOODING_MAGICNO  =  0x4711;
const uint16_t  FLOODING_VERSION  =  0x0001;
const uint16_t  FLOODING_TTL_INIT =  0xFFFF;



// =======================================================================
// =======================================================================

using namespace inet;



// -------------------------------------------

std::string fhToString (const FloodingHeader& fheader)
{
    std::stringstream ss;
    ss << "FloodingHeader[magicNo=" << fheader.getMagicNo()
       << ", floodingVersion=" << fheader.getFloodingVersion()
       << ", timeToLive=" << fheader.getTimeToLive()
       << ", seqno=" << fheader.getSeqno()
       << ", sourceId=" << fheader.getSourceId()
       << "]";
    return ss.str();
}


// =======================================================================
// =======================================================================

Flooding::Flooding ()
    : packetQueue("flooding packet queue")
{
    debugMsgPrefix = "Flooding";
    sequenceNumber = 0;
    currentPacket  = nullptr;
}

// -------------------------------------------

const inet::Protocol& Flooding::getProtocol(void) const
{
    return floodingProtocol;
}

// -------------------------------------------

void Flooding::initialize(int stage)
{
    enter("initialize");

    LBPClientBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        readParameters();
        gidFromHigher  =  findGate("fromHigher");
        gidToHigher    =  findGate("toHigher");
        nextRepetition =  new cMessage("nextRepetition");
    }

    if (stage == INITSTAGE_LAST)
    {
        registerProtocol(floodingProtocol, gate("toLBP"), nullptr);
        registerService(floodingProtocol, nullptr, gate("fromLBP"));

    }
    leave("initialize");
}

// -------------------------------------------

void Flooding::readParameters ()
{
    enter("readParameters");
    numRepetitions      =   par("numRepetitions");
    floodingHeaderSize  =   par("floodingHeaderSize");
    assert(numRepetitions>0);
    assert(floodingHeaderSize>0);
    leave("readParameters");
}

// -------------------------------------------

bool Flooding::headerWellFormed (const FloodingHeader* header)
{
    enter("headerWellFormed");
    assert(header);
    return    (header->getMagicNo() == FLOODING_MAGICNO)
           && (header->getFloodingVersion() == FLOODING_VERSION);
}

// -------------------------------------------

inet::Ptr<FloodingHeader> Flooding::composeHeader (void)
{
    enter ("composeHeader");
    auto fheader = makeShared<FloodingHeader>();
    assert(fheader);
    fheader->setMagicNo(FLOODING_MAGICNO);
    fheader->setFloodingVersion(FLOODING_VERSION);
    fheader->setTimeToLive(FLOODING_TTL_INIT);
    fheader->setSeqno(sequenceNumber);
    fheader->setSourceId(ownIdentifier);
    fheader->setChunkLength(B(floodingHeaderSize));
    fheader->setGenerationTime(simTime());

    sequenceNumber++;

    leave("composeHeader");

    return fheader;
}

// -------------------------------------------

void Flooding::handleOtherMessage(cMessage* msg)
{
    enter("handleOtherMessage");
    if (dynamic_cast<Packet *>(msg)  &&  (msg->arrivedOn(gidFromHigher)))
    {
        // got a new packet to flood from higher layers, i.e. I am the source
        // put the flooding header on it and put it in the queue

        Packet*  packet   = (Packet*) msg;
        auto     fheader  = composeHeader();
        assert(fheader);
        packet->insertAtFront(fheader);

        packetQueue.insert(packet);
        workOnPacketQueue();

        leave("handleOtherMessage");

        return;
    }

    error("Flooding::handleOtherMessage: unknown message type");
}


// -------------------------------------------

void Flooding::handleReceivedBroadcast(Packet* packet)
{
    enter("handleReceivedBroadcast");
    assert(packet);

    // first header must be a flooding header
    const auto& fheader = packet->popAtFront<FloodingHeader>();
    if (!fheader)
    {
        error("Flooding::handleReceivedBroadcast: received packet does not have a flooding header");
    }

    MacAddress    srcid = fheader->getSourceId();
    uint32_t      seqno = fheader->getSeqno();
    uint16_t      ttl   = fheader->getTimeToLive();

    auto it       =  lastSeen.find(srcid);
    bool found    =  it != lastSeen.end();

    // output some debug data about the received packet
    std::stringstream ss;
    ss << "handleReceivedBroadcast: got flooding packet with src = " << srcid
       << " , seqno = " << seqno
       << " , ttl = " << ttl
       << " , found = " << (found ? "TRUE" : "FALSE")
       << " , map contents = " << (found ? (int) (lastSeen[srcid]) : (int) -1 )
       << " , age = " << (simTime() - fheader->getGenerationTime())
       << " , packet queue size = " << packetQueue.getLength()
       << ((found  &&  (lastSeen[srcid] >= seqno)) ? " -- DROPPING" : "");
    debugMsg(ss.str());


    // check whether it comes from ourselves or whether we have
    // already received this
    //
    // #####NOTE: this is not good enough as a test whether I have
    // #####recently seen a request, it could for example fail when a
    // #####source issues two floods quickly and I get to see the
    // #####later one first -- a better implementation is needed
    if (srcid == ownIdentifier)
    {
        debugMsg("handleReceivedBroadcast: This is my own packet, dropping it and stop");
        delete packet;
        leave("handleReceivedBroadcast");
        return;
    }
    if (found  &&  (lastSeen[srcid] >= seqno))
    {
      std::stringstream ss;
      ss << "handleReceivedBroadcast: I have already seen this, dropping it and stop"
	 << " , src = " << srcid
	 << " , packet seqno = " << seqno
	 << " , stored seqno = " << lastSeen[srcid];
      debugMsg(ss.str());
      delete packet;
      leave("handleReceivedBroadcast");
      return;
    }

    
    // update map of already seen requests
    lastSeen[srcid] = seqno;

    // hand over payload to higher layers
    send(packet, gidToHigher);

    // check whether TTL permits further propagation of the packet
    if (ttl <= 0)
    {
        debugMsg("handleReceivedBroadcast: TTL is zero, stopping further propagation");
        return;
    }

    debugMsg("handleReceivedBroadast: put received packet into packetQueue");

    // put header back on, then add packet to the end of queue and trigger further work on the queue
    auto newfheader = makeShared<FloodingHeader>(*fheader);
    auto newpacket  = new Packet(*packet);
    newpacket->trim();
    newfheader->setTimeToLive(ttl-1);
    newpacket->insertAtFront(newfheader);
    packetQueue.insert(newpacket);
    workOnPacketQueue();

    leave("handleReceivedBroadcast");
}

// -------------------------------------------

void Flooding::workOnPacketQueue()
{
    enter("workOnPacketQueue");
    if (currentPacket)
    {
        debugMsg("workOnPacketQueue: we are already working on a packet, stop");
        return;
    }

    assert(!packetQueue.isEmpty());

    currentPacket      = (Packet*) packetQueue.pop();
    repetitionsToGo    = numRepetitions;
    double waitingTime = par("repetitionBackoff");
    scheduleAt(simTime()+waitingTime, nextRepetition);

    leave("workOnPacketQueue");
}

// -------------------------------------------

void Flooding::handleSelfMessage(cMessage* msg)
{
    enter("handleSelfMessage");

    if (msg != nextRepetition)
    {
        error("Flooding::handleSelfMessage: expected nextRepetition");
    }

    if (!currentPacket)
    {
        error("Flooding::handleSelfMessage: currentPacket is null");
    }

    if (repetitionsToGo > 0)
    {
        debugMsg("Flooding::handleSelfMessage: still repetitions to go");
        repetitionsToGo--;
        double waitingTime = par("repetitionBackoff");
        auto fheader = currentPacket->peekAtFront<FloodingHeader>();
        assert(fheader);
        std::stringstream ss;
        ss << "Flooding::handleSelfMessage: next sending packet " << fhToString(*fheader);
        debugMsg(ss.str());
        sendViaLBP(currentPacket->dup());
        scheduleAt(simTime()+waitingTime, nextRepetition);
        leave("handleSelfMessage");
        return;
    }

    delete currentPacket;
    currentPacket = nullptr;

    if (!packetQueue.isEmpty())
    {
        debugMsg("handleSelfMessage: more packets available, calling workOnPacketQueue");
        workOnPacketQueue();
    }

    leave("handleSelfMessage");
}

// -------------------------------------------

Flooding::~Flooding()
{
    cancelAndDelete(nextRepetition);
    if (currentPacket != nullptr) {
        delete currentPacket;
    }
}
