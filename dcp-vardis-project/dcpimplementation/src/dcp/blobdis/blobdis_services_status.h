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
 * @brief This module provides definitions of the Blobdis services and
 *        status names, base classes for Blobdis service primitives,
 *        and helper functions to convert a Blobdis service name or a
 *        Blobdis status name to a string
 */


namespace dcp::blobdis {

  /**
   * @brief Blobdis service names
   */

  enum class BlobdisServiceType : std::uint16_t {
    AppRegister,
    AppDeregister,
    AppListRegistered,
    AppDescribe,

    RxNewBlob,
    RxCompletion,
    RxRelease,
    RxAbort,
    RxProgress,

    TxTransmit,
    TxCompletion,
    TxRelease,
    TxAbort,
    TxProgress,

    MgmtListBlacklist,
    MgmtListActiveBlobs,
    MgmtActivate,
    MgmtDeactivate,
    MgmtStatus,
    
    ShutDown,
    GetStatistics
  };


  /**
   * @brief Blobdis status names
   */


  enum class BlobdisStatus : std::uint16_t {
    Ok,
    Application_Unknown,
    Application_Already_Registered,
    Illegal_Blob_Length,
    Inactive,
    Blob_Unknown,
    Blob_Blacklisted,
    Abort_On_Transmit_Blob,
    Abort_On_Receive_Blob,
    Blob_Already_Aborted,
    Illegal_Part_Size,
    Illegal_Time_Value,
    Illegal_App_Identifier,

    Internal_Error,
    Internal_Shared_Memory_Error
    };

  
  /**
   * @brief Base classes for Blobdis service primitives
   */
  
  class BlobdisServiceRequest : public ServiceRequest {
  public:    
    BlobdisServiceRequest (BlobdisServiceType st) : ServiceRequest(static_cast<uint16_t>(st)){};
    BlobdisServiceType stype() const { return static_cast<BlobdisServiceType>(s_type); };
  };

  class BlobdisServiceConfirm : public ServiceConfirm {
  public:    
    BlobdisServiceConfirm (BlobdisServiceType st) : ServiceConfirm(static_cast<uint16_t>(st)){};
    BlobdisServiceConfirm(BlobdisServiceType st, BlobdisStatus stat)
        : ServiceConfirm(static_cast<uint16_t>(st),
                         static_cast<uint16_t>(stat)){};
    BlobdisServiceType stype() const { return static_cast<BlobdisServiceType>(s_type); };
    BlobdisStatus stat() const { return static_cast<BlobdisStatus>(status_code); };
    };

  class BlobdisServiceIndication : public ServiceIndication {
  public:    
    BlobdisServiceIndication (BlobdisServiceType st) : ServiceIndication(static_cast<uint16_t>(st)){};
    BlobdisServiceType stype() const { return static_cast<BlobdisServiceType>(s_type); };
  };
};  // namespace dcp::blobdis


/**
 * @brief Converts Blobdis service name to string
 *
 * Throws for unknown service name.
 */
constexpr std::string_view to_string (dcp::blobdis::BlobdisServiceType st) {
  switch (st) {
  case dcp::blobdis::BlobdisServiceType::AppRegister:         return "BD-AppRegister";
  case dcp::blobdis::BlobdisServiceType::AppDegister:         return "BD-AppDeregister";
  case dcp::blobdis::BlobdisServiceType::AppListRegistered:   return "BD-AppListRegistered";
  case dcp::blobdis::BlobdisServiceType::AppDescribe:         return "BD-AppDescribe";
  case dcp::blobdis::BlobdisServiceType::RxNewBlob:           return "BD-RxNewBlob";
  case dcp::blobdis::BlobdisServiceType::RxCompletion:        return "BD-RxCompletion";
  case dcp::blobdis::BlobdisServiceType::RxRelease:           return "BD-RxRelease";
  case dcp::blobdis::BlobdisServiceType::RxAbort:             return "BD-RxAbort";
  case dcp::blobdis::BlobdisServiceType::RxProgress:          return "BD-RxProgress";
  case dcp::blobdis::BlobdisServiceType::TxTransmit:          return "BD-TxTransmit";
  case dcp::blobdis::BlobdisServiceType::TxCompletion:        return "BD-TxCompletion";
  case dcp::blobdis::BlobdisServiceType::TxRelease:           return "BD-TxRelease";
  case dcp::blobdis::BlobdisServiceType::TxAbort:             return "BD-TxAbort";
  case dcp::blobdis::BlobdisServiceType::TxProgress:          return "BD-TxProgress";
  case dcp::blobdis::BlobdisServiceType::MgmtListBlacklist:   return "BD-MgmtListBlacklist";
  case dcp::blobdis::BlobdisServiceType::MgmtListActiveBlobs: return "BD-MgmtListActiveBlobs";
  case dcp::blobdis::BlobdisServiceType::MgmtActivate:        return "BD-MgmtActivate";
  case dcp::blobdis::BlobdisServiceType::MgmtDeactivate:      return "BD-MgmtDeactivate";
  case dcp::blobdis::BlobdisServiceType::MgmtStatus:          return "BD-MgmtStatus";
  case dcp::blobdis::BlobdisServiceType::ShutDown:            return "ShutDown";
  case dcp::blobdis::BlobdisServiceType::GetStatistics:       return "GetStatistics"; 
  default:
    throw std::invalid_argument("to_string: unknown Blobdis service type");
  }
  return "";
}


/**
 * @brief Converts Blobdis status name to string
 *
 * Throws for unknown status name
 */

constexpr std::string_view to_string(dcp::blobdis::BlobdisStatus stat) {
    switch (stat)
      {
	// status codes prescribed by the specification
      case dcp::blobdis::BlobdisStatus::Ok:                              return "Ok";
      case dcp::blobdis::BlobdisStatus::Application_Unknown:             return "Application_Unknown";
      case dcp::blobdis::BlobdisStatus::Application_Already_Registered:  return "Application_Already_Registered";
      case dcp::blobdis::BlobdisStatus::Illegal_Blob_Length:             return "Illegal_Blob_Length";
      case dcp::blobdis::BlobdisStatus::Inactive:                        return "Inactive";
      case dcp::blobdis::BlobdisStatus::Blob_Unknown:                    return "Blob_Unknonwn";
      case dcp::blobdis::BlobdisStatus::Blob_Blacklisted:                return "Blob_Blacklisted";
      case dcp::blobdis::BlobdisStatus::Abort_On_Transmit_Blob:          return "Abort_On_Transmit_Blob";
      case dcp::blobdis::BlobdisStatus::Abort_On_Receive_Blob:           return "Abort_On_Receive_Blob";
      case dcp::blobdis::BlobdisStatus::Blob_Already_Aborted:            return "Blob_Already_Aborted";
      case dcp::blobdis::BlobdisStatus::Illegal_Part_Size:               return "Illegal_Part_Size";
      case dcp::blobdis::BlobdisStatus::Illegal_Time_Value:              return "Illegal_Time_Value";
      case dcp::blobdis::BlobdisStatus::Illegal_App_Identifier:          return "Illegal_App_Identifier";
	
	// implementation-dependent status codes
      case dcp::blobdis::BlobdisStatus::Internal_Error:                  return "Internal_Error";
      case dcp::blobdis::BlobdisStatus::Internal_Shared_Memory_Error:    return "Internal_Shared_Memory_Error";
	
      default:
	throw std::invalid_argument(std::format("to_string: illegal Blobdis status code {}", static_cast<uint16_t> (stat)));
      }
    return "";
  }
    




std::ostream& operator<< (std::ostream& os, const dcp::blobdis::BlobdisServiceType st);
std::ostream& operator<< (std::ostream& os, const dcp::blobdis::BlobdisStatus stat);
