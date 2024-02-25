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

package swarmstack.glenorchy;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.common.InterfaceTable;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;
import swarmstack.glenorchy.FloodingNode;
import swarmstack.glenorchy.GlenorchyPacketSink;
import swarmstack.sourcesink.RenewalPacketSource;
import swarmstack.sourcesink.NullPacketSource;


network FloodingNetworkGrid
{
    parameters:
        @statistic[glenorchysinkdelay](record=stats);
        @statistic[glenorchysinkcount](record=count);
        
        int    numNodes   =  default(16);
        @display("bgb=1000,1000");
        nodes[*].wlan[0].radio.typename = "Ieee80211ScalarRadio";
        nodes[*].wlan[0].radio.transmitter.power = 0.01W;
        nodes[*].**.bitrate = 12Mbps;
        nodes[*].wlan[0].radio.displayCommunicationRange = false;
        nodes[*].wlan[0].radio.displayInterferenceRange = false;
        nodes[*].mobility.numHosts = numNodes;

        nodes[*].mobility.initFromDisplayString = false;
        nodes[*].mobility.typename = "StaticGridMobility";
        nodes[*].mobility.initFromDisplayString = false;
        nodes[*].mobility.constraintAreaMinX = 0m;
        nodes[*].mobility.constraintAreaMaxX = 1000m;
        nodes[*].mobility.constraintAreaMinY = 0m;
        nodes[*].mobility.constraintAreaMaxY = 1000m;
        nodes[*].mobility.constraintAreaMinZ = 0m;
        nodes[*].mobility.constraintAreaMaxZ = 1m;
        nodes[*].mobility.speed = 0 mps;

        nodes[0].sourceType       = "RenewalPacketSource";
        nodes[0].source.interArrivalTime = 10s;
        nodes[0].source.packetSize       = 200B;
        nodes[1..].sourceType     = "NullPacketSource";
        nodes[*].sinkType         = "GlenorchyPacketSink";        
        
    submodules:
        
        visualizer: <default("IntegratedCanvasVisualizer")> like IIntegratedVisualizer if hasVisualizer() {
            parameters:
                @display("p=10,10;is=s");
        }
        
        configurator: Ipv4NetworkConfigurator  {
            parameters:
                @display("p=10,20;is=s");
                config = default(xml("<config><interface hosts='**' address='10.x.x.x' netmask='255.x.x.x'/></config>"));
                assignAddresses   = true;
                addStaticRoutes   = false;
                addDefaultRoutes  = false;
                addSubnetRoutes   = false;
                addDirectRoutes   = false;
                optimizeRoutes    = false;
                
        }
        
        radioMedium: Ieee80211ScalarRadioMedium {
            parameters:
                @display("p=10,30;is=s");
        }
        
        nodes[numNodes]    :   FloodingNode {
            parameters:
                //@display("r=,,#707070;p=300,200");
        }
}

