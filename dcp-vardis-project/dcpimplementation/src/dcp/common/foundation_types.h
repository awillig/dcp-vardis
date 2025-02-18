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
#include <iomanip>
#include <string>
#include <vector>
#include <cstdint>

namespace dcp {

  /**
   * @brief This module contains definitions of some basic data types
     used in the DCP implementation
   */
  

  /**
   * @brief Definition of a byte
   */
  typedef uint8_t byte;
  
  
  /**
   * @brief Type definition of bytevect as a std::vector of bytes
   */
  typedef std::vector<byte> bytevect;
    
};  // namespace dcp
