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

#include <iostream>
#include <dcp/common/foundation_types.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/transmissible_type.h>


/**
 * @brief This module provides basic BP-specific transmissible types
 *        (i.e. types providing serialization facilities)
 */


namespace dcp::bp {

  /**
   * @brief Type for a BP length field (length of payloads)
   */
  class BPLengthT : public TransmissibleIntegral<uint16_t> {
  public:
    BPLengthT  () : TransmissibleIntegral<uint16_t> () {};
    BPLengthT (uint16_t v) : TransmissibleIntegral<uint16_t>(v) {};
    BPLengthT (const BPLengthT& other) : TransmissibleIntegral<uint16_t>(other) {};

    BPLengthT& operator= (const BPLengthT& other) { val = other.val; return *this; };
    
    friend std::ostream& operator<< (std::ostream&os, const BPLengthT& protId);
  };


  /**
   * @brief Type for DCP network identifiers
   */
  class BPNetworkIdentifierT : public TransmissibleIntegral<uint16_t> {
  public:
    BPNetworkIdentifierT  () : TransmissibleIntegral<uint16_t> () {};
    BPNetworkIdentifierT (uint16_t v) : TransmissibleIntegral<uint16_t>(v) {};
    BPNetworkIdentifierT (const BPNetworkIdentifierT& other) : TransmissibleIntegral<uint16_t>(other) {};
    
    BPNetworkIdentifierT& operator= (const BPNetworkIdentifierT& other) { val = other.val; return *this; };
    
    friend std::ostream& operator<< (std::ostream&os, const BPNetworkIdentifierT& netwId);
  };

  
};  // namespace dcp::bp
