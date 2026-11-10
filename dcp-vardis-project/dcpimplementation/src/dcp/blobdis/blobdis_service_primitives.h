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
#include <dcp/blobdis/blobdis_status.h>
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
    
    TxTransmit,
    TxCompletion,
    TxRelease,
    TxAbort,
    
    MgmtBlacklist,
    MgmtActiveBlobs,
    MgmtMetadataStatus,  
    MgmtActivate,
    MgmtDeactivate,
    
    ShutDown,
    GetStatistics
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
  case dcp::blobdis::BlobdisServiceType::AppDeregister:       return "BD-AppDeregister";
  case dcp::blobdis::BlobdisServiceType::AppListRegistered:   return "BD-AppListRegistered";
  case dcp::blobdis::BlobdisServiceType::AppDescribe:         return "BD-AppDescribe";
  case dcp::blobdis::BlobdisServiceType::RxNewBlob:           return "BD-RxNewBlob";
  case dcp::blobdis::BlobdisServiceType::RxCompletion:        return "BD-RxCompletion";
  case dcp::blobdis::BlobdisServiceType::RxRelease:           return "BD-RxRelease";
  case dcp::blobdis::BlobdisServiceType::RxAbort:             return "BD-RxAbort";
  case dcp::blobdis::BlobdisServiceType::TxTransmit:          return "BD-TxTransmit";
  case dcp::blobdis::BlobdisServiceType::TxCompletion:        return "BD-TxCompletion";
  case dcp::blobdis::BlobdisServiceType::TxRelease:           return "BD-TxRelease";
  case dcp::blobdis::BlobdisServiceType::TxAbort:             return "BD-TxAbort";
  case dcp::blobdis::BlobdisServiceType::MgmtBlacklist:       return "BD-MgmtBlacklist";
  case dcp::blobdis::BlobdisServiceType::MgmtActiveBlobs:     return "BD-MgmtActiveBlobs";
  case dcp::blobdis::BlobdisServiceType::MgmtMetadataStatus:  return "BD-MgmtMetadataStatus";
  case dcp::blobdis::BlobdisServiceType::MgmtActivate:        return "BD-MgmtActivate";
  case dcp::blobdis::BlobdisServiceType::MgmtDeactivate:      return "BD-MgmtDeactivate";
  case dcp::blobdis::BlobdisServiceType::ShutDown:            return "ShutDown";
  case dcp::blobdis::BlobdisServiceType::GetStatistics:       return "GetStatistics"; 
  default:
    throw std::invalid_argument("to_string: unknown Blobdis service type");
  }
  return "";
}



std::ostream& operator<< (std::ostream& os, const dcp::blobdis::BlobdisServiceType st);
