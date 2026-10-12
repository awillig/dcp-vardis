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
#include <format>
#include <string>
#include <stdexcept>
#include <dcp/common/service_primitives.h>

/**
 * @brief This module provides definitions of the Vardis status names,
 *        base classes for Vardis service primitives, and helper
 *        functions to convert a Vardis status name to a string
 */


namespace dcp::vardis {

  /**
   * @brief Vardis status names
   */


  enum class VardisStatus : std::uint16_t {
    Ok,
    Variable_Exists,
    Description_Too_Long,
    Value_Too_Long,
    Empty_Value,
    Illegal_Repcount,
    Variable_Does_Not_Exist,
    Not_Producer,
    Variable_Is_Deleted,
    Inactive,

    Internal_Error,
    Application_Already_Registered,
    Internal_Shared_Memory_Error,
    Unknown_Application    
    };

  
};  // namespace dcp::vardis


/**
 * @brief Converts Vardis status name to string
 *
 * Throws for unknown status name
 */

constexpr std::string_view to_string(dcp::vardis::VardisStatus stat) {
    switch (stat)
      {
	// status codes prescribed by the specification
      case dcp::vardis::VardisStatus::Ok:                              return "VARDIS-STATUS-OK";
      case dcp::vardis::VardisStatus::Variable_Exists:                 return "VARDIS-STATUS-VARIABLE-EXISTS";
      case dcp::vardis::VardisStatus::Description_Too_Long:            return "VARDIS-STATUS-DESCRIPTION-TOO-LONG";
      case dcp::vardis::VardisStatus::Value_Too_Long:                  return "VARDIS-STATUS-VALUE-TOO-LONG";
      case dcp::vardis::VardisStatus::Empty_Value:                     return "VARDIS-STATUS-EMPTY-VALUE";
      case dcp::vardis::VardisStatus::Illegal_Repcount:                return "VARDIS-STATUS-ILLEGAL-REPCOUNT";
      case dcp::vardis::VardisStatus::Variable_Does_Not_Exist:         return "VARDIS-STATUS-VARIABLE-DOES-NOT-EXIST";
      case dcp::vardis::VardisStatus::Not_Producer:                    return "VARDIS-STATUS-NOT-PRODUCER";
      case dcp::vardis::VardisStatus::Variable_Is_Deleted:             return "VARDIS-STATUS-VARIABLE-IS-DELETED";
      case dcp::vardis::VardisStatus::Inactive:                        return "VARDIS-STATUS-INACTIVE";

	// implementation-dependent status codes
      case dcp::vardis::VardisStatus::Internal_Error:                  return "VARDIS-STATUS-INTERNAL-ERROR";
      case dcp::vardis::VardisStatus::Application_Already_Registered:  return "VARDIS-STATUS-APPLICATION-ALREADY-REGISTERED";
      case dcp::vardis::VardisStatus::Internal_Shared_Memory_Error:    return "VARDIS-STATUS-INTERNAL-SHARED-MEMORY-ERROR";
      case dcp::vardis::VardisStatus::Unknown_Application:             return "VARDIS-STATUS-UNKNOWN-APPLICATION";
	
      default:
	throw std::invalid_argument(std::format("to_string: illegal Vardis status code {}", static_cast<uint16_t> (stat)));
      }
    return "";
  }
    




std::ostream& operator<< (std::ostream& os, const dcp::vardis::VardisStatus stat);
