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

enum IEType {
	SUMMARIES = 1;
	UPDATES = 2;
	REQUEST_VAR_UPDATES = 3;
	REQUEST_VAR_CREATES = 4;
	CREATE_VARIABLES = 5;   
	DELETE_VARIABLES = 6;
}

class IETypeHeader extends inet::FieldsChunk {
	IEType type;
	uint16_t len;
}
