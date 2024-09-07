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

package dcpv1.dcp.networks;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.common.InterfaceTable;
import dcpv1.dcp.node.DcpNode;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
// import inet.visualizer.contract.IIntegratedVisualizer;

network TestNetwork
{
    parameters:
        int     numNodes                    = default(10);
        double  avgInterUpdateTime @unit(s) = default(1s);
        
        @display("bgb=1000,1000");
        nodes[*].wlan[0].radio.typename = "Ieee80211ScalarRadio";
        nodes[*].wlan[0].radio.transmitter.power = 0.01W;
        nodes[*].**.bitrate = 12Mbps;
        nodes[*].wlan[0].radio.displayCommunicationRange = false;
        nodes[*].wlan[0].radio.displayInterferenceRange = false;
        nodes[*].mobility.numHosts = numNodes;
        
        nodes[0].cons.consumerActive = true;
        nodes[0].cons.varIdToObserve = numNodes;
        // nodes[0].prod.deletionTime   = 5s;
                
    submodules:
        
//        visualizer: <default("IntegratedCanvasVisualizer")> like IIntegratedVisualizer if hasVisualizer() {
//            parameters:
//                @display("p=10,10;is=s");
//        }
//        
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
                
        nodes[numNodes]  :  DcpNode {
            parameters:
                //@display("r=,,#707070;p=300,200");
                prod.varId                 =  1 + index;
        		prod.varRepCnt             =  2;
           		prod.variableValue         =  normal(1000*(index+1), 10);
        		prod.interUpdateTimeDistr  =  exponential(parent.avgInterUpdateTime);
                cons.samplingPeriod        =  50ms;          
        }
}

