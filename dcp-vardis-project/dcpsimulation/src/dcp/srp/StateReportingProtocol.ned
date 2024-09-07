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

package dcpv1.dcp.srp;

import dcpv1.dcp.bp.BPClientProtocol;

@namespace(dcp);

simple StateReportingProtocol extends BPClientProtocol
{
    parameters:
        maxPayloadSize = 100B;
  
        volatile double srpGenerationPeriodDistr @unit(s)        = default (uniform(90ms, 110ms));  // time between payload generations
                 double srpPositionSamplingPeriod @unit(s)       = default (100ms);                 // period for sampling own position
                 double srpNeighbourTableTimeout @unit(s)        = default (0.3s);                  // timeout for neighbor table entries
                 double srpNeighbourTableScrubPeriod @unit(s)    = default (0.05s);                 // period for checking neigbor table for stale entries
                 double srpNeighbourTablePrintPeriod @unit(s)    = default (0.1s);                  // period for printing neighbor table (no printing if < 0)
	
		@class(StateReportingProtocol);
}
