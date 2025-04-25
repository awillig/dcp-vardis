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
#include <inet/common/IProtocolRegistrationListener.h>
#include <dcp/bp/bp_queueing_mode.h>
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

using dcp::bp::BP_QMODE_QUEUE_DROPHEAD;

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
        dbg_enter("initialize");

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

        dbg_leave();
    }
}

// ----------------------------------------------------

/**
 * Top-level dispatcher for incoming messages
 */
void VardisProtocol::handleMessage(cMessage *msg)
{
    dbg_assertToplevel();
    dbg_enter("handleMessage");
    dbg_string("---------------------------------------------------------");

    // check if BP has already dealt with this message (e.g. registration as BP client protocol)
    if (hasHandledMessageBPClient(msg))
    {
        dbg_string("hasHandledMessageBPClient did the job");
        dbg_leave();
        return;
    }

    // dispatch genuine message to VarDis

    if (msg->arrivedOn(gidFromApplication))
    {
        handleApplicationMessage(msg);
        dbg_leave();
        return;
    }

    if (msg->arrivedOn(gidFromBP))
    {
        handleBPMessage(msg);
        dbg_leave();
        return;
    }

    if (msg == bufferCheckMsg)
    {
        dbg_string("handling bufferCheckMsg");
        handleBufferCheckMsg();
        dbg_leave();
        return;
    }

    if (msg == sendPayloadMsg)
    {
        dbg_string("handling sendPayloadMsg");
        handleSendPayloadMsg();
        dbg_leave();
        return;
    }

    error("VardisProtocol::handleMessage: unknown message");

    dbg_leave();
}

// ----------------------------------------------------


void VardisProtocol::registerAsBPClient()
{
    dbg_enter("registerAsBPClient");

    DBG_VAR1(maxPayloadSize);

    sendRegisterProtocolRequest(BP_PROTID_VARDIS, "VarDis -- Variable Dissemination Protocol V1.3", maxPayloadSize, BP_QMODE_QUEUE_DROPHEAD, false, 10);

    dbg_leave();
}

// ----------------------------------------------------


bool VardisProtocol::handleBPRegisterProtocol_Confirm (BPRegisterProtocol_Confirm* pConf)
{
    dbg_enter("handleBPRegisterProtocol_Confirm");

    assert (pConf);

    if (pConf->getStatus() == BP_STATUS_OK)
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

    dbg_leave();
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
    dbg_enter("handleApplicationMessage");

    if (dynamic_cast<RTDBUpdate_Request*>(msg))
    {
        dbg_string("handling RTDBUpdate_Request");
        RTDBUpdate_Request* updateReq = (RTDBUpdate_Request*) msg;
        handleRTDBUpdateRequest(updateReq);
        dbg_leave();
        return;
    }

    if (dynamic_cast<RTDBRead_Request*>(msg))
    {
        dbg_string("handling RTDBRead_Request");
        RTDBRead_Request* readReq = (RTDBRead_Request*) msg;
        handleRTDBReadRequest(readReq);
        dbg_leave();
        return;
    }

    if (dynamic_cast<RTDBCreate_Request*>(msg))
    {
        dbg_string("handling RTDBCreate_Request");
        RTDBCreate_Request* createReq = (RTDBCreate_Request*) msg;
        handleRTDBCreateRequest(createReq);
        dbg_leave();
        return;
    }

    if (dynamic_cast<RTDBDelete_Request*>(msg))
    {
        dbg_string("handling RTDBDelete_Request");
        RTDBDelete_Request* deleteReq = (RTDBDelete_Request*) msg;
        handleRTDBDeleteRequest(deleteReq);
        dbg_leave();
        return;
    }

    if (dynamic_cast<RTDBDescribeDatabase_Request*>(msg))
    {
        dbg_string("handling RTDBDescribeDatabase_Request");
        RTDBDescribeDatabase_Request* descrDbReq = (RTDBDescribeDatabase_Request*) msg;
        handleRTDBDescribeDatabaseRequest(descrDbReq);
        dbg_leave();
        return;
    }

    if (dynamic_cast<RTDBDescribeVariable_Request*>(msg))
    {
        dbg_string("handling RTDBDescribeVariable_Request");
        RTDBDescribeVariable_Request* descrVarReq = (RTDBDescribeVariable_Request*) msg;
        handleRTDBDescribeVariableRequest(descrVarReq);
        dbg_leave();
        return;
    }

    error("VardisProtocol::handleApplicationMessage: unknown message");

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Second-level message dispatcher for any message coming from the BP
 * (other than those related to registering VarDis as a client protocol
 * to the BP)
 */
void VardisProtocol::handleBPMessage (cMessage* msg)
{
    dbg_enter("handleBPMessage");

    if (dynamic_cast<BPPayloadTransmitted_Indication*>(msg))
    {
        dbg_string("handling BPPayloadTransmitted_Indication");
        BPPayloadTransmitted_Indication* ptInd = (BPPayloadTransmitted_Indication*) msg;
        handleBPPayloadTransmittedIndication(ptInd);
        dbg_leave();
        return;
    }

    if (dynamic_cast<BPReceivePayload_Indication*>(msg))
    {
        dbg_string("handling BPReceivePayload_Indication");
        BPReceivePayload_Indication *payload = (BPReceivePayload_Indication*) msg;
        handleBPReceivedPayloadIndication(payload);
        dbg_leave();
        return;
    }

    if (dynamic_cast<BPQueryNumberBufferedPayloads_Confirm*>(msg))
    {
        dbg_string("handling BPQueryNumberBufferedPayloads_Confirm");
        BPQueryNumberBufferedPayloads_Confirm* confMsg = (BPQueryNumberBufferedPayloads_Confirm*) msg;
        handleBPQueryNumberBufferedPayloadsConfirm(confMsg);
        dbg_leave();
        return;
    }

    error("VardisProtocol::handleBPMsg: unknown message");

    dbg_leave();
}


// ========================================================================================
// Message handlers for self-messages
// ========================================================================================

/**
 * Periodically query occupancy of buffer in BP
 */
void VardisProtocol::handleBufferCheckMsg()
{
    dbg_enter("handleBufferCheckMsg");

    // schedule next buffer check
    scheduleAt(simTime() + vardisBufferCheckPeriod, bufferCheckMsg);

    // query number of buffered payloads from BP
    BPQueryNumberBufferedPayloads_Request  *qbpReq = new BPQueryNumberBufferedPayloads_Request;
    qbpReq->setProtId(BP_PROTID_VARDIS.val);
    sendToBP(qbpReq);

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Generate a payload and submit it to BP
 */
void VardisProtocol::handleSendPayloadMsg()
{
    dbg_enter("handleSendPayloadMsg");
    generatePayload();
    dbg_leave();
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
    dbg_enter("handleBPPayloadTransmittedIndication");
    assert(ptInd);
    assert(ptInd->getProtId() == BP_PROTID_VARDIS);

    simtime_t nextBeaconTransmissionEpoch = ptInd->getNextBeaconGenerationEpoch();
    simtime_t generationDelay = nextBeaconTransmissionEpoch - simTime();
    assert(generationDelay > 0);
    delete ptInd;
    generationDelay = std::max (generationDelay * 0.99, generationDelay - 0.001);
    payloadSent = false;
    scheduleAt(simTime() + generationDelay, sendPayloadMsg);

    DBG_VAR1(generationDelay);
    dbg_leave();
}

// ----------------------------------------------------

/**
 * Process BPQueryNumberBufferedPayloads.confirm message. If no payload is buffered
 * and we have not already scheduled a self-message to generate a payload, we now
 * generate a payload and hand it over to BP
 */
void VardisProtocol::handleBPQueryNumberBufferedPayloadsConfirm (BPQueryNumberBufferedPayloads_Confirm* confMsg)
{
    dbg_enter("handleBPQueryNumberBufferedPayloadsConfirm");
    assert(confMsg);
    assert(confMsg->getProtId() == BP_PROTID_VARDIS);
    assert(confMsg->getNumberBuffered() >= 0);
    DBG_VAR1(confMsg->getNumberBuffered());

    if ((confMsg->getNumberBuffered() == 0) && (not payloadSent) && (not sendPayloadMsg->isScheduled()))
    {
        dbg_string("triggering transmission of new payload");
        generatePayload();
    }
    delete confMsg;

    dbg_leave();
}

// ----------------------------------------------------


/**
 * Process BPReceivedPayload.indication message. The received VarDis payload
 * is checked/parsed and deconstructed.
 */
void VardisProtocol::handleBPReceivedPayloadIndication(BPReceivePayload_Indication* payload)
{
    dbg_enter("handleBPReceivedPayloadIndication");
    assert(payload);
    assert(payload->getProtId() == BP_PROTID_VARDIS);

    if (not vardisActive())
    {
        dbg_string ("vardis is not active");
        delete payload;
        dbg_leave();
        return;
    }

    const bytevect& bvpayload = payload->getPayload();

    DBG_VAR1(bvpayload.size());

    // In the first step we deconstruct the packet and put all the
    // instruction records into their own lists without yet processing
    // them. We then process them later on in the specified order

    std::deque<VarSummT>       icSummaries;
    std::deque<VarUpdateT>     icUpdates;
    std::deque<VarReqUpdateT>  icRequestVarUpdates;
    std::deque<VarReqCreateT>  icRequestVarCreates;
    std::deque<VarCreateT>     icCreateVariables;
    std::deque<VarDeleteT>     icDeleteVariables;

    ByteVectorDisassemblyArea area ("vardis-handleBPReceivedPayloadIndication", bvpayload);

    // Dispatch on InstructionContainerT
    while (area.used() < area.available())
    {
	ICHeaderT icHeader;
	icHeader.deserialize(area);
	
        DBG_PVAR2 ("Deserializing: considering ICType", (int) area.used(), (int) icHeader.icType.val);
        switch(icHeader.icType.val)
        {
        case ICTYPE_SUMMARIES:
            dbg_string("considering ICTYPE_SUMMARIES");
            extractInstructionContainerElements<VarSummT> (area, icHeader, icSummaries);
            break;
        case ICTYPE_UPDATES:
            dbg_string("considering ICTYPE_UPDATES");
            extractInstructionContainerElements<VarUpdateT> (area, icHeader, icUpdates);
            break;
        case ICTYPE_REQUEST_VARUPDATES:
            dbg_string("considering ICTYPE_REQUEST_VARUPDATES");
            extractInstructionContainerElements<VarReqUpdateT> (area, icHeader, icRequestVarUpdates);
            break;
        case ICTYPE_REQUEST_VARCREATES:
            dbg_string("considering ICTYPE_REQUEST_VARCREATES");
            extractInstructionContainerElements<VarReqCreateT> (area, icHeader, icRequestVarCreates);
            break;
        case ICTYPE_CREATE_VARIABLES:
            dbg_string("considering ICTYPE_CREATE_VARIABLES");
            extractInstructionContainerElements<VarCreateT> (area, icHeader, icCreateVariables);
            break;
        case ICTYPE_DELETE_VARIABLES:
            dbg_string("considering ICTYPE_DELETE_VARIABLES");
            extractInstructionContainerElements<VarDeleteT> (area, icHeader, icDeleteVariables);
            break;
        default:
            error("VardisProtocol::handleReceivedPayload: unknown ICType");
            break;
        }
    }

    // Now process the received containers in the specified order
    // (database updates)
    processVarCreateList(icCreateVariables);
    processVarDeleteList(icDeleteVariables);
    processVarUpdateList(icUpdates);
    processVarSummaryList(icSummaries);
    processVarReqUpdateList(icRequestVarUpdates);
    processVarReqCreateList(icRequestVarCreates);

    delete payload;

    dbg_leave();
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
    dbg_enter("handleRTDBCreateRequest");
    assert(createReq);

    // keep a reference to the client protocol sending this, required for
    // sending a confirmation message back to the client protocol
    Protocol* theProtocol = fetchSenderProtocol(createReq);

    RTDB_Create_Request crReq  = createReq->getCrReq();
    RTDB_Create_Confirm crConf = vardis_protocol_data_p->handle_rtdb_create_request (crReq);
    
    // send confirmation to application and delete request
    sendRTDBCreateConfirm(crConf.status_code, crReq.spec.varId, theProtocol);
    delete createReq;

    dbg_leave();

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
    dbg_enter("handleRTDBUpdateRequest");
    assert(updateReq);

    // keep a reference to the client protocol sending this, required for
    // sending a confirmation message back to the client protocol
    Protocol* theProtocol = fetchSenderProtocol(updateReq);

    RTDB_Update_Request updReq  = updateReq->getUpdReq();
    RTDB_Update_Confirm updConf = vardis_protocol_data_p->handle_rtdb_update_request (updReq);
    

    // send confirmation to application and delete request
    sendRTDBUpdateConfirm(updConf.status_code, updReq.varId, theProtocol);
    delete updateReq;

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Handles RTDBRead.request service request to read variable value
 * from local RTDB. Performs sanity checks, retrieves and returns
 * current value.
 */
void VardisProtocol::handleRTDBReadRequest (RTDBRead_Request* readReq)
{
    dbg_enter("handleRTDBReadRequest");
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

    sendConfirmation(readConf, rConf.status_code, theProtocol);

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Handles RTDBDescribeDatabase.request service request to return
 * descriptions of all currently known variables to an application.
 * Performs sanity checks, retrieves and returns descriptions.
 */
void VardisProtocol::handleRTDBDescribeDatabaseRequest (RTDBDescribeDatabase_Request* descrDbReq)
{
    dbg_enter("handleRTDBDescribeDatabaseRequest");
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
        dbg_string("Vardis is not active, dropping request");
        dbConf->setDescrsArraySize (0);
        sendConfirmation(dbConf, VARDIS_STATUS_INACTIVE, theProtocol);
        dbg_leave();
        return;
    }


    // copy information about each variable into the confirmation message

   VardisProtocolData& PD = *vardis_protocol_data_p;

   int i=0;
   for (auto varId : PD.active_variables)
     {
       DBEntry& db_entry = vardis_store_p->get_db_entry_ref (varId);
       DescribeDatabaseVariableDescription descr;

       DBG_PVAR2("adding description", db_entry.varId, db_entry.prodId);
       
       descr.varId          =  db_entry.varId;
       descr.prodId         =  db_entry.prodId;
       descr.repCnt         =  db_entry.repCnt;
       descr.tStamp         =  db_entry.tStamp;
       descr.toBeDeleted    =  db_entry.toBeDeleted;
       PD.vardis_store.read_description (varId, descr.description);

       dbConf->setDescrs(i,descr);
       i++;	
      }    

    sendConfirmation(dbConf, VARDIS_STATUS_OK, theProtocol);

    dbg_leave();
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
    dbg_enter("handleRTDBDescribeVariableRequest");
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
        dbg_string("Vardis is not active, dropping request");
        sendConfirmation(varDescr, VARDIS_STATUS_INACTIVE, theProtocol);
        dbg_leave();
        return;
    }

    if (not variableExists(varId))
    {
        DBG_PVAR1("requested variable does not exist", varId);
        sendConfirmation(varDescr, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST, theProtocol);
        dbg_leave();
        return;
    }

    DBG_PVAR1("generating description for variable", varId);

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
    varDescr->setToBeDeleted (theEntry.toBeDeleted);

    sendConfirmation(varDescr, VARDIS_STATUS_OK, theProtocol);

    dbg_leave();
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

    dbg_enter("handleRTDBDeleteRequest");
    assert(deleteReq);

    // keep a reference to the client protocol sending this, required for
    // sending a confirmation message back to the client protocol
    Protocol* theProtocol = fetchSenderProtocol(deleteReq);

    RTDB_Delete_Request delReq  = deleteReq->getDelReq();
    RTDB_Delete_Confirm delConf = vardis_protocol_data_p->handle_rtdb_delete_request (delReq);
    
    delete deleteReq;

    // generate and initialize confirmation
    auto deleteConf = new RTDBDelete_Confirm;

    sendConfirmation(deleteConf, delConf.status_code, theProtocol);

    dbg_leave();
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
    dbg_enter("constructPayload");

    ByteVectorAssemblyArea area ("vardis-constructPayload", maxPayloadSize.val, bv);

    vardis_protocol_data_p->makeICTypeCreateVariables (area, containers_added);
    vardis_protocol_data_p->makeICTypeDeleteVariables (area, containers_added);
    vardis_protocol_data_p->makeICTypeRequestVarCreates (area, containers_added);
    vardis_protocol_data_p->makeICTypeSummaries (area, containers_added);
    vardis_protocol_data_p->makeICTypeUpdates (area, containers_added);
    vardis_protocol_data_p->makeICTypeRequestVarUpdates (area, containers_added);

    bv.resize (area.used());

    dbg_leave();

}

// ----------------------------------------------------

/**
 * Checks whether we can generate a Vardis payload. Generates the payload
 * and sends it to the BP for transmission.
 */
void VardisProtocol::generatePayload ()
{
    dbg_enter("generatePayload");

    if (vardisActive())
    {
        dbg_string("we are active");

        BPTransmitPayload_Request  *pldReq = new BPTransmitPayload_Request ("VardisPayload");
        bytevect& bv = pldReq->getBvdataForUpdate();
        bv.resize (maxPayloadSize.val);
        bv.reserve (2*maxPayloadSize.val);

	unsigned int containers_added = 0;
        constructPayload(bv, containers_added);

        if (containers_added > 0)
        {
            DBG_PVAR1("SENDING payload", bv.size());

            dbg_string("constructing the packet");
            pldReq->setProtId(BP_PROTID_VARDIS.val);

            dbg_string("sending the packet/payload to BP");
            sendToBP(pldReq);

            payloadSent = true;
        }
        else
        {
            delete pldReq;
        }
    }

    dbg_leave();
}


// ========================================================================================
// Helpers for deconstructing and processing received packets
// ========================================================================================


// ----------------------------------------------------

/**
 * The following methods process queues of received instruction container
 * records sequentially.
 */

void VardisProtocol::processVarCreateList(const std::deque<VarCreateT>& creates)
{
  dbg_enter("processVarCreateList");
  
  for (auto it = creates.begin(); it != creates.end(); it++)
    {
      vardis_protocol_data_p->process_var_create(*it);
    }
  
  dbg_leave();
}

// ----------------------------------------------------


void VardisProtocol::processVarDeleteList(const std::deque<VarDeleteT>& deletes)
{
    dbg_enter("processVarDeleteList");

    for (auto it = deletes.begin(); it != deletes.end(); it++)
    {
        vardis_protocol_data_p->process_var_delete(*it);
    }

    dbg_leave();
}

// ----------------------------------------------------


void VardisProtocol::processVarUpdateList(const std::deque<VarUpdateT>& updates)
{
    dbg_enter("processVarUpdateList");

    for (auto it = updates.begin(); it != updates.end(); it++)
    {
        vardis_protocol_data_p->process_var_update(*it);
    }

    dbg_leave();
}

// ----------------------------------------------------


void VardisProtocol::processVarSummaryList(const std::deque<VarSummT>& summs)
{
    dbg_enter("processVarSummaryList");

    for (auto it = summs.begin(); it != summs.end(); it++)
    {
        vardis_protocol_data_p->process_var_summary(*it);
    }

    dbg_leave();
}

// ----------------------------------------------------


void VardisProtocol::processVarReqUpdateList(const std::deque<VarReqUpdateT>& requpdates)
{
    dbg_enter("processVarReqUpdateList");

    for (auto it = requpdates.begin(); it != requpdates.end(); it++)
    {
        vardis_protocol_data_p->process_var_requpdate(*it);
    }

    dbg_leave();
}

// ----------------------------------------------------


void VardisProtocol::processVarReqCreateList(const std::deque<VarReqCreateT>& reqcreates)
{
    dbg_enter("processVarReqCreateList");

    for (auto it = reqcreates.begin(); it != reqcreates.end(); it++)
    {
        vardis_protocol_data_p->process_var_reqcreate(*it);
    }

    dbg_leave();
}


// ========================================================================================
// Helpers for sending standard confirmations to higher layers
// ========================================================================================

void VardisProtocol::sendConfirmation(VardisConfirmation *confMsg, DcpStatus status, Protocol* theProtocol)
{
    dbg_enter("sendConfirmation");
    assert(theProtocol);
    assert(confMsg);

    confMsg->setStatus(status);

    auto req = confMsg->addTagIfAbsent<DispatchProtocolReq>();
    req->setProtocol(theProtocol);
    req->setServicePrimitive(SP_INDICATION);

    send(confMsg, gidToApplication);

    dbg_leave();
}

// ----------------------------------------------------

void VardisProtocol::sendRTDBCreateConfirm(DcpStatus status, VarIdT varId, Protocol* theProtocol)
{
    dbg_enter("sendRTDBCreateConfirm");

    auto conf = new RTDBCreate_Confirm;
    conf->setVarId(varId.val);
    sendConfirmation(conf, status, theProtocol);

    dbg_leave();
}

// ----------------------------------------------------

void VardisProtocol::sendRTDBUpdateConfirm(DcpStatus status, VarIdT varId, Protocol* theProtocol)
{
    dbg_enter("sendRTDBUpdateConfirm");

    auto conf = new RTDBUpdate_Confirm;
    conf->setVarId(varId.val);
    sendConfirmation(conf, status, theProtocol);

    dbg_leave();
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
    dbg_enter("fetchSenderProtocol");
    assert(message);

    auto protTag = message->removeTag<DispatchProtocolInd>();
    Protocol* theProtocol = (Protocol*) protTag->getProtocol();
    assert(theProtocol);

    DBG_VAR2(theProtocol->getId(), theProtocol->getDescriptiveName());
    dbg_leave();

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



