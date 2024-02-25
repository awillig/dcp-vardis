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

package swarmstack;

import swarmstack.beaconing.obsolete.IBeaconingProtocol;
import swarmstack.beaconing.neighbortable.NeighborTable;
import swarmstack.lbp.LocalBroadcastProtocol;
import inet.mobility.contract.IMobility;
import inet.linklayer.contract.IMacProtocol;
import inet.common.MessageDispatcher;
import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;

module SwarmNodeLBP extends AdhocHost
{
    parameters:

        // Parameters from ApplicationLayerNodeBase
        numApps = 0;

        // Parameters from TransportLayerNodeBase 
        hasUdp = false;
        hasTcp = false;
        hasSctp = false;

        // Parameters from NetworkLayerNodeBase
        hasIpv4 = true;
        hasIpv6 = false;
        hasGn = false;
        forwarding = false;
        multicastForwarding = false;


        // Parameters from LinkLayerNodeBase
        numPcapRecorders = 0;
        numLoInterfaces = 1;
        numWlanInterfaces = 1;
        numEthInterfaces = 0;
        //numExtInterfaces = 0;
        numPppInterfaces = 0;
        numTunInterfaces = 0;
        numVirtInterfaces = 0;

        // type of beaconing protocol	
        string beaconingType;

        @display("bgb=2827.35,1695.0375");
    submodules:
        lbp:           LocalBroadcastProtocol;
        mdlb:          MessageDispatcher;
        beaconing:     <beaconingType> like IBeaconingProtocol;
        neighborTable: NeighborTable;
    connections:
        //beaconing.lowerLayerOut --> mdlb.in++;
        //beaconing.lowerLayerIn  <-- mdlb.out++;
        //lbp.higherLayerIn       <-- mdlb.out++;
        //lbp.higherLayerOut      --> mdlb.in++;
        beaconing.lowerLayerOut --> lbp.higherLayerIn;
        beaconing.lowerLayerIn  <-- lbp.higherLayerOut;
        beaconing.neighborTableOut --> neighborTable.beaconInput;
        lbp.lowerLayerOut --> nl.in++;
        lbp.lowerLayerIn <-- nl.out++;
}
