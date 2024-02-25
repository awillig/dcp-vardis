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

import srp_vardis.BasicApplication;

//A basic application which records the seq no gap and update delay on
//variables on a per
simple RecordingApplication extends BasicApplication
{
    parameters:
        int numNodes;
        bool onlyFinalLogging = default(false);
        @signal[updateDelaySignal](type="double");
        @statistic[updateDelay](title="update delay (ms)"; source="updateDelaySignal"; unit="milliseconds"; record=histogram,stats; interpolationmode=none);
        @signal[seqnoDeltaSignal](type="long long int");
        @statistic[seqnoDelta](title="Difference in received sequence numbers"; source="seqnoDeltaSignal"; record=histogram,stats; interpolationmode=none);
    @class(RecordingApplication);
}
