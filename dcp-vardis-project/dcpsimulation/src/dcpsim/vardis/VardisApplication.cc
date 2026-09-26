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
#include <dcpsim/common/DcpSimGlobals.h>
#include <dcpsim/vardis/VardisApplication.h>




// ========================================================================================
// ========================================================================================


using namespace dcp;
using inet::INITSTAGE_LAST;


// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================

void VardisApplication::initialize(int stage)
{
    DcpApplication::initialize(stage);

    if (stage == INITSTAGE_LAST)
      {
	dbg_setModuleName ("VardisApplication");
	EvTracing dbg(*this, "VardisApplication::initialize");
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
void VardisApplication::sendToVardis (inet::Message* message)
{
    EvTracing dbg(*this, "sendToVardis/Message");
    sendToDcpProtocol(DcpSimGlobals::protocolDcpVardis, message);
}

// ----------------------------------------------------

/**
 * Send given packet to underlying VarDis protocol instance via message
 * dispatcher
 */
void VardisApplication::sendToVardis (inet::Packet* packet)
{
    EvTracing dbg(*this, "sendToVardis/Packet");
    sendToDcpProtocol(DcpSimGlobals::protocolDcpVardis, packet);
}


// ----------------------------------------------------

/**
 * prints VarDis status value in a log message
 */
void VardisApplication::printStatus (DcpStatus status)
{
    EvTracing dbg(*this, "VardisApplication::printStatus");
    dbg.val ("status", vardis_status_to_string (status));
}

// ----------------------------------------------------

/**
 * Default handler for VarDis confirmation primitives, just prints their
 * status value
 */
void VardisApplication::handleVardisConfirmation(VardisConfirmation* conf)
{
    EvTracing dbg(*this, "VardisApplication::handleVardisConfirmation");
    assert(conf);
    printStatus(conf->getStatus());
}
