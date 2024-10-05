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
#include <dcp/srp/SRPClientProtocol.h>

// ========================================================================================
// ========================================================================================


using namespace dcp;

// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================

void SRPClientProtocol::initialize(int stage)
{
    DcpProtocol::initialize(stage);

    if (stage == INITSTAGE_LAST)
    {
        dbg_enter("SRPClientProtocol::initialize");

        // find gate identifiers
        gidFromSRP =  findGate("fromSRP");
        gidToSRP   =  findGate("toSRP");

        dbg_leave();
    }
}

// ----------------------------------------------------

SRPClientProtocol::~SRPClientProtocol ()
{
    if (theProtocol) delete theProtocol;
}

// ========================================================================================
// Helper methods
// ========================================================================================

/**
 * Creates and registers protocol object with INET runtime, to be used by
 * message dispatcher
 */
void SRPClientProtocol::createProtocol(const char* descr1, const char* descr2)
{
    dbg_enter("SRPClientProtocol::createProtocol");
    assert(!theProtocol);

    theProtocol = new Protocol(descr1, descr2);
    registerProtocol(*theProtocol, gate("toSRP"), gate("fromSRP"));

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Send given message to underlying VarDis protocol instance via message
 * dispatcher
 */
void SRPClientProtocol::sendToSRP (Message* message)
{
    dbg_enter("sendToSRP/Message");
    assert(message);
    assert(getProtocol());

    // add required tags and parameters to message
    message->removeTagIfPresent<DispatchProtocolReq>();
    auto req = message->addTagIfAbsent<DispatchProtocolReq>();
    req->setProtocol(DcpSimGlobals::protocolDcpSRP);
    req->setServicePrimitive(SP_REQUEST);
    message->removeTagIfPresent<DispatchProtocolInd>();
    auto ind = message->addTagIfAbsent<DispatchProtocolInd>();
    ind->setProtocol(getProtocol());

    // send it to SRP
    send(message, gidToSRP);

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Send given packet to underlying SRP protocol instance via message
 * dispatcher
 */
void SRPClientProtocol::sendToSRP (Packet* packet)
{
    dbg_enter("sendToSRP/Packet");
    assert(packet);
    assert(getProtocol());

    // add required tags and parameters to packet
    packet->removeTagIfPresent<DispatchProtocolReq>();
    auto req = packet->addTagIfAbsent<DispatchProtocolReq>();
    req->setProtocol(DcpSimGlobals::protocolDcpSRP);
    req->setServicePrimitive(SP_REQUEST);
    packet->removeTagIfPresent<DispatchProtocolInd>();
    auto ind = packet->addTagIfAbsent<DispatchProtocolInd>();
    ind->setProtocol(getProtocol());

    // send it to SRP
    send(packet, gidToSRP);

    dbg_leave();
}

// ----------------------------------------------------

static const std::map<SRPStatus, std::string> status_texts = {
        {SRP_STATUS_OK, "SRP_STATUS_OK"}
};

/**
 * Convert SRP status value to string
 */
std::string SRPClientProtocol::getSRPStatusString(SRPStatus status)
{
    auto search = status_texts.find(status);
    if (search != status_texts.end())
    {
        return search->second;
    }
    else
    {
        error("SRPClientProtocol::getSRPStatusString: received status value not in status_texts");
    }
    return "";
}

// ----------------------------------------------------

/**
 * prints VarDis status value in a log message
 */
void SRPClientProtocol::printStatus (SRPStatus status)
{
    dbg_enter("SRPClientProtocol::printStatus");

    auto statstr = getSRPStatusString(status);
    dbg_prefix();
    EV << "status value is "
       << status
       << " , text is "
       << statstr
       << endl;

    dbg_leave();

}

// ----------------------------------------------------

/**
 * Default handler for SRP confirmation primitives, just prints their
 * status value
 */
void SRPClientProtocol::handleSRPConfirmation(SRPUpdateSafetyData_Confirm* conf)
{
    dbg_enter("VardisSRPProtocol::handleSRPConfirmation");
    assert(conf);
    printStatus(conf->getStatus());
    dbg_leave();
}





