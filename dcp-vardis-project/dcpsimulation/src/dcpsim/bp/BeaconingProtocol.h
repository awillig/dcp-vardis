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

#pragma once


#include <omnetpp.h>
#include <inet/common/InitStages.h>
#include <inet/common/Protocol.h>
#include <inet/networklayer/common/InterfaceTable.h>
#include <dcp/bp/bp_transmissible_types.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/services_status.h>
#include <dcpsim/bp/BPDeregisterProtocol_m.h>
#include <dcpsim/bp/BPRegisterProtocol_m.h>
#include <dcpsim/bp/BPTransmitPayload_m.h>
#include <dcpsim/bp/BPClearBuffer_m.h>
#include <dcpsim/bp/BPConfirmation_m.h>
#include <dcpsim/bp/BPQueryNumberBufferedPayloads_m.h>
#include <dcpsim/bp/BPClientProtocolData.h>
#include <dcpsim/common/DcpProtocol.h>


using namespace omnetpp;
using namespace inet;

using dcp::bp::BPHeaderT;
using dcp::bp::BPPayloadHeaderT;

// -------------------------------------------------------------------

namespace dcp {

struct RegisteredProtocol {
    BPProtocolIdT           protId;
    BPClientProtocolData    protData;
    Protocol*               protProtocol;
};


/**
 * This module implements the beaconing protocol (BP). It checks frequently
 * whether there are payloads available in input buffers or queues, constructs
 * outgoing beacons out of these and hands them over to the underlying
 * IEEE 802.11 interface. Conversely, it receives and processes received
 * beacons.
 *
 * The implementation is based on the specification of the BP as part of the
 * DCPV1 specification.
 */


class BeaconingProtocol : public DcpProtocol {

public:
    virtual ~BeaconingProtocol();
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage (cMessage* msg) override;


protected:

    // -------------------------------------------
    // data members
    // -------------------------------------------


    // fixed values in the BPHeader, the beacon protocol header
    const uint16_t bpMagicNo         = 0x497E;
    const uint8_t  bpProtocolVersion = 1;

    // parameter for maximum beacon packet size
    BPLengthT   bpParMaximumPacketSizeB;

    // Beacons have sequence numbers
    uint32_t    _seqno = 0;

    // Gate identifiers
    int   gidFromUWB, gidToUWB;
    int   gidFromClients, gidToClients;

    // internal data members
    std::map<BPProtocolIdT, RegisteredProtocol>  registeredProtocols;  // contains all registered client protocols
    cMessage*     generateBeaconMsg;   // timer self-message for beacon generations


    // -------------------------------------------
    // message handlers
    // -------------------------------------------

    /**
     * inspects payload buffers/queues and generates outgoing beacon packet
     */
    void handleGenerateBeaconMsg ();

    /**
     * Dispatcher for all request types offered by the public BP interface
     * (e.g. BPRegisterProtocol.request, BPTransmitPayload.request)
     */
    void handleClientMessage(cMessage* msg);

    /**
     * Processes BPRegisterProtocol.request message, allowing client
     * protocols to register with BP. Only registered client protocols
     * can transmit or receive payloads.
     */
    void handleRegisterProtocolRequestMsg (BPRegisterProtocol_Request* regReq);

    /**
     * Processes BPDeregisterProtocol.request message, un-registers a client
     * protocol.
     */
    void handleDeregisterProtocolRequestMsg (BPDeregisterProtocol_Request* deregReq);

    /**
     * Processes BPTransmitPayload.request message. A client protocol hands over
     * a payload for transmission, it is placed either into a buffer or a queue
     */
    void handleTransmitPayloadRequestMsg (BPTransmitPayload_Request* txplReq);

    /**
     * Processes BPClearBuffer.request message. The buffer / queue associated
     * the client protocol is cleared.
     */
    void handleClearBufferRequestMsg (BPClearBuffer_Request* clearReq);

    /**
     * Processes BPQueryNumberBufferedPayloads.request. Allows a client protocol
     * to query how many yet-to-be transmitted messages it has buffered in the BP
     */
    void handleQueryNumberBufferedPayloadsRequestMsg(BPQueryNumberBufferedPayloads_Request* bpReq);

    /**
     * Processes a received BP packet, performs validity tests, extracts payloads
     * and hands them over to their respective client protocols
     */
    void handleReceivedPacket (Packet* packet);


    // -------------------------------------------
    // various helpers
    // -------------------------------------------

    /**
     * Sends a confirmation message (for a previous service request) to client
     * protocol
     */
    void sendConfirmation(BPConfirmation *confMsg, DcpStatus status, Protocol* theProtocol);

    /**
     * Generates and sends BPRegisterProtocol.confirm message to client protocol
     */
    void sendRegisterProtocolConfirm (DcpStatus status, Protocol* theProtocol);

    /**
     * Generates and sends BPDeregisterProtocol.confirm message to client protocol
     */
    void sendDeregisterProtocolConfirm (DcpStatus status, Protocol* theProtocol);

    /**
     * Generates and sends BPTransmitPayload.confirm message to client protocol
     */
    void sendTransmitPayloadConfirm (DcpStatus status, Protocol* theProtocol);

    /**
     * Generates and sends BPClearBuffer.confirm message to client protocol
     */
    void sendClearBufferConfirm (DcpStatus status, Protocol* theProtocol);


    /**
     * Generates and sends BPQueryNumberBufferedPayloads.confirm message to client protocol
     */
    void sendQueryNumberBufferedPayloadsConfirm (DcpStatus status, unsigned int numPayloads, BPProtocolIdT protocolId, Protocol* theProtocol);

    /**
     * Checks whether BPHeaderT of incoming beacon is well-formed
     */
    bool bpHeaderWellFormed (DisassemblyArea& area, BPHeaderT& bpHdr);


    /**
     * Checks whether BPPayloadHeaderT of incoming beacon is well-formed
     */
    bool bpPayloadHeaderWellFormed (DisassemblyArea& area, BPPayloadHeaderT& bpPHdr);


    /**
     * Checks whether a protocol with given ProtocolId is registered as client protocol
     */
    bool clientProtocolRegistered (BPProtocolIdT protocolId);


    /**
     * Takes the list of chunks that should go into a packet, creates the packet
     * including headers and hands it down to the lower layers for transmission
     */
    void constructAndTransmitBeacon (std::list< Ptr<const Chunk> >& beaconChunks);


    /**
     * if possible length-wise, adds a payload from a registered protocol
     * into beacon and informs the registered protocol about this
     */
    void addPayload (RegisteredProtocol& rp,                         // the registered protocol to consider
                     std::list< Ptr<const Chunk> >& beaconChunks,    // this collects the chunks for the packet
                     BPLengthT& bytesUsed,                           // keeps track of # of bytes used
                     size_t& numberPayloadsAdded,                    // keeps track of # payloads added to beacon
                     BPLengthT maxBytes,                             // max bytes available in beacon
                     simtime_t nextBeaconGenerationEpoch             // time of next beacon generation
                     );

    /**
     * checks if the current registered protocol has a payload ready that may
     * fit into the remaining beacon -- returns it (after removing it) or
     * returns nullptr if there is no such payload
     */
    Ptr<const Chunk> extractFittingPayload (RegisteredProtocol& rp, BPLengthT bytesUsed, BPLengthT maxBytes);

};

} // namespace

