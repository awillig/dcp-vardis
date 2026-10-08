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
   * @brief BP Service names
   */
  enum class BPServiceType : std::uint16_t {
    RegisterProtocol,
    DeregisterProtocol,
    ListRegisteredProtocols,
    ClearBuffer,
    QueryNumberBufferedPayloads,
    ReceivePayload,
    TransmitPayload,
    
    ShutDown,
    Activate,
    Deactivate,
    GetStatistics};


  
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

  
  class BPServiceRequest : public ServiceRequest {
  public:    
    BPServiceRequest (BPServiceType st) : ServiceRequest(static_cast<uint16_t>(st)){};
    BPServiceType stype() const { return static_cast<BPServiceType>(s_type); };
  };

  class BPServiceConfirm : public ServiceConfirm {
  public:    
    BPServiceConfirm (BPServiceType st) : ServiceConfirm(static_cast<uint16_t>(st)){};
    BPServiceConfirm(BPServiceType st, BPStatus stat)
        : ServiceConfirm(static_cast<uint16_t>(st),
			 static_cast<uint16_t> (stat)){};
    BPServiceType stype() const { return static_cast<BPServiceType>(s_type); };
    BPStatus stat() const { return static_cast<BPStatus>(status_code); };
    };

  class BPServiceIndication : public ServiceIndication {
  public:    
    BPServiceIndication (BPServiceType st) : ServiceIndication(static_cast<uint16_t>(st)){};
    BPServiceType stype() const { return static_cast<BPServiceType>(s_type); };
  };
};  // namespace dcp::bp


/**
 * @brief Converts BP service name to string
 *
 * Throws for unknown service name.
 */
constexpr std::string_view to_string (dcp::bp::BPServiceType st) {
  switch (st) {
  case dcp::bp::BPServiceType::RegisterProtocol             :  return "BP-RegisterProtocol";
  case dcp::bp::BPServiceType::DeregisterProtocol           :  return "BP-DeregisterProtocol";
  case dcp::bp::BPServiceType::ListRegisteredProtocols      :  return "BP-ListRegisteredProtocols";
  case dcp::bp::BPServiceType::ClearBuffer                  :  return "BP-ClearBuffer";
  case dcp::bp::BPServiceType::QueryNumberBufferedPayloads  :  return "BP-QueryNumberBufferedPayloads";
  case dcp::bp::BPServiceType::ReceivePayload               :  return "BP-ReceivePayload";
  case dcp::bp::BPServiceType::TransmitPayload              :  return "BP-TransmitPayload";
  case dcp::bp::BPServiceType::ShutDown                     :  return "BP-ShutDown";
  case dcp::bp::BPServiceType::Activate                     :  return "BP-Activate";
  case dcp::bp::BPServiceType::Deactivate                   :  return "BP-Deactivate";
  case dcp::bp::BPServiceType::GetStatistics                :  return "BP-GetStatistics";
  default:
    throw std::invalid_argument("to_string: unknown BP service type");
  }
  return "";
}



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



std::ostream& operator<< (std::ostream& os, const dcp::bp::BPServiceType st);
std::ostream& operator<< (std::ostream& os, const dcp::bp::BPStatus stat);
