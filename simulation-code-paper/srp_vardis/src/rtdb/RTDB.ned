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

package srp_vardis.rtdb;


//The Real-Time Database module.
simple RTDB
{
	parameters:
	    int beaconFreq @unit(Hz) = default(10 Hz);
	    int maxVariableLen @unit(B) = default(32 B);
	    int maxDescriptionLen @unit(B) = default(32 B);
	    int maxRepetitions = default(15);
	    int maxBeaconSize @unit(B) = default(200 B);
	    int maxNumSummaries = default(2); //Maximum number of summaries to
	                                      //include in a beacon

        int runNumber;

	gates:
	    inout application @loose;
	    input net_in;
	    output net_out;
	    
}
