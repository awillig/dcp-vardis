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

#include <cstdint>
#include <iostream>

/**
 * @brief This module declares constants for different BP and Vardis
 *        service types and status codes, defines base classes for
 *        service requests, responses and indications, and some helper
 *        functions to provide string representations of the service
 *        type and status constants
 */


namespace dcp {

  /**
   * @brief Data type holding service type identifier
   */
  typedef uint16_t  DcpServiceType;


  /**
   * @brief Base values for BP and Vardis, their specific services
   *        should be in these corresponding ranges
   */
  const DcpServiceType  BaseBPServiceType      = 0x1000;
  const DcpServiceType  BaseVardisServiceType  = 0x2000;


  /**
   * @brief Invalid service type
   */
  const DcpServiceType  InvalidServiceType     = 0xFFFF;

  
  /*******************************************************************
   * BP Services
   ******************************************************************/
 
  /**
   * @brief These are the BP services defined in the specification
   */
  const DcpServiceType  stBPRegisterProtocol             = BaseBPServiceType + 0x0001;
  const DcpServiceType  stBPDeregisterProtocol           = BaseBPServiceType + 0x0002;
  const DcpServiceType  stBPListRegisteredProtocols      = BaseBPServiceType + 0x0003;
  const DcpServiceType  stBPClearBuffer                  = BaseBPServiceType + 0x0004;
  const DcpServiceType  stBPQueryNumberBufferedPayloads  = BaseBPServiceType + 0x0005;
  const DcpServiceType  stBPReceivePayload               = BaseBPServiceType + 0x0006;
  const DcpServiceType  stBPTransmitPayload              = BaseBPServiceType + 0x0007;

  /**
   * @brief These are additional management-oriented services
   */
  const DcpServiceType  stBPShutDown       = BaseBPServiceType + 0x0100;     /*!< shutting down the BP */
  const DcpServiceType  stBPActivate       = BaseBPServiceType + 0x0101;     /*!< activate BP (enable processing of payloads) */
  const DcpServiceType  stBPDeactivate     = BaseBPServiceType + 0x0102;     /*!< deactivate BP (disable processing of payloads) */


  /**
   * @brief Return string representation of the given BP service type
   *
   * Throws when service type is not one of the known BP services
   */
  std::string bp_service_type_to_string (DcpServiceType st);

  
  /*******************************************************************
   * Vardis Services
   ******************************************************************/

  /**
   * @brief These are the Vardis services pre-defined in the specification
   */
  const DcpServiceType  stVardis_RTDB_DescribeDatabase   =  BaseVardisServiceType + 0x0001;
  const DcpServiceType  stVardis_RTDB_DescribeVariable   =  BaseVardisServiceType + 0x0002;
  const DcpServiceType  stVardis_RTDB_Create             =  BaseVardisServiceType + 0x0003;
  const DcpServiceType  stVardis_RTDB_Delete             =  BaseVardisServiceType + 0x0004;
  const DcpServiceType  stVardis_RTDB_Update             =  BaseVardisServiceType + 0x0005;
  const DcpServiceType  stVardis_RTDB_Read               =  BaseVardisServiceType + 0x0006;


  /**
   * @brief These are additional management-oriented services
   */
  const DcpServiceType  stVardis_Register                =  BaseVardisServiceType + 0x0100;
  const DcpServiceType  stVardis_Deregister              =  BaseVardisServiceType + 0x0101;
  const DcpServiceType  stVardis_Shutdown                =  BaseVardisServiceType + 0x0102;
  const DcpServiceType  stVardis_Activate                =  BaseVardisServiceType + 0x0103;
  const DcpServiceType  stVardis_Deactivate              =  BaseVardisServiceType + 0x0104;
  const DcpServiceType  stVardis_GetStatistics           =  BaseVardisServiceType + 0x0105;
  

  /**
   * @brief Return string representation of the given Vardis service type
   *
   * Throws when service type is not one of the known Vardis services
   */
  std::string vardis_service_type_to_string (DcpServiceType st);


  
  /*******************************************************************
   * Status codes
   ******************************************************************/

  /**
   * @brief Data type holding status codes
   */
  typedef uint16_t DcpStatus;


  /**
   * @brief Base values for DCP and Vardis, their specific status
   *        codes should be in these ranges
   */
  const DcpStatus  BaseBPStatus      =  0x1000;
  const DcpStatus  BaseVardisStatus  =  0x2000;
  const DcpStatus  BaseSRPStatus     =  0x3000;

  
  /*******************************************************************
   * BP Status codes
   ******************************************************************/
  

  /**
   * @brief BP status codes described in the specification
   */
  const DcpStatus BP_STATUS_OK                              = BaseBPStatus + 0x0000;
  const DcpStatus BP_STATUS_PROTOCOL_ALREADY_REGISTERED     = BaseBPStatus + 0x0001;
  const DcpStatus BP_STATUS_ILLEGAL_MAX_PAYLOAD_SIZE        = BaseBPStatus + 0x0002;
  const DcpStatus BP_STATUS_UNKNOWN_PROTOCOL                = BaseBPStatus + 0x0003;
  const DcpStatus BP_STATUS_PAYLOAD_TOO_LARGE               = BaseBPStatus + 0x0004;
  const DcpStatus BP_STATUS_EMPTY_PAYLOAD                   = BaseBPStatus + 0x0005;
  const DcpStatus BP_STATUS_ILLEGAL_DROPPING_QUEUE_SIZE     = BaseBPStatus + 0x0006;
  const DcpStatus BP_STATUS_UNKNOWN_QUEUEING_MODE           = BaseBPStatus + 0x0007;
  const DcpStatus BP_STATUS_INACTIVE                        = BaseBPStatus + 0x0008;


  /**
   * @brief These are additional implementation-dependent status values
   */
  const DcpStatus BP_STATUS_INTERNAL_ERROR                  = BaseBPStatus + 0x0100;
  const DcpStatus BP_STATUS_INTERNAL_SHARED_MEMORY_ERROR    = BaseBPStatus + 0x0101;
  const DcpStatus BP_STATUS_ILLEGAL_SERVICE_TYPE            = BaseBPStatus + 0x0102;
  const DcpStatus BP_STATUS_WRONG_PROTOCOL_TYPE             = BaseBPStatus + 0x0103;


  /**
   * @brief Returns string representation of BP status code.
   *
   * Throws when the status code is not one of the known BP status
   * codes.
   */
  std::string bp_status_to_string (DcpStatus stat);


  /*******************************************************************
   * Vardis Status codes
   ******************************************************************/
  

  /**
   * @brief Vardis status codes prescribed by the specification
   */
  const DcpStatus VARDIS_STATUS_OK                              =  BaseVardisStatus + 0x0000;
  const DcpStatus VARDIS_STATUS_VARIABLE_EXISTS                 =  BaseVardisStatus + 0x0001;
  const DcpStatus VARDIS_STATUS_VARIABLE_DESCRIPTION_TOO_LONG   =  BaseVardisStatus + 0x0002;
  const DcpStatus VARDIS_STATUS_VALUE_TOO_LONG                  =  BaseVardisStatus + 0x0003;
  const DcpStatus VARDIS_STATUS_EMPTY_VALUE                     =  BaseVardisStatus + 0x0004;
  const DcpStatus VARDIS_STATUS_ILLEGAL_REPCOUNT                =  BaseVardisStatus + 0x0005;
  const DcpStatus VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST         =  BaseVardisStatus + 0x0006;
  const DcpStatus VARDIS_STATUS_NOT_PRODUCER                    =  BaseVardisStatus + 0x0007;
  const DcpStatus VARDIS_STATUS_VARIABLE_BEING_DELETED          =  BaseVardisStatus + 0x0008;
  const DcpStatus VARDIS_STATUS_INACTIVE                        =  BaseVardisStatus + 0x0009;

  
  /**
   * @brief Additional implementation-dependent status codes
   */
  const DcpStatus VARDIS_STATUS_INTERNAL_ERROR                  =  BaseVardisStatus + 0x0100;
  const DcpStatus VARDIS_STATUS_APPLICATION_ALREADY_REGISTERED  =  BaseVardisStatus + 0x0101;
  const DcpStatus VARDIS_STATUS_INTERNAL_SHARED_MEMORY_ERROR    =  BaseVardisStatus + 0x0102;
  const DcpStatus VARDIS_STATUS_UNKNOWN_APPLICATION             =  BaseVardisStatus + 0x0103;


  /**
   * @brief Returns string representation of Vardis status code.
   *
   * Throws when the status code is not one of the known Vardis status
   * codes.
   */
  std::string vardis_status_to_string (DcpStatus stat);


  /*******************************************************************
   * SRP Status codes
   ******************************************************************/
  

  /**
   * @brief SRP status codes prescribed by the specification
   */
  const DcpStatus SRP_STATUS_OK     =  BaseSRPStatus + 0x0000;

  
  /**
   * @brief Returns string representation of SRP status code.
   *
   * Throws when the status code is not one of the known SRP status
   * codes.
   */
  std::string srp_status_to_string (DcpStatus stat);
  
  
  /*******************************************************************
   * Base classes for service primitives
   ******************************************************************/


  /**
   * @brief Base class for service requests
   */
  typedef struct ServiceRequest {
    DcpServiceType    s_type;
    ServiceRequest (DcpServiceType st) : s_type(st) {};
  } ServiceRequest;


  /**
   * @brief Base class for service confirms
   */
  typedef struct ServiceConfirm {
    DcpServiceType  s_type;
    DcpStatus       status_code;
    ServiceConfirm (DcpServiceType st) : s_type(st) {};
    ServiceConfirm (DcpServiceType st, DcpStatus stat) : s_type(st), status_code(stat) {};
  } ServiceConfirm;


  /**
   * @brief Base class for service indications
   */
  typedef struct ServiceIndication {
    DcpServiceType  s_type;
    ServiceIndication (DcpServiceType st) : s_type(st) {};
  } ServiceIndication;


  
  
  
};  // namespace dcp
