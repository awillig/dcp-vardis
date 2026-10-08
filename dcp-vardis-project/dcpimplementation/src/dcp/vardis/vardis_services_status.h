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
 * @brief This module provides definitions of the Vardis services and
 *        status names, base classes for Vardis service primitives,
 *        and helper functions to convert a Vardis service name or a
 *        Vardis status name to a string
 */


namespace dcp::vardis {

  /**
   * @brief Vardis service names
   */

  enum class VardisServiceType : std::uint16_t {
    RTDB_DescribeDatabase,
    RTDB_DescribeVariable,
    RTDB_Create,
    RTDB_Delete,
    RTDB_Update,
    RTDB_Read,
    
    Register,
    Deregister,
    
    ShutDown,
    Activate,
    Deactivate,
    GetStatistics};


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

  
  /**
   * @brief Base classes for Vardis service primitives
   */
  
  class VardisServiceRequest : public ServiceRequest {
  public:    
    VardisServiceRequest (VardisServiceType st) : ServiceRequest(static_cast<uint16_t>(st)){};
    VardisServiceType stype() const { return static_cast<VardisServiceType>(s_type); };
  };

  class VardisServiceConfirm : public ServiceConfirm {
  public:    
    VardisServiceConfirm (VardisServiceType st) : ServiceConfirm(static_cast<uint16_t>(st)){};
    VardisServiceConfirm(VardisServiceType st, VardisStatus stat)
        : ServiceConfirm(static_cast<uint16_t>(st),
                         static_cast<uint16_t>(stat)){};
    VardisServiceType stype() const { return static_cast<VardisServiceType>(s_type); };
    VardisStatus stat() const { return static_cast<VardisStatus>(status_code); };
    };

  class VardisServiceIndication : public ServiceIndication {
  public:    
    VardisServiceIndication (VardisServiceType st) : ServiceIndication(static_cast<uint16_t>(st)){};
    VardisServiceType stype() const { return static_cast<VardisServiceType>(s_type); };
  };
};  // namespace dcp::vardis


/**
 * @brief Converts Vardis service name to string
 *
 * Throws for unknown service name.
 */
constexpr std::string_view to_string (dcp::vardis::VardisServiceType st) {
  switch (st) {
  case dcp::vardis::VardisServiceType::RTDB_DescribeDatabase : return "Vardis-RTDB_DescribeDatabase";
  case dcp::vardis::VardisServiceType::RTDB_DescribeVariable : return "Vardis-RTDB_DescribeVariable";
  case dcp::vardis::VardisServiceType::RTDB_Create           : return "Vardis-RTDB_Create";
  case dcp::vardis::VardisServiceType::RTDB_Delete           : return "Vardis-RTDB_Delete";
  case dcp::vardis::VardisServiceType::RTDB_Update           : return "Vardis-RTDB_Update";
  case dcp::vardis::VardisServiceType::RTDB_Read             : return "Vardis-RTDB_Read";
  case dcp::vardis::VardisServiceType::Register              :  return "Vardis-RegisterP";
  case dcp::vardis::VardisServiceType::Deregister            :  return "Vardis-Deregister";
  case dcp::vardis::VardisServiceType::ShutDown              :  return "Vardis-ShutDown";
  case dcp::vardis::VardisServiceType::Activate              :  return "Vardis-Activate";
  case dcp::vardis::VardisServiceType::Deactivate            :  return "Vardis-Deactivate";
  case dcp::vardis::VardisServiceType::GetStatistics         :  return "Vardis-GetStatistics";
  default:
    throw std::invalid_argument("to_string: unknown Vardis service type");
  }
  return "";
}


/**
 * @brief Converts Vardis status name to string
 *
 * Throws for unknown status name
 */

constexpr std::string_view to_string(dcp::vardis::VardisStatus stat) {
    switch (stat)
      {
	// status codes prescribed by the specification
      case dcp::vardis::VardisStatus::Ok:                              return "Ok";
      case dcp::vardis::VardisStatus::Variable_Exists:                 return "Variable_Exists";
      case dcp::vardis::VardisStatus::Description_Too_Long:            return "Description_Too_Long";
      case dcp::vardis::VardisStatus::Value_Too_Long:                  return "Value_Too_Long";
      case dcp::vardis::VardisStatus::Empty_Value:                     return "Empty_Value";
      case dcp::vardis::VardisStatus::Illegal_Repcount:                return "Illegal_Repcount";
      case dcp::vardis::VardisStatus::Variable_Does_Not_Exist:         return "Variable_Does_Not_Exist";
      case dcp::vardis::VardisStatus::Not_Producer:                    return "Not_Producer";
      case dcp::vardis::VardisStatus::Variable_Is_Deleted:             return "Variable_Is_Deleted";
      case dcp::vardis::VardisStatus::Inactive:                        return "Inactive";

	// implementation-dependent status codes
      case dcp::vardis::VardisStatus::Internal_Error:                  return "Internal_Error";
      case dcp::vardis::VardisStatus::Application_Already_Registered:  return "Application_Already_Registered";
      case dcp::vardis::VardisStatus::Internal_Shared_Memory_Error:    return "Internal_Shared_Memory_Error";
      case dcp::vardis::VardisStatus::Unknown_Application:             return "Unknown_Application";
	
      default:
	throw std::invalid_argument(std::format("to_string: illegal Vardis status code {}", static_cast<uint16_t> (stat)));
      }
    return "";
  }
    




std::ostream& operator<< (std::ostream& os, const dcp::vardis::VardisServiceType st);
std::ostream& operator<< (std::ostream& os, const dcp::vardis::VardisStatus stat);
