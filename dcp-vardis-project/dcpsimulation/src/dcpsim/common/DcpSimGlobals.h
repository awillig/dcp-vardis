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


#include <iostream>
#include <iomanip>
#include <string>
#include <omnetpp.h>
#include <inet/linklayer/common/MacAddress.h>
#include <inet/common/Protocol.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/foundation_types.h>
#include <dcp/common/memblock.h>
#include <dcp/common/transmissible_type.h>


using namespace omnetpp;
using namespace inet;

// --------------------------------------------------------------------
namespace dcp {

// --------------------------------------------------------------------


std::string bv_to_str (const bytevect& bv, size_t numbytes);

// --------------------------------------------------------------------


class NodeIdentifierT : public TransmissibleType<MAC_ADDRESS_SIZE> {
public:
      byte nodeId[MAC_ADDRESS_SIZE] = {0, 0, 0, 0, 0, 0};

      NodeIdentifierT () { std::memset(nodeId, 0, MAC_ADDRESS_SIZE); };
      NodeIdentifierT (const inet::MacAddress addr)
      {
          for (int i=0; i<MAC_ADDRESS_SIZE; i++)
              nodeId[i] = addr.getAddressByte(i);
      };

      bool operator== (const NodeIdentifierT& other) const { return (0 == std::memcmp(nodeId, other.nodeId, MAC_ADDRESS_SIZE)); };
      bool operator!= (const NodeIdentifierT& other) const { return (0 != std::memcmp(nodeId, other.nodeId, MAC_ADDRESS_SIZE)); };
      std::string to_str () const
      {
          std::stringstream ss;
          ss << std::hex;

          for (int i=0; i<MAC_ADDRESS_SIZE-1; i++)
              ss << std::setw(2) << std::setfill('0') << (int) nodeId[i] << ":";
          ss << std::setw(2) << std::setfill('0') << (int) nodeId[MAC_ADDRESS_SIZE-1];

          return ss.str();
      };
      inet::MacAddress to_macaddr () const
      {
          inet::MacAddress rv;
          for (int i=0; i<MAC_ADDRESS_SIZE; i++)
              rv.setAddressByte(i, nodeId[i]);
          return rv;
      };

      friend std::ostream& operator<<(std::ostream& os, const NodeIdentifierT& nodeid);
      friend bool operator< (const dcp::NodeIdentifierT& left, const dcp::NodeIdentifierT& right);

      virtual size_t total_size () const { return fixed_size(); };
      virtual void serialize (AssemblyArea& area) { area.serialize_byte_block(MAC_ADDRESS_SIZE, nodeId); };
      virtual void deserialize (DisassemblyArea& area) { area.deserialize_byte_block(MAC_ADDRESS_SIZE, nodeId); };
};

const NodeIdentifierT nullIdentifier;



// --------------------------------------------------------------------

typedef omnetpp::SimTime   TimeStampT;

// --------------------------------------------------------------------


typedef uint16_t      BPProtocolIdT;     // Identifier for BP client protocols

// Pre-defined BP client protocol id's
const BPProtocolIdT BP_PROTID_SRP     =  0x0001;
const BPProtocolIdT BP_PROTID_VARDIS  =  0x0002;

// --------------------------------------------------------------------


class StringT : public MemBlock, public TransmissibleType<sizeof(byte)> {
public:
    StringT () : MemBlock() {};
    StringT (const char* str) : MemBlock((byte) std::strlen(str), (byte*) str) {};
    StringT (const std::string& str) : MemBlock((byte) str.size(), (byte*) str.c_str()) {};

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
	  if (data) throw DisassemblyAreaException ("StringT::deserialize", "already contains data");
            data = new byte [length];
            area.deserialize_byte_block (length, data);
        }
    };
};




// --------------------------------------------------------------------
// Stuff relevant for OMNeT++



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

