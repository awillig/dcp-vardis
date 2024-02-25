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

package srp_vardis.applications;

import IApplication;

//A basic application for the VarDis protocol which produces a single variable
//which it updates at a random rate.
simple BasicApplication like IApplication
{
    parameters:
        //Variable update period default to a Poisson distribution
        volatile double variableUpdatePeriodDist = default(poisson(1000)); //milliseconds
        
        //The value the application will use the the RTDB variable descriptor.
        //If set to -1 (the default) the application will pull the MAC address
        //and use the lowest bytes as the variable ID. If set to -2, the
        //application does not generate a variable.
        int variableID = default(-1);
        
        //Length of the variable description in bytes
        int variableSize @unit(B) = default(5B);
        
        //Number of repeated transmissions requested for this application's variables
        int variableRepetitions = default(2);
        
    gates:
        inout rtdb;

}
