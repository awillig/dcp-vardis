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

#ifndef DCP_VARDIS_VARDISDATATYPES_H_
#define DCP_VARDIS_VARDISDATATYPES_H_

#include <inet/linklayer/common/MacAddress.h>
#include <dcp/common/DcpTypesGlobals.h>

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


typedef uint8_t  VarIdT;
typedef uint8_t  VarLenT;
typedef uint8_t  VarRepCntT;
typedef uint8_t  VarSeqnoT;

const int maxVarId     = 255;
const int maxVarLen    = 255;
const int maxVarRepCnt = 15;
const int maxVarSeqno  = 256;

const size_t serializedSizeVarId_B      = 1;
const size_t serializedSizeVarLen_B     = 1;
const size_t serializedSizeVarRepCnt_B  = 1;
const size_t serializedSizeVarSeqno_B   = 1;

// -----------------------------------------

typedef struct VarSummT {
    VarIdT     varId;
    VarSeqnoT  seqno;
} VarSummT;

const size_t serializedSizeVarSummT_B = serializedSizeVarId_B + serializedSizeVarSeqno_B;

// -----------------------------------------

typedef struct VarUpdateT {
    VarIdT    varId;
    VarSeqnoT seqno;
    VarLenT   length;
    uint8_t* value;
} VarUpdHeaderT;

const size_t serializedSizeVarUpdateT_FixedPart_B = serializedSizeVarId_B + serializedSizeVarSeqno_B + serializedSizeVarLen_B ;

// -----------------------------------------

typedef struct VarSpecT {
    VarIdT      varId;
    uint8_t     prodId [MAC_ADDRESS_SIZE];
    VarRepCntT  repCnt;
    VarLenT     descrLen;   // must be length of C string descr plus one (for null terminator)
    uint8_t    *descr;      // encoded as C string, includes null terminator at the end
} VarSpecT;

const size_t serializedSizeVarSpecT_FixedPart_B = serializedSizeVarId_B + MAC_ADDRESS_SIZE + serializedSizeVarRepCnt_B + serializedSizeVarLen_B;

// -----------------------------------------

typedef struct VarCreateT {
    VarSpecT       spec;
    VarUpdateT     update;
} VarCreateT;

const size_t serializedSizeVarCreateT_FixedPart_B = serializedSizeVarSpecT_FixedPart_B + serializedSizeVarUpdateT_FixedPart_B;

// -----------------------------------------

typedef struct VarDeleteT {
    VarIdT varId;
} VarDeleteT;

const size_t serializedSizeVarDeleteT_B = serializedSizeVarId_B;

// -----------------------------------------

typedef struct VarReqUpdateT {
    VarSummT updSpec;
} VarReqUpdateT;

const size_t serializedSizeVarReqUpdateT_B = serializedSizeVarSummT_B;

// -----------------------------------------

typedef struct VarReqCreateT {
    VarIdT varId;
} VarReqCreateT;

const size_t serializedSizeVarReqCreateT_B = serializedSizeVarId_B;

// -----------------------------------------

typedef enum IEType
{
    IETYPE_SUMMARIES           =  1,
    IETYPE_UPDATES             =  2,
    IETYPE_REQUEST_VARUPDATES  =  3,
    IETYPE_REQUEST_VARCREATES  =  4,
    IETYPE_CREATE_VARIABLES    =  5,
    IETYPE_DELETE_VARIABLES    =  6
} IEType;

typedef struct IEHeaderT {
    uint8_t ieType;
    uint8_t ieNumRecords;
} IEHeaderT;

const size_t serializedSizeIEHeaderT_B = 2;
const int maxRecordsInInformationElement = 255;

// -----------------------------------------


const unsigned int maxInformationElementRecords = 255;


// checks if the first seqno is more recent than the second one
#define MORE_RECENT_SEQNO(a,b)  ((((a) > (b)) && (((a)-(b)) < (maxVarSeqno/2))) || (((a) < (b)) && (((b)-(a)) > (maxVarSeqno/2))))

#endif /* DCP_VARDIS_VARDISDATATYPES_H_ */
