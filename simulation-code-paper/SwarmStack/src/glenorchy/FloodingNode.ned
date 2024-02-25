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

package swarmstack.glenorchy;

import swarmstack.sourcesink.IPacketSource;
import swarmstack.sourcesink.IPacketSink;
import swarmstack.flooding.Flooding;
import swarmstack.lbp.LocalBroadcastProtocol;
import swarmstack.lbp.AdhocLBPHost;


module FloodingNode extends AdhocLBPHost
{
    parameters:
        string        sourceType;
        string        sinkType;
    submodules:
        flooding:     Flooding;
        sink:         <sinkType>   like IPacketSink;
        source:       <sourceType> like IPacketSource;
	connections:
	    flooding.toLBP       -->  lbp.higherLayerIn;
	    flooding.fromLBP     <--  lbp.higherLayerOut;
	    flooding.toHigher    -->  sink.fromLower;
	    flooding.fromHigher  <--  source.toLower;
}