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

import swarmstack.lbp.ILbpClient;

simple SRPVarDisBeaconing like ILbpClient {
    parameters:
        double beaconPeriod @unit(s) = default(100 ms);
        double jitter @unit(pct) = default(10 pct);
        
        int maxBeaconSize @unit(B) = default(200 B);
        
        bool disableSRP = default(true);
        bool randomPeriod = default(false);
        
    gates:
        input rtdb_in;
        output rtdb_out;
        input srp_in;
        output srp_out;
        
        input  fromLBP @labels(LBPClient/up);
      	output toLBP @labels(LBPClient/down);
}
