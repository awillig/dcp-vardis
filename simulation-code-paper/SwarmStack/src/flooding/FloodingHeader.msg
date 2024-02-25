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

import inet.common.packet.chunk.Chunk;
import inet.linklayer.common.MacAddress;


class FloodingHeader extends inet::FieldsChunk
{
    uint16_t          magicNo;
    uint16_t          floodingVersion;
    uint16_t          timeToLive;
    uint32_t          seqno;         // sequence number
    inet::MacAddress  sourceId;      // Mac address of original source, used as sender id
    simtime_t         generationTime;
}
