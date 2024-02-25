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

package srp_vardis;

import swarmstack.lbp.AdhocLBPHost;
import srp_vardis.applications.IApplication;
import srp_vardis.rtdb.RTDB;

module Drone extends AdhocLBPHost
{
    parameters:
        @display("bgb=1477.0599,1073.15");

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

    submodules:
        application: <default("RecordingApplication")> like IApplication if typename != "" {
            @display("p=45,45");
        }

        rtdb: RTDB {
            @display("p=45,115");
        }

		beaconing: SRPVarDisBeaconing {
		    @display("p=45,185");
		}
		
		srp: SRP {
		    @display("p=45,255");
		}

    connections:
        application.rtdb <--> rtdb.application;
        beaconing.toLBP --> lbp.higherLayerIn;
        beaconing.fromLBP  <-- lbp.higherLayerOut;
        beaconing.rtdb_out --> rtdb.net_in;
        beaconing.rtdb_in <-- rtdb.net_out; 
        beaconing.srp_out --> srp.net_in;
        beaconing.srp_in <-- srp.net_out;
}
