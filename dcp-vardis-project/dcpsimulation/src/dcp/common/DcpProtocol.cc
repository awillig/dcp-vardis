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

#include <inet/common/ModuleAccess.h>
#include <dcp/common/DcpProtocol.h>
#include <inet/networklayer/common/InterfaceTable.h>


// ========================================================================================
// ========================================================================================


using namespace dcp;

Define_Module(DcpProtocol);

// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================


void DcpProtocol::initialize(int stage)
{
    if (stage == INITSTAGE_LAST)
    {
        dbg_enter("DcpProtocol::initialize");

        // reading and checking module parameters
        _interfaceName =  par("interfaceName").stdstringValue();
        determineOwnNodeId(_interfaceName);

        dbg_leave();
    }
}



// ===================================================================================
// Determining the own MAC address / node id by querying the network interface
// ===================================================================================


void DcpProtocol::determineOwnNodeId (std::string interfaceName)
{
    dbg_enter("determineOwnNodeId");

    cModule *host                       = getContainingNode(this);
    assert(host);
    inet::InterfaceTable *interfaces    = check_and_cast<InterfaceTable *>(host->getSubmodule("interfaceTable"));
    assert(interfaces);

    // find the right interface to link the LBP to
    for (int i = 0; i<interfaces->getNumInterfaces(); i++)
    {
        NetworkInterface*  iface = interfaces->getInterface(i);
        assert(iface);
        std::string  s    = iface->str();
        std::string  sfst = s.substr(0, s.find(' '));

        if (sfst.compare(interfaceName) == 0)
        {
            _ownNodeId     = iface->getMacAddress();
            _wlanInterface = iface;
            dbg_leave();
            return;
        }
    }

    error("DcpProtocol::determineOwnNodeId: interface not found");
    return;
}

// ===================================================================================
// Debug helpers
// ===================================================================================

void DcpProtocol::dbg_assertToplevel()
{
    assert(_methname_stack.empty());
}

/**
 * Sets the current module name to use, will show up in every line
 */
void DcpProtocol::dbg_setModuleName(std::string name)
{
    _dbg_module_name = name;
}

/**
 * Outputs the starting part of every log message: timestamp, module name,
 * own node identifier, and the current method name
 */
void DcpProtocol::dbg_prefix()
{
    EV << "t = " << simTime()
       << " - " << _dbg_module_name
       << "[Id=" << _ownNodeId
       << "]." << _methname_stack.top()
       << ": ";
}

/**
 * Declare that we now enter the given method; allows for nesting
 */
void DcpProtocol::dbg_enter(std::string methname)
{
    _methname_stack.push(methname);
    dbg_prefix();
    EV << "entering"
       << endl;
}


/**
 * Declare that we now leave the method, allows for nesting
 */
void DcpProtocol::dbg_leave()
{
    dbg_prefix();
    EV << "leaving"
       << endl;
    _methname_stack.pop();
}

/**
 * generates log message with prefix and the given string
 */
void DcpProtocol::dbg_string(std::string str)
{
    dbg_prefix();
    EV << str
       << endl;
}



