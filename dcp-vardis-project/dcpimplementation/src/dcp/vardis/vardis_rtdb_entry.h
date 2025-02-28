/**
 * Copyright (C) 2025 Andreas Willig, University of Canterbury
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */


#pragma once

#include <dcp/common/global_types_constants.h>
#include <dcp/vardis/vardis_transmissible_types.h>

using dcp::TimeStampT;

// -----------------------------------------

/**
 * @brief Declares the structure of an entry into the VarDis variable
 *        database (RTDB -- Real-Time DataBase).
 *
 * Note: after introducing the VariableStore concept, the DBEntry type
 * does *not* contain the variable value or variable description
 * anymore. This turns the DBEntry type into a fixed-size type.
 */


// -----------------------------------------

namespace dcp::vardis {

  typedef struct DBEntry {
    VarIdT            varId;
    NodeIdentifierT   prodId;
    VarRepCntT        repCnt;
    
    VarSeqnoT       seqno;                   /*!< Last received sequence number for this variable */
    TimeStampT      tStamp;                  /*!< Timestamp where the last update (or create) instruction has been processed */
    VarRepCntT      countUpdate = 0;         /*!< Repetition counter for VarUpdateT instructions */
    VarRepCntT      countCreate = 0;         /*!< Repetition counter for VarCreateT instructions */
    VarRepCntT      countDelete = 0;         /*!< Repetition counter for VarDeleteT instructions */
    bool            toBeDeleted = false;     /*!< Indicates whether variable is currently in the process of being deleted */
  } DBEntry;

};  // namespace dcp::vardis
