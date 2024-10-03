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


#include <omnetpp.h>
#include <inet/linklayer/common/MacAddress.h>
#include <inet/common/Protocol.h>
#include <dcp/common/MemBlockT.h>
#include <dcp/common/FoundationTypes.h>
#include <dcp/common/TransmissibleType.h>
#include <dcp/bp/BPTypesConstants.h>

using namespace omnetpp;
using namespace inet;

// --------------------------------------------------------------------
namespace dcp {


typedef inet::MacAddress   MacAddress;
typedef inet::MacAddress   NodeIdentifierT;
typedef omnetpp::SimTime   TimeStampT;

const NodeIdentifierT      nullIdentifier = MacAddress::UNSPECIFIED_ADDRESS;


class StringT : public MemBlockT<byte>, public TransmissibleType<sizeof(byte)> {
public:
    StringT () : MemBlockT<byte>() {};
    StringT (const char* str) : MemBlockT<byte>((byte) std::strlen(str), (byte*) str) {};
    StringT (const std::string& str) : MemBlockT<byte>((byte) str.size(), (byte*) str.c_str()) {};

    std::string to_str() const
    {
        return std::string((char*) data, (size_t) length);
    };

    char* to_cstr() const
    {
        if (length > 0 && data)
        {
            char* rv = new char [length+1];
            std::memcpy(rv, data, length);
            rv[length] = 0;
            return rv;
        }
    };

    virtual size_t total_size () const { return fixed_size() + length; };

    virtual void serialize (AssemblyArea& area)
    {
        area.serialize_byte(length);
        if (length > 0)
            area.serialize_byte_block(length, data);
    };

    virtual void deserialize (DisassemblyArea& area)
    {
        length = area.deserialize_byte ();
        if (length > 0)
        {
            if (data) throw SerializationException ("StringT::deserialize: already contains data");
            data = new byte [length];
            area.deserialize_byte_block (length, data);
        }
    };
};





// --------------------------------------------------------------------
// Basic type definitions and constants



class DcpSimGlobals {

public:
    DcpSimGlobals();
    virtual ~DcpSimGlobals();

    static Protocol* protocolDcpBP;
    static Protocol* protocolDcpSRP;
    static Protocol* protocolDcpVardis;
};

// returns for the given protId a pointer to the right protocol
// object, but only for BP client protocols
Protocol *convertProtocolIdToProtocol(BPProtocolIdT protId);

} // namespace

