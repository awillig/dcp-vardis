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

#include <dcpsim/common/DcpProtocol.h>
#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/common/InterfaceTable.h>


using inet::INITSTAGE_LAST;
using inet::InterfaceTable;
using inet::NetworkInterface;
using inet::getContainingNode;

// ========================================================================================
// ========================================================================================


using namespace dcp;

// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================


void DcpProtocol::initialize(int stage)
{
    if (stage == INITSTAGE_LAST)
    {
        EvLogging dbg(*this, "DcpProtocol::initialize");

        // reading and checking module parameters
        _interfaceName =  par("interfaceName").stdstringValue();
        determineOwnNodeId(_interfaceName);
    }
}



// ===================================================================================
// Determining the own MAC address / node id by querying the network interface
// ===================================================================================


void DcpProtocol::determineOwnNodeId (std::string interfaceName)
{
    EvLogging dbg(*this, "DcpProtocol::determineOwnNodeId");

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
	  byte addr_bytes [MAC_ADDRESS_SIZE];
          iface->getMacAddress().getAddressBytes (addr_bytes);
	  _ownNodeId     = NodeIdentifierT (addr_bytes);
	  _wlanInterface = iface;
	  return;
        }
    }

    error("DcpProtocol::determineOwnNodeId: interface not found");
}

// ===================================================================================
// Debug helpers
// ===================================================================================


/**
 * Sets the current module name to use, will show up in every line
 */
void DcpProtocol::dbg_setModuleName(std::string name)
{
    _dbg_module_name = name;
}

