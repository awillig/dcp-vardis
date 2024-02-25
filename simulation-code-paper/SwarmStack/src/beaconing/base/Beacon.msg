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

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import inet.linklayer.common.MacAddress;
import inet.common.geometry.Geometry;

class Beacon extends inet::FieldsChunk
{
    uint16_t    magicNo;       // a magic number identifying beacons
    uint16_t    version;       // version of the SwarmStack protocol
    uint32_t    seqno;         // sequence number
    inet::MacAddress  senderId;      // Mac address of sender, used as sender id

    inet::Coord        currPosition;  // current position in given coordinate system
    inet::Coord        currVelocity;  // current direction vector
}
