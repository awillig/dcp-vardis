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

#include <iostream>
#include <cstdint>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/services_status.h>
#include <dcp/common/shared_mem_area.h>
#include <dcp/bp/bp_queueing_mode.h>
#include <dcp/bp/bp_transmissible_types.h>


/**
 * @brief This module defines the structure of BP services, both the
 *        ones defined in the specification and additional ones.
 *
 * All defined structures here must have a fixed length (so that
 * sizeof() gives sensible results). However, for some of these
 * structures additional data follows, that then must be read
 * separately.
 */


namespace dcp::bp {

  /*************************************************************************
   * Base classes and constants for primitives
   ************************************************************************/

  /**
   * @brief Maximum length of protocol name string buffer (including zero byte)
   */
  const size_t maximumProtocolNameLength = 128;


  /*************************************************************************
   * Service BPRegisterProtocol
   ************************************************************************/
  
  
  typedef struct BPRegisterProtocol_Request : ServiceRequest {
    /**
     * @brief These are the parameters according to the protocol specification
     */
    BPProtocolIdT     protocolId = 0;                    /*!< Client protocol identifier */
    char              name[maximumProtocolNameLength];   /*!< Client protocol name (zero-terminated string) */
    BPLengthT         maxPayloadSize =0;                 /*!< Maximum payload size of client protocol */
    BPQueueingMode    queueingMode;                      /*!< Queueing mode to be used */
    uint16_t          maxEntries = 0;                    /*!< Maximum number of entries in payload queue */
    bool              allowMultiplePayloads = false;     /*!< Can multiple payloads be included in a beacon */
    
    
    /**
     * @brief Additional implementation-dependent parameters
     */
    char      shm_area_name[maxShmAreaNameLength+1];  /*!< name of shared memory block area used to exchange payloads */
    bool      generateTransmitPayloadConfirms = false;  /*!< Specify whether client protocol wants BP demon to send BP-TransmitPayload.confirm primitives */

    
    BPRegisterProtocol_Request () : ServiceRequest(stBP_RegisterProtocol) {};

    friend std::ostream& operator<<(std::ostream& os, const BPRegisterProtocol_Request& req);
    
  } BPRegisterProtocol_Request;


  // -------------------------------------------------------------
  
  typedef struct BPRegisterProtocol_Confirm : ServiceConfirm {
    NodeIdentifierT ownNodeIdentifier;
    
    BPRegisterProtocol_Confirm () : ServiceConfirm(stBP_RegisterProtocol) {};
    BPRegisterProtocol_Confirm (DcpStatus scode) : ServiceConfirm(stBP_RegisterProtocol, scode) {};

    friend std::ostream& operator<<(std::ostream& os, const BPRegisterProtocol_Confirm& conf);
  } BPRegisterProtocol_Confirm;


  /*************************************************************************
   * Service BPDeregisterProtocol
   ************************************************************************/
  
  typedef struct BPDeregisterProtocol_Request : ServiceRequest {
    BPProtocolIdT     protocolId;

    BPDeregisterProtocol_Request () : ServiceRequest(stBP_DeregisterProtocol) {};

    friend std::ostream& operator<<(std::ostream& os, const BPDeregisterProtocol_Request& req);
  } BPDeregisterProtocol_Request;


  // -------------------------------------------------------------
  
  typedef struct BPDeregisterProtocol_Confirm : ServiceConfirm {
    BPDeregisterProtocol_Confirm () : ServiceConfirm(stBP_DeregisterProtocol) {};
    BPDeregisterProtocol_Confirm (DcpStatus scode) : ServiceConfirm(stBP_DeregisterProtocol, scode) {};

    friend std::ostream& operator<<(std::ostream& os, const BPDeregisterProtocol_Confirm& conf);
  } BPDeregisterProtocol_Confirm;


  /*************************************************************************
   * Service BPListRegisteredProtocols
   ************************************************************************/
  

  typedef struct BPListRegisteredProtocols_Request : ServiceRequest {
    BPListRegisteredProtocols_Request () : ServiceRequest(stBP_ListRegisteredProtocols) {};

    friend std::ostream& operator<<(std::ostream& os, const BPListRegisteredProtocols_Request& req);
  } BPListRegisteredProtocols_Request;

  // -------------------------------------------------------------

  typedef struct BPRegisteredProtocolDataDescription {

    // fields foreseen by the specification
    BPProtocolIdT     protocolId;
    char              protocolName[maximumProtocolNameLength];
    BPLengthT         maxPayloadSize;
    BPQueueingMode    queueingMode;
    TimeStampT        timeStampRegistration;
    uint16_t          maxEntries;
    bool              allowMultiplePayloads;

    // statistics
    unsigned int cntOutgoingPayloads;
    unsigned int cntReceivedPayloads;
    unsigned int cntDroppedOutgoingPayloads;
    unsigned int cntDroppedIncomingPayloads;


    friend std::ostream& operator<<(std::ostream& os, const BPRegisteredProtocolDataDescription& descr);
  } BPRegisteredProtocolDataDescription;

  // -------------------------------------------------------------

  typedef struct BPListRegisteredProtocols_Confirm : ServiceConfirm {

    /**
     * @brief Number of BPRegisteredProtocolDataDescription records that follow.
     */
    uint64_t  numberProtocols;
    bool      bpIsActive;
    
    BPListRegisteredProtocols_Confirm () : ServiceConfirm(stBP_ListRegisteredProtocols) {};
    BPListRegisteredProtocols_Confirm (DcpStatus scode) : ServiceConfirm(stBP_ListRegisteredProtocols, scode) {};

    friend std::ostream& operator<<(std::ostream& os, const BPListRegisteredProtocols_Confirm& conf);
  } BPListRegisteredProtocols_Confirm;


  /*************************************************************************
   * Service BPReceivePayload
   ************************************************************************/
    

  
  typedef struct BPReceivePayload_Indication : ServiceIndication {
    /**
     * @brief Indicates how many bytes follow immediately after this struct
     */
    BPLengthT   length;
    
    BPReceivePayload_Indication () : ServiceIndication(stBP_ReceivePayload) {};

    friend std::ostream& operator<<(std::ostream& os, const BPReceivePayload_Indication& ind);
  } BPReceivePayload_Indication;


  
  /*************************************************************************
   * Service BPTransmitPayload
   ************************************************************************/
    
  typedef struct BPTransmitPayload_Request : ServiceRequest {
    BPProtocolIdT     protocolId;
    BPLengthT         length;

    BPTransmitPayload_Request () : ServiceRequest (stBP_TransmitPayload) {};

    friend std::ostream& operator<<(std::ostream& os, const BPTransmitPayload_Request& req);
  } BPTransmitPayload_Request;

  // -------------------------------------------------------------
  
  typedef struct BPTransmitPayload_Confirm : ServiceConfirm {
    BPTransmitPayload_Confirm () : ServiceConfirm (stBP_TransmitPayload) {};

    friend std::ostream& operator<<(std::ostream& os, const BPTransmitPayload_Confirm& conf);
  } BPTransmitPayload_Confirm;
  

  /*************************************************************************
   * Service ClearBuffer
   ************************************************************************/

  typedef struct BPClearBuffer_Request : ServiceRequest {
    BPProtocolIdT   protocolId;
    BPClearBuffer_Request () : ServiceRequest (stBP_ClearBuffer) {};

    friend std::ostream& operator<<(std::ostream& os, const BPClearBuffer_Request& req);
  } BPClearBuffer_Request;

  // -------------------------------------------------------------

  typedef struct BPClearBuffer_Confirm : ServiceConfirm {
    BPClearBuffer_Confirm () : ServiceConfirm (stBP_ClearBuffer) {};

    friend std::ostream& operator<<(std::ostream& os, const BPClearBuffer_Confirm& conf);
  } BPClearBuffer_Confirm;


  /*************************************************************************
   * Service QueryNumberBufferedPayloads
   ************************************************************************/

  typedef struct BPQueryNumberBufferedPayloads_Request : ServiceRequest {
    BPProtocolIdT   protocolId;
    BPQueryNumberBufferedPayloads_Request () : ServiceRequest (stBP_QueryNumberBufferedPayloads) {};

    friend std::ostream& operator<<(std::ostream& os, const BPQueryNumberBufferedPayloads_Request& req);
  } BPQueryNumberBufferedPayloads_Request;

  // -------------------------------------------------------------

  typedef struct BPQueryNumberBufferedPayloads_Confirm : ServiceConfirm {
    unsigned long num_payloads_buffered;
    BPQueryNumberBufferedPayloads_Confirm () : ServiceConfirm (stBP_QueryNumberBufferedPayloads) {};

    friend std::ostream& operator<<(std::ostream& os, const BPQueryNumberBufferedPayloads_Confirm& conf);
  } BPQueryNumberBufferedPayloads_Confirm;
  
  
  /*************************************************************************
   * Service ShutDown
   ************************************************************************/

  /**
   * @brief Service request for shutting down BP
   *
   * Note: there is no confirm primitive, as there is no guarantee
   * that the command socket will client protocol will still exist
   * when the client attempts to read a response from it
   */
  
  typedef struct BPShutDown_Request : ServiceRequest {
    BPShutDown_Request () : ServiceRequest(stBP_ShutDown) {};

    friend std::ostream& operator<<(std::ostream& os, const BPShutDown_Request& req);
  } BPShutDown_Request;

  
  /*************************************************************************
   * Service Activate
   ************************************************************************/

  typedef struct BPActivate_Request : ServiceRequest {
    BPActivate_Request () : ServiceRequest(stBP_Activate) {};

    friend std::ostream& operator<<(std::ostream& os, const BPActivate_Request& req);
  } BPActivate_Request;

  // -------------------------------------------------------------
  
  typedef struct BPActivate_Confirm : ServiceConfirm {
    BPActivate_Confirm () : ServiceConfirm(stBP_Activate) {};
    BPActivate_Confirm (DcpStatus scode) : ServiceConfirm(stBP_Activate, scode) {};

    friend std::ostream& operator<<(std::ostream& os, const BPActivate_Confirm& conf);
  } BPActivate_Confirm;


  /*************************************************************************
   * Service Deactivate
   ************************************************************************/
    
  typedef struct BPDeactivate_Request : ServiceRequest {
    BPDeactivate_Request () : ServiceRequest(stBP_Deactivate) {};

    friend std::ostream& operator<<(std::ostream& os, const BPDeactivate_Request& req);
  } BPDeactivate_Request;
  
  // -------------------------------------------------------------
  
  typedef struct BPDeactivate_Confirm : ServiceConfirm {
    BPDeactivate_Confirm () : ServiceConfirm(stBP_Deactivate) {};
    BPDeactivate_Confirm (DcpStatus scode) : ServiceConfirm(stBP_Deactivate, scode) {};

    friend std::ostream& operator<<(std::ostream& os, const BPDeactivate_Confirm& conf);
  } BPDeactivate_Confirm;

  /*************************************************************************
   * Service BP Statistics
   ************************************************************************/
    
  typedef struct BPGetStatistics_Request : ServiceRequest {
    BPGetStatistics_Request () : ServiceRequest(stBP_GetStatistics) {};

    friend std::ostream& operator<<(std::ostream& os, const BPGetStatistics_Request& req);
  } BPGetStatistics_Request;
  
  // -------------------------------------------------------------
  
  typedef struct BPGetStatistics_Confirm : ServiceConfirm {
    double        avg_inter_beacon_time;
    double        avg_beacon_size;
    unsigned int  number_received_beacons;
    BPGetStatistics_Confirm () : ServiceConfirm(stBP_GetStatistics) {};
    BPGetStatistics_Confirm (DcpStatus scode) : ServiceConfirm(stBP_GetStatistics, scode) {};

    friend std::ostream& operator<<(std::ostream& os, const BPGetStatistics_Confirm& conf);
  } BPGetStatistics_Confirm;


  

};  // namespace dcp::bp
