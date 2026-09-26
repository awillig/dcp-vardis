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
#include <dcpsim/srp/SRPApplication.h>

// ========================================================================================
// ========================================================================================


using namespace dcp;
using inet::INITSTAGE_LAST;

Define_Module (SRPApplication);

// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================

void SRPApplication::initialize(int stage)
{
    DcpApplication::initialize(stage);

    if (stage == INITSTAGE_LAST)
    {
      dbg_setModuleName("SRPApplication");
      EvTracing dbg(*this, "SRPClientProtocol::initialize");
    }
}


// ========================================================================================
// Helper methods
// ========================================================================================

// ----------------------------------------------------

/**
 * Send given message to underlying SRP protocol instance via message
 * dispatcher
 */
void SRPApplication::sendToSRP (inet::Message* message)
{
    EvTracing dbg(*this, "sendToSRP/Message");
    sendToDcpProtocol(DcpSimGlobals::protocolDcpSRP, message);
}

// ----------------------------------------------------

/**
 * Send given packet to underlying SRP protocol instance via message
 * dispatcher
 */
void SRPApplication::sendToSRP (inet::Packet* packet)
{
    EvTracing dbg(*this, "sendToSRP/Packet");
    sendToDcpProtocol(DcpSimGlobals::protocolDcpSRP, packet);
}

// ----------------------------------------------------

static const std::map<SRPStatus, std::string> status_texts = {
        {SRP_STATUS_OK, "SRP_STATUS_OK"}
};

/**
 * Convert SRP status value to string
 */
std::string SRPApplication::getSRPStatusString(SRPStatus status)
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
void SRPApplication::printStatus (SRPStatus status)
{
    EvTracing dbg(*this, "SRPClientProtocol::printStatus");

    auto statstr = getSRPStatusString(status);
    dbg.val ("status", status, "text", statstr);
}

// ----------------------------------------------------

/**
 * Default handler for SRP confirmation primitives, just prints their
 * status value
 */
void SRPApplication::handleSRPConfirmation(SRPUpdateSafetyData_Confirm* conf)
{
    EvTracing dbg(*this, "VardisSRPProtocol::handleSRPConfirmation");
    assert(conf);
    printStatus(conf->getStatus());
}





