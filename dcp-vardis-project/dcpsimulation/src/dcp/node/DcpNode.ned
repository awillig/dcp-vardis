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

package dcpsimulation.dcp.node;

import inet.mobility.contract.IMobility;
import inet.linklayer.contract.IMacProtocol;
import inet.common.MessageDispatcher;
import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;
import dcpsimulation.dcp.node.DcpStack;
import dcpsimulation.dcp.applications.VardisVariableProducer;
import dcpsimulation.dcp.applications.VardisVariableConsumer;
import dcpsimulation.dcp.applications.SRPSimpleApp;

module DcpNode extends AdhocHost
{
       parameters:

        // Parameters from ApplicationLayerNodeBase
        numApps = default(0);

        // Parameters from TransportLayerNodeBase 
        hasUdp  = default(false);
        hasTcp  = default(false);
        hasSctp = default(false);

        // Parameters from NetworkLayerNodeBase
        hasIpv4              = default(false);
        hasIpv6              = default(false);
        hasGn                = default(false);
        forwarding           = default(false);
        multicastForwarding  = default(false);


        // Parameters from LinkLayerNodeBase
        numPcapRecorders  = default(0);
        numLoInterfaces   = default(1);
        numWlanInterfaces = default(1);
        numEthInterfaces  = default(0);
        //numExtInterfaces  = default(0);
        numPppInterfaces  = default(0);
        numTunInterfaces  = default(0);
        numVirtInterfaces = default(0);

        
    submodules:
        dcp:           DcpStack;
        vmd:           MessageDispatcher;
        vardisprod:    VardisVariableProducer;
        vardiscons:    VardisVariableConsumer;
        srpapp:        SRPSimpleApp;

    connections:
        dcp.toUWB   --> nl.in++;
        dcp.fromUWB <-- nl.out++;
    
    	dcp.toVardisApp    --> vmd.in++;
    	dcp.fromVardisApp  <-- vmd.out++;
    	
    	dcp.toSRPApp     --> vmd.in++;
    	dcp.fromSRPApp   <-- vmd.out++;
    	
    	vardisprod.toDcpProtocol   --> vmd.in++;
    	vardiscons.toDcpProtocol   --> vmd.in++;
    	
    	vardisprod.fromDcpProtocol   <-- vmd.out++;
    	vardiscons.fromDcpProtocol   <-- vmd.out++;
    	
    	srpapp.toDcpProtocol    --> vmd.in++;
    	srpapp.fromDcpProtocol  <-- vmd.out++;
}
