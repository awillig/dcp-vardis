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
#include <dcp/srp/StateReportingProtocol.h>
#include <dcp/srp/ExtendedSafetyDataT_m.h>
#include <dcp/common/DcpTypesGlobals.h>
#include <dcp/bp/BPQueueingMode_m.h>
#include <dcp/bp/BPTransmitPayload_m.h>
#include <dcp/bp/BPPayloadTransmitted_m.h>
#include <dcp/bp/BPReceivePayload_m.h>

// ========================================================================================
// ========================================================================================


using namespace omnetpp;
using namespace inet;
using namespace dcp;


Define_Module(StateReportingProtocol);


// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================


void StateReportingProtocol::initialize(int stage)
{
    assert(DcpSimGlobals::protocolDcpSRP);

    BPClientProtocol::initialize(stage);

    if (stage == INITSTAGE_LAST)
    {
        dbg_setModuleName("SRP");
        dbg_enter("initialize");

        // read and check module parameters
        _srpNeighbourTableTimeout      = par("srpNeighbourTableTimeout");
        _srpNeighbourTableScrubPeriod  = par("srpNeighbourTableScrubPeriod");
        _srpNeighbourTablePrintPeriod  = par("srpNeighbourTablePrintPeriod");
        assert(_srpNeighbourTableTimeout > 0);
        assert(_srpNeighbourTableScrubPeriod > 0);
        assert(_srpNeighbourTableTimeout > 4*_srpNeighbourTableScrubPeriod);

        // find gate identifiers
        gidFromApplication  = findGate ("fromApplication");
        gidToApplication    = findGate ("toApplication");

        // register ourselves as BP client protocol with dispatcher
        registerProtocol(*DcpSimGlobals::protocolDcpSRP, gate("toBP"), gate("fromBP"));

        // and register ourselves as a service for SRP applications
        registerService(*DcpSimGlobals::protocolDcpSRP, gate("fromApplication"), gate("toApplication"));

        // get periodic scrubbing going
        _scrubNeighbourTableMsg = new cMessage("srpScrubNeighbourTableMsg");
        scheduleAt(simTime() + _srpNeighbourTableScrubPeriod, _scrubNeighbourTableMsg);

        // get periodic printing going when requested
        if (_srpNeighbourTablePrintPeriod > 0)
        {
            _printNeighbourTableMsg = new cMessage("srpPrintNeighbourTableMsg");
            scheduleAt(simTime() + _srpNeighbourTablePrintPeriod, _printNeighbourTableMsg);
        }

        dbg_leave();
    }
}

// ----------------------------------------------------

void StateReportingProtocol::handleMessage(cMessage *msg)
{
    dbg_assertToplevel();
    dbg_enter("handleMessage");

    // dispatch on received message type

    if (hasHandledMessageBPClient(msg))
    {
        dbg_leave();
        return;
    }

    if (msg == _scrubNeighbourTableMsg)
    {
        dbg_string("handling _scrubNeighbourTableMsg");
        handleScrubNeighbourTableMsg();
        dbg_leave();
        return;
    }

    if (msg == _printNeighbourTableMsg)
    {
        dbg_string("handling _printNeighbourTableMsg");
        handlePrintNeighbourTableMsg();
        dbg_leave();
        return;
    }

    if ((msg->arrivedOn(gidFromApplication)) && dynamic_cast<SRPUpdateSafetyData_Request*>(msg))
    {
        dbg_string("handling _generatePayloadMsg");
        SRPUpdateSafetyData_Request*  srpReq = (SRPUpdateSafetyData_Request*) msg;
        handleUpdateSafetyDataRequestMsg(srpReq);
        dbg_leave();
        return;
    }

    if ((msg->arrivedOn(gidFromBP)) && dynamic_cast<BPPayloadTransmitted_Indication*>(msg))
     {
        dbg_string("handling BPPayloadTransmitted_Indication");
        delete msg;
        dbg_leave();
        return;
     }

    if ((msg->arrivedOn(gidFromBP)) && dynamic_cast<BPReceivePayload_Indication*>(msg))
     {
        dbg_string("handling BPReceivePayload_Indication");
        BPReceivePayload_Indication *payload = (BPReceivePayload_Indication*) msg;
        handleReceivedPayload(payload);
        dbg_leave();
        return;
     }

    error("StateReportingProtocol::handleMessage: unknown message");

    dbg_leave();
}

// -------------------------------------------

StateReportingProtocol::~StateReportingProtocol()
{
    if (_scrubNeighbourTableMsg)  cancelAndDelete(_scrubNeighbourTableMsg);
    if (_printNeighbourTableMsg)  cancelAndDelete(_printNeighbourTableMsg);
}


// ========================================================================================
// Message handlers
// ========================================================================================

/**
 * Generates and sends an SRP payload, schedules next generation of payload
 */
void StateReportingProtocol::handleUpdateSafetyDataRequestMsg (SRPUpdateSafetyData_Request* srpReq)
{
    dbg_enter("handleUpdateSafetyDataRequestMsg");
    assert(srpReq);


    // check if we are registered with BP
    if (isSuccessfullyRegisteredWithBP())
    {
        dbg_string("handleUpdateSafetyDataRequestMsg: we are successfully registered");
        dbg_string("handleUpdateSafetyDataRequestMsg: generating the payload");

        // create the actual SRP message content
        auto esd = makeShared<ExtendedSafetyDataT>();
        assert(esd);
        esd->setSafetyData(srpReq->getSafetyData());
        esd->setNodeId(getOwnNodeId());
        esd->setTimeStamp(simTime());
        esd->setSeqno(_seqno++);

        DBG_PVAR1("generated payload size is ", esd->getChunkLength().get() / 8);

        // construct and send payload to BP
        dbg_string("constructing the packet");
        BPTransmitPayload_Request  *pldReq = new BPTransmitPayload_Request ("SRPPayload");
        assert(pldReq);
        pldReq->setProtId(BP_PROTID_SRP);
        pldReq->insertAtFront(esd);
        dbg_string("sending the packet/payload to BP");
        sendToBP(pldReq);

    }

    delete srpReq;

    dbg_leave();
}


// -------------------------------------------

/**
 * Traverses neighbor table to delete too old entries, schedules next scrubbing
 * traversal
 */
void StateReportingProtocol::handleScrubNeighbourTableMsg ()
{
    dbg_enter("handleScrubNeighbourTableMsg");

    std::list<NodeIdentifierT>  toBeDeleted;

    // traverse neighbor table and mark all too old entries for deletion
    for (auto nte : neighbourTable)
    {
        if ((simTime() - nte.second.receptionTime) > _srpNeighbourTableTimeout)
        {
            DBG_PVAR1("scrubbing node ", nte.second.nodeId);
            toBeDeleted.push_front(nte.second.nodeId);
        }
    }

    // delete the marked entries
    for (auto tbd : toBeDeleted)
    {
        neighbourTable.erase(tbd);
    }

    // Schedule next scrubbing action
    scheduleAt(simTime() + _srpNeighbourTableScrubPeriod, _scrubNeighbourTableMsg);

    dbg_leave();
}

// -------------------------------------------

/**
 * Prints contents of neighbor table as logging output, schedules next printing
 */
void StateReportingProtocol::handlePrintNeighbourTableMsg ()
{
    dbg_enter("handlePrintNeighbourTableMsg");

    for (auto nte : neighbourTable)
    {
        NeighbourTableEntry& theEntry = nte.second;

        dbg_prefix();
        EV << "neighbour-Id " << theEntry.nodeId
           << " with generation timestamp = " << theEntry.extSD.getTimeStamp()
           << " , age = " << simTime() - theEntry.extSD.getTimeStamp()
           << " , seqno = " << theEntry.extSD.getSeqno()
           << " , from position ("
           << theEntry.extSD.getSafetyData().position_x
           << ", " << theEntry.extSD.getSafetyData().position_y
           << ", " << theEntry.extSD.getSafetyData().position_z
           << ") and with age "
           << (simTime() - theEntry.extSD.getTimeStamp())
           << endl;
    }

    // Schedule next scrubbing action
    scheduleAt(simTime() + _srpNeighbourTablePrintPeriod, _printNeighbourTableMsg);

    dbg_leave();
}



// -------------------------------------------

/**
 * Adds received SRP payload to neighbor table
 */
void StateReportingProtocol::handleReceivedPayload(BPReceivePayload_Indication* payload)
{
    dbg_enter("handleReceivedPayload");
    assert(payload);
    assert(payload->getProtId() == BP_PROTID_SRP);

    auto esd = payload->popAtFront<ExtendedSafetyDataT>();
    delete payload;

    NodeIdentifierT senderId = esd->getNodeId();

    dbg_prefix();
    EV << "received payload from sender " << senderId
       << " with generation timestamp = " << esd->getTimeStamp()
       << " , seqno = " << esd->getSeqno()
       << " , from position ("
       << esd->getSafetyData().position_x
       << ", " << esd->getSafetyData().position_y
       << ", " << esd->getSafetyData().position_z
       << ") and with delay "
       << (simTime() - esd->getTimeStamp())
       << endl;

    NeighbourTableEntry entry;
    entry.nodeId         = senderId;
    entry.extSD          = *esd;
    entry.receptionTime  = simTime();
    neighbourTable[senderId] = entry;

    dbg_leave();
}

// ========================================================================================
// Helpers
// ========================================================================================

void StateReportingProtocol::registerAsBPClient(void)
{
    dbg_enter("registerAsBPClient");

    // register ourselves directly as a client protocol with BP
    sendRegisterProtocolRequest(BP_PROTID_SRP, "SRP -- State Reporting Protocol V1.1", maximumSRPPayloadLength, BP_QMODE_REPEAT, false, 0);

    dbg_leave();
}
