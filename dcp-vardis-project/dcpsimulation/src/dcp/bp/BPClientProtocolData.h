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

#ifndef DCP_BP_BPCLIENTPROTOCOLDATA_H_
#define DCP_BP_BPCLIENTPROTOCOLDATA_H_

#include <queue>
#include <list>

#include <inet/common/packet/chunk/Chunk.h>
#include <inet/common/Ptr.h>
#include <dcp/common/DcpTypesGlobals.h>
#include <dcp/bp/BPDataTypes.h>
#include <dcp/bp/BPQueueingMode_m.h>
#include <dcp/common/DcpTypesGlobals.h>


// -----------------------------------------------------------------

namespace dcp {


typedef struct BPBufferEntry {
    Ptr<const Chunk>  theChunk = nullptr;
} BPBufferEntry;




typedef struct BPClientProtocolData {
    BPProtocolIdT               protocolId;              // unique identifier of the client protocol
    std::string                 protocolName;            // human-readable name of the protocol
    BPLengthT                   maxPayloadSizeB;         // maximum size of client protocol payload in bytes
    BPQueueingMode              queueMode;               // queueing mode to be used for client protocol
    unsigned int                maxEntries;              // maximum number of entries in a droptail queue
    TimeStampT                  timeStampRegistration;   // time at which client protocol was registered
    std::queue<BPBufferEntry>   queue;                   // queue of all payloads when operating in mode BP_QMODE_QUEUE
    bool                        bufferOccupied;          // tells whether buffer is occupied or not
                                                         // (in modes BP_QMODE_ONCE and BP_QMODE_REPEAT)
    BPBufferEntry               bufferEntry;             // the single buffer used in modes BP_QMODE_ONCE and BP_QMODE_REPEAT
    bool                        allowMultiplePayloads;   // allow multiple client payloads in the same beacon? IGNORED
} BPClientProtocolData;



class ClientProtocolList {

private:
    std::list<BPClientProtocolData>   _theProtocolList;

public:
    // bool   lProtocol
};



} // namespace


#endif /* DCP_BP_BPCLIENTPROTOCOLDATA_H_ */
