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


#include <algorithm>
#include <format>
#include <inet/common/IProtocolRegistrationListener.h>
#include <dcp/bp/bp_queueing_mode.h>
#include <dcp/bp/bp_status.h>
#include <dcp/vardis/vardis_payload_handling.h>
#include <dcp/vardis/vardis_protocol_name.h>
#include <dcpsim/bp/BPTransmitPayload_m.h>
#include <dcpsim/bp/BPPayloadTransmitted_m.h>
#include <dcpsim/common/DcpSimGlobals.h>
#include <dcpsim/vardis/VardisProtocol.h>

// ========================================================================================
// ========================================================================================

using namespace omnetpp;
using namespace inet;
using namespace dcp;
using namespace dcp::vardis;
using dcp::bp::BPQueueingMode;
using dcp::bp::BPStatus;

Define_Module(VardisProtocol);

// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================


void VardisProtocol::initialize(int stage)
{
    BPClientProtocol::initialize(stage);

    if (stage == INITSTAGE_LAST)
    {
        dbg_setModuleName("VarDis");
        EvTracing dbg (*this, "initialize");

        // read and check parameters
        vardisMaxValueLength        = (size_t) par("vardisMaxValueLength");
        vardisMaxDescriptionLength  = (size_t) par("vardisMaxDescriptionLength");
        vardisMaxRepetitions        = (uint8_t) par("vardisMaxRepetitions");
        vardisMaxSummaries          = (uint8_t) par("vardisMaxSummaries");
        vardisBufferCheckPeriod     = par("vardisBufferCheckPeriod");

        // sanity-check parameters
        assert(maxPayloadSize > 0);
        assert(maxPayloadSize <= 1400);   // this deviates from specification (would require config data from BP)
        assert(vardisMaxValueLength > 0);
        assert(vardisMaxValueLength <= std::min((size_t) VarLenT::max_val(), maxPayloadSize.val - ICHeaderT::fixed_size()));
        assert(vardisMaxDescriptionLength > 0);
        assert(vardisMaxDescriptionLength <=   maxPayloadSize
                                             - (   ICHeaderT::fixed_size()
                                                 + VarSpecT::fixed_size()
                                                 + VarUpdateT::fixed_size()
                                                 + vardisMaxValueLength
                                                 ));
        assert(vardisMaxRepetitions > 0);
        assert(vardisMaxRepetitions <= 15);
        assert(vardisMaxSummaries <= (maxPayloadSize.val - ICHeaderT::fixed_size())/VarSummT::fixed_size());
        assert(vardisBufferCheckPeriod > 0);

        // find gate identifiers
        gidFromApplication =  findGate("fromApplication");
        gidToApplication   =  findGate("toApplication");

        // register ourselves as BP client protocol with dispatcher
        registerProtocol(*DcpSimGlobals::protocolDcpVardis, gate("toBP"), gate("fromBP"));

        // and register ourselves as a service for Vardis client protocols
        registerService(*DcpSimGlobals::protocolDcpVardis, gate("fromApplication"), gate("toApplication"));

        // get generation timer ticks going
        bufferCheckMsg = new cMessage("vardisBufferCheckMsg");
        scheduleAt(simTime() + vardisBufferCheckPeriod, bufferCheckMsg);

        sendPayloadMsg = new cMessage ("vardisSendPayloadMsg");
        payloadSent = false;

    }
}

// ----------------------------------------------------

/**
 * Top-level dispatcher for incoming messages
 */
void VardisProtocol::handleMessage(cMessage *msg)
{
    EvTracing dbg (*this, "handleMessage");
    dbg.text("---------------------------------------------------------");

    // check if BP has already dealt with this message (e.g. registration as BP client protocol)
    if (hasHandledMessageBPClient(msg))
    {
        dbg.text("hasHandledMessageBPClient did the job");
        return;
    }

    // dispatch genuine message to VarDis

    if (msg->arrivedOn(gidFromApplication))
    {
        handleApplicationMessage(msg);
        return;
    }

    if (msg->arrivedOn(gidFromBP))
    {
        handleBPMessage(msg);
        return;
    }

    if (msg == bufferCheckMsg)
    {
        dbg.text("handling bufferCheckMsg");
        handleBufferCheckMsg();
        return;
    }

    if (msg == sendPayloadMsg)
    {
        dbg.text("handling sendPayloadMsg");
        generatePayload();
        return;
    }

    error("VardisProtocol::handleMessage: unknown message");
}

// ----------------------------------------------------


void VardisProtocol::registerAsBPClient()
{
    EvTracing dbg(*this, "registerAsBPClient");

    dbg.val("maxPayloadSize", maxPayloadSize);

    sendRegisterProtocolRequest(BP_PROTID_VARDIS,
				dcp::vardis::get_protocol_name(),
				maxPayloadSize,
				BPQueueingMode::Q_DropHead,
				false,
				10);

}

// ----------------------------------------------------


bool VardisProtocol::handleBPRegisterProtocol_Confirm (BPRegisterProtocol_Confirm* pConf)
{
    EvTracing dbg(*this, "handleBPRegisterProtocol_Confirm");

    assert (pConf);

    if (static_cast<BPStatus>(pConf->getStatus()) == BPStatus::Ok)
    {
      vardis_store_p = std::make_unique<VardisVariableStoreInMemory> (true,
								      vardisMaxSummaries,
								      vardisMaxDescriptionLength,
								      vardisMaxValueLength,
								      vardisMaxRepetitions,
								      getOwnNodeId());
      vardis_store_p->set_vardis_isactive (true);
      vardis_protocol_data_p = std::make_unique<VardisProtocolData> (*vardis_store_p);
    }
    else
    {
        error("VardisProtocol::handleBPRegisterProtocol_Confirm: unexpected BP_STATUS value");
    }

    return BPClientProtocol::handleBPRegisterProtocol_Confirm (pConf);
}



// ----------------------------------------------------


VardisProtocol::~VardisProtocol()
{
    cancelAndDelete(bufferCheckMsg);
    cancelAndDelete(sendPayloadMsg);
}


// ========================================================================================
// Second-level message dispatchers
// ========================================================================================

/**
 * Second-level message dispatcher for all messages from VarDis applications
 * (which are service requests)
 */
void VardisProtocol::handleApplicationMessage (cMessage* msg)
{
    EvTracing dbg(*this, "handleApplicationMessage");

    if (dynamic_cast<RTDBUpdate_Request*>(msg))
    {
        handleRTDBUpdateRequest((RTDBUpdate_Request*) msg);
        return;
    }

    if (dynamic_cast<RTDBRead_Request*>(msg))
    {
        handleRTDBReadRequest((RTDBRead_Request*) msg);
        return;
    }

    if (dynamic_cast<RTDBCreate_Request*>(msg))
    {
        handleRTDBCreateRequest((RTDBCreate_Request*) msg);
        return;
    }

    if (dynamic_cast<RTDBDelete_Request*>(msg))
    {
        handleRTDBDeleteRequest((RTDBDelete_Request*) msg);
        return;
    }

    if (dynamic_cast<RTDBDescribeDatabase_Request*>(msg))
    {
        handleRTDBDescribeDatabaseRequest((RTDBDescribeDatabase_Request*) msg);
        return;
    }

    if (dynamic_cast<RTDBDescribeVariable_Request*>(msg))
    {
        handleRTDBDescribeVariableRequest((RTDBDescribeVariable_Request*) msg);
        return;
    }

    error("VardisProtocol::handleApplicationMessage: unknown message");
}

// ----------------------------------------------------

/**
 * Second-level message dispatcher for any message coming from the BP
 * (other than those related to registering VarDis as a client protocol
 * to the BP)
 */
void VardisProtocol::handleBPMessage (cMessage* msg)
{
    EvTracing dbg(*this, "handleBPMessage");

    if (dynamic_cast<BPPayloadTransmitted_Indication*>(msg))
    {
        handleBPPayloadTransmittedIndication((BPPayloadTransmitted_Indication*) msg);
        return;
    }

    if (dynamic_cast<BPReceivePayload_Indication*>(msg))
    {
        handleBPReceivedPayloadIndication((BPReceivePayload_Indication*) msg);
        return;
    }

    if (dynamic_cast<BPQueryNumberBufferedPayloads_Confirm*>(msg))
    {
        handleBPQueryNumberBufferedPayloadsConfirm((BPQueryNumberBufferedPayloads_Confirm*) msg);
        return;
    }

    error("VardisProtocol::handleBPMsg: unknown message");
}


// ========================================================================================
// Message handlers for self-messages
// ========================================================================================

/**
 * Periodically query occupancy of buffer in BP
 */
void VardisProtocol::handleBufferCheckMsg()
{
    EvTracing dbg(*this, "handleBufferCheckMsg");

    // schedule next buffer check
    scheduleAt(simTime() + vardisBufferCheckPeriod, bufferCheckMsg);

    // query number of buffered payloads from BP
    BPQueryNumberBufferedPayloads_Request  *qbpReq = new BPQueryNumberBufferedPayloads_Request;
    qbpReq->setProtId(BP_PROTID_VARDIS.val);
    sendToBP(qbpReq);
}


// ========================================================================================
// Message handlers for BP messages
// ========================================================================================

/**
 * Processes BPPayloadTransmitted.indication primitive, schedules next point
 * in time to generate next VarDis payload (shortly before BP generates its
 * next beacon)
 */
void VardisProtocol::handleBPPayloadTransmittedIndication (BPPayloadTransmitted_Indication* ptInd)
{
    EvTracing dbg(*this, "handleBPPayloadTransmittedIndication");
    assert(ptInd);
    assert(ptInd->getProtId() == BP_PROTID_VARDIS);

    simtime_t nextBeaconTransmissionEpoch = ptInd->getNextBeaconGenerationEpoch();
    simtime_t generationDelay = nextBeaconTransmissionEpoch - simTime();
    assert(generationDelay > 0);
    delete ptInd;
    generationDelay = std::max (generationDelay * 0.99, generationDelay - 0.001);
    payloadSent = false;
    scheduleAt(simTime() + generationDelay, sendPayloadMsg);

    dbg.val("generationDelay", generationDelay);
}

// ----------------------------------------------------

/**
 * Process BPQueryNumberBufferedPayloads.confirm message. If no payload is buffered
 * and we have not already scheduled a self-message to generate a payload, we now
 * generate a payload and hand it over to BP
 */
void VardisProtocol::handleBPQueryNumberBufferedPayloadsConfirm (BPQueryNumberBufferedPayloads_Confirm* confMsg)
{
    EvTracing dbg(*this, "handleBPQueryNumberBufferedPayloadsConfirm");
    assert(confMsg);
    assert(confMsg->getProtId() == BP_PROTID_VARDIS);
    assert(confMsg->getNumberBuffered() >= 0);
    dbg.val("confMsg->getNumberBuffered", confMsg->getNumberBuffered());

    if ((confMsg->getNumberBuffered() == 0) && (not payloadSent) && (not sendPayloadMsg->isScheduled()))
    {
        dbg.text("triggering transmission of new payload");
        generatePayload();
    }
    delete confMsg;
}

// ----------------------------------------------------


/**
 * Process BPReceivedPayload.indication message. The received VarDis payload
 * is checked/parsed and deconstructed.
 */
void VardisProtocol::handleBPReceivedPayloadIndication(BPReceivePayload_Indication* payload)
{
    EvTracing dbg(*this, "handleBPReceivedPayloadIndication");
    assert(payload);
    assert(payload->getProtId() == BP_PROTID_VARDIS);

    if (not vardisActive())
    {
        dbg.text ("vardis is not active");
        delete payload;
        return;
    }

    const bytevect& bvpayload = payload->getPayload();
    ByteVectorDisassemblyArea area ("vardis-handleBPReceivedPayloadIndication", bvpayload);
    dbg.val("bvpayload.size", bvpayload.size());

    process_received_payload (*vardis_protocol_data_p, area);
    
    delete payload;
}


// ========================================================================================
// Message handlers for higher-layer requests
// ========================================================================================

/**
 * Handles RTDBCreate.request service request to create a new variable in
 * the RTDB. Performs sanity checks, adds new variable to local RTDB and
 * schedules transmission of suitable instruction records in beacons.
 */
void VardisProtocol::handleRTDBCreateRequest(RTDBCreate_Request* createReq)
{
    EvTracing dbg(*this, "handleRTDBCreateRequest");
    assert(createReq);

    // keep a reference to the client protocol sending this, required for
    // sending a confirmation message back to the client protocol
    Protocol* theProtocol = fetchSenderProtocol(createReq);

    RTDB_Create_Request crReq  = createReq->getCrReq();
    RTDB_Create_Confirm crConf = vardis_protocol_data_p->handle_rtdb_create_request (crReq);
    
    // send confirmation to application and delete request
    sendRTDBCreateConfirm(static_cast<VardisStatus>(crConf.status_code),
                          crReq.spec.varId,
                          theProtocol);
    delete createReq;
}

// ----------------------------------------------------

/**
 * Handles RTDBUpdate.request service request to update a variable in
 * the RTDB. Performs sanity checks, updates variable with new value
 * in local RTDB and schedules transmission of suitable information
 * records in beacons.
 */
void VardisProtocol::handleRTDBUpdateRequest(RTDBUpdate_Request* updateReq)
{
    EvTracing dbg(*this, "handleRTDBUpdateRequest");
    assert(updateReq);

    // keep a reference to the client protocol sending this, required for
    // sending a confirmation message back to the client protocol
    Protocol* theProtocol = fetchSenderProtocol(updateReq);

    RTDB_Update_Request updReq  = updateReq->getUpdReq();
    RTDB_Update_Confirm updConf = vardis_protocol_data_p->handle_rtdb_update_request (updReq);
    

    // send confirmation to application and delete request
    sendRTDBUpdateConfirm(static_cast<VardisStatus>(updConf.status_code),
                          updReq.varId,
                          theProtocol);

    delete updateReq;
}

// ----------------------------------------------------

/**
 * Handles RTDBRead.request service request to read variable value
 * from local RTDB. Performs sanity checks, retrieves and returns
 * current value.
 */
void VardisProtocol::handleRTDBReadRequest (RTDBRead_Request* readReq)
{
    EvTracing dbg(*this, "handleRTDBReadRequest");
    assert(readReq);

    // keep a reference to the client protocol sending this, required for
    // sending a confirmation message back to the client protocol
    Protocol* theProtocol = fetchSenderProtocol(readReq);

    RTDB_Read_Request rReq  = readReq->getReadReq ();
    RTDB_Read_Confirm rConf = vardis_protocol_data_p->handle_rtdb_read_request (rReq);
    
    delete readReq;

    // generate and initialize confirmation
    auto readConf = new RTDBRead_Confirm;
    readConf->setReadConf (rConf);

    sendConfirmation(readConf,
                     static_cast<VardisStatus>(rConf.status_code),
                     theProtocol);
}

// ----------------------------------------------------

/**
 * Handles RTDBDescribeDatabase.request service request to return
 * descriptions of all currently known variables to an application.
 * Performs sanity checks, retrieves and returns descriptions.
 */
void VardisProtocol::handleRTDBDescribeDatabaseRequest (RTDBDescribeDatabase_Request* descrDbReq)
{
    EvTracing dbg(*this, "handleRTDBDescribeDatabaseRequest");
    assert(descrDbReq);

    // keep a reference to the client protocol sending this, required for
    // sending a confirmation message back to the client protocol
    Protocol* theProtocol = fetchSenderProtocol(descrDbReq);
    delete descrDbReq;

    auto dbConf = new RTDBDescribeDatabase_Confirm;
    dbConf->setDescrsArraySize(vardis_store_p->get_number_variables());

    // check whether Vardis protocol is actually active
    if (not vardisActive())
    {
        dbg.text("Vardis is not active, dropping request");
        dbConf->setDescrsArraySize (0);
        sendConfirmation(dbConf, VardisStatus::Inactive, theProtocol);
        return;
    }


    // copy information about each variable into the confirmation message

   VardisProtocolData& PD = *vardis_protocol_data_p;

   int i=0;
   for (auto varId : PD.active_variables)
     {
       DBEntry& db_entry = vardis_store_p->get_db_entry_ref (varId);
       DescribeDatabaseVariableDescription descr;

       dbg.val("adding description, db_entry.varId", db_entry.varId, "db_entry.prodId", db_entry.prodId);
       
       descr.varId          =  db_entry.varId;
       descr.prodId         =  db_entry.prodId;
       descr.repCnt         =  db_entry.repCnt;
       descr.tStamp         =  db_entry.tStamp;
       descr.isDeleted      =  db_entry.isDeleted;
       PD.vardis_store.read_description (varId, descr.description);

       dbConf->setDescrs(i,descr);
       i++;	
      }    

    sendConfirmation(dbConf, VardisStatus::Ok, theProtocol);
}

// ----------------------------------------------------

/**
 * Handles RTDBDescribeVariable.request service request to return
 * the current value, description and meta-data for one specific
 * variable in the local RTDB.
 * Performs sanity checks, retrieves and returns variable data.
 */
void VardisProtocol::handleRTDBDescribeVariableRequest (RTDBDescribeVariable_Request* descrVarReq)
{
    EvTracing dbg(*this, "handleRTDBDescribeVariableRequest");
    assert(descrVarReq);

    // keep a reference to the client protocol sending this, required for
    // sending a confirmation message back to the client protocol
    Protocol* theProtocol = fetchSenderProtocol(descrVarReq);

    VarIdT    varId       = descrVarReq->getVarId();
    delete descrVarReq;
    auto varDescr = new RTDBDescribeVariable_Confirm;

    // perform some checks

    if (not vardisActive())
    {
        dbg.text("Vardis is not active, dropping request");
        sendConfirmation(varDescr, VardisStatus::Inactive, theProtocol);
        return;
    }

    if (not variableExists(varId))
    {
        dbg.val("requested variable does not exist", varId);
        sendConfirmation(varDescr, VardisStatus::Variable_Does_Not_Exist, theProtocol);
        return;
    }

    dbg.val("generating description for variable", varId);

    // retrieve variable and generate response data about it
    DBEntry& theEntry = vardis_store_p->get_db_entry_ref(varId);
    varDescr->setVarId (varId.val);
    varDescr->setProdId (theEntry.prodId);
    varDescr->setRepCnt (theEntry.repCnt.val);
    varDescr->setValue (vardis_store_p->read_value (varId));
    varDescr->setDescr (vardis_store_p->read_description (varId));
    varDescr->setSeqno (theEntry.seqno.val);
    varDescr->setTstamp (theEntry.tStamp);
    varDescr->setCountUpdate (theEntry.countUpdate);
    varDescr->setCountCreate (theEntry.countCreate);
    varDescr->setCountDelete (theEntry.countDelete);
    varDescr->setIsDeleted (theEntry.isDeleted);

    sendConfirmation(varDescr, VardisStatus::Ok, theProtocol);
}


// ----------------------------------------------------

/**
 * Handles RTDBDelete.request service request to delete a variable
 * from the RTDB. Performs sanity checks, modifies variable state
 * to be in the to-be-deleted state and schedules transmission of
 * suitable instruction records.
 */
void VardisProtocol::handleRTDBDeleteRequest (RTDBDelete_Request* deleteReq)
{

    EvTracing dbg(*this, "handleRTDBDeleteRequest");
    assert(deleteReq);

    // keep a reference to the client protocol sending this, required for
    // sending a confirmation message back to the client protocol
    Protocol* theProtocol = fetchSenderProtocol(deleteReq);

    RTDB_Delete_Request delReq  = deleteReq->getDelReq();
    RTDB_Delete_Confirm delConf = vardis_protocol_data_p->handle_rtdb_delete_request (delReq);
    
    delete deleteReq;

    // generate and initialize confirmation
    auto deleteConf = new RTDBDelete_Confirm;

    sendConfirmation(deleteConf,
                     static_cast<VardisStatus>(delConf.status_code),
                     theProtocol);
}




// ========================================================================================
// Construction of instruction containers for outgoing packets
// ========================================================================================


// ----------------------------------------------------

/**
 * Constructs a Vardis payload for BP by adding instruction containers in the
 * specified order.
 */
void VardisProtocol::constructPayload(bytevect& bv, unsigned int& containers_added)
{
    EvTracing dbg(*this, "constructPayload");
    ByteVectorAssemblyArea area ("vardis-constructPayload", maxPayloadSize.val, bv);
    construct_payload (*vardis_protocol_data_p, area, containers_added);
    bv.resize (area.used());
}

// ----------------------------------------------------

/**
 * Checks whether we can generate a Vardis payload. Generates the payload
 * and sends it to the BP for transmission.
 */
void VardisProtocol::generatePayload ()
{
    EvTracing dbg(*this, "generatePayload");

    if (vardisActive())
    {
        dbg.text("we are active");

        BPTransmitPayload_Request  *pldReq = new BPTransmitPayload_Request ("VardisPayload");
        bytevect& bv = pldReq->getBvdataForUpdate();
        bv.resize (maxPayloadSize.val);
        bv.reserve (2*maxPayloadSize.val);

	unsigned int containers_added = 0;
        constructPayload(bv, containers_added);

        if (containers_added > 0)
        {
            dbg.val("SENDING payload of size", bv.size());

            dbg.text("constructing the packet");
            pldReq->setProtId(BP_PROTID_VARDIS.val);

            dbg.text("sending the packet/payload to BP");
            sendToBP(pldReq);

            payloadSent = true;
        }
        else
        {
            delete pldReq;
        }
    }
}



// ========================================================================================
// Helpers for sending standard confirmations to higher layers
// ========================================================================================

void VardisProtocol::sendConfirmation(VardisConfirmation *confMsg, VardisStatus status, Protocol* theProtocol)
{
    EvTracing dbg(*this, "sendConfirmation");
    assert(theProtocol);
    assert(confMsg);

    confMsg->setStatus(static_cast<uint16_t>(status));

    auto req = confMsg->addTagIfAbsent<DispatchProtocolReq>();
    req->setProtocol(theProtocol);
    req->setServicePrimitive(SP_INDICATION);

    send(confMsg, gidToApplication);
}

// ----------------------------------------------------

void VardisProtocol::sendRTDBCreateConfirm(VardisStatus status, VarIdT varId, Protocol* theProtocol)
{
    EvTracing dbg(*this, "sendRTDBCreateConfirm");

    auto conf = new RTDBCreate_Confirm;
    conf->setVarId(varId.val);
    sendConfirmation(conf, status, theProtocol);
}

// ----------------------------------------------------

void VardisProtocol::sendRTDBUpdateConfirm(VardisStatus status, VarIdT varId, Protocol* theProtocol)
{
    EvTracing dbg(*this, "sendRTDBUpdateConfirm");

    auto conf = new RTDBUpdate_Confirm;
    conf->setVarId(varId.val);
    sendConfirmation(conf, status, theProtocol);
}

// ========================================================================================
// Miscellaneous helpers
// ========================================================================================

// ----------------------------------------------------

/**
 * Retrieves a pointer to the application protocol that sent a message
 * via the message dispatcher, so we know where to send a confirmation
 */
Protocol* VardisProtocol::fetchSenderProtocol(Message* message)
{
    EvTracing dbg(*this, "fetchSenderProtocol");
    assert(message);

    auto protTag = message->removeTag<DispatchProtocolInd>();
    Protocol* theProtocol = (Protocol*) protTag->getProtocol();
    assert(theProtocol);

    dbg.val("theProtocol->getId", theProtocol->getId(), "theProtocol->getDescriptiveName", theProtocol->getDescriptiveName());

    return theProtocol;
}

// ----------------------------------------------------

bool VardisProtocol::variableExists(VarIdT varId)
{
  return vardis_store_p->identifier_is_allocated (varId);
}

// ----------------------------------------------------

bool VardisProtocol::producerIsMe(VarIdT varId)
{
    DBEntry& theEntry = vardis_store_p->get_db_entry_ref (varId);

    return theEntry.prodId == getOwnNodeId();
}



