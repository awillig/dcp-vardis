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

#pragma once

#include <dcp/common/FoundationTypes.h>
#include <dcp/common/DcpTypesGlobals.h>


namespace dcp {

typedef uint16_t      BPLengthT;         // length of a payload block

/*
class BPHeaderT : public TransmissibleType<sizeof(byte)+sizeof(uint16_t)+MAC_ADDRESS_SIZE+sizeof(byte)+sizeof(uint32_t)> {
public:
    byte             version;
    uint16_t         magicNo;
    NodeIdentifierT  senderId;
    byte             numPayloads;
    uint32_t         seqno;

    virtual size_t total_size () const { return fixed_size(); };
    virtual void serialize (AssemblyArea& area)
    {
        area.serialize_byte (version);
        area.serialize_byte_block (sizeof(uint16_t), (byte*) &magicNo);
        senderId.serialize (area);
        area.serialize_byte (numPayloads);
        area.serialize_byte_block (sizeof(uint32_t), (byte*) &seqno);
    };
    virtual void deserialize (DisassemblyArea& area)
    {
        version = area.deserialize_byte ();
        area.deserialize_byte_block (sizeof(uint16_t), (byte*) &magicNo);
        senderId.deserialize (area);
        numPayloads = area.deserialize_byte ();
        area.deserialize_byte_block (sizeof(uint32_t), (byte*) &seqno);
    };
};


class BPPayloadHeaderT : public TransmissibleType<sizeof(BPProtocolIdT)+sizeof(BPLengthT)> {
public:
    BPProtocolIdT   protocolId;
    BPLengthT       length;

    virtual size_t total_size () const { return fixed_size(); };
    virtual void serialize (AssemblyArea& area)
    {
        area.serialize_byte_block (sizeof(BPProtocolIdT), (byte*) &protocolId);
        area.serialize_byte_block (sizeof(BPLengthT), (byte*) &length);
    };
    virtual void deserialize (DisassemblyArea& area)
    {
        area.deserialize_byte_block (sizeof(BPProtocolIdT), (byte*) &protocolId);
        area.deserialize_byte_block (sizeof(BPLengthT), (byte*) &length);
    };
};
*/

}; // namespace dcp
