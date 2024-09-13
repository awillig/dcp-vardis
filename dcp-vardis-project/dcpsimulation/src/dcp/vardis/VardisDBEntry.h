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

#ifndef DCP_VARDIS_VARDISDBENTRY_H_
#define DCP_VARDIS_VARDISDBENTRY_H_

#include <dcp/vardis/VardisDatatypes.h>

// -----------------------------------------

/**
 * This header declares the structure of an entry into the VarDis
 * variable database (RTDB -- Real-Time DataBase).
 */


// -----------------------------------------


// structure for an entry in the real-time database
typedef struct DBEntry {
    VarSpecT        spec;
    VarSeqnoT       seqno;
    simtime_t       tStamp;
    VarRepCntT      countUpdate = 0;
    VarRepCntT      countCreate = 0;
    VarRepCntT      countDelete = 0;
    bool            toBeDeleted = false;
    VarLenT         length      = 0;
    uint8_t        *value       = nullptr;
} DBEntry;



#endif /* DCP_VARDIS_VARDISDBENTRY_H_ */
