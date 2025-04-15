// Copyright (C) 2024 Andreas Willig, University of Canterbury
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 


#include <list>
#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/common/ProtocolGroup.h>
#include <inet/linklayer/common/MacAddress.h>
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/linklayer/common/MacAddressTag_m.h>
#include <dcp/common/DcpTypesGlobals.h>
#include <dcp/bp/BeaconingProtocol.h>
#include <dcp/bp/BPConfirmation_m.h>
#include <dcp/bp/BPClientProtocolData.h>
#include <dcp/bp/BPPayloadTransmitted_m.h>
#include <dcp/bp/BPReceivePayload_m.h>

// ========================================================================================
// ========================================================================================


using namespace omnetpp;
using namespace inet;
using namespace dcp;

Define_Module(BeaconingProtocol);


// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================


void BeaconingProtocol::initialize (int stage)
{
    DcpProtocol::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        // register BP protocol with INET message dispatcher
        DcpSimGlobals::protocolDcpBP       = new Protocol ("dcp-bp", "DCP Beaconing Protocol");
        ProtocolGroup::getEthertypeProtocolGroup()->addProtocol(0x8999, DcpSimGlobals::protocolDcpBP);
    }


    if (stage == INITSTAGE_LAST)
    {
        dbg_setModuleName("BP");
        dbg_enter("initialize");

        // reading and checking module parameters
        bpParMaximumPacketSizeB  =  (BPLengthT) par("bpParMaximumPacketSize");
        assert(bpParMaximumPacketSizeB > 0);

        DBG_VAR2(dcp::BPPayloadHeaderT::fixed_size(), dcp::BPHeaderT::fixed_size());

        // find gate identifiers
        gidFromUWB      =  findGate("fromUWB");
        gidToUWB        =  findGate("toUWB");
        gidFromClients  =  findGate("fromClients");
        gidToClients    =  findGate("toClients");

        // create protocol, register with Ethertype protocol and message dispatcher
        registerService(*DcpSimGlobals::protocolDcpBP, gate("fromClients"), gate("toClients"));
        registerProtocol(*DcpSimGlobals::protocolDcpBP, gate("toUWB"), gate("fromUWB"));

        // get generation timer ticks going
        generateBeaconMsg = new cMessage("generateBeaconMsg");
        scheduleAt(simTime() + par("bpParBeaconPeriodDistr"), generateBeaconMsg);

        dbg_leave();
    }
}

// ----------------------------------------------------

void BeaconingProtocol::handleMessage (cMessage* msg)
{
    dbg_assertToplevel();
    dbg_enter("handleMessage");

    // dispatch on type of received message

    if (msg->arrivedOn(gidFromClients))
    {
        handleClientMessage(msg);
        dbg_leave();
        return;
    }

    if ((msg->arrivedOn(gidFromUWB)) && dynamic_cast<Packet*>(msg))
     {
        dbg_string("handling received packet");
        auto packet = check_and_cast<Packet*>(msg);
        handleReceivedPacket(packet);
        dbg_leave();
        return;
     }


    if (msg == generateBeaconMsg)
    {
        handleGenerateBeaconMsg();
        dbg_leave();
        return;
    }

    error("BeaconingProtocol::handleMessage: unknown message");
}

// ----------------------------------------------------

BeaconingProtocol::~BeaconingProtocol()
{
    cancelAndDelete(generateBeaconMsg);

    // do *NOT* delete DcpSimGlobals::protocolDcpBP, as it is oddly done in
    // ProtocolGroup.cc (in the destructor)
}


// ========================================================================================
// Beacon generation handler and related methods
// ========================================================================================

/**
 * Checks if current registered protocol has a payload ready and whether it fits
 * into current beacon -- if so, returns the payload (after removing it when
 * necessary)
 */

Ptr<const Chunk> BeaconingProtocol::extractFittingPayload(RegisteredProtocol& rp, BPLengthT bytesUsed, BPLengthT maxBytes)
{
    dbg_enter("extractFittingPayload");
    DBG_VAR2(bytesUsed, maxBytes);
    assert(bytesUsed <= maxBytes);

    // how many bytes can still fit into beacon packet?
    BPLengthT remainingBytes = maxBytes - bytesUsed;

    if (not rp.protData.queue.empty())
    {
        dbg_prefix();
        EV << "inspecting queue with non-empty front element of length(B) = "
           << (rp.protData.queue.front().theChunk->getChunkLength().get() / 8)
           << endl;
    }


    // If client protocol uses queue mode and has a fitting payload at the head of
    // the queue, extract it
    if (    (    (rp.protData.queueMode == BP_QMODE_QUEUE_DROPTAIL)
              || (rp.protData.queueMode == BP_QMODE_QUEUE_DROPHEAD))
         && (not (rp.protData.queue.empty()))
         && (((rp.protData.queue.front().theChunk->getChunkLength().get() / 8) + dcp::BPPayloadHeaderT::fixed_size()) <= remainingBytes))
    {
        dbg_string("found payload for protocol with BP_QMODE_QUEUE_DROPTAIL or BP_QMODE_QUEUE_DROPHEAD");
        auto qe = rp.protData.queue.front();
        rp.protData.queue.pop();
        dbg_leave();
        return qe.theChunk;
    }

    // We are in one of the buffered modes -- leave if buffer is empty
    if (not rp.protData.bufferOccupied)
    {
        dbg_string("buffer is empty, returning nothing");
        dbg_leave();
        return nullptr;
    }

    DBG_PVAR1("inspecting buffer with non-empty element of length(B) = ", (rp.protData.bufferEntry.theChunk->getChunkLength().get() / 8))

    //  leave if buffere element does not fit into remaining beacon
    if (((rp.protData.bufferEntry.theChunk->getChunkLength().get() / 8) + dcp::BPPayloadHeaderT::fixed_size()) > remainingBytes)
    {
        dbg_string("buffer payload is too large, returning nothing");
        dbg_leave();
        return nullptr;
    }

    // return buffer contents and clear buffer (or not) according to buffer mode
    auto thePayload = rp.protData.bufferEntry.theChunk;
    if (rp.protData.queueMode == BP_QMODE_ONCE)
    {
        dbg_string("found payload for protocol with BP_QMODE_ONCE");
        rp.protData.bufferEntry.theChunk = nullptr;
        rp.protData.bufferOccupied = false;
        dbg_leave();
        return thePayload;
    }

    if (rp.protData.queueMode == BP_QMODE_REPEAT)
    {
        dbg_string("found payload for protocol with BP_QMODE_REPEAT");
        dbg_leave();
        return thePayload;
    }


    error("BeaconingProtocol::extractFittingPayload: unknown queue type");

    dbg_leave();
    return nullptr;
}


// ----------------------------------------------------

/**
 * Checks for the given registered protocol whether a payload can be added to beacon
 * and does so if possible, also informs client protocol that this has happened by
 * sending BPPayloadTransmitted.indication primitive
 */

void BeaconingProtocol::addPayload(RegisteredProtocol& rp,
                                   std::list< Ptr<const Chunk> >& beaconChunks,
                                   BPLengthT& bytesUsed,
                                   size_t& numberPayloadsAdded,
                                   BPLengthT maxBytes,
                                   simtime_t nextBeaconGenerationEpoch
                                   )
{
    dbg_enter("addPayload");

    // determine protocol id
    BPProtocolIdT       protId = rp.protId;

    DBG_PVAR4("considering client protocol", rp.protData.protocolName, protId, bytesUsed, maxBytes);

    // retrieve the actual payload
    Ptr<const Chunk> thePayload = extractFittingPayload(rp, bytesUsed, maxBytes);

    // add it to packet and let registered protocol know
    if (thePayload)
    {
        BPLengthT payloadSizeB = (BPLengthT) (thePayload->getChunkLength().get() / 8);

        DBG_PVAR4("adding payload", payloadSizeB, rp.protData.protocolName, bytesUsed, maxBytes);

        // we can add this chunk to the packet, preceded by a payload header

        // construct payload header
        bytevect bv (dcp::BPPayloadHeaderT::fixed_size());
        bv.reserve(2*dcp::BPPayloadHeaderT::fixed_size());
        ByteVectorAssemblyArea area (dcp::BPPayloadHeaderT::fixed_size(), bv);
        BPPayloadHeaderT pldHeader;
        pldHeader.protocolId = protId;
        pldHeader.length     = payloadSizeB;
        pldHeader.serialize (area);
        bv.resize (area.used());
        auto payloadHeader = makeShared<BytesChunk>(bv);

        beaconChunks.push_back(payloadHeader);
        beaconChunks.push_back(thePayload);
        bytesUsed += dcp::BPPayloadHeaderT::fixed_size();
        bytesUsed += payloadSizeB;
        numberPayloadsAdded += 1;

        DBG_PVAR2("added payload", payloadSizeB, bytesUsed);

        // send transmission indication to client protocol
        auto txInd = new BPPayloadTransmitted_Indication;
        txInd->setProtId(protId);
        txInd->setNextBeaconGenerationEpoch(nextBeaconGenerationEpoch);
        auto req = txInd->addTagIfAbsent<DispatchProtocolReq>();
        req->setProtocol(convertProtocolIdToProtocol(protId));
        req->setServicePrimitive(SP_INDICATION);
        send(txInd, gidToClients);


    }

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Takes the list of chunks that go into a beacon, creates the beacon packet including
 * headers and all relevant chunks, and hands it down to lower layers for transmission.
 * If there are no chunks, then no beacon is generated.
 */

void BeaconingProtocol::constructAndTransmitBeacon (std::list< Ptr<const Chunk> >& beaconChunks)
{
    dbg_enter("constructAndTransmitBeacon");

    if (beaconChunks.empty())
    {
        dbg_string("no client protocol had a (suitable) chunk ready for transmission, exiting");
        dbg_leave();
        return;
    }

    // now we construct the actual packet for transmission
    auto theBeaconPacket = new Packet;

    // construct BPHeader
    bytevect bv (dcp::BPHeaderT::fixed_size());
    bv.reserve (2*dcp::BPHeaderT::fixed_size());
    ByteVectorAssemblyArea area (dcp::BPHeaderT::fixed_size(), bv);
    BPHeaderT bpHdr;
    bpHdr.version     =  bpProtocolVersion;
    bpHdr.magicNo     =  bpMagicNo;
    bpHdr.senderId    =  getOwnNodeId();
    bpHdr.length      =  BPHeaderT::fixed_size();   /* ISSUE: Just so that well-formedness check does not fail, but is otherwise ignored for now */
    bpHdr.numPayloads =  beaconChunks.size() / 2;
    bpHdr.seqno       =  _seqno++;
    bpHdr.serialize (area);
    bv.resize (area.used());
    auto theBPHeader     = makeShared<BytesChunk>(bv);

    // now insert BPHeader and all the payload header / data chunks
    theBeaconPacket->insertAtBack(theBPHeader);
    for (auto ch : beaconChunks)
    {
        theBeaconPacket->insertAtBack(ch);
    }

    // hand the packet over to UWB for transmission
    theBeaconPacket->removeTagIfPresent<DispatchProtocolReq>();
    theBeaconPacket->addTagIfAbsent<PacketProtocolTag>()->setProtocol(DcpSimGlobals::protocolDcpBP);
    theBeaconPacket->addTagIfAbsent<InterfaceReq>()->setInterfaceId(getWlanInterface()->getInterfaceId());
    theBeaconPacket->addTagIfAbsent<MacAddressReq>()->setDestAddress(MacAddress::BROADCAST_ADDRESS);
    send(theBeaconPacket, gidToUWB);

    dbg_leave();
}


// ----------------------------------------------------


/*
 * This implements a very simplistic method of generating beacon
 * packets: we go sequentially through all registered protocols
 * (each time starting from the front) and add a payload when
 * one is available and fits into the remaining beacon
 */

void BeaconingProtocol::handleGenerateBeaconMsg ()
{
    dbg_enter("handleGenerateBeaconMsg");

    // Schedule next generation of beacon packet
    simtime_t nextBeaconGenerationEpoch = simTime() + par("bpParBeaconPeriodDistr");
    scheduleAt(nextBeaconGenerationEpoch, generateBeaconMsg);


    // check if any protocols are registered, exit if not
    if (registeredProtocols.empty())
    {
        dbg_string("no protocol registered, exiting");
        dbg_leave();
        return;
    }

    std::list< Ptr<const Chunk> >  beaconChunks;
    BPLengthT  bytesUsed         = dcp::BPHeaderT::fixed_size();
    BPLengthT  maxBytes          = bpParMaximumPacketSizeB;
    size_t     numPayloadsAdded  = 0;
    auto       rpiter            = registeredProtocols.begin();

    // iterate over all registered client protocols, add payload when possible
    while (rpiter != registeredProtocols.end())
    {
        RegisteredProtocol& rp     = rpiter->second;
        addPayload(rp, beaconChunks, bytesUsed, numPayloadsAdded, maxBytes, nextBeaconGenerationEpoch);
        rpiter++;
    }
    assert(beaconChunks.size() % 2 == 0);

    if (numPayloadsAdded > 0)
    {
        dbg_string("found payloads, generating beacon");
        constructAndTransmitBeacon(beaconChunks);
    }

    dbg_leave();
}


// ========================================================================================
// Processing received packets
// ========================================================================================

/**
 * Sanity checks for incoming BPHeaderT: right magicno, making sure I don't get my
 * own packet, checking protocol version
 */
bool BeaconingProtocol::bpHeaderWellFormed (DisassemblyArea& area, BPHeaderT& bpHdr)
{
    dbg_enter ("bpHeaderWellFormed");

    bpHdr.deserialize (area);

    if (bpHdr.magicNo != bpMagicNo)
    {
        DBG_PVAR1 ("did not find magicno", bpHdr.magicNo);
        dbg_leave ();
        return false;
    }

    if (bpHdr.senderId == getOwnNodeId())
    {
        dbg_string ("got my own packet");
        dbg_leave ();
        return false;
    }

    if (bpHdr.version != bpProtocolVersion)
    {
        DBG_PVAR1 ("wrong protocol version", bpHdr.version);
        dbg_leave ();
        return false;
    }

    if (bpHdr.numPayloads == 0)
    {
        dbg_string("numPayloads is zero");
        dbg_leave();
        return false;
    }

    if (bpHdr.length == 0)
    {
        dbg_string ("length is zero");
        dbg_leave ();
        return false;
    }

    dbg_leave ();
    return true;
}

// ----------------------------------------------------

/**
 * Sanity checks for incoming BPHeaderT: right magicno, making sure I don't get my
 * own packet, checking protocol version
 */
bool BeaconingProtocol::bpPayloadHeaderWellFormed (DisassemblyArea& area, BPPayloadHeaderT& bpPHdr)
{
    dbg_enter("bpPayloadHeaderWellFormed");

    bpPHdr.deserialize (area);

    dbg_leave();
    return true;
}


// ----------------------------------------------------

void BeaconingProtocol::handleReceivedPacket (Packet* packet)
{
    dbg_enter("handleReceivedPacket");

    assert(packet);

    // Note: when constructing a beacon, it happens by appending a number of BytesChunk
    // payloads into a packet handed to the WiFi MAC. The INET framework merges all the
    // previously separate BytesChunks into a single one/

    // first extract BP header and check its validity
    const auto& tmpBC = packet->popAtFront<BytesChunk>();
    assert(tmpBC);

    const bytevect& bvPacket = tmpBC->getBytes();
    ByteVectorDisassemblyArea area (bvPacket);

    DBG_PVAR1("got packet with size", bvPacket.size());

    BPHeaderT bpHdr;
    if (not bpHeaderWellFormed(area, bpHdr))
    {
        error ("BPHeader is not well-formed, stop processing");
    }

    uint8_t numberPayloads = bpHdr.numPayloads;

    DBG_PVAR3("got packet from sender", bpHdr.senderId, (int) numberPayloads, bpHdr.seqno);

    // now extract the payloads and send them to the respective client protocols
    for (int cntPayload = 0; cntPayload < numberPayloads; cntPayload++)
    {
        BPPayloadHeaderT bpPHdr;
        if (not bpPayloadHeaderWellFormed(area, bpPHdr))
        {
            error ("BPPayloadHeader is not well-formed, stop processing");
        }

        Protocol *theProtocol        = convertProtocolIdToProtocol(bpPHdr.protocolId);
        if (not theProtocol)
        {
            DBG_VAR1(bpPHdr.protocolId);
            error("BeaconingProtocol::handleReceivedPacket: unknown protocol id");
        }
        DBG_VAR3(cntPayload, bpPHdr.protocolId, theProtocol->getName());

        BPReceivePayload_Indication *pldInd = new BPReceivePayload_Indication;
        pldInd->setProtId(bpPHdr.protocolId);
        bytevect& bvPld = pldInd->getPayloadForUpdate();
        bvPld.resize(bpPHdr.length);

        area.deserialize_byte_block (bpPHdr.length, (byte*) &(bvPld[0]));

        auto req = pldInd->addTagIfAbsent<DispatchProtocolReq>();
        req->setProtocol(theProtocol);
        req->setServicePrimitive(SP_INDICATION);
        send(pldInd, gidToClients);
    }

    // check if there are any unaccounted chunks left
    if (packet->hasAtFront<Chunk>())
    {
        error("BeaconingProtocol::handleReceivedPacket: there is a leftover chunk in a received packet");
    }

    delete packet;

    dbg_leave();
}


// ========================================================================================
// Event handlers for events sent by client protocols
// ========================================================================================

/**
 * Process BPRegisterProtocol.request message. Check if protocol is known or
 * already registered, and check maximum payload size value. Generate confirmation
 * for client protocol.
 */
void BeaconingProtocol::handleRegisterProtocolRequestMsg (BPRegisterProtocol_Request* regReq)
{

    dbg_enter("handleRegisterProtocolRequestMsg");
    assert(regReq);
    DBG_PVAR2("got BPRegisterProtocol_Request message", regReq->getProtId(), regReq->getProtName());

    // first retrieve parameters and delete message
    BPProtocolIdT        protocolId = regReq->getProtId();
    BPClientProtocolData clientProtData;
    clientProtData.protocolId             =  protocolId;
    clientProtData.protocolName           =  regReq->getProtName();
    clientProtData.maxPayloadSizeB        =  regReq->getMaxPayloadSizeB();
    clientProtData.queueMode              =  regReq->getQueueingMode();
    clientProtData.allowMultiplePayloads  =  regReq->getAllowMultiplePayloads();
    clientProtData.maxEntries             =  regReq->getMaxEntries();
    clientProtData.timeStampRegistration  =  simTime();
    clientProtData.bufferOccupied         =  false;
    delete regReq;

    // look up the referenced protocol object
    Protocol *theProtocol = convertProtocolIdToProtocol(protocolId);
    assert(theProtocol);

    // check if protocol is already registered and register it if not,
    // otherwise return appropriate status indication
    if (clientProtocolRegistered(protocolId))
    {
        dbg_string ("attempting to register an already existing protocol");
        sendRegisterProtocolConfirm(BP_STATUS_PROTOCOL_ALREADY_REGISTERED, theProtocol);
        dbg_leave();
        return;
    }

    // check if maximum payload size is zero or less
    if (clientProtData.maxPayloadSizeB <= 0)
    {
        dbg_string ("illegal maximum payload size -- payload too small");
        sendRegisterProtocolConfirm(BP_STATUS_ILLEGAL_MAX_PAYLOAD_SIZE, theProtocol);
        dbg_leave();
        return;
    }

    // check if maximum payload size is too large
    if (clientProtData.maxPayloadSizeB > (bpParMaximumPacketSizeB - (dcp::BPHeaderT::fixed_size() + dcp::BPPayloadHeaderT::fixed_size())))
    {
        dbg_string ("illegal maximum payload size -- payload too large");
        sendRegisterProtocolConfirm(BP_STATUS_ILLEGAL_MAX_PAYLOAD_SIZE, theProtocol);
        dbg_leave();
        return;
    }

    if (    (    (clientProtData.queueMode == BP_QMODE_QUEUE_DROPTAIL)
              || (clientProtData.queueMode == BP_QMODE_QUEUE_DROPHEAD))
         && (clientProtData.maxEntries <= 0))
    {
        dbg_string ("illegal maxEntries value for dropping queue");
        sendRegisterProtocolConfirm(BP_STATUS_ILLEGAL_DROPPING_QUEUE_SIZE, theProtocol);
        dbg_leave();
        return;
    }

    // register protocol
    dbg_string ("registering new protocol");

    RegisteredProtocol  rp;
    rp.protId        =  protocolId;
    rp.protData      =  clientProtData;
    rp.protProtocol  =  theProtocol;

    registeredProtocols[protocolId] = rp;

    sendRegisterProtocolConfirm(BP_STATUS_OK, theProtocol);

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Process BPDeregisterProtocol.request message. Check if protocol is known
 * and de-register it. Generate confirmation for client protocol.
 */
void BeaconingProtocol::handleDeregisterProtocolRequestMsg (BPDeregisterProtocol_Request* deregReq)
{
    dbg_enter("handleDeregisterProtocolRequestMsg");
    assert(deregReq);
    DBG_PVAR1("got BPDeregisterProtocol_Request message", deregReq->getProtId());

    // first retrieve parameters and delete message
    BPProtocolIdT   protocolId = deregReq->getProtId();
    delete deregReq;

    // look up the referenced protocol object
    Protocol *theProtocol = convertProtocolIdToProtocol(protocolId);
    assert(theProtocol);

    // check if protocol is already registered, if not return negative
    // status indication
    if (not clientProtocolRegistered(protocolId))
    {
        dbg_string ("handleDeregisterProtocolRequestMsg: attempting to deregister a non-registered protocol");
        sendDeregisterProtocolConfirm(BP_STATUS_UNKNOWN_PROTOCOL, theProtocol);
        dbg_leave();
        return;
    }

    registeredProtocols.erase(protocolId);

    sendDeregisterProtocolConfirm(BP_STATUS_OK, theProtocol);

    dbg_leave();

}

// ----------------------------------------------------

/**
 * Process BPTransmitPayload.request message. Check if protocol is known,
 * if payload length is acceptable and add payload to buffer/queue as
 * appropriate. Generate confirmation for client protocol.
 */

void BeaconingProtocol::handleTransmitPayloadRequestMsg (BPTransmitPayload_Request *txplReq)
{
    dbg_enter("handleTransmitPayloadRequestMsg");
    assert(txplReq);

    BPProtocolIdT protocolId    = txplReq->getProtId();
    auto dataChunk              = makeShared<BytesChunk>(txplReq->getBvdata());
    BPLengthT dataChunkLengthB  = (BPLengthT) (dataChunk->getChunkLength().get() / 8);
    delete txplReq;

    // look up the referenced protocol object
    Protocol *theProtocol = convertProtocolIdToProtocol(protocolId);
    assert(theProtocol);

    // check if protocol exists, and stop with error status if not
    if (not clientProtocolRegistered(protocolId))
    {
        dbg_string ("attempting to send payload for non-registered protocol");
        sendTransmitPayloadConfirm(BP_STATUS_UNKNOWN_PROTOCOL, theProtocol);
        dbg_leave();
        return;
    }

    RegisteredProtocol& rp = registeredProtocols[protocolId];

    // check length of payload
    uint32_t  maxSize = (uint32_t) rp.protData.maxPayloadSizeB;
    DBG_VAR3(dataChunkLengthB, maxSize, dataChunk);
    if (    (dataChunkLengthB > maxSize)
         || (dataChunkLengthB > (bpParMaximumPacketSizeB - (dcp::BPPayloadHeaderT::fixed_size() + dcp::BPHeaderT::fixed_size()))))
    {
        dbg_string ("payload too large");
        sendTransmitPayloadConfirm(BP_STATUS_PAYLOAD_TOO_LARGE, theProtocol);
        dbg_leave();
        return;
    }

    if (dataChunkLengthB == 0)
    {
        dbg_string ("payload is empty");
        sendTransmitPayloadConfirm(BP_STATUS_EMPTY_PAYLOAD, theProtocol);
        dbg_leave();
        return;
    }


    // handle buffering modes -- it is allowed to clear buffer with a chunk of
    // length zero
    if ((rp.protData.queueMode == BP_QMODE_ONCE) || (rp.protData.queueMode == BP_QMODE_REPEAT))
    {
        dbg_string ("handling the case of QMODE_ONCE or QMODE_REPEAT");

        rp.protData.bufferOccupied        = true;
        rp.protData.bufferEntry.theChunk  = dataChunk;

        sendTransmitPayloadConfirm(BP_STATUS_OK, theProtocol);

        dbg_leave();
        return;
    }

    // handle queueing mode
    if (    (rp.protData.queueMode == BP_QMODE_QUEUE_DROPTAIL)
         || (rp.protData.queueMode == BP_QMODE_QUEUE_DROPHEAD))
    {
        dbg_string ("handling the case of BP_QMODE_QUEUE_DROPTAIL or BP_QMODE_QUEUE_DROPHEAD");

        if (    (rp.protData.queueMode == BP_QMODE_QUEUE_DROPTAIL)
             && (rp.protData.queue.size() >= rp.protData.maxEntries))
        {
            dbg_string ("dropping payload at droptail queue");
            sendTransmitPayloadConfirm(BP_STATUS_OK, theProtocol);
        }
        else
        {
            if (    (rp.protData.queueMode == BP_QMODE_QUEUE_DROPHEAD)
                 && (rp.protData.queue.size() >= rp.protData.maxEntries))
            {
                dbg_string ("QMODE_DROPHEAD: dropping head-of-line element");
                rp.protData.queue.pop();
            }

            BPBufferEntry newEntry;
            newEntry.theChunk = dataChunk;
            rp.protData.queue.push(newEntry);
            sendTransmitPayloadConfirm(BP_STATUS_OK, theProtocol);
        }

        dbg_leave();
        return;
    }

    error("handleTransmitPayloadRequestMsg: unknown / un-implemented case");

    dbg_leave();

}


// ----------------------------------------------------

/**
 * Process BPClearBuffer.request message. Check if protocol is known,
 * then empty the associated buffer / queue. Generate confirmation for
 * client protocol.
 */

void BeaconingProtocol::handleClearBufferRequestMsg (BPClearBuffer_Request *clearReq)
{
    dbg_enter("handleClearBufferRequestMsg");
    assert(clearReq);

    BPProtocolIdT protocolId    = clearReq->getProtId();
    delete clearReq;

    // look up the referenced protocol object
    Protocol *theProtocol = convertProtocolIdToProtocol(protocolId);
    assert(theProtocol);

    // check if protocol exists, and stop with error status if not
    if (not clientProtocolRegistered(protocolId))
    {
        dbg_string ("attempting to send payload for non-registered protocol");
        sendTransmitPayloadConfirm(BP_STATUS_UNKNOWN_PROTOCOL, theProtocol);
        dbg_leave();
        return;
    }

    RegisteredProtocol& rp = registeredProtocols[protocolId];

    if ((rp.protData.queueMode == BP_QMODE_ONCE) || (rp.protData.queueMode == BP_QMODE_REPEAT))
    {
        dbg_string ("handling the case of QMODE_ONCE or QMODE_REPEAT");

        rp.protData.bufferOccupied        = false;
        rp.protData.bufferEntry.theChunk  = nullptr;

        sendClearBufferConfirm(BP_STATUS_OK, theProtocol);

        dbg_leave();
        return;
    }

    // handle queueing mode
    if (    (rp.protData.queueMode == BP_QMODE_QUEUE_DROPTAIL)
         || (rp.protData.queueMode == BP_QMODE_QUEUE_DROPHEAD))
    {
        dbg_string ("handling the case of BP_QMODE_QUEUE_DROPTAIL or BP_QMODE_QUEUE_DROPHEAD");

        while (not rp.protData.queue.empty())
        {
            rp.protData.queue.pop();
        }

        sendClearBufferConfirm(BP_STATUS_OK, theProtocol);

        dbg_leave();
        return;
    }

    sendClearBufferConfirm(BP_STATUS_UNKNOWN_QUEUEING_MODE, theProtocol);

    error("handleClearBufferRequestMsg: unknown queueing mode");

    dbg_leave();

}



// ----------------------------------------------------

/**
 * Process BPQueryNumberBufferedPayloads.request message. Check whether client
 * protocol is registered, and if so return the number of payloads buffered in
 * the confirm primitive.
 */
void BeaconingProtocol::handleQueryNumberBufferedPayloadsRequestMsg(BPQueryNumberBufferedPayloads_Request* bpReq)
{
    dbg_enter("handleQueryNumberBufferedPayloadsRequest");
    assert(bpReq);

    BPProtocolIdT protocolId    = bpReq->getProtId();
    delete bpReq;

    // look up the referenced protocol object
    Protocol *theProtocol = convertProtocolIdToProtocol(protocolId);
    assert(theProtocol);

    // check if protocol exists, and stop with error status if not
    if (not clientProtocolRegistered(protocolId))
    {
        dbg_string ("attempting to send payload for non-registered protocol");
        sendQueryNumberBufferedPayloadsConfirm(BP_STATUS_UNKNOWN_PROTOCOL, 0, protocolId, theProtocol);
        dbg_leave();
        return;
    }

    unsigned int numberBuffered = 0;
    RegisteredProtocol& rp = registeredProtocols[protocolId];

    // handling buffered modes
    if ((rp.protData.queueMode == BP_QMODE_ONCE) || (rp.protData.queueMode == BP_QMODE_REPEAT))
    {
        dbg_string ("handling the case of QMODE_ONCE or QMODE_REPEAT");
        DBG_VAR2(rp.protData.bufferOccupied, rp.protData.protocolName);
        numberBuffered = (rp.protData.bufferOccupied) ? 1 : 0;
        sendQueryNumberBufferedPayloadsConfirm(BP_STATUS_OK, numberBuffered, protocolId, theProtocol);
        dbg_leave();
        return;
    }

    // handling queueing mode
    if (    (rp.protData.queueMode == BP_QMODE_QUEUE_DROPTAIL)
         || (rp.protData.queueMode == BP_QMODE_QUEUE_DROPHEAD))
    {
        dbg_string ("handling the case of BP_QMODE_QUEUE_DROPTAIL or BP_QMODE_QUEUE_DROPHEAD");
        numberBuffered = rp.protData.queue.size();
        sendQueryNumberBufferedPayloadsConfirm(BP_STATUS_OK, numberBuffered, protocolId, theProtocol);
        dbg_leave();
        return;
    }

    error("handleQueryNumberBufferedPayloadsRequestMsg: unknown / un-implemented case");

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Dispatcher for all valid request types coming from client protocols
 */
void BeaconingProtocol::handleClientMessage (cMessage* msg)
{
    dbg_enter("handleClientMessage");

    if (dynamic_cast<BPRegisterProtocol_Request*>(msg))
    {
        dbg_string("handling protocol registration request");
        BPRegisterProtocol_Request *reqMsg = (BPRegisterProtocol_Request*) msg;
        handleRegisterProtocolRequestMsg (reqMsg);
        dbg_leave();
        return;
    }

    if (dynamic_cast<BPDeregisterProtocol_Request*>(msg))
    {
        dbg_string("handling protocol deregistration request");
        BPDeregisterProtocol_Request *reqMsg = (BPDeregisterProtocol_Request*) msg;
        handleDeregisterProtocolRequestMsg (reqMsg);
        dbg_leave();
        return;
    }

    if (dynamic_cast<BPTransmitPayload_Request*>(msg))
    {
        dbg_string("handling payload transmit request");
        BPTransmitPayload_Request *reqMsg = (BPTransmitPayload_Request*) msg;
        handleTransmitPayloadRequestMsg (reqMsg);
        dbg_leave();
        return;
    }

    if (dynamic_cast<BPQueryNumberBufferedPayloads_Request*>(msg))
    {
        dbg_string("handling query-number-buffered-payloads request");
        BPQueryNumberBufferedPayloads_Request *reqMsg = (BPQueryNumberBufferedPayloads_Request*) msg;
        handleQueryNumberBufferedPayloadsRequestMsg (reqMsg);
        dbg_leave();
        return;
    }

    error("BeaconingProtocol::handleClientMessage: unknown message type");
    dbg_leave();
}


// ========================================================================================
// Helpers
// ========================================================================================

/**
 * Checks whether the indicated client protocol is registered
 */
bool BeaconingProtocol::clientProtocolRegistered(BPProtocolIdT protocolId)
{
    dbg_enter("clientProtocolRegistered");
    auto search = registeredProtocols.find(protocolId);
    dbg_leave();
    return (search != registeredProtocols.end());
}

// ----------------------------------------------------

/**
 * Send the given confirmation message with the given status code to the given
 * client protocol
 */
void BeaconingProtocol::sendConfirmation(BPConfirmation *confMsg, BPStatus status, Protocol* theProtocol)
{
    dbg_enter("sendConfirmation");
    assert(theProtocol);
    assert(confMsg);

    confMsg->setStatus(status);

    auto req = confMsg->addTagIfAbsent<DispatchProtocolReq>();
    req->setProtocol(theProtocol);
    req->setServicePrimitive(SP_INDICATION);

    send(confMsg, gidToClients);

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Prepares and sends a BPRegisterProtocol.confirm message
 */
void BeaconingProtocol::sendRegisterProtocolConfirm(BPStatus status, Protocol* theProtocol)
{
    dbg_enter("sendRegisterProtocolConfirm");
    BPRegisterProtocol_Confirm*  conf = new BPRegisterProtocol_Confirm;
    conf->setStatus (status);
    conf->setOwnNodeIdentifier (getOwnNodeId());
    sendConfirmation(conf, status, theProtocol);
    dbg_leave();
}

// ----------------------------------------------------

/**
 * Prepares and sends a BPDeregisterProtocol.confirm message
 */
void BeaconingProtocol::sendDeregisterProtocolConfirm(BPStatus status, Protocol* theProtocol)
{
    dbg_enter("sendDeregisterProtocolConfirm");
    sendConfirmation(new BPDeregisterProtocol_Confirm, status, theProtocol);
    dbg_leave();
}

// ----------------------------------------------------

/**
 * Prepares and sends a BPTransmitPayload.confirm message
 */
void BeaconingProtocol::sendTransmitPayloadConfirm(BPStatus status, Protocol* theProtocol)
{
    dbg_enter("sendTransmitPayloadConfirm");
    sendConfirmation(new BPTransmitPayload_Confirm, status, theProtocol);
    dbg_leave();
}

// ----------------------------------------------------

/**
 * Prepares and sends a BPClearBuffer.confirm message
 */
void BeaconingProtocol::sendClearBufferConfirm(BPStatus status, Protocol* theProtocol)
{
    dbg_enter("sendClearBufferConfirm");
    sendConfirmation(new BPClearBuffer_Confirm, status, theProtocol);
    dbg_leave();
}


// ----------------------------------------------------

/**
 * Prepares and sends a BPQueryNumberBufferedPayloads.confirm message
 */
void BeaconingProtocol::sendQueryNumberBufferedPayloadsConfirm (BPStatus status, unsigned int numPayloads, BPProtocolIdT protocolId, Protocol* theProtocol)
{
    dbg_enter("sendQueryNumberBufferedPayloadsConfirm");
    auto confMsg = new BPQueryNumberBufferedPayloads_Confirm;
    confMsg->setNumberBuffered((int) numPayloads);
    confMsg->setProtId(protocolId);
    sendConfirmation(confMsg, status, theProtocol);
    dbg_leave();
}



