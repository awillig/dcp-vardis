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

#include <dcpsim/applications/VardisVariableConsumer.h>
#include <dcpsim/applications/VardisVariableExample.h>

/*
 * This is a simple consumer module, pairing with the 'VariableProducer' module.
 * We assume here that *all* variables are just individual ExampleVariable values.
 * The consumer periodically requests a database description from VarDis and
 * requests the values of all currently existing variables.
 */

using namespace dcp;

Define_Module(VardisVariableConsumer);



// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================


void VardisVariableConsumer::initialize(int stage)
{
    VardisApplication::initialize(stage);

    if (stage == INITSTAGE_LAST)
    {
        dbg_setModuleName("VardisVariableConsumer");
        dbg_enter("initialize");
        assert(getOwnNodeId() != nullNodeIdentifier);

        // read and check module parameters
        consumerActive    = par("consumerActive");
        samplingPeriod    = par("samplingPeriod");
        varIdToObserve    = par("varIdToObserve");
        assert(samplingPeriod > 0);

        // register statistics signals
        delaySig  = registerSignal("updateDelaySignal");
        seqnoSig  = registerSignal("seqnoDeltaSignal");
        rxTimeSig = registerSignal("receptionTimeSignal");

        if (consumerActive)
        {
            // create and schedule sampleMessage. Sampling refers to the process of
            // asking VarDis for a database description and querying all current
            // variables
            sampleMsg =  new cMessage("VardisVariableConsumer:sampleMsg");
            state = cState_WaitForSampling;
            scheduleAt(simTime() + samplingPeriod, sampleMsg);

            // register a separate protocol for this consumer and register it
            // as Vardis client protocol with dispatcher
            std::stringstream ssLc, ssUc;
            ssLc << "vardisvariableconsumer[" << getOwnNodeId() << "]";
            ssUc << "VARDISVARIABLECONSUMER[" << getOwnNodeId() << "]";
            createProtocol(ssLc.str().c_str(), ssUc.str().c_str());
        }

        dbg_leave();
     }

}

// ----------------------------------------------------

void VardisVariableConsumer::handleMessage(cMessage *msg)
{
    dbg_assertToplevel();
    dbg_enter("handleMessage");
    DBG_VAR1(state);

    // dispatch on type of received message

    if (msg == sampleMsg)
    {
        handleSampleMsg();
        dbg_leave();
        return;
    }

    if ((msg->arrivedOn(gidFromDcpProtocol)) && dynamic_cast<RTDBDescribeDatabase_Confirm*>(msg))
     {
         RTDBDescribeDatabase_Confirm* dbConf = (RTDBDescribeDatabase_Confirm*) msg;
         handleRTDBDescribeDatabaseConfirm(dbConf);
         dbg_leave();
         return;
     }

    if ((msg->arrivedOn(gidFromDcpProtocol)) && dynamic_cast<RTDBRead_Confirm*>(msg))
     {
         RTDBRead_Confirm* readConf = (RTDBRead_Confirm*) msg;
         handleRTDBReadConfirm(readConf);
         dbg_leave();
         return;
     }

    error("VardisVariableConsumer::handleMessage: unknown message type");

    dbg_leave();
}

// ----------------------------------------------------

VardisVariableConsumer::~VardisVariableConsumer()
{
    cancelAndDelete(sampleMsg);
}


// ========================================================================================
// Message handlers
// ========================================================================================


void VardisVariableConsumer::handleSampleMsg()
{
    dbg_enter("handleSampleMsg");
    assert(state == cState_WaitForSampling);

    // schedule next sampling message
    scheduleAt(simTime() + samplingPeriod, sampleMsg);

    // send database description request and change state
    auto dbReq = new RTDBDescribeDatabase_Request;
    sendToVardis (dbReq);
    state = cState_WaitForDBDescription;

    dbg_leave();
}

// ----------------------------------------------------

void VardisVariableConsumer::handleRTDBDescribeDatabaseConfirm(RTDBDescribeDatabase_Confirm* dbConf)
{
    dbg_enter("handleRTDBDescribeDatabaseConfirm");
    assert(dbConf);
    assert(state == cState_WaitForDBDescription);
    assert(readsRequested == 0);

    // check for empty database
    if (dbConf->getDescrsArraySize() == 0)
    {
        dbg_string("database is empty");
        state = cState_WaitForSampling;
        dbg_leave();
        delete dbConf;
        return;
    }

    // non-empty database: generate read request for each listed variable
    for (size_t i=0; i<dbConf->getDescrsArraySize(); i++)
    {
        auto spec = dbConf->getDescrs (i);

        DBG_PVAR2("requesting read", spec.varId, spec.prodId);

        auto readReq = new RTDBRead_Request;
	RTDB_Read_Request rReq;
	rReq.varId = spec.varId;
	readReq->setReadReq (rReq);
        sendToVardis(readReq);
    }

    // change state
    state           =  cState_WaitForReadResponses;
    readsRequested  =  dbConf->getDescrsArraySize();

    delete dbConf;

    dbg_leave();
}

// ----------------------------------------------------

void VardisVariableConsumer::handleRTDBReadConfirm(RTDBRead_Confirm* readConf)
{
    dbg_enter("handleRTDBReadConfirm");

    RTDB_Read_Confirm rConf = readConf->getReadConf();
    
    assert(readConf);
    assert(state == cState_WaitForReadResponses);
    assert(rConf.status_code == VARDIS_STATUS_OK);
    assert(readsRequested > 0);
    assert(rConf.value.len == sizeof(VardisExampleVariable));

    // copy received data into local variable
    VardisExampleVariable theValue;
    uint8_t *thePtr = (uint8_t*) &theValue;
    std::memcpy (thePtr, rConf.value.data, sizeof(VardisExampleVariable));

    // if variable is new or has updated value print debug output and record statistics
    // (for one selected variable)
    VarIdT varId = rConf.varId;

    DBG_PVAR3("CONSIDERING", varId, readsRequested, (lastReceived.find(varId) == lastReceived.end()));

    if (    (lastReceived.find(varId) == lastReceived.end())
         || (theValue.seqno != lastReceived[varId].seqno))
    {
        DBG_PVAR5("UPDATING VARIABLE VALUE", varId, theValue.value, theValue.seqno, theValue.tstamp, (simTime() - theValue.tstamp));

        if (varIdToObserve == varId.val)
        {
            DBG_PVAR4("EMITTING statistics", varId, 1000.0d * (SIMTIME_DBL(simTime() - theValue.tstamp)), theValue.seqno - lastReceived[varId].seqno, simTime());
            emit(delaySig, 1000.0d * (SIMTIME_DBL(simTime() - theValue.tstamp)));
            emit(seqnoSig, theValue.seqno - lastReceived[varId].seqno);
            emit(rxTimeSig, simTime());
        }

    }

    // update database and adjust state when all variables have been received
    lastReceived[varId] = theValue;
    readsRequested--;
    if (readsRequested == 0)
    {
        dbg_string("going back to state cState_WaitForSampling");
        state = cState_WaitForSampling;
    }

    delete readConf;

    dbg_leave();
}

