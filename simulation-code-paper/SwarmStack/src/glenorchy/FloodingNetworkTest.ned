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
import swarmstack.glenorchy.FloodingSource;
import swarmstack.glenorchy.FloodingSink;


network FloodingNetworkTest
{
    parameters:
        int    numNodes   =  3;
        @display("bgb=200,200");
        sinks[*].wlan[0].radio.typename = "Ieee80211ScalarRadio";
        sinks[*].wlan[0].radio.transmitter.power = 0.01W;
        sinks[*].**.bitrate = 12Mbps;
        sinks[*].wlan[0].radio.displayCommunicationRange = false;
        sinks[*].wlan[0].radio.displayInterferenceRange = false;
        sinks[*].mobility.numHosts = numNodes;

        source.wlan[0].radio.typename = "Ieee80211ScalarRadio";
        source.wlan[0].radio.transmitter.power = 0.01W;
        source.**.bitrate = 12Mbps;
        source.wlan[0].radio.displayCommunicationRange = false;
        source.wlan[0].radio.displayInterferenceRange = false;
        source.**.mobility.numHosts = numNodes;        

        **.mobility.numHost = numNodes;
        **.mobility.initFromDisplayString = false;
        **.mobility.typename = "StationaryMobility";
        **.mobility.initFromDisplayString = false;
        **.mobility.constraintAreaMinX = 0m;
        **.mobility.constraintAreaMaxX = 200m;
        **.mobility.constraintAreaMinY = 0m;
        **.mobility.constraintAreaMaxY = 200m;
        **.mobility.constraintAreaMinZ = 0m;
        **.mobility.constraintAreaMaxZ = 2m;
        **.mobility.speed = 0 mps;

        source.mobility.initialX    =  0m;
        source.mobility.initialY    =  0m;
        source.mobility.initialZ    =  0m;
        sinks[0].mobility.initialX  =  100m;
        sinks[0].mobility.initialY  =  0m;
        sinks[0].mobility.initialZ  =  0m;
        sinks[1].mobility.initialX  =  75m;
        sinks[1].mobility.initialY  =  75m;
        sinks[1].mobility.initialZ  =  0m;
        
        
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
        
        source  :  FloodingSource;
        sinks[numNodes - 1]  :  FloodingSink {
            parameters:
                //@display("r=,,#707070;p=300,200");
        }
}

