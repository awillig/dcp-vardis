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

#ifndef DCP_VARDIS_VARDISPACKET_H_
#define DCP_VARDIS_VARDISPACKET_H_

#include <dcp/vardis/VardisDatatypes.h>

// -----------------------------------------

/**
 * This module provides functions to support constructing / serializing packets
 * as byte vectors, as well as deconstructing / deserializing them.
 *
 * The actual data type representing the byte vector is given its own name, and
 * the (inline) functions bvPushByte, bvPopByte, bvPushByteArray, bvPopByteArray
 * contain code that requires knowledge of the actual data type. All the other
 * functions (encoding/decoding all the transmissible data types of VarDis)
 * are expressed solely in terms of these four functions. These four functions
 * should normally not be used in other code.
 *
 * As convention, the 'bvPush*' functions are used in constructing payload,
 * the 'bvPop*' functions in deconstructing. Note that the 'bvPop*' functions
 * allocate memory where needed.
 *
 * It needs to be ensured that the number of bytes pushed onto / popped from
 * byte vectors is consistent with the 'serializedSizeT_B' constraints from
 * VardisDatatypes.h.
 */


// ----------------------------------------------------------------
// Data type and elementary operations for byte array
// ----------------------------------------------------------------


typedef std::vector<uint8_t>  bytevect;

/**
 * add one byte to the end of byte vector, update counters accordingly
 */
inline void bvPushByte (bytevect& bv, const uint8_t byte, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    assert(bytesAvailable > 0);
    bv.push_back(byte);
    bytesUsed++;
    bytesAvailable--;
}

/**
 * remove one byte from the current index of byte vector, update counters accordingly
 */
inline void bvPopByte  (bytevect& bv, uint8_t& byte, unsigned int& bytesUsed)
{
    assert(bytesUsed < bv.size());
    byte = bv[bytesUsed];
    bytesUsed++;
}

/**
 * Adds a given byte array (pointer to bytes) to the output byte vector,
 * update counters accordingly
 */
inline void bvPushByteArray (bytevect& bv, const uint8_t* value, const VarLenT length, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    assert(length > 0);
    assert(bytesAvailable >= length);
    assert(value);

    for (int i=0; i<length; i++)
        bv.push_back(value[i]);
    bytesUsed      += length;
    bytesAvailable -= length;
}

/**
 * Pops a byte array of given length from the byte vector (the memory for the
 * byte array is to be provided by the caller), update counters accordingly
 */
inline void bvPopByteArray (bytevect& bv, uint8_t* value, const VarLenT length, unsigned int& bytesUsed)
{
    assert(length > 0);
    assert(bytesUsed + length <= bv.size());
    assert(value);

    std::memcpy(value, &(bv[bytesUsed]), length);
    bytesUsed += length;
}



// ----------------------------------------------------------------
// Operations for constructing the transmissible Vardis datatypes
// ----------------------------------------------------------------


void bvPushVarId (bytevect& bv, const VarIdT& varId, unsigned int& bytesUsed, unsigned int& bytesAvailable);
void bvPopVarId  (bytevect& bv, VarIdT& varId, unsigned int& bytesUsed);


void bvPushVarSeqno (bytevect& bv, const VarSeqnoT& seqno, unsigned int& bytesUsed, unsigned int& bytesAvailable);
void bvPopVarSeqno  (bytevect& bv, VarSeqnoT& seqno, unsigned int& bytesUsed);


void bvPushVarLen (bytevect& bv, const VarLenT& length, unsigned int& bytesUsed, unsigned int& bytesAvailable);
void bvPopVarLen  (bytevect& bv, VarLenT& length, unsigned int& bytesUsed);


void bvPushVarRepCnt (bytevect& bv, const VarRepCntT& repCnt, unsigned int& bytesUsed, unsigned int& bytesAvailable);
void bvPopVarRepCnt  (bytevect& bv, VarRepCntT& repCnt, unsigned int& bytesUsed);


void bvPushVarSumm (bytevect& bv, const VarSummT& summ, unsigned int& bytesUsed, unsigned int& bytesAvailable);
void bvPopVarSumm  (bytevect& bv, VarSummT& summ, unsigned int& bytesUsed);


void bvPushVarUpdate (bytevect& bv, const VarUpdateT& update, unsigned int& bytesUsed, unsigned int& bytesAvailable);
void bvPopVarUpdate  (bytevect& bv, VarUpdateT& update, unsigned int& bytesUsed);


void bvPushVarSpec (bytevect& bv, const VarSpecT& spec, unsigned int& bytesUsed, unsigned int& bytesAvailable);
void bvPopVarSpec (bytevect& bv, VarSpecT& spec, unsigned int& bytesUsed);


void bvPushVarCreate (bytevect& bv, const VarCreateT& create, unsigned int& bytesUsed, unsigned int& bytesAvailable);
void bvPopVarCreate  (bytevect& bv, VarCreateT& create, unsigned int& bytesUsed);


void bvPushVarDelete (bytevect& bv, const VarDeleteT& del, unsigned int& bytesUsed, unsigned int& bytesAvailable);
void bvPopVarDelete  (bytevect& bv, VarDeleteT& del, unsigned int& bytesUsed);


void bvPushVarReqUpdate (bytevect& bv, const VarReqUpdateT& requpd, unsigned int& bytesUsed, unsigned int& bytesAvailable);
void bvPopVarReqUpdate  (bytevect& bv, VarReqUpdateT& requpd, unsigned int& bytesUsed);


void bvPushVarReqCreate  (bytevect& bv, const VarReqCreateT& reqcr, unsigned int& bytesUsed, unsigned int& bytesAvailable);
void bvPopVarReqCreate   (bytevect& bv, VarReqCreateT& reqcr, unsigned int& bytesUsed);


void bvPushIEHeader (bytevect& bv, const IEHeaderT& ieHdr, unsigned int& bytesUsed, unsigned int& bytesAvailable);
void bvPopIEHeader  (bytevect& bv, IEHeaderT& ieHdr, unsigned int& bytesUsed);


#endif /* DCP_VARDIS_VARDISPACKET_H_ */
