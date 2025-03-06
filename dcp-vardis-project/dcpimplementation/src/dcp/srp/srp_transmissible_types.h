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

/**
 * @brief This module contains the two key transmissible data types of
 *        the SRP protocol
 *
 * Note that for simplicity we do not make use of the serialization
 * framework, and just use these types as is.
 */


namespace dcp::srp {


  /**
   * @brief Contains the position of the sender node
   *
   * To be refined by applications, e.g. possibly be extended to
   * contain more (e.g. heading).
   */
  typedef struct SafetyDataT {
    double  position_x;
    double  position_y;
    double  position_z;
    friend std::ostream& operator<<(std::ostream& os, const SafetyDataT& sd);
  } SafetyDataT;
  

  /**
   * @brief Contains the ExtendedSafetyDataT structure, including
   *        actual safety data and metadata such as a sequence number
   */
  typedef struct ExtendedSafetyDataT {
    SafetyDataT       safetyData;
    NodeIdentifierT   nodeId;
    TimeStampT        timeStamp;
    uint32_t          seqno;

    friend std::ostream& operator<<(std::ostream& os, const ExtendedSafetyDataT& esd);    
  } ExtendedSafetyDataT;

  
};  // namespace dcp::srp
