//
// 

package swarmstack.lbp;

import swarmstack.lbp.LocalBroadcastProtocol;
import swarmstack.lbp.ILbpClient;
import inet.mobility.contract.IMobility;
import inet.linklayer.contract.IMacProtocol;
import inet.common.MessageDispatcher;
import inet.networklayer.common.InterfaceTable;
import inet.node.inet.AdhocHost;

module AdhocLBPHost extends AdhocHost
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

        @display("bgb=2827.35,1695.0375");
        
        // string  lbpClientType;
        
    submodules:
        lbp:           LocalBroadcastProtocol;
        mdlb:          MessageDispatcher;
    connections:
        //lbp.higherLayerIn       <-- mdlb.out++;
        //lbp.higherLayerOut      --> mdlb.in++;
        lbp.lowerLayerOut --> nl.in++;
        lbp.lowerLayerIn <-- nl.out++;
}

