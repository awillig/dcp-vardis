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


#include <dcp/vardis/VardisPacket.h>


// ----------------------------------------------------

void bvPushVarId (bytevect& bv, const VarIdT& varId, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushByte(bv, varId, bytesUsed, bytesAvailable);
}

void bvPopVarId (bytevect& bv, VarIdT& varId, unsigned int& bytesUsed)
{
    bvPopByte(bv, varId, bytesUsed);
}

// ----------------------------------------------------

void bvPushVarSeqno (bytevect& bv, const VarSeqnoT& seqno, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushByte(bv, seqno, bytesUsed, bytesAvailable);
}

void bvPopVarSeqno  (bytevect& bv, VarSeqnoT& seqno, unsigned int& bytesUsed)
{
    bvPopByte(bv, seqno, bytesUsed);
}

// ----------------------------------------------------

void bvPushVarLen (bytevect& bv, const VarLenT& length, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushByte(bv, length, bytesUsed, bytesAvailable);
}

void bvPopVarLen  (bytevect& bv, VarLenT& length, unsigned int& bytesUsed)
{
    bvPopByte(bv, length, bytesUsed);
}

// ----------------------------------------------------

void bvPushVarRepCnt (bytevect& bv, const VarRepCntT& repCnt, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushByte(bv, repCnt, bytesUsed, bytesAvailable);
}

void bvPopVarRepCnt  (bytevect& bv, VarRepCntT& repCnt, unsigned int& bytesUsed)
{
    bvPopByte(bv, repCnt, bytesUsed);
}

// ----------------------------------------------------

void bvPushVarSumm (bytevect& bv, const VarSummT& summ, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushVarId(bv, summ.varId, bytesUsed, bytesAvailable);
    bvPushVarSeqno(bv, summ.seqno, bytesUsed, bytesAvailable);
}

void bvPopVarSumm  (bytevect& bv, VarSummT& summ, unsigned int& bytesUsed)
{
    bvPopVarId(bv, summ.varId, bytesUsed);
    bvPopVarSeqno(bv, summ.seqno, bytesUsed);
}

// ----------------------------------------------------

void bvPushVarUpdHeader (bytevect& bv, const VarUpdHeaderT& updhdr, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushVarId(bv, updhdr.varId, bytesUsed, bytesAvailable);
    bvPushVarSeqno(bv, updhdr.seqno, bytesUsed, bytesAvailable);
    bvPushVarLen(bv, updhdr.length, bytesUsed, bytesAvailable);
}

void bvPopVarUpdHeader  (bytevect& bv, VarUpdHeaderT& updhdr, unsigned int& bytesUsed)
{
    bvPopVarId(bv, updhdr.varId, bytesUsed);
    bvPopVarSeqno(bv, updhdr.seqno, bytesUsed);
    bvPopVarLen(bv, updhdr.length, bytesUsed);
}

// ----------------------------------------------------

void bvPushVarUpdate (bytevect& bv, const VarUpdateT& update, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushVarId(bv, update.varId, bytesUsed, bytesAvailable);
    bvPushVarSeqno(bv, update.seqno, bytesUsed, bytesAvailable);
    bvPushVarLen(bv, update.length, bytesUsed, bytesAvailable);
    bvPushByteArray(bv, update.value, update.length, bytesUsed, bytesAvailable);
}

void bvPopVarUpdate  (bytevect& bv, VarUpdHeaderT& update, unsigned int& bytesUsed)
{
    bvPopVarId(bv, update.varId, bytesUsed);
    bvPopVarSeqno(bv, update.seqno, bytesUsed);
    bvPopVarLen(bv, update.length, bytesUsed);
    update.value = new uint8_t [update.length];
    bvPopByteArray(bv, update.value, update.length, bytesUsed);
}

// ----------------------------------------------------

void bvPushVarSpec (bytevect& bv, const VarSpecT& spec, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushVarId(bv, spec.varId, bytesUsed, bytesAvailable);
    bvPushByteArray(bv, spec.prodId, MAC_ADDRESS_SIZE, bytesUsed, bytesAvailable);
    bvPushVarRepCnt(bv, spec.repCnt, bytesUsed, bytesAvailable);
    bvPushVarLen(bv, spec.descrLen, bytesUsed, bytesAvailable);
    bvPushByteArray(bv, spec.descr, spec.descrLen, bytesUsed, bytesAvailable);
}

void bvPopVarSpec (bytevect& bv, VarSpecT& spec, unsigned int& bytesUsed)
{
    bvPopVarId(bv, spec.varId, bytesUsed);
    bvPopByteArray(bv, spec.prodId, MAC_ADDRESS_SIZE, bytesUsed);
    bvPopVarRepCnt(bv, spec.repCnt, bytesUsed);
    bvPopVarLen(bv, spec.descrLen, bytesUsed);
    spec.descr = new uint8_t [spec.descrLen];
    bvPopByteArray(bv, spec.descr, spec.descrLen, bytesUsed);
}

// ----------------------------------------------------

void bvPushVarCreate (bytevect& bv, const VarCreateT& create, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushVarSpec(bv, create.spec, bytesUsed, bytesAvailable);
    bvPushVarUpdate(bv, create.update, bytesUsed, bytesAvailable);
}

void bvPopVarCreate (bytevect& bv, VarCreateT& create, unsigned int& bytesUsed)
{
    bvPopVarSpec(bv, create.spec, bytesUsed);
    bvPopVarUpdate(bv, create.update, bytesUsed);
}

// ----------------------------------------------------

void bvPushVarDelete (bytevect& bv, const VarDeleteT& del, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushVarId(bv, del.varId, bytesUsed, bytesAvailable);
}

void bvPopVarDelete  (bytevect& bv, VarDeleteT& del, unsigned int& bytesUsed)
{
    bvPopVarId(bv, del.varId, bytesUsed);
}

// ----------------------------------------------------

void bvPushVarReqUpdate  (bytevect& bv, const VarReqUpdateT& requpd, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushVarSumm(bv, requpd.updSpec, bytesUsed, bytesAvailable);
}

void bvPopVarReqUpdate  (bytevect& bv, VarReqUpdateT& requpd, unsigned int& bytesUsed)
{
    bvPopVarSumm(bv, requpd.updSpec, bytesUsed);
}

// ----------------------------------------------------

void bvPushVarReqCreate  (bytevect& bv, const VarReqCreateT& reqcr, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushVarId(bv, reqcr.varId, bytesUsed, bytesAvailable);
}

void bvPopVarReqCreate  (bytevect& bv, VarReqCreateT& reqcr, unsigned int& bytesUsed)
{
    bvPopVarId(bv, reqcr.varId, bytesUsed);
}

// ----------------------------------------------------

void bvPushIEHeader (bytevect& bv, const IEHeaderT& ieHdr, unsigned int& bytesUsed, unsigned int& bytesAvailable)
{
    bvPushByte(bv, (uint8_t) ieHdr.ieType, bytesUsed, bytesAvailable);
    bvPushByte(bv, (uint8_t) ieHdr.ieNumRecords, bytesUsed, bytesAvailable);
}

void bvPopIEHeader  (bytevect& bv, IEHeaderT& ieHdr, unsigned int& bytesUsed)
{
    bvPopByte(bv, ieHdr.ieType, bytesUsed);
    bvPopByte(bv, ieHdr.ieNumRecords, bytesUsed);
}


