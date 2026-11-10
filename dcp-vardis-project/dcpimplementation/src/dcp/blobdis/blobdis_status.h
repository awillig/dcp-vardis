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

/**
 * @brief This module provides definitions of the Blobdis status names,
 *        and helper functions to convert a status name to a string
 */


namespace dcp::blobdis {


  /**
   * @brief Blobdis status names
   */


  enum class BlobdisStatus : std::uint16_t {
    Ok,
    Application_Unknown,
    Application_Already_Registered,
    Zero_Blob_Length,
    Illegal_Blob_Status,
    Invalid_Blob_Metadata,
    Inactive,
    Blob_Unknown,
    Blob_Blacklisted,
    Abort_On_Transmit_Blob,
    Abort_On_Receive_Blob,
    Blob_Already_Aborted,
    Illegal_Part_Size,
    Illegal_Chunk_Size,
    Illegal_Time_Value,
    Illegal_App_Identifier,

    Internal_Error,
    Internal_Shared_Memory_Error
  };

  
};  // namespace dcp::blobdis



/**
 * @brief Converts Blobdis status name to string
 *
 * Throws for unknown status name
 */

constexpr std::string_view to_string(dcp::blobdis::BlobdisStatus stat) {
    switch (stat)
      {
	// status codes prescribed by the specification
      case dcp::blobdis::BlobdisStatus::Ok:                              return "BLOBDIS-STATUS-OK";
      case dcp::blobdis::BlobdisStatus::Application_Unknown:             return "BLOBDIS-STATUS-APPLICATION_UNKNOWN";
      case dcp::blobdis::BlobdisStatus::Application_Already_Registered:  return "BLOBDIS-STATUS-APPLICATION-ALREADY-REGISTERED";
      case dcp::blobdis::BlobdisStatus::Zero_Blob_Length:                return "BLOBDIS-STATUS-ZERO-BLOB-LENGTH";
      case dcp::blobdis::BlobdisStatus::Illegal_Blob_Status:             return "BLOBDIS-STATUS-ILLEGAL-BLOB-STATUS";
      case dcp::blobdis::BlobdisStatus::Invalid_Blob_Metadata:           return "BLOBDIS-STATUS-INVALID-BLOB-METADATA";
      case dcp::blobdis::BlobdisStatus::Inactive:                        return "BLOBDIS-STATUS-INACTIVE";
      case dcp::blobdis::BlobdisStatus::Blob_Unknown:                    return "BLOBDIS-STATUS-BLOB-UNKNOWN";
      case dcp::blobdis::BlobdisStatus::Blob_Blacklisted:                return "BLOBDIS-STATUS-BLOB-BLACKLISTED";
      case dcp::blobdis::BlobdisStatus::Abort_On_Transmit_Blob:          return "BLOBDIS-STATUS-ABORT-ON-TRANSMIT-BLOB";
      case dcp::blobdis::BlobdisStatus::Abort_On_Receive_Blob:           return "BLOBDIS-STATUS-ABORT-ON-RECEIVE-BLOB";
      case dcp::blobdis::BlobdisStatus::Blob_Already_Aborted:            return "BLOBDIS-STATUS-BLOB-ALREADY-ABORTED";
      case dcp::blobdis::BlobdisStatus::Illegal_Part_Size:               return "BLOBDIS-STATUS-ILLEGAL-PART-SIZE";
      case dcp::blobdis::BlobdisStatus::Illegal_Chunk_Size:              return "BLOBDIS-STATUS-ILLEGAL-CHUNK-SIZE";
      case dcp::blobdis::BlobdisStatus::Illegal_Time_Value:              return "BLOBDIS-STATUS-ILLEGAL-TIME-VALUE";
      case dcp::blobdis::BlobdisStatus::Illegal_App_Identifier:          return "BLOBDIS-STATUS-ILLEGAL-APP-IDENTIFIER";
	
	// implementation-dependent status codes
      case dcp::blobdis::BlobdisStatus::Internal_Error:                  return "BLOBDIS-STATUS-INTERNAL-ERROR";
      case dcp::blobdis::BlobdisStatus::Internal_Shared_Memory_Error:    return "BLOBDIS-STATUS-INTERNAL-SHARED-MEMORY-ERROR";
	
      default:
	throw std::invalid_argument(std::format("to_string: illegal Blobdis status code {}", static_cast<uint16_t> (stat)));
      }
    return "";
  }
    

std::ostream& operator<< (std::ostream& os, const dcp::blobdis::BlobdisStatus stat);
