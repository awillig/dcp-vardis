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

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.common.InterfaceTable;
import swarmstack.beaconing.renewalbeaconing.RenewalBeaconingNodeSafety;
import swarmstack.beaconing.renewalbeaconing.RenewalBeaconingNode;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;

module Interferer extends RenewalBeaconingNode {
    parameters:
        double  rvar;
        double  thetavar;
} 


network PoissonNetwork
{
    parameters:
        double  lambda                    =  default(0.0001);
        double  deploymentRadiusMeter     =  default(1000);
        double  targetDistance @unit(m)   =  default(20m);
        int     numInterferers            =  poisson(lambda*deploymentRadiusMeter*deploymentRadiusMeter*3.1415926536);
        int     numNodes                  =  numInterferers + 2;
        double  centerX @unit(m)          =  m(deploymentRadiusMeter);
        double  centerY @unit(m)          =  m(deploymentRadiusMeter);
        @display("bgb=2000,2000");
        
        volatile  double  rparval       = deploymentRadiusMeter*sqrt(uniform(0,1));
        volatile  double  thetaparval   = 2*3.1415926536*uniform(0,1);

        targets[0].mobility.initialX  = centerX;
        targets[0].mobility.initialY  = centerY;
        targets[1].mobility.initialX  = centerX + targetDistance;
        targets[1].mobility.initialY  = centerY;
        
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
                
        targets[2] : RenewalBeaconingNodeSafety {
            parameters:
                //@display("r=,,#707070;p=300,200");
                        targets[*].wlan[0].radio.typename = "Ieee80211ScalarRadio";
                wlan[0].radio.transmitter.power = 0.01W;
                **.bitrate = 12Mbps;
                wlan[0].radio.displayCommunicationRange = false;
                wlan[0].radio.displayInterferenceRange = false;
                mobility.numHosts = numNodes;
                mobility.typename = "StationaryMobility";           
                mobility.initFromDisplayString = false;
                mobility.constraintAreaMinX = 0m;
                mobility.constraintAreaMaxX = m(2*deploymentRadiusMeter);
                mobility.constraintAreaMinY = 0m;
                mobility.constraintAreaMaxY = m(2*deploymentRadiusMeter);
                mobility.constraintAreaMinZ = 0m;
                mobility.constraintAreaMaxZ = 2m;
                
        }
        interferers[numInterferers]  :  Interferer {
            parameters:
                rvar     = rparval;
                thetavar = thetaparval;
                //@display("r=,,#707070;p=300,200");
                interferers[*].wlan[0].radio.typename = "Ieee80211ScalarRadio";
                wlan[0].radio.transmitter.power = 0.01W;
                **.bitrate = 12Mbps;
                wlan[0].radio.displayCommunicationRange = false;
                wlan[0].radio.displayInterferenceRange = false;
                mobility.numHosts = numNodes;
                mobility.typename = "StationaryMobility";           
                mobility.initFromDisplayString = false;
                mobility.constraintAreaMinX = 0m;
                mobility.constraintAreaMaxX = m(2*deploymentRadiusMeter);
                mobility.constraintAreaMinY = 0m;
                mobility.constraintAreaMaxY = m(2*deploymentRadiusMeter);
                mobility.constraintAreaMinZ = 0m;
                mobility.constraintAreaMaxZ = 2m;
                mobility.initialX           = centerX + m(this.rvar * (cos(this.thetavar)));
                mobility.initialY           = centerY + m(this.rvar * (sin(this.thetavar)));
        }
}
