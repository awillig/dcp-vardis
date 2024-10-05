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

package dcpsimulation.dcp.applications;

import dcpsimulation.dcp.vardis.VardisClientProtocol;

@namespace(dcp);


// Produces a single variable with given varId and varRepCnt
// Values are of type ExampleVariable, their double value can be drawn
// iid from a distribution, as can the inter-arrival times for updates.
// Variable becomes active after creationDelay, and is deleted after
// deletionTime. In between, new updates are generated with inter-update
// times given by interUpdateTimeDistr.
simple VardisVariableProducer extends VardisClientProtocol
{
    parameters:

		int  varId;
		int  varRepCnt;

        // time of variable creation
        volatile double creationTime @unit(s)          = default (exponential(50ms));  

		// time between variable updates
		volatile double interUpdateTimeDistr @unit(s)  = default (exponential(50ms));
		
		// time for sending variable deletion
        volatile double deletionTime @unit(s)          = default (100000s);  
		
		// variable values (variable is a double)
		volatile double variableValue                  = default (normal(0,1));

		@class(VardisVariableProducer);
		  
}
