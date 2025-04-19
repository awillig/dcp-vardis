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
#include <dcpsim/vardis/VardisApplication.h>




// ========================================================================================
// ========================================================================================


using namespace dcp;

// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================

void VardisApplication::initialize(int stage)
{
    DcpApplication::initialize(stage);

    if (stage == INITSTAGE_LAST)
    {
        dbg_enter("VardisApplication::initialize");
        dbg_leave();
    }
}


// ========================================================================================
// Helper methods
// ========================================================================================


// ----------------------------------------------------

/**
 * Send given message to underlying VarDis protocol instance via message
 * dispatcher
 */
void VardisApplication::sendToVardis (Message* message)
{
    dbg_enter("sendToVardis/Message");
    sendToDcpProtocol(DcpSimGlobals::protocolDcpVardis, message);
    dbg_leave();
}

// ----------------------------------------------------

/**
 * Send given packet to underlying VarDis protocol instance via message
 * dispatcher
 */
void VardisApplication::sendToVardis (Packet* packet)
{
    dbg_enter("sendToVardis/Packet");
    sendToDcpProtocol(DcpSimGlobals::protocolDcpVardis, packet);
    dbg_leave();
}


// ----------------------------------------------------

/**
 * prints VarDis status value in a log message
 */
void VardisApplication::printStatus (DcpStatus status)
{
    dbg_enter("VardisApplication::printStatus");

    auto statstr = vardis_status_to_string (status);
    dbg_prefix();
    EV << "status is "
       << statstr
       << endl;

    dbg_leave();

}

// ----------------------------------------------------

/**
 * Default handler for VarDis confirmation primitives, just prints their
 * status value
 */
void VardisApplication::handleVardisConfirmation(VardisConfirmation* conf)
{
    dbg_enter("VardisApplication::handleVardisConfirmation");
    assert(conf);
    printStatus(conf->getStatus());
    dbg_leave();
}


