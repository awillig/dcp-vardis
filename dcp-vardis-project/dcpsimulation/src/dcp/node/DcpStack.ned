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

package dcpv1.dcp.node;

import dcpv1.dcp.bp.BeaconingProtocol;
import dcpv1.dcp.srp.StateReportingProtocol;
import dcpv1.dcp.vardis.VardisProtocol;
import inet.common.MessageDispatcher;

module DcpStack
{
    parameters:

    gates:
        output toUWB;
        input  fromUWB;

        output toVardisApp;
        input  fromVardisApp;

    submodules:
        bp:        BeaconingProtocol;
        srp:       StateReportingProtocol;
        vardis:    VardisProtocol;
        bpmd:      MessageDispatcher;    // message dispatcher between BP and its clients (SRP, Vardis)
        // vardismd:  MessageDispatcher;   // message dispatcher between Vardis and its applications

    connections allowunconnected:
        bp.toUWB --> toUWB;
        fromUWB --> bp.fromUWB;

        srp.toBP --> bpmd.in++;
        vardis.toBP --> bpmd.in++;
        bp.toClients --> bpmd.in++;
        bpmd.out++ --> srp.fromBP;
        bpmd.out++ --> vardis.fromBP;
        bpmd.out++ --> bp.fromClients;
        
        vardis.toApplication --> toVardisApp;
        fromVardisApp --> vardis.fromApplication;
}
