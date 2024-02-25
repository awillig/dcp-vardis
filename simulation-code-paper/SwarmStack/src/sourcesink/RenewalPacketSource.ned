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

package swarmstack.sourcesink;

import swarmstack.sourcesink.IPacketSource;

simple RenewalPacketSource like IPacketSource
{
	parameters:
  		volatile  double interArrivalTime @unit(s) = default(exponential(50ms));
  		volatile  int packetSize @unit(byte)       = default(intuniform(100B, 500B));
	              int overheadSize @unit(byte)     = default(100B);
 	gates:
  		output  toLower;
}