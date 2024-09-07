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


// This module, if activated (cf consumerActive parameter), interacts periodically
// with the local VarDis instance to obtain a database description and then query
// and output the current values of all registered variables listed in that
// description. The variables have fixed type.
simple VariableConsumer extends VardisClientProtocol
{
    parameters:
		bool   consumerActive             = default(false);  // variables are only sampled when consumerActive=true
		double samplingPeriod @unit(s)    = default(50ms);   // fixed sampling period
		int    varIdToObserve             = default(-1);     // varId for which to update statistics, other varIds are just printed

        // signal-based statistics for selected varIdToObserve
		@signal[updateDelaySignal](type=double);
		@signal[seqnoDeltaSignal](type=unsigned long);
		@signal[receptionTimeSignal](type=simtime_t);
		@statistic[updateDelaySignal](record=stats; title="update delay (ms)"; unit="milliseconds");
		@statistic[seqnoDeltaSignal](record=stats; title="Difference in received sequence numbers");
		@statistic[receptionTimeSignal](record=vector; title="Reception times (s)");
	
		@class(VariableConsumer);		  
}
