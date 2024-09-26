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
#include <dcp/vardis/VardisClientProtocol.h>

// ========================================================================================
// ========================================================================================


using namespace dcp;

// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================

void VardisClientProtocol::initialize(int stage)
{
    DcpProtocol::initialize(stage);

    if (stage == INITSTAGE_LAST)
    {
        dbg_enter("VardisClientProtocol::initialize");

        // find gate identifiers
        gidFromVardis =  findGate("fromVardis");
        gidToVardis   =  findGate("toVardis");

        dbg_leave();
    }
}

// ----------------------------------------------------

VardisClientProtocol::~VardisClientProtocol ()
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
void VardisClientProtocol::createProtocol(const char* descr1, const char* descr2)
{
    dbg_enter("VardisClientProtocol::createProtocol");
    assert(!theProtocol);

    theProtocol = new Protocol(descr1, descr2);
    registerProtocol(*theProtocol, gate("toVardis"), gate("fromVardis"));

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Send given message to underlying VarDis protocol instance via message
 * dispatcher
 */
void VardisClientProtocol::sendToVardis (Message* message)
{
    dbg_enter("sendToVardis/Message");
    assert(message);
    assert(getProtocol());

    // add required tags and parameters to message
    message->removeTagIfPresent<DispatchProtocolReq>();
    auto req = message->addTagIfAbsent<DispatchProtocolReq>();
    req->setProtocol(DcpSimGlobals::protocolDcpVardis);
    req->setServicePrimitive(SP_REQUEST);
    message->removeTagIfPresent<DispatchProtocolInd>();
    auto ind = message->addTagIfAbsent<DispatchProtocolInd>();
    ind->setProtocol(getProtocol());

    // send it to VarDis
    send(message, gidToVardis);

    dbg_leave();
}

// ----------------------------------------------------

/**
 * Send given packet to underlying VarDis protocol instance via message
 * dispatcher
 */
void VardisClientProtocol::sendToVardis (Packet* packet)
{
    dbg_enter("sendToVardis/Packet");
    assert(packet);
    assert(getProtocol());

    // add required tags and parameters to packet
    packet->removeTagIfPresent<DispatchProtocolReq>();
    auto req = packet->addTagIfAbsent<DispatchProtocolReq>();
    req->setProtocol(DcpSimGlobals::protocolDcpVardis);
    req->setServicePrimitive(SP_REQUEST);
    packet->removeTagIfPresent<DispatchProtocolInd>();
    auto ind = packet->addTagIfAbsent<DispatchProtocolInd>();
    ind->setProtocol(getProtocol());

    // send it to VarDis
    send(packet, gidToVardis);

    dbg_leave();
}

// ----------------------------------------------------

static const std::map<VardisStatus, std::string> status_texts = {
        {VARDIS_STATUS_OK, "VARDIS_STATUS_OK"},
        {VARDIS_STATUS_VARIABLE_EXISTS, "VARDIS_STATUS_VARIABLE_EXISTS"},
        {VARDIS_STATUS_VARIABLE_DESCRIPTION_TOO_LONG, "VARDIS_STATUS_VARIABLE_DESCRIPTION_TOO_LONG"},
        {VARDIS_STATUS_VALUE_TOO_LONG, "VARDIS_STATUS_VALUE_TOO_LONG"},
        {VARDIS_STATUS_EMPTY_VALUE, "VARDIS_STATUS_EMPTY_VALUE"},
        {VARDIS_STATUS_ILLEGAL_REPCOUNT, "VARDIS_STATUS_ILLEGAL_REPCOUNT"},
        {VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST, "VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST"},
        {VARDIS_STATUS_NOT_PRODUCER, "VARDIS_STATUS_NOT_PRODUCER"},
        {VARDIS_STATUS_VARIABLE_BEING_DELETED, "VARDIS_STATUS_VARIABLE_BEING_DELETED"}
};

/**
 * Convert VarDis status value to string
 */
std::string VardisClientProtocol::getVardisStatusString(VardisStatus status)
{
    auto search = status_texts.find(status);
    if (search != status_texts.end())
    {
        return search->second;
    }
    else
    {
        error("VardisClientProtocol::getVardisStatusString: received status value not in status_texts");
    }
    return "";
}

// ----------------------------------------------------

/**
 * prints VarDis status value in a log message
 */
void VardisClientProtocol::printStatus (VardisStatus status)
{
    dbg_enter("VardisClientProtocol::printStatus");

    auto statstr = getVardisStatusString(status);
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
 * Default handler for VarDis confirmation primitives, just prints their
 * status value
 */
void VardisClientProtocol::handleVardisConfirmation(VardisConfirmation* conf)
{
    dbg_enter("VardisClientProtocol::handleVardisConfirmation");
    assert(conf);
    printStatus(conf->getStatus());
    dbg_leave();
}


