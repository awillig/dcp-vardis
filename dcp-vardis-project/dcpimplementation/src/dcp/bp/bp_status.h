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
#include <iostream>
#include <string>
#include <stdexcept>
#include <dcp/common/service_primitives.h>

/**
 * @brief This module provides the definition of the BP services
 *        names, base classes for BP service primitives, and a helper
 *        function to convert a BP service name to a string
 */


namespace dcp::bp {

  
  /**
   * @brief BP Status value names
   */
  
  enum class BPStatus : std::uint16_t {
    Ok,
    Protocol_Already_Registered,
    Illegal_Max_Payload_Size,
    Unknown_Protocol,
    Payload_Too_Large,
    Empty_Payload,
    Illegal_Dropping_Queue_Size,
    Unknown_Queueing_Mode,
    Inactive,

    Internal_Error,
    Internal_Shared_Memory_Error,
    Illegal_Service_Type,
    Illegal_Protocol_Type    
  };


  /**
   * @brief Base types for BP Service primitives
   */

  

};  // namespace dcp::bp



/**
 * @brief Converts BP Status value to string
 */
constexpr std::string_view to_string(dcp::bp::BPStatus stat) {
  switch (stat) {
    case dcp::bp::BPStatus::Ok                             : return "Ok";
    case dcp::bp::BPStatus::Protocol_Already_Registered    : return "Protocol_Already_Registered";
    case dcp::bp::BPStatus::Illegal_Max_Payload_Size       : return "Illegal_Max_Payload_Size";
    case dcp::bp::BPStatus::Unknown_Protocol               : return "Unknown_Protocol";
    case dcp::bp::BPStatus::Payload_Too_Large              : return "Payload_Too_Large";
    case dcp::bp::BPStatus::Empty_Payload                  : return "Empty_Payload";
    case dcp::bp::BPStatus::Illegal_Dropping_Queue_Size    : return "Illegal_Dropping_Queue_Size";
    case dcp::bp::BPStatus::Unknown_Queueing_Mode          : return "Unknown_Queueing_Mode";
    case dcp::bp::BPStatus::Inactive                       : return "Inactive";
    case dcp::bp::BPStatus::Internal_Error                 : return "Internal_Error";
    case dcp::bp::BPStatus::Internal_Shared_Memory_Error   : return "Internal_Shared_Memory_Error";
    case dcp::bp::BPStatus::Illegal_Service_Type           : return "Illegal_Service_Type";
    case dcp::bp::BPStatus::Illegal_Protocol_Type          : return "Illegal Protocol Type";
    default:
      throw std::invalid_argument("to_string: unknown BP status value");    
    }
  return "";  
}  




std::ostream& operator<< (std::ostream& os, const dcp::bp::BPStatus stat);
