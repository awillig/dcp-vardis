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
#include <dcp/common/DcpApplication.h>

// ========================================================================================
// ========================================================================================


using namespace dcp;

// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================

void DcpApplication::initialize(int stage)
{
    DcpProtocol::initialize(stage);

    if (stage == INITSTAGE_LAST)
    {
        dbg_enter("DcpApplication::initialize");

        // find gate identifiers
        gidFromDcpProtocol =  findGate("fromDcpProtocol");
        gidToDcpProtocol   =  findGate("toDcpProtocol");

        dbg_leave();
    }
}

// ----------------------------------------------------

DcpApplication::~DcpApplication ()
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
void DcpApplication::createProtocol(const char* descr1, const char* descr2)
{
    dbg_enter("DcpApplication::createProtocol");
    assert(!theProtocol);

    theProtocol = new Protocol(descr1, descr2);
    registerProtocol(*theProtocol, gate("toDcpProtocol"), gate("fromDcpProtocol"));

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Send given message to underlying VarDis protocol instance via message
 * dispatcher
 */
void DcpApplication::sendToDcpProtocol (Protocol* targetProtocol, Message* message)
{
    dbg_enter("sendToDcpProtocol/Message");
    assert(targetProtocol);
    assert(message);
    assert(getProtocol());

    // add required tags and parameters to message
    message->removeTagIfPresent<DispatchProtocolReq>();
    auto req = message->addTagIfAbsent<DispatchProtocolReq>();
    req->setProtocol(targetProtocol);
    req->setServicePrimitive(SP_REQUEST);
    message->removeTagIfPresent<DispatchProtocolInd>();
    auto ind = message->addTagIfAbsent<DispatchProtocolInd>();
    ind->setProtocol(getProtocol());

    // send it to SRP
    send(message, gidToDcpProtocol);

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Send given packet to underlying SRP protocol instance via message
 * dispatcher
 */
void DcpApplication::sendToDcpProtocol (Protocol* targetProtocol, Packet* packet)
{
    dbg_enter("sendToSRP/Packet");
    assert(targetProtocol);
    assert(packet);
    assert(getProtocol());

    // add required tags and parameters to packet
    packet->removeTagIfPresent<DispatchProtocolReq>();
    auto req = packet->addTagIfAbsent<DispatchProtocolReq>();
    req->setProtocol(targetProtocol);
    req->setServicePrimitive(SP_REQUEST);
    packet->removeTagIfPresent<DispatchProtocolInd>();
    auto ind = packet->addTagIfAbsent<DispatchProtocolInd>();
    ind->setProtocol(getProtocol());

    // send it to SRP
    send(packet, gidToDcpProtocol);

    dbg_leave();
}
