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


#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/common/Protocol.h>
#include <dcp/vardis/vardis_service_primitives.h>
#include <dcpsim/applications/VardisVariableProducer.h>
#include <dcpsim/vardis/VardisRTDBCreate_m.h>
#include <dcpsim/vardis/VardisRTDBDelete_m.h>
#include <dcpsim/vardis/VardisRTDBUpdate_m.h>
#include <dcpsim/applications/VardisVariableExample.h>


using namespace dcp;
using namespace dcp::vardis;


Define_Module(VardisVariableProducer);

// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================


void VardisVariableProducer::initialize(int stage)
{
    VardisApplication::initialize(stage);

    if (stage == INITSTAGE_LAST)
    {
        dbg_setModuleName("VardisVariableProducer");
        dbg_enter("initialize");
        assert(getOwnNodeId() != nullNodeIdentifier);

        // read parameters
        varId         = (VarIdT) par("varId");
        varRepCnt     = (VarRepCntT) par("varRepCnt");
        creationTime  = par("creationTime");
        deletionTime  = par("deletionTime");

        // check parameters
        assert (varId >= 0);
        assert (varId <= VarIdT::max_val());
        assert (varRepCnt >= 0);
        assert (varRepCnt <= VarRepCntT::max_val());
        assert (creationTime >= 0);
        assert (deletionTime > creationTime);

        DBG_PVAR2("Starting producer", varId, varRepCnt);

        // initialize internal state
        isActivelyGenerating = false;
        seqno                = 0;

        // create and schedule self-messages
        createMsg =  new cMessage("VardisVariableProducer:createMsg");
        updateMsg =  new cMessage("VardisVariableProducer:updateMsg");
        deleteMsg =  new cMessage("VardisVariableProducer:deleteMsg");
        scheduleAt(simTime() + creationTime, createMsg);
        scheduleAt(simTime() + deletionTime, deleteMsg);

        // register a separate protocol for this producer and register it
        // as Vardis client protocol with dispatcher
        std::stringstream ssLc, ssUc;
        ssLc << "vardisvariableproducer[" << getOwnNodeId() << "]-varId:" << varId;
        ssUc << "VARDISVARIABLEPRODUCER[" << getOwnNodeId() << "]-varId:" << varId;
        createProtocol(ssLc.str().c_str(), ssUc.str().c_str());

        dbg_leave();
     }

}

// ----------------------------------------------------

void VardisVariableProducer::handleMessage(cMessage *msg)
{
    dbg_assertToplevel();
    dbg_enter("handleMessage");

    // dispatch based on type of message

    if (msg == createMsg)
    {
        handleCreateMsg();
        dbg_leave();
        return;
    }

    if (msg == updateMsg)
    {
        handleUpdateMsg();
        dbg_leave();
        return;
    }

    if (msg == deleteMsg)
    {
        handleDeleteMsg();
        dbg_leave();
        return;
    }

    if ((msg->arrivedOn(gidFromDcpProtocol)) && dynamic_cast<RTDBCreate_Confirm*>(msg))
     {
         RTDBCreate_Confirm* createConf = (RTDBCreate_Confirm*) msg;
         handleRTDBCreateConfirm(createConf);
         dbg_leave();
         return;
     }

    if ((msg->arrivedOn(gidFromDcpProtocol)) && dynamic_cast<RTDBDelete_Confirm*>(msg))
     {
         RTDBDelete_Confirm* deleteConf = (RTDBDelete_Confirm*) msg;
         handleRTDBDeleteConfirm(deleteConf);
         dbg_leave();
         return;
     }

    if ((msg->arrivedOn(gidFromDcpProtocol)) && dynamic_cast<RTDBUpdate_Confirm*>(msg))
     {
         RTDBUpdate_Confirm* createConf = (RTDBUpdate_Confirm*) msg;
         handleRTDBUpdateConfirm(createConf);
         dbg_leave();
         return;
     }


    error("VardisVariableProducer::handleMessage: unknown message type");

    dbg_leave();
}

// ----------------------------------------------------

VardisVariableProducer::~VardisVariableProducer ()
{
    if (createMsg) cancelAndDelete(createMsg);
    if (updateMsg) cancelAndDelete(updateMsg);
    if (deleteMsg) cancelAndDelete(deleteMsg);
}


// ========================================================================================
// Message handlers
// ========================================================================================


/**
 * Generates a RTDBCreate_Request, fills it in and sends it to VarDis
 */
void VardisVariableProducer::handleCreateMsg()
{
    dbg_enter("handleCreateMsg");
    assert(not isActivelyGenerating);
    assert(nullNodeIdentifier != getOwnNodeId());

    // construction description string
    std::stringstream ssdescr;
    ssdescr << "variable/producer=" << getOwnNodeId()
            << "/varId=" << varId;

    auto              createReq = new RTDBCreate_Request;

    // Create initial value for variable
    VardisExampleVariable   varExmpl;
    varExmpl.seqno   = seqno++;
    varExmpl.value   = par("variableValue");
    varExmpl.tstamp  = simTime();

    // fill in the RTDB_Create_Request
    uint8_t      *valPtr = (uint8_t*) &varExmpl;
    RTDB_Create_Request crReq;
    crReq.spec.varId   = varId;
    crReq.spec.prodId  = getOwnNodeId();
    crReq.spec.repCnt  = varRepCnt;
    crReq.spec.descr   = StringT (ssdescr.str());
    crReq.value        = VarValueT (sizeof(VardisExampleVariable), valPtr);
    createReq->setCrReq (crReq);

    // hand over to VarDis
    sendToVardis(createReq);

    dbg_leave();
}

// ----------------------------------------------------

/**
 * If producer is active, create and fill in a RTDBUpdate_Request and send to VarDis
 */
void VardisVariableProducer::handleUpdateMsg()
{
    dbg_enter("handleUpdateMsg");

    if (isActivelyGenerating)
    {
        DBG_PVAR2("Generating update", varId, seqno);

        // construct the updated value
        VardisExampleVariable newVal;
        newVal.seqno  =  seqno++;
        newVal.value  =  par("variableValue");
        newVal.tstamp =  simTime();

        // create and fill in RTDBUpdate_Request
        uint8_t *valPtr      = (uint8_t*) &newVal;
        auto   updateReq     = new RTDBUpdate_Request;
	RTDB_Update_Request  updReq;
        updReq.varId = varId;
	updReq.value = VarValueT (sizeof(VardisExampleVariable), valPtr);

	updateReq->setUpdReq (updReq);
	
        // hand over to VarDis
        sendToVardis(updateReq);
    }

    // schedule next update
    scheduleNextUpdate();

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Generate RTDBDelete_Request and send to VarDis
 */
void VardisVariableProducer::handleDeleteMsg()
{
    dbg_enter("handleDeleteMsg");

    isActivelyGenerating = false;

    RTDB_Delete_Request del_req;
    del_req.varId = varId;
    auto deleteReq = new RTDBDelete_Request;
    deleteReq->setDelReq(del_req);
    sendToVardis(deleteReq);

    dbg_leave();
}



// ----------------------------------------------------

/**
 * Process RTDBCreate_Confirm message (check if ok, update internal state)
 */
void VardisVariableProducer::handleRTDBCreateConfirm(RTDBCreate_Confirm* createConf)
{
    dbg_enter("handleRTDBCreateConfirm");
    assert(createConf);
    assert(not isActivelyGenerating);

    // extract information
    handleVardisConfirmation(createConf);
    DcpStatus status = createConf->getStatus();
    VarIdT    varId  = createConf->getVarId();
    delete createConf;

    DBG_PVAR2("got confirm", varId, status);

    // check outcome
    if (status != VARDIS_STATUS_OK)
    {
        error("VardisVariableProducer::handleRTDBCreateConfirm: variable creation failed, stopping with error");
    }

    // update status and schedule next update
    isActivelyGenerating = true;
    scheduleNextUpdate();

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Process RTDBDelete_Confirm (check if ok, update internal state)
 */
void VardisVariableProducer::handleRTDBDeleteConfirm(RTDBDelete_Confirm* deleteConf)
{
    dbg_enter("handleRTDBDeleteConfirm");
    assert(deleteConf);
    assert(not isActivelyGenerating);

    // extract information
    handleVardisConfirmation(deleteConf);
    DcpStatus status = deleteConf->getStatus();
    delete deleteConf;

    // check outcome
    if (status != VARDIS_STATUS_OK)
    {
        error("VardisVariableProducer::handleRTDBDeleteConfirm: variable deletion failed, stopping with error");
    }

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Process RTDBUpdate_Confirm (check if ok)
 */
void VardisVariableProducer::handleRTDBUpdateConfirm(RTDBUpdate_Confirm* updateConf)
{
    dbg_enter("handleRTDBUpdateConfirm");
    assert(updateConf);

    // extract information
    handleVardisConfirmation(updateConf);
    DcpStatus status  = updateConf->getStatus();
    VarIdT    cVarId  = updateConf->getVarId();
    delete updateConf;

    DBG_PVAR2("got confirm", cVarId, status);

    // check outcome
    assert(cVarId == varId);
    if (status != VARDIS_STATUS_OK)
    {
        error("VardisVariableProducer::handleRTDBUpdateConfirm: variable update failed, stopping with error");
    }

    dbg_leave();
}


// ========================================================================================
// Other helpers
// ========================================================================================


// ----------------------------------------------------

/**
 * Schedule next update in the future
 */
void VardisVariableProducer::scheduleNextUpdate()
{
    dbg_enter("scheduleNextUpdate");

    simtime_t updDelay = par("interUpdateTimeDistr");

    DBG_VAR1(updDelay);
    assert(updDelay > 0);

    // schedule first update message and set state
    scheduleAt(simTime() + updDelay, updateMsg);

    dbg_leave();

    return;
}

// ----------------------------------------------------
