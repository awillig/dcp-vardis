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

#include <queue>
#include <omnetpp.h>
#include <inet/common/InitStages.h>
#include <dcp/common/area.h>
#include <dcp/common/services_status.h>
#include <dcp/vardis/vardis_transmissible_types.h>
#include <dcpsim/bp/BPClientProtocol.h>
#include <dcpsim/bp/BPReceivePayload_m.h>
#include <dcpsim/bp/BPPayloadTransmitted_m.h>
#include <dcpsim/bp/BPQueryNumberBufferedPayloads_m.h>
#include <dcpsim/vardis/VardisDBEntry.h>
#include <dcpsim/vardis/VardisRTDBConfirmation_m.h>
#include <dcpsim/vardis/VardisRTDBCreate_m.h>
#include <dcpsim/vardis/VardisRTDBDelete_m.h>
#include <dcpsim/vardis/VardisRTDBDescribeDatabase_m.h>
#include <dcpsim/vardis/VardisRTDBDescribeVariable_m.h>
#include <dcpsim/vardis/VardisRTDBRead_m.h>
#include <dcpsim/vardis/VardisRTDBUpdate_m.h>

using namespace omnetpp;
using namespace inet;

// ------------------------------------------------------------------------

namespace dcp {

/**
 * This module implements the VarDis (or Vardis) protocol as a BP client
 * protocol, generally (but not in all detail) following the VarDis
 * specification document. Broadly, it constructs outgoing Vardis payloads
 * and processes incoming Vardis payloads.
 */


class VardisProtocol : public BPClientProtocol
{
public:
    virtual ~VardisProtocol();

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

    virtual void handleMessage (cMessage* msg) override;

    virtual void registerAsBPClient(void) override;
    virtual bool handleBPRegisterProtocol_Confirm (BPRegisterProtocol_Confirm* pConf);

protected:

    // -------------------------------------------
    // data members
    // -------------------------------------------


    // Module parameters
    BPLengthT      vardisMaxValueLength;          // maximum length of a variable value in bytes
    BPLengthT      vardisMaxDescriptionLength;    // maximum length of variable description text in bytes
    unsigned int   vardisMaxRepetitions;          // maximum allowed repCnt for a variable
    unsigned int   vardisMaxSummaries;            // maximum number of summaries included in a payload
    double         vardisBufferCheckPeriod;       // how often is buffer occupancy of BP checked

    // Gate identifiers
    int   gidFromApplication, gidToApplication;

    // Timer self message for periodic buffer checks
    cMessage*     bufferCheckMsg;
    cMessage*     sendPayloadMsg;

    // the queues
    std::deque<VarIdT>    createQ;
    std::deque<VarIdT>    deleteQ;
    std::deque<VarIdT>    updateQ;
    std::deque<VarIdT>    summaryQ;
    std::deque<VarIdT>    reqUpdQ;
    std::deque<VarIdT>    reqCreateQ;

    // the current variable database
    std::map<VarIdT, DBEntry>  theVariableDatabase;

    // indicates whether Vardis is active or not
    bool   vardisActive = false;

    // other data members
    bool   payloadSent = false;

protected:


    // ---------------------------------------------------------------------
    // top-level message handlers
    // ---------------------------------------------------------------------

    /**
     * Queries buffer occupancy from BP, schedules next buffer check self-message
     */
    virtual void handleBufferCheckMsg ();

    /**
     * Triggers generation of a Vardis payload and hands it over to BP
     */
    virtual void handleSendPayloadMsg ();

    /**
     * Second-level message dispatcher for any message coming from Vardis
     * applications
     */
    virtual void handleApplicationMessage(cMessage* msg);

    /**
     * Second-level message dispatcher for any message coming from BP
     */
    virtual void handleBPMessage(cMessage* msg);


    // ---------------------------------------------------------------------
    // individual message handlers for messages from BP
    // ---------------------------------------------------------------------

    /**
     * Processes BPPayloadTransmitted.indication (checks outcome) and schedules next
     * payload generation time to happen shortly before BP generates next beacon
     */
    virtual void handleBPPayloadTransmittedIndication (BPPayloadTransmitted_Indication* ptInd);

    /**
     * Processes BPQueryNumberBufferedPayloads.confirm message, by which BP
     * reports buffer occupancy. If buffer is empty, trigger generation of
     * a Vardis payload for transmission
     */
    virtual void handleBPQueryNumberBufferedPayloadsConfirm (BPQueryNumberBufferedPayloads_Confirm* confMsg);

    /**
     * Processes BPReceivedPayload.indication message. Top-level method for
     * processing incoming payloads
     */
    virtual void handleBPReceivedPayloadIndication (BPReceivePayload_Indication* payload);


    // ---------------------------------------------------------------------
    // individual message handlers for messages from applications
    // ---------------------------------------------------------------------

    /**
     * Processes an incoming RTDBCreate.request message. Checks its validity,
     * creates entry into RTDB, adds variable for transmission of VarCreate
     * to neighbors and generates confirmation message for application.
     */
    virtual void handleRTDBCreateRequest (RTDBCreate_Request* createReq);

    /**
     * Processes an incoming RTDBUpdate.request message. Checks its validity,
     * updates RTDB entry, adds variable for transmission of VarUpdate
     * to neighbors, and generates confirmation message for application.
     */
    virtual void handleRTDBUpdateRequest (RTDBUpdate_Request* updateReq);

    /**
     * Processes an incoming RTDBRead.request message. Checks its validity,
     * retrieves variable variable and returns confirmation message to
     * application that includes this value.
     */
    virtual void handleRTDBReadRequest (RTDBRead_Request* readReq);

    /**
     * Processes an incoming RTDBDelete.request message. Checks its validity,
     * adds variable for transmission of VarDelete to neighbors, and generates
     * confirmation message for application
     */
    virtual void handleRTDBDeleteRequest (RTDBDelete_Request* delReq);


    /**
     * Processes RTDBDescribeDatabase.request message. Generates and sends
     * a confirmation that contains a short summary description for each
     * currently known variable in the RTDB
     */
    virtual void handleRTDBDescribeDatabaseRequest (RTDBDescribeDatabase_Request* descrDbReq);

    /**
     * Processes RTDBDescribeVariable.request message. Generates and sends
     * a confirmation that contains detailed information about the requested
     * variable (value and metadata)
     */
    virtual void handleRTDBDescribeVariableRequest (RTDBDescribeVariable_Request* descrVarReq);


    // ---------------------------------------------------------------------
    // Construction of outgoing Vardis payloads
    // ---------------------------------------------------------------------


    ///**
    // * The following helpers return the size of the various transmissible
    // * instruction records being included in instruction containers
    // */
    unsigned int instructionSizeVarCreate (VarIdT varId);
    unsigned int instructionSizeVarSummary (VarIdT varId);
    unsigned int instructionSizeVarUpdate (VarIdT varId);
    unsigned int instructionSizeVarDelete (VarIdT varId);
    unsigned int instructionSizeReqCreate (VarIdT varId);
    unsigned int instructionSizeReqUpdate (VarIdT varId);

    /**
     * Calculates how many records from the given queue can be
     * added to the remaining packet. This traverses the queue
     * at most once, without repetitions of records.
     * In this calculation the addition of an ICHeaderT is included.
     * There is also an assumption that there are no duplicates.
     * The result is capped to maxInstructionContainerRecords
     * */
    unsigned int numberFittingRecords(const std::deque<VarIdT>& queue,
                                      AssemblyArea& area,
                                      std::function<unsigned int (VarIdT)> instructionSizeFunction);


    /**
     * These helpers all add the various types of instruction records
     * to the outgoing byte vector (serialization), taking the DBEntry
     * for a variable and the current byte counters as input. They build
     * up the serialized output sequentially
     */
    void addVarCreate(VarIdT varId, DBEntry& theEntry, AssemblyArea& area);
    void addVarSummary(VarIdT varId, DBEntry& theEntry, AssemblyArea& area);
    void addVarUpdate(VarIdT varId, DBEntry& theEntry, AssemblyArea& area);
    void addVarReqUpdate(VarIdT varId, DBEntry& theEntry, AssemblyArea& area);
    void addVarDelete(VarIdT varId, AssemblyArea& area);
    void addVarReqCreate(VarIdT varId, AssemblyArea& area);
    void addICHeader(ICHeaderT icHdr, AssemblyArea& area);


    /**
     * These helpers construct complete instruction containers (ICHeader and a
     * list of individual instruction records of the right type), subject to
     * remaining space in payload and availability
     */
    void makeICTypeCreateVariables (AssemblyArea& area);
    void makeICTypeSummaries (AssemblyArea& area);
    void makeICTypeUpdates (AssemblyArea& area);
    void makeICTypeDeleteVariables (AssemblyArea& area);
    void makeICTypeRequestVarUpdates (AssemblyArea& area);
    void makeICTypeRequestVarCreates (AssemblyArea& area);


    /**
     * Almost top-level method to construct the overall Vardis payload
     * as a byte vector (called from 'generatePayload'). Inserts the
     * full instruction containers in the order given in the protocol
     * specification
     */
    void constructPayload (bytevect& bv);


    /**
     * This top-level method constructs a Vardis payload as a byte vector
     * and hands it over to BP for transmission.
     */
    void generatePayload();


    // ---------------------------------------------------------------------
    // Helpers for deconstructing and processing received packets
    // ---------------------------------------------------------------------

    /**
     * These methods process individual instruction records extracted
     * from instruction containers.
     */
    void processVarCreate(const VarCreateT& create);
    void processVarDelete(const VarDeleteT& del);
    void processVarUpdate(const VarUpdateT& update);
    void processVarSummary(const VarSummT& summ);
    void processVarReqUpdate(const VarReqUpdateT& requpd);
    void processVarReqCreate(const VarReqCreateT& reqcreate);


    /**
     * These methods iterate over the records of a received instruction
     * container (given as a list) and call the 'processX' methods to
     * process individual records
     */
    void processVarCreateList(const std::deque<VarCreateT>& creates);
    void processVarDeleteList(const std::deque<VarDeleteT>& deletes);
    void processVarUpdateList(const std::deque<VarUpdateT>& updates);
    void processVarSummaryList(const std::deque<VarSummT>& summs);
    void processVarReqUpdateList(const std::deque<VarReqUpdateT>& requpdates);
    void processVarReqCreateList(const std::deque<VarReqCreateT>& reqcreates);

    /**
     * These methods parse instruction containers from a received payload, perform
     * sanity checks, store the entries of an instruction container in a list and
     * call 'processXXList' to process the entries on that list
     */
    void extractVarCreateList(DisassemblyArea& area, std::deque<VarCreateT>& creates);
    void extractVarDeleteList(DisassemblyArea& area, std::deque<VarDeleteT>& deletes);
    void extractVarUpdateList(DisassemblyArea& area, std::deque<VarUpdateT>& updates);
    void extractVarSummaryList(DisassemblyArea& area, std::deque<VarSummT>& summs);
    void extractVarReqUpdateList(DisassemblyArea& area, std::deque<VarReqUpdateT>& requpdates);
    void extractVarReqCreateList(DisassemblyArea& area, std::deque<VarReqCreateT>& reqcreates);


    // ---------------------------------------------------------------------
    // Helpers for sending standard confirmations to higher layers
    // ---------------------------------------------------------------------

    /**
     * Sends the given confirmation with given return status to the indicated
     * application through INET message dispatcher
     */
    void sendConfirmation(VardisConfirmation *confMsg, DcpStatus status, Protocol* theProtocol);

    /**
     * Creates RTDBCreate.confirm message with given status and hands it over
     * to 'sendConfirmation' for sending to given protocol
     */
    void sendRTDBCreateConfirm (DcpStatus status, VarIdT varId, Protocol* theProtocol);

    /**
     * Creates RTDBUpdate.confirm message
     */
    void sendRTDBUpdateConfirm (DcpStatus status, VarIdT varId, Protocol* theProtocol);


    // ---------------------------------------------------------------------
    // Queue management helpers
    // ---------------------------------------------------------------------

    /**
     * Checks whether the given varId is in the given queue
     */
    bool isVarIdInQueue(const std::deque<VarIdT>& q, VarIdT varId);

    /**
     * Erases varId from the given queue, and raises an error if the
     * varId can then still be found
     */
    void removeVarIdFromQueue(std::deque<VarIdT>& q, VarIdT varId);

    /**
     * Drops from the given queue all varId's which do not exist in the
     * local RTDB or which exist but have been set to be deleted
     */
    void dropNonexistingDeleted(std::deque<VarIdT>& q);

    /**
     * Drops from the given queue all varId's which do not exist in the
     * local RTDB
     */
    void dropNonexisting(std::deque<VarIdT>& q);

    /**
     * Drops from the given queue all varId's which do exist in the RTDB
     * and which have been set to be deleted
     */
    void dropDeleted(std::deque<VarIdT>& q);

    // ---------------------------------------------------------------------
    // Miscellaneous helpers
    // ---------------------------------------------------------------------

    /**
     * Retrieves the sender protocol from an incoming message, throws error if
     * none is found
     */
    Protocol* fetchSenderProtocol(Message* message);

    /**
     * Checks if the given variable exists in the RTDB
     */
    bool variableExists(VarIdT varId);

    /**
     * Checks if this node is itself the producer of the given variable
     */
    bool producerIsMe(VarIdT varId);


    // ---------------------------------------------------------------------
    // Debug helpers
    // ---------------------------------------------------------------------

    /**
     * These methods log various pieces of information
     */
    void dbg_queueSizes();
    void dbg_summaryQ();
    void dbg_createQ();
    void dbg_updateQ();
    void dbg_reqCreateQ();
    void dbg_reqUpdateQ();
    void dbg_database();
    void dbg_database_complete();
    void dbg_comprehensive(std::string methname);

    /**
     * These perform consistency checks: for each variable in the
     * indicated queue it is checked whether the variable is indeed
     * contained in the RTDB and whether the relevant counter is
     * larger than zero.
     */
    void assert_createQ();
    void assert_updateQ();

    /**
     * Consistency checks for three key queues: createQ and updateQ
     */
    void assert_queues();

};

} // namespace

