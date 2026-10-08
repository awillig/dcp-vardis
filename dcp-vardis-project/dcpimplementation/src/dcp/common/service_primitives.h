/**
 * Copyright (C) 2026 Andreas Willig, University of Canterbury
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

#include <cstdint>

/**
 * @brief This module declares constants for different BP and Vardis
 *        service types and status codes, defines base classes for
 *        service requests, responses and indications, and some helper
 *        functions to provide string representations of the service
 *        type and status constants
 */


namespace dcp {
  
  /*******************************************************************
   * Base classes for service primitives
   ******************************************************************/


  /**
   * @brief Base class for service requests
   */
  typedef struct ServiceRequest {
    std::uint16_t  s_type;
    ServiceRequest (std::uint16_t st) : s_type(st) {};
  } ServiceRequest;


  /**
   * @brief Base class for service confirms
   */
  typedef struct ServiceConfirm {
    std::uint16_t   s_type;
    std::uint16_t   status_code;
    ServiceConfirm (std::uint16_t st) : s_type(st) {};
    ServiceConfirm (std::uint16_t st, std::uint16_t stat) : s_type(st), status_code(stat) {};
  } ServiceConfirm;


  /**
   * @brief Base class for service indications
   */
  typedef struct ServiceIndication {
    std::uint16_t  s_type;
    ServiceIndication (std::uint16_t st) : s_type(st) {};
  } ServiceIndication;
  
  
};  // namespace dcp
