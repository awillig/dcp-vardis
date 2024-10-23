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


#include <dcp/bp/BPClientProtocol.h>

#include <map>
#include <inet/common/IProtocolRegistrationListener.h>

// ========================================================================================
// ========================================================================================


using namespace dcp;


// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================


void BPClientProtocol::initialize (int stage)
{
    DcpProtocol::initialize(stage);

    if (stage == INITSTAGE_LAST)
    {
        dbg_enter("BPClientProtocol::initialize");

        maxPayloadSize = (BPLengthT) par("maxPayloadSize");
        assert(maxPayloadSize > 0);

        assert(_registerMsg == nullptr);
        assert(!_registrationRequested);
        assert(!_successfullyRegistered);
        _registerMsg = new cMessage("BPClientProtocol::_registerMsg");
        scheduleAt(simTime(), _registerMsg);

        // find gate identifiers for underlying BP
        gidFromBP   =  findGate("fromBP");
        gidToBP     =  findGate("toBP");

        dbg_leave();
    }
}

// ----------------------------------------------------

BPClientProtocol::~BPClientProtocol()
{
    if (_registerMsg)
    {
        cancelAndDelete(_registerMsg);
    }
}

// ========================================================================================
// Methods related to registration and deregistration procedures
// ========================================================================================

bool BPClientProtocol::hasHandledMessageBPClient(cMessage* msg)
{
    dbg_enter("BPClientProtocol::hasHandledMessageBPClient");

    // Start the registration process
    if (msg == _registerMsg)
    {
        dbg_string("BPClientProtocol::handleMessage: processing _registerMsg");
        assert(!_registrationRequested);
        _registrationRequested = true;
        cancelAndDelete(_registerMsg);
        _registerMsg = nullptr;
        registerAsBPClient();
        dbg_leave();
        return true;
    }

    // Check outcome of registration attempt
    if ((msg->arrivedOn(gidFromBP)) && dynamic_cast<BPRegisterProtocol_Confirm*>(msg))
     {
        dbg_string("handling BPRegisterProtocol_Confirm");
        assert(_registrationRequested);
        assert(!_successfullyRegistered);
        BPRegisterProtocol_Confirm *confMsg = (BPRegisterProtocol_Confirm*) msg;
        if (handleBPRegisterProtocol_Confirm (confMsg))
        {
            _successfullyRegistered = true;
            _registrationRequested  = false;
        }
        delete confMsg;
        dbg_leave();
        return true;
     }

    // check outcome of deregistration attempt
    if ((msg->arrivedOn(gidFromBP)) && dynamic_cast<BPDeregisterProtocol_Confirm*>(msg))
     {
        dbg_string("handling BPDeregisterProtocol_Confirm");
        assert(!_registrationRequested);
        assert(_successfullyRegistered);
        BPDeregisterProtocol_Confirm *confMsg = (BPDeregisterProtocol_Confirm*) msg;
        if (handleBPDeregisterProtocol_Confirm (confMsg))
        {
            _successfullyRegistered = true;
            _registrationRequested  = false;
        }
        delete confMsg;
        dbg_leave();
        return true;
     }

    // check outcome of BPTransmitPayload request
    if ((msg->arrivedOn(gidFromBP)) && dynamic_cast<BPTransmitPayload_Confirm*>(msg))
     {
        dbg_string("handling BPTransmitPayload_Confirm");
        assert(!_registrationRequested);
        assert(_successfullyRegistered);
        BPTransmitPayload_Confirm *confMsg = (BPTransmitPayload_Confirm*) msg;
        handleBPTransmitPayload_Confirm (confMsg);
        delete confMsg;
        dbg_leave();
        return true;
     }

    dbg_leave();
    return false;
}

// ----------------------------------------------------


void BPClientProtocol::sendRegisterProtocolRequest (BPProtocolIdT protId,
                                                    std::string protName,
                                                    BPLengthT maxPayloadLenB,
                                                    BPQueueingMode queueingMode,
                                                    bool allowMultiplePayloads,
                                                    unsigned int maxEntries)
{
    dbg_enter("sendRegisterProtocolRequest");

    BPRegisterProtocol_Request  *reqMsg = new BPRegisterProtocol_Request;
    reqMsg->setProtId(protId);
    reqMsg->setProtName(protName.c_str());
    reqMsg->setMaxPayloadSizeB(maxPayloadLenB);
    reqMsg->setQueueingMode(queueingMode);
    reqMsg->setAllowMultiplePayloads(allowMultiplePayloads);
    reqMsg->setMaxEntries(maxEntries);
    sendToBP(reqMsg);

    dbg_leave();
}

// ----------------------------------------------------


void BPClientProtocol::sendDeregisterProtocolRequest (BPProtocolIdT protId)
{
    dbg_enter("sendDeregisterProtocolRequest");

    BPDeregisterProtocol_Request  *reqMsg = new BPDeregisterProtocol_Request;
    reqMsg->setProtId(protId);
    sendToBP(reqMsg);

    dbg_leave();

}

// ========================================================================================
// Methods related to sending packets or messages to the BP
// ========================================================================================

void BPClientProtocol::sendToBP (Message *message)
{
    dbg_enter("sendToBP[Message]");

    assert(message);

    message->removeTagIfPresent<DispatchProtocolReq>();
    auto req = message->addTagIfAbsent<DispatchProtocolReq>();
    req->setProtocol(DcpSimGlobals::protocolDcpBP);
    req->setServicePrimitive(SP_REQUEST);

    send(message, gidToBP);

    dbg_leave();
}

// ----------------------------------------------------

void BPClientProtocol::sendToBP (Packet *packet)
{
    dbg_enter("sendToBP[Packet]");
    assert(packet);

    packet->removeTagIfPresent<DispatchProtocolReq>();
    auto req = packet->addTagIfAbsent<DispatchProtocolReq>();
    req->setProtocol(DcpSimGlobals::protocolDcpBP);
    req->setServicePrimitive(SP_REQUEST);

    send(packet, gidToBP);

    dbg_leave();
}

// ========================================================================================
// Default handlers for confirmations received from BP
// ========================================================================================

bool BPClientProtocol::handleBPRegisterProtocol_Confirm (BPRegisterProtocol_Confirm *pConf)
{
    dbg_enter("BPClientProtocol::BPRegisterProtocol_Confirm");
    assert(pConf);

    handleStatus(pConf->getStatus());

    dbg_leave();

    return (pConf->getStatus() == BP_STATUS_OK);
}

// ----------------------------------------------------

bool BPClientProtocol::handleBPDeregisterProtocol_Confirm (BPDeregisterProtocol_Confirm *pConf)
{
    dbg_enter("BPClientProtocol::BPDeregisterProtocol_Confirm");
    assert(pConf);

    handleStatus(pConf->getStatus());

    dbg_leave();

    return (pConf->getStatus() == BP_STATUS_OK);
}

// ----------------------------------------------------

bool BPClientProtocol::handleBPTransmitPayload_Confirm (BPTransmitPayload_Confirm *pConf)
{
    dbg_enter("BPClientProtocol::BPTransmitPayload_Confirm");
    assert(pConf);

    handleStatus(pConf->getStatus());

    dbg_leave();

    return (pConf->getStatus() == BP_STATUS_OK);
}

// ========================================================================================
// Printing information about status report contained in confirmation message
// ========================================================================================


const std::map<BPStatus, std::string> status_texts = {
        {BP_STATUS_OK, "BP_STATUS_OK"},
        {BP_STATUS_PROTOCOL_ALREADY_REGISTERED, "BP_STATUS_PROTOCOL_ALREADY_REGISTERED"},
        {BP_STATUS_ILLEGAL_MAX_PAYLOAD_SIZE, "BP_STATUS_ILLEGAL_MAX_PAYLOAD_SIZE"},
        {BP_STATUS_UNKNOWN_PROTOCOL, "BP_STATUS_UNKNOWN_PROTOCOL"},
        {BP_STATUS_PAYLOAD_TOO_LARGE, "BP_STATUS_PAYLOAD_TOO_LARGE"},
        {BP_STATUS_EMPTY_PAYLOAD, "BP_STATUS_EMPTY_PAYLOAD"}
};


void BPClientProtocol::handleStatus (BPStatus status)
{
    dbg_enter("BPClientProtocol::handleStatus");

    auto search = status_texts.find(status);
    if (search != status_texts.end())
    {
        dbg_prefix();
        EV << "status value is "
           << status
           << " , text is "
           << search->second
           << endl;
    }
    else
    {
        error("BPClientProtocol::handleStatus: received status value not in status_texts");
    }

    dbg_leave();

}



