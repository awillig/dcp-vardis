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
#include <dcp/common/global_types_constants.h>
#include <dcp/common/service_primitives.h>
#include <dcp/bp/bp_queueing_mode.h>
#include <dcp/bp/bp_client_static_info.h>
#include <dcp/bp/bp_services_status.h>
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


  /*************************************************************************
   * Service BPRegisterProtocol
   ************************************************************************/

  
  typedef struct BPRegisterProtocol_Request : public BPServiceRequest {
    /**
     * @brief These are the parameters according to the protocol specification
     */
    BPStaticClientInfo  static_info;
    
    /**
     * @brief Additional implementation-dependent parameters
     */
    char      shm_area_name[maxShmAreaNameLength+1];  /*!< name of shared memory block area used to exchange payloads */
    bool      generateTransmitPayloadConfirms = false;  /*!< Specify whether client protocol wants BP demon to send BP-TransmitPayload.confirm primitives */

    
    BPRegisterProtocol_Request () : BPServiceRequest(BPServiceType::RegisterProtocol) {};

    friend std::ostream& operator<<(std::ostream& os, const BPRegisterProtocol_Request& req);
    
  } BPRegisterProtocol_Request;


  // -------------------------------------------------------------
  
  typedef struct BPRegisterProtocol_Confirm : BPServiceConfirm {
    NodeIdentifierT ownNodeIdentifier;
    
    BPRegisterProtocol_Confirm () : BPServiceConfirm(BPServiceType::RegisterProtocol) {};
    BPRegisterProtocol_Confirm (BPStatus scode) : BPServiceConfirm(BPServiceType::RegisterProtocol, scode) {};

    friend std::ostream& operator<<(std::ostream& os, const BPRegisterProtocol_Confirm& conf);
  } BPRegisterProtocol_Confirm;


  /*************************************************************************
   * Service BPDeregisterProtocol
   ************************************************************************/
  
  typedef struct BPDeregisterProtocol_Request : BPServiceRequest {
    BPProtocolIdT     protocolId;

    BPDeregisterProtocol_Request () : BPServiceRequest(BPServiceType::DeregisterProtocol) {};

    friend std::ostream& operator<<(std::ostream& os, const BPDeregisterProtocol_Request& req);
  } BPDeregisterProtocol_Request;


  // -------------------------------------------------------------
  
  typedef struct BPDeregisterProtocol_Confirm : BPServiceConfirm {
    BPDeregisterProtocol_Confirm () : BPServiceConfirm(BPServiceType::DeregisterProtocol) {};
    BPDeregisterProtocol_Confirm (BPStatus scode) : BPServiceConfirm(BPServiceType::DeregisterProtocol, scode) {};

    friend std::ostream& operator<<(std::ostream& os, const BPDeregisterProtocol_Confirm& conf);
  } BPDeregisterProtocol_Confirm;


  /*************************************************************************
   * Service BPListRegisteredProtocols
   ************************************************************************/
  

  typedef struct BPListRegisteredProtocols_Request : BPServiceRequest {
    BPListRegisteredProtocols_Request () : BPServiceRequest(BPServiceType::ListRegisteredProtocols) {};

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

  typedef struct BPListRegisteredProtocols_Confirm : BPServiceConfirm {

    /**
     * @brief Number of BPRegisteredProtocolDataDescription records that follow.
     */
    uint64_t  numberProtocols;
    bool      bpIsActive;
    
    BPListRegisteredProtocols_Confirm () : BPServiceConfirm(BPServiceType::ListRegisteredProtocols) {};
    BPListRegisteredProtocols_Confirm (BPStatus scode) : BPServiceConfirm(BPServiceType::ListRegisteredProtocols, scode) {};

    friend std::ostream& operator<<(std::ostream& os, const BPListRegisteredProtocols_Confirm& conf);
  } BPListRegisteredProtocols_Confirm;


  /*************************************************************************
   * Service BPReceivePayload
   ************************************************************************/
    

  
  typedef struct BPReceivePayload_Indication : BPServiceIndication {
    /**
     * @brief Indicates how many bytes follow immediately after this struct
     */
    BPLengthT   length;
    
    BPReceivePayload_Indication () : BPServiceIndication(BPServiceType::ReceivePayload) {};

    friend std::ostream& operator<<(std::ostream& os, const BPReceivePayload_Indication& ind);
  } BPReceivePayload_Indication;


  
  /*************************************************************************
   * Service BPTransmitPayload
   ************************************************************************/
    
  typedef struct BPTransmitPayload_Request : BPServiceRequest {
    BPProtocolIdT     protocolId;
    BPLengthT         length;

    BPTransmitPayload_Request () : BPServiceRequest (BPServiceType::TransmitPayload) {};

    friend std::ostream& operator<<(std::ostream& os, const BPTransmitPayload_Request& req);
  } BPTransmitPayload_Request;

  // -------------------------------------------------------------
  
  typedef struct BPTransmitPayload_Confirm : BPServiceConfirm {
    BPTransmitPayload_Confirm () : BPServiceConfirm (BPServiceType::TransmitPayload) {};

    friend std::ostream& operator<<(std::ostream& os, const BPTransmitPayload_Confirm& conf);
  } BPTransmitPayload_Confirm;
  

  /*************************************************************************
   * Service ClearBuffer
   ************************************************************************/

  typedef struct BPClearBuffer_Request : BPServiceRequest {
    BPProtocolIdT   protocolId;
    BPClearBuffer_Request () : BPServiceRequest (BPServiceType::ClearBuffer) {};

    friend std::ostream& operator<<(std::ostream& os, const BPClearBuffer_Request& req);
  } BPClearBuffer_Request;

  // -------------------------------------------------------------

  typedef struct BPClearBuffer_Confirm : BPServiceConfirm {
    BPClearBuffer_Confirm () : BPServiceConfirm (BPServiceType::ClearBuffer) {};

    friend std::ostream& operator<<(std::ostream& os, const BPClearBuffer_Confirm& conf);
  } BPClearBuffer_Confirm;


  /*************************************************************************
   * Service QueryNumberBufferedPayloads
   ************************************************************************/

  typedef struct BPQueryNumberBufferedPayloads_Request : BPServiceRequest {
    BPProtocolIdT   protocolId;
    BPQueryNumberBufferedPayloads_Request () : BPServiceRequest (BPServiceType::QueryNumberBufferedPayloads) {};

    friend std::ostream& operator<<(std::ostream& os, const BPQueryNumberBufferedPayloads_Request& req);
  } BPQueryNumberBufferedPayloads_Request;

  // -------------------------------------------------------------

  typedef struct BPQueryNumberBufferedPayloads_Confirm : BPServiceConfirm {
    unsigned long num_payloads_buffered;
    BPQueryNumberBufferedPayloads_Confirm () : BPServiceConfirm (BPServiceType::QueryNumberBufferedPayloads) {};

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
  
  typedef struct BPShutDown_Request : BPServiceRequest {
    BPShutDown_Request () : BPServiceRequest(BPServiceType::ShutDown) {};

    friend std::ostream& operator<<(std::ostream& os, const BPShutDown_Request& req);
  } BPShutDown_Request;

  
  /*************************************************************************
   * Service Activate
   ************************************************************************/

  typedef struct BPActivate_Request : BPServiceRequest {
    BPActivate_Request () : BPServiceRequest(BPServiceType::Activate) {};

    friend std::ostream& operator<<(std::ostream& os, const BPActivate_Request& req);
  } BPActivate_Request;

  // -------------------------------------------------------------
  
  typedef struct BPActivate_Confirm : BPServiceConfirm {
    BPActivate_Confirm () : BPServiceConfirm(BPServiceType::Activate) {};
    BPActivate_Confirm (BPStatus scode) : BPServiceConfirm(BPServiceType::Activate, scode) {};

    friend std::ostream& operator<<(std::ostream& os, const BPActivate_Confirm& conf);
  } BPActivate_Confirm;


  /*************************************************************************
   * Service Deactivate
   ************************************************************************/
    
  typedef struct BPDeactivate_Request : BPServiceRequest {
    BPDeactivate_Request () : BPServiceRequest(BPServiceType::Deactivate) {};

    friend std::ostream& operator<<(std::ostream& os, const BPDeactivate_Request& req);
  } BPDeactivate_Request;
  
  // -------------------------------------------------------------
  
  typedef struct BPDeactivate_Confirm : BPServiceConfirm {
    BPDeactivate_Confirm () : BPServiceConfirm(BPServiceType::Deactivate) {};
    BPDeactivate_Confirm (BPStatus scode) : BPServiceConfirm(BPServiceType::Deactivate, scode) {};

    friend std::ostream& operator<<(std::ostream& os, const BPDeactivate_Confirm& conf);
  } BPDeactivate_Confirm;

  /*************************************************************************
   * Service BP Statistics
   ************************************************************************/
    
  typedef struct BPGetStatistics_Request : BPServiceRequest {
    BPGetStatistics_Request () : BPServiceRequest(BPServiceType::GetStatistics) {};

    friend std::ostream& operator<<(std::ostream& os, const BPGetStatistics_Request& req);
  } BPGetStatistics_Request;
  
  // -------------------------------------------------------------
  
  typedef struct BPGetStatistics_Confirm : BPServiceConfirm {
    double        avg_inter_beacon_time;
    double        avg_beacon_size;
    unsigned int  number_received_beacons;
    BPGetStatistics_Confirm () : BPServiceConfirm(BPServiceType::GetStatistics) {};
    BPGetStatistics_Confirm (BPStatus scode) : BPServiceConfirm(BPServiceType::GetStatistics, scode) {};

    friend std::ostream& operator<<(std::ostream& os, const BPGetStatistics_Confirm& conf);
  } BPGetStatistics_Confirm;


  

};  // namespace dcp::bp
