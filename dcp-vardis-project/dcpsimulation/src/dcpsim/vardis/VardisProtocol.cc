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
#include <dcpsim/vardis/VardisProtocol.h>

// ========================================================================================
// ========================================================================================

using namespace omnetpp;
using namespace inet;
using namespace dcp;

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

        vardisActive = false;

        // read and check parameters
        vardisMaxValueLength        = (BPLengthT) par("vardisMaxValueLength");
        vardisMaxDescriptionLength  = (BPLengthT) par("vardisMaxDescriptionLength");
        vardisMaxRepetitions        = (unsigned int) par("vardisMaxRepetitions");
        vardisMaxSummaries          = (unsigned int) par("vardisMaxSummaries");
        vardisBufferCheckPeriod     = par("vardisBufferCheckPeriod");

        // sanity-check parameters
        assert(maxPayloadSize > 0);
        assert(maxPayloadSize <= 1400);   // this deviates from specification (would require config data from BP)
        assert(vardisMaxValueLength > 0);
        assert(vardisMaxValueLength <= std::min(maxVarLen, maxPayloadSize - dcp::ICHeaderT::fixed_size()));
        assert(vardisMaxDescriptionLength > 0);
        assert(vardisMaxDescriptionLength <=   maxPayloadSize
                                             - (   dcp::ICHeaderT::fixed_size()
                                                 + dcp::VarSpecT::fixed_size()
                                                 + dcp::VarUpdateT::fixed_size()
                                                 + vardisMaxValueLength
                                                 ));
        assert(vardisMaxRepetitions > 0);
        assert(vardisMaxRepetitions <= 15);
        assert(vardisMaxSummaries <= (maxPayloadSize - dcp::ICHeaderT::fixed_size())/dcp::VarSummT::fixed_size());
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
        vardisActive = true;
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
        dbg_queueSizes();
        RTDBUpdate_Request* updateReq = (RTDBUpdate_Request*) msg;
        handleRTDBUpdateRequest(updateReq);
        dbg_queueSizes();
        dbg_leave();
        return;
    }

    if (dynamic_cast<RTDBRead_Request*>(msg))
    {
        dbg_string("handling RTDBRead_Request");
        dbg_queueSizes();
        RTDBRead_Request* readReq = (RTDBRead_Request*) msg;
        handleRTDBReadRequest(readReq);
        dbg_queueSizes();
        dbg_leave();
        return;
    }

    if (dynamic_cast<RTDBCreate_Request*>(msg))
    {
        dbg_string("handling RTDBCreate_Request");
        dbg_queueSizes();
        RTDBCreate_Request* createReq = (RTDBCreate_Request*) msg;
        handleRTDBCreateRequest(createReq);
        dbg_queueSizes();
        dbg_leave();
        return;
    }

    if (dynamic_cast<RTDBDelete_Request*>(msg))
    {
        dbg_string("handling RTDBDelete_Request");
        dbg_queueSizes();
        RTDBDelete_Request* deleteReq = (RTDBDelete_Request*) msg;
        handleRTDBDeleteRequest(deleteReq);
        dbg_queueSizes();
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
    qbpReq->setProtId(BP_PROTID_VARDIS);
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

    if (not vardisActive)
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

    // Dispatch on ICType
    while (area.used() < area.available())
    {
        DBG_PVAR2 ("Deserializing: considering ICType", (int) area.used(), (int) ((ICType) area.peek_byte()));
        switch((ICType) area.peek_byte())
        {
        case ICTYPE_SUMMARIES:
            dbg_string("considering ICTYPE_SUMMARIES");
            extractVarSummaryList(area, icSummaries);
            break;
        case ICTYPE_UPDATES:
            dbg_string("considering ICTYPE_UPDATES");
            extractVarUpdateList(area, icUpdates);
            break;
        case ICTYPE_REQUEST_VARUPDATES:
            dbg_string("considering ICTYPE_REQUEST_VARUPDATES");
            extractVarReqUpdateList(area, icRequestVarUpdates);
            break;
        case ICTYPE_REQUEST_VARCREATES:
            dbg_string("considering ICTYPE_REQUEST_VARCREATES");
            extractVarReqCreateList(area, icRequestVarCreates);
            break;
        case ICTYPE_CREATE_VARIABLES:
            dbg_string("considering ICTYPE_CREATE_VARIABLES");
            extractVarCreateList(area, icCreateVariables);
            break;
        case ICTYPE_DELETE_VARIABLES:
            dbg_string("considering ICTYPE_DELETE_VARIABLES");
            extractVarDeleteList(area, icDeleteVariables);
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
    dbg_comprehensive("handleRTDBCreateRequest/enter");

    // keep a reference to the client protocol sending this, required for
    // sending a confirmation message back to the client protocol
    Protocol* theProtocol = fetchSenderProtocol(createReq);

    // copy description string (including terminating zero) to newly allocated memory
    std::string descr      = createReq->getDescr();
    auto        descrLen   = descr.length();
    uint8_t*    descr_cstr = new uint8_t [descrLen];
    std::memcpy ((char*) descr_cstr, (char*) descr.c_str(), descrLen);

    // Fill in the VarSpecT record
    VarSpecT    spec;
    spec.varId         =  createReq->getVarId();
    spec.repCnt        =  createReq->getRepCnt();
    spec.descr.length  =  descrLen;
    spec.descr.data    =  descr_cstr;
    spec.prodId        =  createReq->getProdId();

    auto length  = createReq->getUpdlen();

    // perform various checks


    if (not vardisActive)
    {
        dbg_string("Vardis is not active, dropping request");
        sendRTDBCreateConfirm(VARDIS_STATUS_INACTIVE, spec.varId, theProtocol);
        delete createReq;
        dbg_leave();
        return;
    }

    if (variableExists(spec.varId))
    {
        dbg_string("variable exists, dropping request");
        sendRTDBCreateConfirm(VARDIS_STATUS_VARIABLE_EXISTS, spec.varId, theProtocol);
        delete createReq;
        dbg_leave();
        return;
    }

    if (descrLen > vardisMaxDescriptionLength)
    {
        DBG_PVAR4("description is too long", descrLen, vardisMaxDescriptionLength, descr, descr.length());
        sendRTDBCreateConfirm(VARDIS_STATUS_VARIABLE_DESCRIPTION_TOO_LONG, spec.varId, theProtocol);
        delete createReq;
        dbg_leave();
        return;
    }

    if (length > vardisMaxValueLength)
    {
        dbg_string("value length is too long, dropping request");
        sendRTDBCreateConfirm(VARDIS_STATUS_VALUE_TOO_LONG, spec.varId, theProtocol);
        delete createReq;
        dbg_leave();
        return;
    }

    if (length == 0)
    {
        dbg_string("value length is zero, dropping request");
        sendRTDBCreateConfirm(VARDIS_STATUS_EMPTY_VALUE, spec.varId, theProtocol);
        delete createReq;
        dbg_leave();
        return;
    }

    if ((spec.repCnt == 0) || (spec.repCnt > vardisMaxRepetitions))
    {
        dbg_string("illegal repCnt value, dropping request");
        sendRTDBCreateConfirm(VARDIS_STATUS_ILLEGAL_REPCOUNT, spec.varId, theProtocol);
        delete createReq;
        dbg_leave();
        return;
    }


    DBG_PVAR1("creating new variable", (int) spec.varId);

    // initialize new database entry and add it
    DBEntry newent;
    newent.spec          =  spec;
    newent.spec.prodId   =  getOwnNodeId();
    newent.seqno         =  0;
    newent.tStamp        =  simTime();
    newent.countUpdate   =  0;
    newent.countCreate   =  spec.repCnt;
    newent.countDelete   =  0;
    newent.toBeDeleted   =  false;
    newent.value.length  =  length;
    newent.value.data    =  new uint8_t [length];
    for (int i=0; i<length; i++)
        newent.value.data[i] = createReq->getUpddata(i);
    theVariableDatabase[spec.varId] = newent;

    // clean out varId from all queues, just to be safe
    removeVarIdFromQueue(createQ, spec.varId);
    removeVarIdFromQueue(updateQ, spec.varId);
    removeVarIdFromQueue(summaryQ, spec.varId);
    removeVarIdFromQueue(deleteQ, spec.varId);
    removeVarIdFromQueue(reqUpdQ, spec.varId);
    removeVarIdFromQueue(reqCreateQ, spec.varId);

    // add new variable to relevant queues
    createQ.push_back(spec.varId);
    summaryQ.push_back(spec.varId);

    // send confirmation to application and delete request
    sendRTDBCreateConfirm(VARDIS_STATUS_OK, spec.varId, theProtocol);
    delete createReq;

    dbg_comprehensive("handleRTDBCreateRequest/leave");
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
    dbg_comprehensive("handleRTDBUpdateRequest/enter");

    // keep a reference to the client protocol sending this, required for
    // sending a confirmation message back to the client protocol
    Protocol* theProtocol = fetchSenderProtocol(updateReq);

    VarIdT  varId  = updateReq->getVarId();
    VarLenT varLen = updateReq->getUpdlen();

    // perform various checks

    if (not vardisActive)
    {
        dbg_string("Vardis is active, dropping request");
        sendRTDBUpdateConfirm(VARDIS_STATUS_INACTIVE, varId, theProtocol);
        delete updateReq;
        dbg_leave();
        return;
    }

    if (not variableExists(varId))
    {
        dbg_string("attempting to update non-existing variable, dropping request");
        sendRTDBUpdateConfirm(VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST, varId, theProtocol);
        delete updateReq;
        dbg_leave();
        return;
    }

    DBEntry& theEntry = theVariableDatabase.at(varId);

    if (not producerIsMe(varId))
    {
        dbg_string("attempting to update variable for which I am not the producer, dropping request");
        sendRTDBUpdateConfirm(VARDIS_STATUS_NOT_PRODUCER, varId, theProtocol);
        delete updateReq;
        dbg_leave();
        return;
    }

    if (theEntry.toBeDeleted)
    {
        dbg_string("attempting to update a to-be-deleted variable, dropping request");
        sendRTDBUpdateConfirm(VARDIS_STATUS_VARIABLE_BEING_DELETED, varId, theProtocol);
        delete updateReq;
        dbg_leave();
        return;
    }

    if (varLen > vardisMaxValueLength)
    {
        dbg_string("value length is too long, dropping request");
        sendRTDBUpdateConfirm(VARDIS_STATUS_VALUE_TOO_LONG, varId, theProtocol);
        delete updateReq;
        dbg_leave();
        return;
    }

    if (varLen == 0)
    {
        dbg_string("value length is zero, dropping request");
        sendRTDBUpdateConfirm(VARDIS_STATUS_EMPTY_VALUE, varId, theProtocol);
        delete updateReq;
        dbg_leave();
        return;
    }

    DBG_PVAR1("updating variable with varId = ", (int) varId);

    // update the DB entry
    theEntry.seqno        = (theEntry.seqno + 1) % maxVarSeqno;
    theEntry.countUpdate  = theEntry.spec.repCnt;
    theEntry.tStamp       = simTime();
    delete [] theEntry.value.data;
    theEntry.value.length = varLen;
    theEntry.value.data   =  new uint8_t [varLen];
    for (int i=0; i<varLen; i++)
        theEntry.value.data[i] = updateReq->getUpddata(i);

    // add varId to updateQ if necessary
    if (not isVarIdInQueue(updateQ, varId))
    {
        updateQ.push_back(varId);
    }

    // send confirmation to application and delete request
    sendRTDBUpdateConfirm(VARDIS_STATUS_OK, varId, theProtocol);
    delete updateReq;

    dbg_comprehensive("handleRTDBUpdateRequest/leave");
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

    VarIdT varId = readReq->getVarId();
    delete readReq;

    // generate and initialize confirmation
    auto readConf = new RTDBRead_Confirm;
    readConf->setVarId(varId);
    readConf->setDataLen(0);
    readConf->setDataArraySize(0);

    // perform various checks

    if (not vardisActive)
    {
        dbg_string("Vardis is not active, dropping request");
        sendConfirmation(readConf, VARDIS_STATUS_INACTIVE, theProtocol);
        dbg_leave();
        return;
    }

    if (not variableExists(varId))
    {
        dbg_string("attempting to read non-existing variable, dropping request");
        sendConfirmation(readConf, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST, theProtocol);
        dbg_leave();
        return;
    }

    // retrieve and copy variable value into confirmation
    DBEntry& theEntry = theVariableDatabase.at(varId);
    assert(theEntry.value.data);
    readConf->setDataLen(theEntry.value.length);
    readConf->setDataArraySize(theEntry.value.length);
    for (int i=0; i<theEntry.value.length; i++)
        readConf->setData(i,theEntry.value.data[i]);

    sendConfirmation(readConf, VARDIS_STATUS_OK, theProtocol);

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
    dbConf->setSpecArraySize(theVariableDatabase.size());

    // check whether Vardis protocol is actually active
    if (not vardisActive)
    {
        dbg_string("Vardis is not active, dropping request");
        dbConf->setSpecArraySize(0);
        sendConfirmation(dbConf, VARDIS_STATUS_INACTIVE, theProtocol);
        dbg_leave();
        return;
    }


    // copy information about each variable into the confirmation message
    int i=0;
    for (auto it=theVariableDatabase.begin(); it!=theVariableDatabase.end(); it++)
    {
        auto theVar = it->second;

        DBG_PVAR3("adding description", (int) theVar.spec.varId, theVar.spec.prodId, theVar.spec.descr.to_str());

        VarSpecEntry vse;

        vse.varId   = theVar.spec.varId;
        vse.prodId  = theVar.spec.prodId;
        vse.repCnt  = theVar.spec.repCnt;
        vse.descr   = std::string(theVar.spec.descr.to_str());
        dbConf->setSpec(i,vse);
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

    if (not vardisActive)
    {
        dbg_string("Vardis is not active, dropping request");
        sendConfirmation(varDescr, VARDIS_STATUS_INACTIVE, theProtocol);
        dbg_leave();
        return;
    }

    if (not variableExists(varId))
    {
        DBG_PVAR1("requested variable does not exist", (int) varId);
        sendConfirmation(varDescr, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST, theProtocol);
        dbg_leave();
        return;
    }

    DBG_PVAR1("generating description for variable", (int) varId);

    // retrieve variable and generate response data about it
    DBEntry& theEntry = theVariableDatabase.at(varId);
    varDescr->setVarId(varId);
    varDescr->setProdId(theEntry.spec.prodId);
    varDescr->setRepCnt(theEntry.spec.repCnt);
    varDescr->setLength(theEntry.value.length);
    varDescr->setDescr(theEntry.spec.descr.to_str().c_str());
    varDescr->setSeqno(theEntry.seqno);
    varDescr->setTstamp(theEntry.tStamp);
    varDescr->setCountUpdate(theEntry.countUpdate);
    varDescr->setCountCreate(theEntry.countCreate);
    varDescr->setCountDelete(theEntry.countDelete);
    varDescr->setToBeDeleted(theEntry.toBeDeleted);
    varDescr->setValueArraySize(theEntry.value.length);
    for (int i=0; i<theEntry.value.length; i++)
        varDescr->setValue(i,theEntry.value.data[i]);


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
void VardisProtocol::handleRTDBDeleteRequest (RTDBDelete_Request* delReq)
{

    dbg_enter("handleRTDBDeleteRequest");
    assert(delReq);

    // keep a reference to the client protocol sending this, required for
    // sending a confirmation message back to the client protocol
    Protocol* theProtocol = fetchSenderProtocol(delReq);

    VarIdT varId = delReq->getVarId();
    delete delReq;

    // generate and initialize confirmation
    auto deleteConf = new RTDBDelete_Confirm;
    deleteConf->setVarId(varId);

    // perform some checks

    if (not vardisActive)
    {
        dbg_string("Vardis is not active, dropping request");
        sendConfirmation(deleteConf, VARDIS_STATUS_INACTIVE, theProtocol);
        dbg_leave();
        return;
    }

    if (not variableExists(varId))
    {
        dbg_string("attempting to delete non-existing variable, dropping request");
        sendConfirmation(deleteConf, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST, theProtocol);
        dbg_leave();
        return;
    }

    if (not producerIsMe(varId))
    {
        dbg_string("attempting to delete variable owned by someone else, dropping request");
        sendConfirmation(deleteConf, VARDIS_STATUS_NOT_PRODUCER, theProtocol);
        dbg_leave();
        return;
    }

    DBEntry& theEntry = theVariableDatabase.at(varId);

    if (theEntry.toBeDeleted)
    {
        dbg_string("attempting to delete variable that is already in deletion process, dropping request");
        sendConfirmation(deleteConf, VARDIS_STATUS_VARIABLE_BEING_DELETED, theProtocol);
        dbg_leave();
        return;
    }

    // add varId to deleteQ, remove it from any other queue
    assert(not isVarIdInQueue(deleteQ, varId));
    deleteQ.push_back(varId);
    removeVarIdFromQueue(createQ, varId);
    removeVarIdFromQueue(summaryQ, varId);
    removeVarIdFromQueue(updateQ, varId);
    removeVarIdFromQueue(reqUpdQ, varId);
    removeVarIdFromQueue(reqCreateQ, varId);

    // update variable status
    theEntry.toBeDeleted = true;
    theEntry.countDelete = theEntry.spec.repCnt;
    theEntry.countCreate = 0;
    theEntry.countUpdate = 0;

    sendConfirmation(deleteConf, VARDIS_STATUS_OK, theProtocol);

    dbg_leave();
}




// ========================================================================================
// Construction of instruction containers for outgoing packets
// ========================================================================================

/* the following 'instructionSizeXX' functions shall return the number of bytes
 * that the respective instruction container entries (given their varId's) will
 * need in their serialization.The numbers here reflect a 'packed' realization
 * of these types.
 */

unsigned int VardisProtocol::instructionSizeVarCreate(VarIdT varId)
{
    DBEntry& theEntry = theVariableDatabase.at(varId);
    return    theEntry.spec.total_size()
            + dcp::VarUpdateT::fixed_size()
            + theEntry.value.length;
}

// ----------------------------------------------------

unsigned int VardisProtocol::instructionSizeVarSummary(VarIdT varId)
{
    return dcp::VarSummT::fixed_size();
}

// ----------------------------------------------------

unsigned int VardisProtocol::instructionSizeVarUpdate(VarIdT varId)
{
    DBEntry& theEntry = theVariableDatabase.at(varId);
    return dcp::VarUpdateT::fixed_size() + theEntry.value.length;
}

// ----------------------------------------------------

unsigned int VardisProtocol::instructionSizeVarDelete(VarIdT varId)
{
    return dcp::VarDeleteT::fixed_size();
}

// ----------------------------------------------------

unsigned int VardisProtocol::instructionSizeReqCreate(VarIdT varId)
{
    return dcp::VarReqCreateT::fixed_size();
}

// ----------------------------------------------------

unsigned int VardisProtocol::instructionSizeReqUpdate(VarIdT varId)
{
    return dcp::VarReqUpdateT::fixed_size();
}

// ----------------------------------------------------

/* the following 'addXX' functions perform the serialization of the
 * known instruction container entries, assuming a 'packed' representation.
 */


void VardisProtocol::addVarCreate (VarIdT varId,
                                   DBEntry& theEntry,
                                   AssemblyArea& area)
{
    dbg_enter("addVarCreate");

    VarCreateT create;
    create.spec = theEntry.spec;
    create.update.varId   =  theEntry.spec.varId;
    create.update.seqno   =  theEntry.seqno;
    create.update.value   =  theEntry.value;

    create.serialize (area);

    dbg_leave();
}

// ----------------------------------------------------


void VardisProtocol::addVarSummary (VarIdT varId,
                                    DBEntry& theEntry,
                                    AssemblyArea& area)
{
    dbg_enter("addVarSummary");

    VarSummT summ;
    summ.varId  = varId;
    summ.seqno  = theEntry.seqno;

    summ.serialize (area);

    dbg_leave();
}

// ----------------------------------------------------

void VardisProtocol::addVarUpdate (VarIdT varId,
                                   DBEntry& theEntry,
                                   AssemblyArea& area)
{
    dbg_enter("addVarUpdate");

    VarUpdateT update;
    update.varId   =  theEntry.spec.varId;
    update.seqno   =  theEntry.seqno;
    update.value   =  theEntry.value;

    update.serialize (area);

    dbg_leave();
}

// ----------------------------------------------------

void VardisProtocol::addVarDelete (VarIdT varId,
                                   AssemblyArea& area)
{
    dbg_enter("addVarDelete");

    VarDeleteT del;
    del.varId = varId;

    del.serialize (area);

    dbg_leave();
}


// ----------------------------------------------------

void VardisProtocol::addVarReqCreate (VarIdT varId,
                                      AssemblyArea& area)
{
    dbg_enter("addVarReqCreate");

    VarReqCreateT cr;
    cr.varId = varId;

    cr.serialize (area);

    dbg_leave();
}

// ----------------------------------------------------

void VardisProtocol::addVarReqUpdate (VarIdT varId,
                                      DBEntry& theEntry,
                                      AssemblyArea& area)
{
    dbg_enter("addVarReqUpdate");

    VarReqUpdateT upd;
    upd.updSpec.varId = varId;
    upd.updSpec.seqno = theEntry.seqno;

    upd.serialize (area);

    dbg_leave();
}


// ----------------------------------------------------

void VardisProtocol::addICHeader (ICHeaderT icHdr,
                                  AssemblyArea& area)
{
    dbg_enter("addICHeader");

    icHdr.serialize (area);

    dbg_leave();
}


// ----------------------------------------------------

/**
 * This function calculates how many information instruction records referenced
 * in the given queue and of the given type (cf 'instructionSizeFunction' parameter)
 * fit into the number of bytes still available in the VarDis payload
 */
unsigned int VardisProtocol::numberFittingRecords(
                  const std::deque<VarIdT>& queue,
                  AssemblyArea& area,
                  std::function<unsigned int (VarIdT)> instructionSizeFunction
                  )
{
    // first work out how many records we can add
    unsigned int   numberRecordsToAdd = 0;
    unsigned int   bytesToBeAdded = dcp::ICHeaderT::fixed_size();
    auto           it = queue.begin();
    while(    (it != queue.end())
           && (bytesToBeAdded + instructionSizeFunction(*it) <= area.available())
           && (numberRecordsToAdd < dcp::ICHeaderT::max_records()))
    {
        numberRecordsToAdd++;
        bytesToBeAdded += instructionSizeFunction(*it);
        it++;
    }

    return (std::min(numberRecordsToAdd, (unsigned int) dcp::ICHeaderT::max_records()));
}


// ----------------------------------------------------

/**
 * This serializes an instruction container for VarCreateT's, it generates
 * an ICHeader and a as many VarCreateT records as possible / available.
 */
void VardisProtocol::makeICTypeCreateVariables (AssemblyArea& area)
{
    dbg_enter("makeICTypeCreateVariables");
    DBG_VAR3(createQ.size(), area.used(), area.available());

    dropNonexistingDeleted(createQ);

    // check for empty createQ or insufficient size to add at least the first instruction record
    if (    createQ.empty()
         || (instructionSizeVarCreate(createQ.front()) + dcp::ICHeaderT::fixed_size()) > area.available())
    {
        dbg_string("queue empty or insufficient space available");
        dbg_leave();
        return;
    }

    // first work out how many records we will add
    std::function<unsigned int(VarIdT)> instSizeFn = [&] (VarIdT varId) { return instructionSizeVarCreate(varId); };
    auto numberRecordsToAdd = numberFittingRecords(createQ, area, instSizeFn);
    assert(numberRecordsToAdd > 0);
    DBG_VAR1(numberRecordsToAdd);

    // initialize and serialize ICHeader
    ICHeaderT   icHeader;
    icHeader.icType       = ICTYPE_CREATE_VARIABLES;
    icHeader.icNumRecords = numberRecordsToAdd;
    addICHeader(icHeader, area);

    // serialize the records
    for (unsigned int i=0; i<numberRecordsToAdd; i++)
    {
        VarIdT nextVarId = createQ.front();
        createQ.pop_front();
        DBEntry& nextVar = theVariableDatabase.at(nextVarId);

        DBG_PVAR5("adding", (int) nextVarId, instructionSizeVarCreate(nextVarId), (int) nextVar.countCreate, area.used(), area.available());
        assert(nextVar.countCreate > 0);

        nextVar.countCreate--;

        addVarCreate(nextVarId, nextVar, area);

        if (nextVar.countCreate > 0)
        {
            createQ.push_back(nextVarId);
        }
    }

    dbg_comprehensive("makeICTypeCreateVariables");
    dbg_leave();
    return;
}

// ----------------------------------------------------

/**
 * This serializes an instruction container for VarSummT's, it generates
 * an ICHeader and a as many VarSummT records as possible / available.
 */
void VardisProtocol::makeICTypeSummaries (AssemblyArea& area)
{
    dbg_enter("makeICTypeSummaries");
    DBG_VAR3(summaryQ.size(), area.used(), area.available());

    dropNonexistingDeleted(summaryQ);

    // check for empty summaryQ, insufficient size to add at least the first instruction record,
    // or whether summaries function is enabled
    if (    summaryQ.empty()
         || (instructionSizeVarSummary(summaryQ.front()) + dcp::ICHeaderT::fixed_size() > area.available())
         || (vardisMaxSummaries == 0))
    {
        dbg_string("queue empty, insufficient space available or no summaries to be created");
        dbg_leave();
        return;
    }

    // first work out how many records we will add, cap at vardisMaxSummaries
    std::function<unsigned int(VarIdT)> instSizeFn = [&] (VarIdT varId) { return instructionSizeVarSummary(varId); };
    auto numberRecordsToAdd = numberFittingRecords(summaryQ, area, instSizeFn);
    assert(numberRecordsToAdd > 0);
    numberRecordsToAdd = std::min(numberRecordsToAdd, vardisMaxSummaries);
    DBG_VAR1(numberRecordsToAdd);

    // initialize and serialize ICHeader
    ICHeaderT   icHeader;
    icHeader.icType       = ICTYPE_SUMMARIES;
    icHeader.icNumRecords = numberRecordsToAdd;
    addICHeader(icHeader, area);

    // serialize the records
    for (unsigned int i=0; i<numberRecordsToAdd; i++)
    {
        VarIdT nextVarId  = summaryQ.front();

        DBG_PVAR5("adding", (int) nextVarId, instructionSizeVarSummary(nextVarId), (int) theVariableDatabase.at(nextVarId).seqno, area.used(), area.available());

        summaryQ.pop_front();
        summaryQ.push_back(nextVarId);
        DBEntry&   theNextEntry  = theVariableDatabase.at(nextVarId);
        addVarSummary(nextVarId, theNextEntry, area);
    }

    dbg_comprehensive("makeICTypeSummaries");
    dbg_leave();
    return;
}

// ----------------------------------------------------

/**
 * This serializes an instruction container for VarUpdateT's, it generates
 * an ICHeader and a as many VarUpdateT records as possible / available.
 */
void VardisProtocol::makeICTypeUpdates (AssemblyArea& area)
{
    dbg_enter("makeICTypeUpdates");
    DBG_VAR3(updateQ.size(), area.used(), area.available());

    dropNonexistingDeleted(updateQ);

    // check for empty updateQ or insufficient size to add at least the first instruction record
    if (    updateQ.empty()
         || (instructionSizeVarUpdate(updateQ.front()) + dcp::ICHeaderT::fixed_size() > area.available()))
    {
        dbg_string("queue empty or insufficient space available");
        dbg_leave();
        return;
    }

    // first work out how many records we will add
    std::function<unsigned int(VarIdT)> instSizeFn = [&] (VarIdT varId) { return instructionSizeVarUpdate(varId); };
    auto numberRecordsToAdd = numberFittingRecords(updateQ, area, instSizeFn);
    assert(numberRecordsToAdd > 0);
    DBG_VAR1(numberRecordsToAdd);

    // initialize and serialize ICHeader
    ICHeaderT   icHeader;
    icHeader.icType       = ICTYPE_UPDATES;
    icHeader.icNumRecords = numberRecordsToAdd;
    addICHeader(icHeader, area);

    // serialize required records
    for (unsigned int i=0; i<numberRecordsToAdd; i++)
    {
        VarIdT nextVarId = updateQ.front();
        updateQ.pop_front();
        DBEntry& nextVar = theVariableDatabase.at(nextVarId);

        DBG_PVAR6("adding", (int) nextVarId, instructionSizeVarUpdate(nextVarId), nextVar.countUpdate, (int) nextVar.seqno, area.used(), area.available());
        assert(nextVar.countUpdate > 0);

        nextVar.countUpdate--;

        addVarUpdate(nextVarId, nextVar, area);

        if (nextVar.countUpdate > 0)
        {
            updateQ.push_back(nextVarId);
        }
    }

    dbg_comprehensive("makeICTypeUpdates");
    dbg_leave();
    return;
}

// ----------------------------------------------------

/**
 * This serializes an instruction container for VarDeleteT's, it generates
 * an ICHeader and a as many VarDeleteT records as possible / available.
 */
void VardisProtocol::makeICTypeDeleteVariables (AssemblyArea& area)
{
    dbg_enter("makeICTypeDeleteVariables");
    DBG_VAR3(deleteQ.size(), area.used(), area.available());

    dropNonexisting(deleteQ);

    // check for empty deleteQ or insufficient size to add at least the first instruction record
    if (    deleteQ.empty()
         || (instructionSizeVarDelete(deleteQ.front()) + dcp::ICHeaderT::fixed_size() > area.available()))
    {
        dbg_string("queue empty or insufficient space available");
        dbg_leave();
        return;
    }

    // first work out how many records we will add
    std::function<unsigned int(VarIdT)> instSizeFn = [&] (VarIdT varId) { return instructionSizeVarDelete(varId); };
    auto numberRecordsToAdd = numberFittingRecords(deleteQ, area, instSizeFn);
    assert(numberRecordsToAdd > 0);
    DBG_VAR1(numberRecordsToAdd);

    // initialize and serialize ICHeader
    ICHeaderT   icHeader;
    icHeader.icType       = ICTYPE_DELETE_VARIABLES;
    icHeader.icNumRecords = numberRecordsToAdd;
    addICHeader(icHeader, area);

    // serialize required records
    for (unsigned int i=0; i<numberRecordsToAdd; i++)
    {
        VarIdT nextVarId = deleteQ.front();
        deleteQ.pop_front();
        assert(variableExists(nextVarId));
        DBEntry& nextVar = theVariableDatabase.at(nextVarId);

        DBG_PVAR5("adding", (int) nextVarId, instructionSizeVarDelete(nextVarId), (int) nextVar.countDelete, area.used(), area.available());
        assert(nextVar.countDelete > 0);

        nextVar.countDelete--;

        addVarDelete(nextVarId, area);

        if (nextVar.countDelete > 0)
        {
            deleteQ.push_back(nextVarId);
        }
        else
        {
            DBG_PVAR2("now we actually DELETE variable", (int) nextVarId, nextVar.spec.descr.to_str());
            theVariableDatabase.erase(nextVarId);
        }
    }

    dbg_comprehensive("makeICTypeDeleteVariables");
    dbg_leave();
    return;
}

// ----------------------------------------------------

/**
 * This serializes an instruction container for VarReqUpdateT's, it
 * generates an ICHeader and a as many VarReqUpdateT records as
 * possible / available.
 */
void VardisProtocol::makeICTypeRequestVarUpdates (AssemblyArea& area)
{
    dbg_enter("makeICTypeRequestVarUpdates");
    DBG_VAR3(reqUpdQ.size(), area.used(), area.available());

    dropNonexistingDeleted(reqUpdQ);

    // check for empty reqUpdQ or insufficient size to add at least the first instruction record
    if (    reqUpdQ.empty()
         || (instructionSizeReqUpdate(reqUpdQ.front()) + dcp::ICHeaderT::fixed_size() > area.available()))
    {
        dbg_string("queue empty or insufficient space available");
        dbg_leave();
        return;
    }

    // first work out how many records we will add
    std::function<unsigned int(VarIdT)> instSizeFn = [&] (VarIdT varId) { return instructionSizeReqUpdate(varId); };
    auto numberRecordsToAdd = numberFittingRecords(reqUpdQ, area, instSizeFn);
    assert(numberRecordsToAdd > 0);
    DBG_VAR1(numberRecordsToAdd);

    // initialize and serialize ICHeader
    ICHeaderT   icHeader;
    icHeader.icType       = ICTYPE_REQUEST_VARUPDATES;
    icHeader.icNumRecords = numberRecordsToAdd;
    addICHeader(icHeader, area);

    // serialize required records
    for (unsigned int i=0; i<numberRecordsToAdd; i++)
    {
        VarIdT nextVarId = reqUpdQ.front();
        reqUpdQ.pop_front();
        DBEntry& nextVar = theVariableDatabase.at(nextVarId);

        DBG_PVAR4("adding", (int) nextVarId, instructionSizeReqUpdate(nextVarId), area.used(), area.available());

        addVarReqUpdate(nextVarId, nextVar, area);
    }

    dbg_comprehensive("makeICTypeRequestVarUpdates");
    dbg_leave();
}


// ----------------------------------------------------

/**
 * This serializes an instruction container for VarReqCreateT's, it
 * generates an ICHeader and a as many VarReqCreateT records as
 * possible / available.
 */
void VardisProtocol::makeICTypeRequestVarCreates (AssemblyArea& area)
{
    dbg_enter("makeICTypeRequestVarCreates");
    DBG_VAR3(reqCreateQ.size(), area.used(), area.available());

    dropDeleted(reqCreateQ);

    // check for empty reqCreateQ or insufficient size to add at least the first instruction record
    if (    reqCreateQ.empty()
         || (instructionSizeReqCreate(reqCreateQ.front()) + dcp::ICHeaderT::fixed_size() > area.available()))
    {
        dbg_string("queue empty or insufficient space available");
        dbg_leave();
        return;
    }

    // first work out how many records we will add
    std::function<unsigned int(VarIdT)> instSizeFn = [&] (VarIdT varId) { return instructionSizeReqCreate(varId); };
    auto numberRecordsToAdd = numberFittingRecords(reqCreateQ, area, instSizeFn);
    assert(numberRecordsToAdd > 0);
    DBG_VAR1(numberRecordsToAdd);

    // initialize and serialize ICHeader
    ICHeaderT   icHeader;
    icHeader.icType       = ICTYPE_REQUEST_VARCREATES;
    icHeader.icNumRecords = numberRecordsToAdd;
    addICHeader(icHeader, area);

    // serialize required records
    for (unsigned int i=0; i<numberRecordsToAdd; i++)
    {
        VarIdT nextVarId = reqCreateQ.front();
        reqCreateQ.pop_front();

        DBG_PVAR4("adding VarReqCreate", (int) nextVarId, instructionSizeReqCreate(nextVarId), area.used(), area.available());

        addVarReqCreate(nextVarId, area);
    }

    dbg_comprehensive("makeICTypeRequestVarCreates");
    dbg_leave();
}


// ----------------------------------------------------

/**
 * Constructs a Vardis payload for BP by adding instruction containers in the
 * specified order.
 */
void VardisProtocol::constructPayload(bytevect& bv)
{
    dbg_enter("constructPayload");

    dbg_comprehensive("constructPayload/enter");

    ByteVectorAssemblyArea area ("vardis-constructPayload", maxPayloadSize, bv);

    makeICTypeCreateVariables (area);
    makeICTypeDeleteVariables (area);
    makeICTypeRequestVarCreates (area);
    makeICTypeSummaries (area);
    makeICTypeUpdates (area);
    makeICTypeRequestVarUpdates (area);

    bv.resize (area.used());

    dbg_comprehensive("constructPayload/leave");

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

    if (vardisActive)
    {
        dbg_string("we are active");

        BPTransmitPayload_Request  *pldReq = new BPTransmitPayload_Request ("VardisPayload");
        bytevect& bv = pldReq->getBvdataForUpdate();
        bv.resize (maxPayloadSize);
        bv.reserve (2*maxPayloadSize);

        constructPayload(bv);

        if (bv.size() > 0)
        {
            DBG_PVAR1("SENDING payload", bv.size());

            dbg_string("constructing the packet");
            pldReq->setProtId(BP_PROTID_VARDIS);

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

/**
 * Processes a received VarCreate entry. If variable does not already exist,
 * it will be added to the local RTDB, including description and value, and
 * its metadata will be initialized.
 */
void VardisProtocol::processVarCreate(const VarCreateT& create)
{
    dbg_enter("processVarCreate");

    // const VarSpecT&    spec   = create.spec;
    // const VarUpdateT&  update = create.update;
    VarIdT            varId  = create.spec.varId;
    NodeIdentifierT   prodId = create.spec.prodId;

    assert(create.update.value.length > 0);
    DBG_PVAR2("considering", (int) varId, prodId);

    if (    (not variableExists(varId))
         && (prodId != getOwnNodeId())
         && (create.spec.descr.length <= vardisMaxDescriptionLength)
         && (create.spec.descr.length > 0)
         && (create.update.value.length <= vardisMaxValueLength)
         && (create.update.value.length > 0)
         && (create.spec.repCnt > 0)
         && (create.spec.repCnt <= vardisMaxRepetitions)
       )
    {
        DBG_PVAR3("ADDING new variable to database", (int) varId, prodId, create.spec.descr.to_str());

        // create and initialize new DBEntry
        DBEntry newEntry;
        newEntry.spec         =  create.spec;
        newEntry.seqno        =  create.update.seqno;
        newEntry.tStamp       =  simTime();
        newEntry.countUpdate  =  0;
        newEntry.countCreate  =  create.spec.repCnt;
        newEntry.countDelete  =  0;
        newEntry.toBeDeleted  =  false;
        newEntry.value        =  create.update.value;
        theVariableDatabase[varId] = newEntry;

        // just to be safe, delete varId from all queues before inserting it
        // into the right ones
        removeVarIdFromQueue (createQ, varId);
        removeVarIdFromQueue (deleteQ, varId);
        removeVarIdFromQueue (updateQ, varId);
        removeVarIdFromQueue (summaryQ, varId);
        removeVarIdFromQueue (reqUpdQ, varId);
        removeVarIdFromQueue (reqCreateQ, varId);

        // add varId to relevant queues
        createQ.push_back (varId);
        summaryQ.push_back (varId);
        removeVarIdFromQueue (reqCreateQ, varId);
    }

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Processes a received VarDelete entry. If variable exists, its state will
 * be set to move into the to-be-deleted state, and it will be removed from
 * the relevant queues
 */
void VardisProtocol::processVarDelete(const VarDeleteT& del)
{
    dbg_enter("processVarDelete");

    VarIdT   varId   = del.varId;

    DBG_PVAR1("considering", (int) varId);

    if (variableExists(varId))
    {
        DBEntry&        theEntry = theVariableDatabase.at(varId);
        NodeIdentifierT prodId   = theEntry.spec.prodId;

        DBG_PVAR3("considering", (int) varId, prodId, theEntry.toBeDeleted);

        if (    (not theEntry.toBeDeleted)
             && (not producerIsMe(varId)))
        {
            DBG_PVAR1("DELETING", (int) varId);

            // update variable state
            theEntry.toBeDeleted  = true;
            theEntry.countUpdate  = 0;
            theEntry.countCreate  = 0;
            theEntry.countDelete  = theEntry.spec.repCnt;

            // remove varId from relevant queues
            removeVarIdFromQueue(updateQ, varId);
            removeVarIdFromQueue(createQ, varId);
            removeVarIdFromQueue(reqUpdQ, varId);
            removeVarIdFromQueue(reqCreateQ, varId);
            removeVarIdFromQueue(summaryQ, varId);
            removeVarIdFromQueue(deleteQ, varId);

            // add it to deleteQ
            deleteQ.push_back(varId);
        }
    }

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Processes a received VarUpdate entry. If variable exists and certain
 * conditions are met, its value will be updated and the variable will
 * be added to the relevant queues.
 */
void VardisProtocol::processVarUpdate(const VarUpdateT& update)
{
    dbg_enter("processVarUpdate");
    assert(update.value.length > 0);

    VarIdT  varId   = update.varId;

    DBG_PVAR3("considering", (int) varId, (int) update.seqno, (int) update.value.length);

    // check if variable exists -- if not, add it to queue to generate ReqVarCreate
    if (not variableExists(varId))
    {
        DBG_PVAR1("variable does not exist in my database", (int) varId);
        if (not isVarIdInQueue(reqCreateQ, varId))
            reqCreateQ.push_back(varId);

        dbg_leave();
        return;
    }

    DBEntry& theEntry = theVariableDatabase.at(varId);

    // perform some checks

    if (theEntry.toBeDeleted)
    {
        dbg_string("variable has toBeDeleted set");
        dbg_leave();
        return;
    }

    if (producerIsMe(varId))
    {
        dbg_string("variable is produced by me");
        dbg_leave();
        return;
    }

    if (update.value.length > vardisMaxValueLength)
    {
        dbg_string("variable value is too long");
        dbg_leave();
        return;
    }

    if (theEntry.seqno == update.seqno)
    {
        dbg_string("variable has same sequence number");
        dbg_leave();
        return;
    }

    // If received update is older than what I have, schedule transmissions of
    // VarUpdate's for this variable to educate the sender
    if (MORE_RECENT_SEQNO((int) theEntry.seqno, (int) update.seqno))
    {
        dbg_string("received variable has strictly older sequence number than I have");
        // I have a more recent sequence number
        if (not isVarIdInQueue(updateQ, varId))
        {
            updateQ.push_back(varId);
            theEntry.countUpdate = theEntry.spec.repCnt;
        }
        dbg_leave();
        return;
    }

    DBG_PVAR2("UPDATING", (int) varId, (int) update.seqno);

    // update variable with new value, update relevant queues
    theEntry.seqno        =  update.seqno;
    theEntry.tStamp       =  simTime();
    theEntry.countUpdate  =  theEntry.spec.repCnt;
    theEntry.value        =  update.value;

    if (not isVarIdInQueue(updateQ, varId))
    {
        updateQ.push_back(varId);
    }
    removeVarIdFromQueue(reqUpdQ, varId);

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Processes a received VarSummary entry. If variable exists but we have
 * it only in outdated version, we send a ReqVarUpdate for this variable
 */
void VardisProtocol::processVarSummary(const VarSummT& summ)
{
    dbg_enter("processVarSummary");

    VarIdT     varId = summ.varId;
    VarSeqnoT  seqno = summ.seqno;

    DBG_PVAR2("considering", (int) varId, (int) seqno);

    // if variable does not exist in local RTDB, request a VarCreate
    if (not variableExists(varId))
    {
        DBG_PVAR1("variable does not exist in my database", (int) varId);
        if (not isVarIdInQueue(reqCreateQ, varId))
        {
            reqCreateQ.push_back(varId);
        }
        dbg_leave();
        return;
    }

    DBEntry& theEntry = theVariableDatabase.at(varId);

    // perform some checks

    if (theEntry.toBeDeleted)
    {
        dbg_string("variable has toBeDeleted set");
        dbg_leave();
        return;
    }

    if (producerIsMe(varId))
    {
        dbg_string("variable is produced by me");
        dbg_leave();
        return;
    }

    if (theEntry.seqno == seqno)
    {
        dbg_string("variable has same sequence number");
        dbg_leave();
        return;
    }

    // schedule transmission of VarUpdate's if the received seqno is too
    // old
    if (MORE_RECENT_SEQNO(theEntry.seqno, seqno))
    {
        dbg_string("received variable summary has strictly older sequence number than I have");
        if (not isVarIdInQueue(updateQ, varId))
        {
            updateQ.push_back(varId);
            theEntry.countUpdate = theEntry.spec.repCnt;
        }
        dbg_leave();
        return;
    }

    // If my own value is too old, schedule transmission of VarReqUpdate
    if (not isVarIdInQueue(reqUpdQ, varId))
    {
        reqUpdQ.push_back(varId);
    }

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Processes a received VarReqUpdate entry. If variable exists and we
 * have it in a more recent version, schedule transmissions of VarUpdate's
 * for this variable
 */
void VardisProtocol::processVarReqUpdate(const VarReqUpdateT& requpd)
{
    dbg_enter("processVarReqUpdate");

    VarIdT     varId = requpd.updSpec.varId;
    VarSeqnoT  seqno = requpd.updSpec.seqno;

    DBG_PVAR2("considering", (int) varId, (int) seqno);

    // check some conditions

    if (not variableExists(varId))
    {
        DBG_PVAR1("variable does not exist in my database", (int) varId);
        if (not isVarIdInQueue(reqCreateQ, varId))
        {
            reqCreateQ.push_back(varId);
        }
        dbg_leave();
        return;
    }

    DBEntry& theEntry = theVariableDatabase.at(varId);

    if (theEntry.toBeDeleted)
    {
        dbg_string("variable has toBeDeleted set");
        dbg_leave();
        return;
    }

    if (MORE_RECENT_SEQNO(seqno, theEntry.seqno))
    {
        dbg_string("received variable summary has more recent sequence number than I have");
        dbg_leave();
        return;
    }

    theEntry.countUpdate = theEntry.spec.repCnt;

    if (not isVarIdInQueue(updateQ, varId))
    {
        updateQ.push_back(varId);
    }

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Processes a received VarReqCreate entry. If variable exists, schedule
 * transmissions of VarCreate's for this variable
 */
void VardisProtocol::processVarReqCreate(const VarReqCreateT& reqcreate)
{
    dbg_enter("processVarReqCreate");

    VarIdT varId = reqcreate.varId;

    DBG_PVAR1("considering", (int) varId);

    if (not variableExists(varId))
    {
        dbg_string("variable does not exist in my database");
        if (not isVarIdInQueue(reqCreateQ, varId))
        {
            reqCreateQ.push_back(varId);
        }
        dbg_leave();
        return;
    }

    DBEntry& theEntry = theVariableDatabase.at(varId);

    if (theEntry.toBeDeleted)
    {
        dbg_string("variable has toBeDeleted set");
        dbg_leave();
        return;
    }

    theEntry.countCreate = theEntry.spec.repCnt;

    if (not isVarIdInQueue(createQ, varId))
    {
        DBG_PVAR1("scheduling future VarCreate transmissions", (int) varId);
        createQ.push_back(varId);
    }


    dbg_leave();
}


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
        processVarCreate(*it);
    }

    dbg_leave();
}

// ----------------------------------------------------


void VardisProtocol::processVarDeleteList(const std::deque<VarDeleteT>& deletes)
{
    dbg_enter("processVarDeleteList");

    for (auto it = deletes.begin(); it != deletes.end(); it++)
    {
        processVarDelete(*it);
    }

    dbg_leave();
}

// ----------------------------------------------------


void VardisProtocol::processVarUpdateList(const std::deque<VarUpdateT>& updates)
{
    dbg_enter("processVarUpdateList");

    for (auto it = updates.begin(); it != updates.end(); it++)
    {
        processVarUpdate(*it);
    }

    dbg_leave();
}

// ----------------------------------------------------


void VardisProtocol::processVarSummaryList(const std::deque<VarSummT>& summs)
{
    dbg_enter("processVarSummaryList");

    for (auto it = summs.begin(); it != summs.end(); it++)
    {
        processVarSummary(*it);
    }

    dbg_leave();
}

// ----------------------------------------------------


void VardisProtocol::processVarReqUpdateList(const std::deque<VarReqUpdateT>& requpdates)
{
    dbg_enter("processVarReqUpdateList");

    for (auto it = requpdates.begin(); it != requpdates.end(); it++)
    {
        processVarReqUpdate(*it);
    }

    dbg_leave();
}

// ----------------------------------------------------


void VardisProtocol::processVarReqCreateList(const std::deque<VarReqCreateT>& reqcreates)
{
    dbg_enter("processVarReqCreateList");

    for (auto it = reqcreates.begin(); it != reqcreates.end(); it++)
    {
        processVarReqCreate(*it);
    }

    dbg_leave();
}

// ----------------------------------------------------

/**
 * The following methods all extract / parse an entire instruction container,
 * both the ICHeader and the instruction records, which are stored in a list
 */

void VardisProtocol::extractVarCreateList(DisassemblyArea& area, std::deque<VarCreateT>& creates)
{
    dbg_enter("extractVarCreateList");

    ICHeaderT icHeader;
    icHeader.deserialize (area);
    assert(icHeader.icType == ICTYPE_CREATE_VARIABLES);
    assert(icHeader.icNumRecords > 0);

    for (int i=0; i<icHeader.icNumRecords; i++)
    {
        VarCreateT create;
        create.deserialize (area);

        DBG_VAR4((int) create.spec.varId, (int) create.spec.repCnt, create.spec.prodId, create.spec.descr.to_str());

        creates.push_back(create);
    }

    dbg_leave();
}


// ----------------------------------------------------

void VardisProtocol::extractVarDeleteList(DisassemblyArea& area, std::deque<VarDeleteT>& deletes)
{
    dbg_enter("extractVarDeleteList");

    ICHeaderT icHeader;
    icHeader.deserialize (area);
    assert(icHeader.icType == ICTYPE_DELETE_VARIABLES);
    assert(icHeader.icNumRecords > 0);

    for (int i=0; i<icHeader.icNumRecords; i++)
    {
        VarDeleteT del;
        del.deserialize(area);

        DBG_VAR1((int) del.varId);

        deletes.push_back(del);
    }

    dbg_leave();
}

// ----------------------------------------------------

void VardisProtocol::extractVarUpdateList(DisassemblyArea& area, std::deque<VarUpdateT>& updates)
{
    dbg_enter("extractVarUpdateList");

    ICHeaderT icHeader;
    icHeader.deserialize (area);
    assert(icHeader.icType == ICTYPE_UPDATES);
    assert(icHeader.icNumRecords > 0);

    for (int i=0; i<icHeader.icNumRecords; i++)
    {
        VarUpdateT upd;
        upd.deserialize (area);

        DBG_VAR2((int) upd.varId, (int) upd.seqno);

        updates.push_back(upd);
    }

    dbg_leave();
}

// ----------------------------------------------------

void VardisProtocol::extractVarSummaryList(DisassemblyArea& area, std::deque<VarSummT>& summs)
{
    dbg_enter("extractVarSummaryList");

    ICHeaderT icHeader;
    icHeader.deserialize (area);
    assert(icHeader.icType == ICTYPE_SUMMARIES);
    assert(icHeader.icNumRecords > 0);

    for (int i=0; i<icHeader.icNumRecords; i++)
    {
        VarSummT summ;
        summ.deserialize (area);

        DBG_VAR2((int) summ.varId, (int) summ.seqno);

        summs.push_back(summ);
    }

    dbg_leave();
}


// ----------------------------------------------------

void VardisProtocol::extractVarReqUpdateList (DisassemblyArea& area, std::deque<VarReqUpdateT>& requpdates)
{
    dbg_enter("extractVarReqUpdateList");

    ICHeaderT icHeader;
    icHeader.deserialize (area);
    assert(icHeader.icType == ICTYPE_REQUEST_VARUPDATES);
    assert(icHeader.icNumRecords > 0);

    for (int i=0; i<icHeader.icNumRecords; i++)
    {
        VarReqUpdateT requpd;
        requpd.deserialize (area);

        DBG_VAR2((int) requpd.updSpec.varId, (int) requpd.updSpec.seqno);

        requpdates.push_back(requpd);
    }

    dbg_leave();
}

// ----------------------------------------------------

void VardisProtocol::extractVarReqCreateList(DisassemblyArea& area, std::deque<VarReqCreateT>& reqcreates)
{
    dbg_enter("extractVarReqCreateList");

    ICHeaderT icHeader;
    icHeader.deserialize (area);
    assert(icHeader.icType == ICTYPE_REQUEST_VARCREATES);
    assert(icHeader.icNumRecords > 0);

    for (int i=0; i<icHeader.icNumRecords; i++)
    {
        VarReqCreateT reqcr;
        reqcr.deserialize (area);

        DBG_VAR1((int) reqcr.varId);

        reqcreates.push_back(reqcr);
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
    conf->setVarId(varId);
    sendConfirmation(conf, status, theProtocol);

    dbg_leave();
}

// ----------------------------------------------------

void VardisProtocol::sendRTDBUpdateConfirm(DcpStatus status, VarIdT varId, Protocol* theProtocol)
{
    dbg_enter("sendRTDBUpdateConfirm");

    auto conf = new RTDBUpdate_Confirm;
    conf->setVarId(varId);
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
    return (theVariableDatabase.find(varId) != theVariableDatabase.end());
}

// ----------------------------------------------------

bool VardisProtocol::producerIsMe(VarIdT varId)
{
    DBEntry& theEntry = theVariableDatabase.at(varId);

    return theEntry.spec.prodId == getOwnNodeId();
}


// ========================================================================================
// Queue management helpers
// ========================================================================================


bool VardisProtocol::isVarIdInQueue(const std::deque<VarIdT>& q, VarIdT varId)
{
    return (std::find(q.begin(), q.end(), varId) != q.end());
}

// ----------------------------------------------------

void VardisProtocol::removeVarIdFromQueue(std::deque<VarIdT>& q, VarIdT varId)
{
    auto rems = std::remove(q.begin(), q.end(), varId);
    q.erase(rems, q.end());
}

// ----------------------------------------------------

void VardisProtocol::dropNonexistingDeleted(std::deque<VarIdT>& q)
{
    auto rems = std::remove_if(q.begin(),
                               q.end(),
                               [&](VarIdT varId){ return (    (theVariableDatabase.find(varId) == theVariableDatabase.end())
                                                           || (theVariableDatabase.at(varId).toBeDeleted));}
                              );
    q.erase(rems, q.end());
}

// ----------------------------------------------------

void VardisProtocol::dropNonexisting(std::deque<VarIdT>& q)
{
    auto rems = std::remove_if(q.begin(),
                               q.end(),
                               [&](VarIdT varId){ return (theVariableDatabase.find(varId) == theVariableDatabase.end());}
                              );
    q.erase(rems, q.end());
}

// ----------------------------------------------------

void VardisProtocol::dropDeleted(std::deque<VarIdT>& q)
{
    auto rems = std::remove_if(q.begin(),
                               q.end(),
                               [&](VarIdT varId){ return (    (theVariableDatabase.find(varId) != theVariableDatabase.end())
                                                           && (theVariableDatabase.at(varId).toBeDeleted));}
                              );
    q.erase(rems, q.end());
}


// ========================================================================================
// Debug helpers
// ========================================================================================

void VardisProtocol::dbg_queueSizes()
{
    DBG_VAR6(createQ.size(), deleteQ.size(), updateQ.size(), summaryQ.size(), reqUpdQ.size(), reqCreateQ.size());
}

// ----------------------------------------------------

void VardisProtocol::dbg_summaryQ()
{
    if (summaryQ.empty()) return;
    dbg_prefix();
    EV << "summaryQ.size = " << summaryQ.size()
       << " , contents = {";
    for (auto it = summaryQ.begin(); it != summaryQ.end(); it++)
    {
        EV << " (i:" << (int) *it
           << ", s:" << (int) theVariableDatabase.at(*it).seqno
           << ")";
    }
    EV << "}" << endl;
}

// ----------------------------------------------------

void VardisProtocol::dbg_createQ()
{
    if (createQ.empty()) return;

    dbg_prefix();
    EV << "createQ.size = " << createQ.size()
       << " , contents = {";
    for (auto it = createQ.begin(); it != createQ.end(); it++)
    {
        EV << " (i:" << (int) *it
           << ", s:" << (int) theVariableDatabase.at(*it).seqno
           << ", c:" << (int) theVariableDatabase.at(*it).countCreate
           << ")";
    }
    EV << "}" << endl;
}

// ----------------------------------------------------

void VardisProtocol::dbg_updateQ()
{
    if (updateQ.empty()) return;

    dbg_prefix();
    EV << "updateQ.size = " << updateQ.size()
       << " , contents = {";
    for (auto it = updateQ.begin(); it != updateQ.end(); it++)
    {
        EV << " (i:" << (int) *it
           << ", s:" << (int) theVariableDatabase.at(*it).seqno
           << ", c:" << (int) theVariableDatabase.at(*it).countUpdate
           << ")";
    }
    EV << "}" << endl;
}

// ----------------------------------------------------

void VardisProtocol::dbg_reqCreateQ()
{
    if (reqCreateQ.empty()) return;

    dbg_prefix();
    EV << "reqCreateQ.size = " << reqCreateQ.size()
       << " , contents = {";
    for (auto it = reqCreateQ.begin(); it != reqCreateQ.end(); it++)
    {
        EV << " (i:" << (int) *it
           << ")";
    }
    EV << "}" << endl;
}

// ----------------------------------------------------

void VardisProtocol::dbg_reqUpdateQ()
{
    if (reqUpdQ.empty()) return;

    dbg_prefix();
    EV << "reqUpdQ.size = " << reqUpdQ.size()
       << " , contents = {";
    for (auto it = reqUpdQ.begin(); it != reqUpdQ.end(); it++)
    {
        EV << " (i:" << (int) *it
           << ", c:" << (int) theVariableDatabase.at(*it).seqno
           << ")";
    }
    EV << "}" << endl;
}

// ----------------------------------------------------

void VardisProtocol::dbg_database()
{
    if (theVariableDatabase.empty()) return;

    dbg_prefix();
    EV << "database.size = " << theVariableDatabase.size()
       << " , contents = {";
    for (auto it = theVariableDatabase.begin(); it != theVariableDatabase.end(); it++)
    {
        EV << " (i:" << (int) (it->second.spec.varId)
           << ", s:" << (int) (it->second.seqno)
           << ", r:" << (int) (it->second.spec.repCnt)
           << ", cc:" << (int) (it->second.countCreate)
           << ", cu:" << (int) (it->second.countUpdate)
           << ", cd:" << (int) (it->second.countDelete)
           << ")";
    }
    EV << "}" << endl;
}

// ----------------------------------------------------

void VardisProtocol::dbg_database_complete()
{
    if (theVariableDatabase.empty()) return;

    dbg_enter("dbg_database_complete");
    dbg_prefix();
    EV << "database.size = " << theVariableDatabase.size()
       << " , contents = {" << endl;
    for (auto it = theVariableDatabase.begin(); it != theVariableDatabase.end(); it++)
    {
        DBEntry& theEntry = it->second;
        dbg_prefix();
        EV << "      (i:" << (int) (theEntry.spec.varId)
           << ", s:" << (int) (theEntry.seqno)
           << ", r:" << (int) (theEntry.spec.repCnt)
           << ", cc:" << (int) (theEntry.countCreate)
           << ", cu:" << (int) (theEntry.countUpdate)
           << ", cd:" << (int) (theEntry.countDelete)
           << " , descrLen = " << (int) (theEntry.spec.descr.length)
           << " , descr = " << (theEntry.spec.descr.to_str())
           << (theEntry.toBeDeleted ? " TO-BE-DELETED" : "")
           << ")" << endl;
    }
    dbg_prefix();
    EV << "}" << endl;
    dbg_leave();
}

// ----------------------------------------------------

void VardisProtocol::dbg_comprehensive(std::string methname)
{
    dbg_enter(methname);

    dbg_queueSizes();
    dbg_summaryQ();
    dbg_createQ();
    dbg_updateQ();
    dbg_reqCreateQ();
    dbg_reqUpdateQ();
    dbg_database();

    dbg_leave();
}

// ----------------------------------------------------

void VardisProtocol::assert_createQ()
{
    if (createQ.empty()) return;
    for (auto it : createQ)
    {
        VarIdT           varId = it;
        NodeIdentifierT  ownId = getOwnNodeId();
        std::stringstream addrss;
        addrss << "address = " << ownId;
        std::string addrstring = addrss.str();
        if (theVariableDatabase.find(varId) == theVariableDatabase.end())
        {
            DBG_PVAR3("no database entry", (int) varId, ownId, addrstring);
            error("assert_createQ: varId not contained in database");
        }

        if (theVariableDatabase.at(varId).countCreate == 0)
        {
            DBG_PVAR3("database entry has countCreate = 0", (int) varId, ownId, addrstring);
            error("assert_createQ: countCreate is zero");
        }
    }
}

// ----------------------------------------------------

void VardisProtocol::assert_updateQ()
{
    if (updateQ.empty()) return;
    for (auto it : updateQ)
    {
        VarIdT varId = it;
        if (theVariableDatabase.find(varId) == theVariableDatabase.end())
        {
            DBG_PVAR1("no database entry for variable", (int) varId);
            error("assert_updateQ: varId not contained in database");
        }

        if (theVariableDatabase.at(varId).countUpdate == 0)
        {
            DBG_PVAR1("database entry for variable has countUpdate = 0", (int) varId);
            error("assert_updateQ: countUpdate is zero");
        }
    }
}

// ----------------------------------------------------

void VardisProtocol::assert_queues()
{
    dbg_enter("assert_queues");
    assert_createQ();
    assert_updateQ();
    dbg_leave();
}
