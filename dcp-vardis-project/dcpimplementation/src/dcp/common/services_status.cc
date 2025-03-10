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



#include <dcp/common/services_status.h>


namespace dcp {

  std::string bp_service_type_to_string (DcpServiceType st)
  {
    switch(st)
      {
	// services given in the specification
      case  stBP_RegisterProtocol             :  return "stBP_RegisterProtocol";
      case  stBP_DeregisterProtocol           :  return "stBP_DeregisterProtocol";
      case  stBP_ListRegisteredProtocols      :  return "stBP_ListRegisteredProtocols";
      case  stBP_ClearBuffer                  :  return "stBP_ClearBuffer";
      case  stBP_QueryNumberBufferedPayloads  :  return "stBP_QueryNumberBufferedPayloads";

	// implementation-dependent services
      case  stBP_ShutDown                     :  return "stBP_ShutDown";
      case  stBP_Activate                     :  return "stBP_Activate";
      case  stBP_Deactivate                   :  return "stBP_Deactivate";
      case  stBP_GetStatistics                :  return "stBP_GetStatistics";
	
      default:
	throw std::invalid_argument("bp_service_type_to_string: illegal service type");
      }
    return "";
  }

  // -----------------------------------------

  std::string vardis_service_type_to_string (DcpServiceType st)
  {
    switch(st)
      {
	// services given in the specification
	
      case  stVardis_RTDB_DescribeDatabase:  return "stRTDB_DescribeDatabase";
      case  stVardis_RTDB_DescribeVariable:  return "stRTDB_DescribeVariable";
      case  stVardis_RTDB_Create:            return "stRTDB_Create";
      case  stVardis_RTDB_Delete:            return "stRTDB_Delete";
      case  stVardis_RTDB_Update:            return "stRTDB_Update";
      case  stVardis_RTDB_Read:              return "stRTDB_Read";

      case  stVardis_Register:               return "stVardis_Register";
      case  stVardis_Deregister:             return "stVardis_Deregister";

      case  stVardis_Shutdown:               return "stVardis_Shutdown";
      case  stVardis_Activate:               return "stVardis_Activate";
      case  stVardis_Deactivate:             return "stVardis_Deactivate";
      case  stVardis_GetStatistics:          return "stVardis_GetStatistics";
	
      // implementation-dependent services

	
      default:
	throw std::invalid_argument("vardis_service_type_to_string: illegal service type");
      }
    return "";
  }

  // -----------------------------------------

  std::string vardis_status_to_string (DcpStatus stat)
  {
    switch (stat)
      {
	// status codes prescribed by the specification
      case VARDIS_STATUS_OK:                              return "VARDIS_STATUS_OK";
      case VARDIS_STATUS_VARIABLE_EXISTS:                 return "VARDIS_STATUS_VARIABLE_EXISTS";
      case VARDIS_STATUS_VARIABLE_DESCRIPTION_TOO_LONG:   return "VARDIS_STATUS_VARIABLE_DESCRIPTION_TOO_LONG";
      case VARDIS_STATUS_VALUE_TOO_LONG:                  return "VARDIS_STATUS_VALUE_TOO_LONG";
      case VARDIS_STATUS_EMPTY_VALUE:                     return "VARDIS_STATUS_EMPTY_VALUE";
      case VARDIS_STATUS_ILLEGAL_REPCOUNT:                return "VARDIS_STATUS_ILLEGAL_REPCOUNT";
      case VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST:         return "VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST";
      case VARDIS_STATUS_NOT_PRODUCER:                    return "VARDIS_STATUS_NOT_PRODUCER";
      case VARDIS_STATUS_VARIABLE_BEING_DELETED:          return "VARDIS_STATUS_VARIABLE_BEING_DELETED";
      case VARDIS_STATUS_INACTIVE:                        return "VARDIS_STATUS_INACTIVE";

	// implementation-dependent status codes
      case VARDIS_STATUS_INTERNAL_ERROR:                  return "VARDIS_STATUS_INTERNAL_ERROR";
      case VARDIS_STATUS_APPLICATION_ALREADY_REGISTERED:  return "VARDIS_STATUS_APPLICATION_ALREAD_REGISTERED";
      case VARDIS_STATUS_INTERNAL_SHARED_MEMORY_ERROR:    return "VARDIS_STATUS_INTERNAL_SHARED_MEMORY_ERROR";
      case VARDIS_STATUS_UNKNOWN_APPLICATION:             return "VARDIS_STATUS_UNKNOWN_APPLICATION";
	
      default:
	throw std::invalid_argument("vardis_status_to_string: illegal status code");
      }
    return "";
  }

  // -----------------------------------------
  
 std::string bp_status_to_string (DcpStatus stat)
  {
    switch (stat)
      {
	// status codes prescribed by the specification
      case BP_STATUS_OK:                             return "BP_STATUS_OK";
      case BP_STATUS_PROTOCOL_ALREADY_REGISTERED:    return "BP_STATUS_PROTOCOL_ALREADY_REGISTERED";
      case BP_STATUS_ILLEGAL_MAX_PAYLOAD_SIZE:       return "BP_STATUS_ILLEGAL_MAX_PAYLOAD_SIZE";
      case BP_STATUS_UNKNOWN_PROTOCOL:               return "BP_STATUS_UNKNOWN_PROTOCOL";
      case BP_STATUS_PAYLOAD_TOO_LARGE:              return "BP_STATUS_PAYLOAD_TOO_LARGE";
      case BP_STATUS_EMPTY_PAYLOAD:                  return "BP_STATUS_EMPTY_PAYLOAD";
      case BP_STATUS_ILLEGAL_DROPPING_QUEUE_SIZE:    return "BP_STATUS_ILLEGAL_DROPPING_QUEUE_SIZE";
      case BP_STATUS_UNKNOWN_QUEUEING_MODE:          return "BP_STATUS_UNKNOWN_QUEUEING_MODE";
      case BP_STATUS_INACTIVE:                       return "BP_STATUS_INACTIVE";

	// implementation-dependent status codes
      case BP_STATUS_INTERNAL_ERROR:                 return "BP_STATUS_INTERNAL_ERROR";
      case BP_STATUS_INTERNAL_SHARED_MEMORY_ERROR:   return "BP_STATUS_INTERNAL_SHARED_MEMORY_ERROR";
      case BP_STATUS_ILLEGAL_SERVICE_TYPE:           return "BP_STATUS_ILLEGAL_SERVICE_TYPE";

      case BP_STATUS_WRONG_PROTOCOL_TYPE:            return "BP_STATUS_WRONG_PROTOCOL_TYPE";
	
      default:
	throw std::invalid_argument("bp_status_to_string: illegal status code");
      }
    return "";
  }

    // -----------------------------------------

  std::string srp_status_to_string (DcpStatus stat)
  {
    switch (stat)
      {
	// status codes prescribed by the specification
      case SRP_STATUS_OK:                              return "SRP_STATUS_OK";
	
      default:
	throw std::invalid_argument("srp_status_to_string: illegal status code");
      }
    return "";
  }
  
};  // namespace dcp
