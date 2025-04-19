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

#include <limits>
#include <inet/linklayer/common/MacAddress.h>
#include <dcp/common/memblock.h>
#include <dcpsim/common/DcpTypesGlobals.h>
#include <dcpsim/common/TransmissibleType.h>

// -----------------------------------------

/**
 * This header declares the various data types defined in the VarDis specification
 * and used by the VarDis protocol implementation, in particular all the transmissible
 * data types. For each data type we define:
 *   - the data type T as such
 *   - a size_t-typed constant 'serializedSizeT_B' saying how many bytes a
 *     serialized representation of this data type will need -- to be used
 *     in the packet construction code
 * For some data types that are directly mapped to simple types we also declare
 * their maximum possible value.
 * For structure types S that have previously defined types as field, it is best to
 * express their 'serializedSizeS_B' value as a sum of the 'serializedSize*' values
 * of the field types, as appropriate
 */


// -----------------------------------------

using namespace dcp;

namespace dcp {

// -----------------------------------------

typedef uint8_t  VarIdT;
typedef uint8_t  VarLenT;
typedef uint8_t  VarRepCntT;
typedef uint8_t  VarSeqnoT;

const size_t maxVarId     = 255;
const size_t maxVarLen    = 255;
const size_t maxVarRepCnt = 15;
const size_t maxVarSeqno  = 256;

// checks if the first seqno is more recent than the second one
#define MORE_RECENT_SEQNO(a,b)  ((((a) > (b)) && (((a)-(b)) < ((int)(maxVarSeqno/2)))) || (((a) < (b)) && (((b)-(a)) > ((int)(maxVarSeqno/2)))))


// -----------------------------------------

/*
 * Type representing a VarDis value, made up of one field indicating the
 * length, and a byte array of that exact length
 */
class VarValueT : public MemBlock, public TransmissibleType<sizeof(VarLenT)> {
 public:

    VarValueT () : MemBlock() {};
    VarValueT (VarLenT size, byte* data) : MemBlock(size, data) {};

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
            if (data) throw SerializationException ("VarValueT::deserialize: already contains data");

            data = new byte [length];
            area.deserialize_byte_block (length, data);
        }
    };
};




// -----------------------------------------


class VarSummT : public TransmissibleType<sizeof(VarIdT)+sizeof(VarSeqnoT)> {
public:
    VarIdT     varId;
    VarSeqnoT  seqno;

    virtual void serialize (AssemblyArea& area)
    {
        area.serialize_byte(varId);
        area.serialize_byte(seqno);
    };
    virtual void deserialize (DisassemblyArea& area)
    {
        varId = area.deserialize_byte();
        seqno = area.deserialize_byte();
    };
};



// -----------------------------------------

class VarUpdateT : public TransmissibleType<sizeof(VarIdT)+sizeof(VarSeqnoT)+sizeof(VarLenT)> {
public:
    VarIdT     varId;
    VarSeqnoT  seqno;
    VarValueT  value;

    virtual size_t total_size () const { return fixed_size() + value.length; };
    virtual void serialize (AssemblyArea& area)
    {
        area.serialize_byte (varId);
        area.serialize_byte (seqno);
        value.serialize (area);
    };
    virtual void deserialize (DisassemblyArea& area)
    {
        varId = area.deserialize_byte ();
        seqno = area.deserialize_byte ();
        value.deserialize (area);
    };
};



// -----------------------------------------


class VarSpecT : public TransmissibleType<sizeof(VarIdT)+MAC_ADDRESS_SIZE+sizeof(VarRepCntT)+sizeof(VarLenT)> {
public:
    VarIdT            varId;
    NodeIdentifierT   prodId;
    VarRepCntT        repCnt;
    StringT           descr;

    virtual size_t total_size () const { return fixed_size() + descr.length; };
    virtual void serialize (AssemblyArea& area)
    {
        area.serialize_byte (varId);
        prodId.serialize (area);
        area.serialize_byte (repCnt);
        descr.serialize (area);
    };
    virtual void deserialize (DisassemblyArea& area)
    {
        varId  = area.deserialize_byte ();
        prodId.deserialize (area);
        repCnt = area.deserialize_byte ();
        descr.deserialize (area);
    };
};



// -----------------------------------------


class VarCreateT : public TransmissibleType<sizeof(VarIdT)+MAC_ADDRESS_SIZE+sizeof(VarRepCntT)+sizeof(VarLenT)+sizeof(VarIdT)+sizeof(VarSeqnoT)+sizeof(VarLenT)> {
public:
    VarSpecT    spec;
    VarUpdateT  update;

    virtual size_t total_size () const { return fixed_size() + spec.descr.length + update.value.length; };
    virtual void serialize (AssemblyArea& area)
    {
        spec.serialize (area);
        update.serialize (area);
    };
    virtual void deserialize (DisassemblyArea& area)
    {
        spec.deserialize (area);
        update.deserialize (area);
    };
};


// -----------------------------------------


class VarDeleteT : public TransmissibleType<sizeof(VarIdT)> {
public:
    VarIdT  varId;
    virtual void serialize (AssemblyArea& area) { area.serialize_byte(varId); };
    virtual void deserialize (DisassemblyArea& area) { varId = area.deserialize_byte (); };
};

// -----------------------------------------


class VarReqUpdateT : public TransmissibleType<sizeof(VarIdT)+sizeof(VarSeqnoT)> {
public:
    VarSummT updSpec;
    virtual void serialize (AssemblyArea& area) { updSpec.serialize (area); };
    virtual void deserialize (DisassemblyArea& area) { updSpec.deserialize (area); };
};


// -----------------------------------------


class VarReqCreateT : public TransmissibleType<sizeof(VarIdT)> {
public:
    VarIdT  varId;
    virtual void serialize (AssemblyArea& area) { area.serialize_byte(varId); };
    virtual void deserialize (DisassemblyArea& area) { varId = area.deserialize_byte(); };
};


// -----------------------------------------

typedef enum ICType
{
    ICTYPE_SUMMARIES           =  1,
    ICTYPE_UPDATES             =  2,
    ICTYPE_REQUEST_VARUPDATES  =  3,
    ICTYPE_REQUEST_VARCREATES  =  4,
    ICTYPE_CREATE_VARIABLES    =  5,
    ICTYPE_DELETE_VARIABLES    =  6
} ICType;


class ICHeaderT : public TransmissibleType<sizeof(byte)+sizeof(byte)> {
public:
    byte icType;
    byte icNumRecords;

    static byte max_records() { return std::numeric_limits<byte>::max(); };

    virtual void serialize (AssemblyArea& area)
    {
        area.serialize_byte (icType);
        area.serialize_byte (icNumRecords);
    };
    virtual void deserialize (DisassemblyArea& area)
    {
        icType        = area.deserialize_byte();
        icNumRecords  = area.deserialize_byte();
    };
};


// -----------------------------------------





};  // namespace dcp
