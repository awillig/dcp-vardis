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

package swarmstack.flooding;

import swarmstack.lbp.ILbpClient;

simple Flooding like ILbpClient
{
    parameters:
                  int     floodingHeaderSize @unit(byte)  = default(14B);
                  int     numRepetitions                  = default(1);                   // how often is a flooded message repeated
        volatile  double  repetitionBackoff @unit(s)      = default(exponential(10ms));   // random backoff between attempts
    gates:
        // these gates interconnect with the underlying LBP
        input    fromLBP @labels(LBPClient/up);
        output   toLBP @labels(LBPClient/down);
        
        // connects to producer/consumer of messages to be flooded
        input    fromHigher;
        output   toHigher;
}