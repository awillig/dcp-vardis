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

package swarmstack.beaconing.renewalbeaconing;

import swarmstack.lbp.ILbpClient;

simple RenewalBeaconing like ILbpClient
{
    parameters:
      @signal[renewalBeaconingBeaconsSent](type=bool);
      @statistic[renewalBeaconingBeaconsSent](record=count; title="RenewalBeaconing: Number of generated beacons");
      volatile double  iatDistribution @unit(s)         =  default(0.1s);
      volatile double  initialWaitTime @unit(s)         =  default(0s);
               double  positionSamplingPeriod @unit(s)  =  default(0.01s);
               int     beaconLength @unit(byte)         =  default(144B);
	gates:
        // these interconnect with the underlying LBP
		input  fromLBP @labels(LBPClient/up);
      	output toLBP @labels(LBPClient/down);

        // an output gate that beacon reception reports are being sent to
      	output beaconReport @labels(BeaconReport/up)  @loose;
}
