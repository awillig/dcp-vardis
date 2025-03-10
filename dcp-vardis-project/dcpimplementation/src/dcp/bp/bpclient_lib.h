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
#include <memory>
#include <list>
#include <sys/time.h>
#include <dcp/common/command_socket.h>
#include <dcp/common/foundation_types.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/services_status.h>
#include <dcp/common/shared_mem_area.h>
#include <dcp/bp/bp_queueing_mode.h>
#include <dcp/bp/bp_service_primitives.h>
#include <dcp/bp/bp_shm_control_segment.h>
#include <dcp/bp/bp_transmissible_types.h>
#include <dcp/bp/bpclient_configuration.h>




using dcp::DcpStatus;
using dcp::BP_STATUS_OK;
using dcp::bp_status_to_string;
using dcp::ShmBufferPool;
using dcp::ShmException;
using dcp::bp::BPQueueingMode;
using dcp::bp::BPLengthT;
using dcp::bp::BPRegisteredProtocolDataDescription;
using dcp::bp::BPShmControlSegment;


/**
 * @brief This module collects the data and operations that a BP
 *        client protocol needs, except operations for payload
 *        transfer
 */


namespace dcp {

  /**
   * @brief Type collecting all runtime data and operations needed by
   *        a BP client protocol
   */
  class BPClientRuntime : public BaseClientRuntime {
  protected:

    BPProtocolIdT   protId;                              /*!< BP protocol identifier of client protocol */
    std::string     protName;                            /*!< BP Protocol name */
    std::string     shmAreaName;                         /*!< Name of shared memory area between BP and client protocol */
    BPLengthT       maxPayloadSize;                      /*!< Maximum payload size of BP client protocol, only valid after call to registration */
    BPQueueingMode  queueingMode;                        /*!< BP queueing mode for client protocol */
    uint16_t        maxEntries;                          /*!< Maximum number of entries in BP queue */
    bool            allowMultiplePayloads;               /*!< Can multiple payloads for this client protocol be included in one beacon */
    bool            generateTransmitPayloadConfirms;     /*!< Does the client protocol expect confirms for BP-TransmitPayload.request primitives? */
    
    /**
     * @brief DCP Node identifier of this node / station.
     *
     * Valid after successful call to register_with_bp()
     */
    NodeIdentifierT ownNodeIdentifier;


    /**
     * @brief The BPClientConfiguration structure
     */
    BPClientConfiguration client_configuration;


    /**
     * @brief Register BP client protocol with BP (service 'BP-RegisterProtocol')
     *
     * @param queueingMode: the BP queueing mode to use for this
     *        client protocol
     * @param maxEntries: max entries in the buffering queue
     * @param allowMultiplePayloads: can BP include multiple payloads
     *        of this client protocol in the same beacon
     * @param generateTransmitPayloadConfirms: specify whether client
     *        protocol expects BP demon to generate
     *        BP-TransmitPayload.confirm primitives
     *
     * After successful registration, BP can process payloads related
       to this client protocol if it is active.
     */
    DcpStatus register_with_bp (const BPQueueingMode queueingMode,
				const uint16_t maxEntries,
				const bool allowMultiplePayloads,
				const bool generateTransmitPayloadConfirms
				);


    /**
     * @brief Deregister BP client protocol
     *
     * After successful deregistration, payloads for this client
     * protocol are not processed anymore. Note that the shared memory
     * area remains in place after de-registration, until the lifetime
     * of this object ends
     */
    DcpStatus deregister_with_bp ();


    /**
     * @brief Checks protocol and shared memory area names, throws if
     *        invalid
     */
    void check_names (std::string protName, std::string shmAreaName)
    {
      if (protName.empty())
	throw BPClientLibException ("BPClientRuntime: no protocol name given");
      
      if (protName.capacity() > dcp::bp::maximumProtocolNameLength - 1)
	throw BPClientLibException ("BPClientRuntime: protocol name is too long");

      if (shmAreaName.empty())
	throw BPClientLibException ("BPClientRuntime: no shared memory area name given");

      if (shmAreaName.capacity() > maxShmAreaNameLength - 1)
	throw BPClientLibException ("BPClientRuntime: shared memory area name is too long");
    };


    /**
     *
     */
    template <class RT, class CT>
    DcpStatus simple_bp_request_confirm_service (const std::string& methname, CT& conf)
      {
	ScopedClientSocket cl_sock (commandSock);
	RT req;
	req.protocolId = protId;
    
	byte buffer [command_sock_buffer_size];
	int nrcvd = cl_sock.sendRequestAndReadResponseBlock<RT> (req, buffer, command_sock_buffer_size);
    
	if (nrcvd != sizeof(CT)) cl_sock.abort (methname + ": response has wrong size");
    
	CT *pConf = (CT*) buffer;
	conf = *pConf;
    
	if (pConf->s_type != req.s_type)
	  cl_sock.abort (methname + ": response has wrong service type");

	return pConf->status_code;
      }


    
  public:

    /**
     * @brief Pointer to the shared memory area descriptor
     *
     * The shared memory area is constructed in the constructor and
     * used for payload exchange between client protocol and BP
     */
    std::shared_ptr<ShmBufferPool>  bp_shm_area_ptr;
    
    

    

    BPClientRuntime () = delete;


    /**
     * @brief Constructor, which includes registration of a client protocol
     *
     * @param pid: BP Protocol identifier of client protocol
     * @param protocol_name: textual name of BP client protocol
     * @param max_payload_size: maximum size of a payload generated by
     *        BP client protocol, BP instance checks every payload
     *        against this size
     * @param qMode: queueing mode for client protocol
     * @param max_entries: number of entries in the BP buffer queue
     *        for this client protocol
     * @param allow_multiple_payloads: whether or not multiple
     *        payloads can be included in one beacon for this client
     *        protocol
     * @param gen_pld_conf: specify whether client protocol expects BP
     *        demon to generate BP-TransmitPayload.confirm primitives
     * @param client_conf: configuration data for BP client protocol
     */
    BPClientRuntime (const BPProtocolIdT pid,
		     const std::string protocol_name,
		     const BPLengthT max_payload_size,
		     const BPQueueingMode qMode,
		     const uint16_t max_entries,
		     const bool allow_multiple_payloads,
		     const bool gen_pld_conf,
		     const BPClientConfiguration& client_conf)
    : BaseClientRuntime (client_conf.bp_cmdsock_conf.commandSocketFile.c_str(), client_conf.bp_cmdsock_conf.commandSocketTimeoutMS)
    , protId(pid)
    , protName (protocol_name)
    , shmAreaName (client_conf.bp_shm_conf.shmAreaName)
    , maxPayloadSize (max_payload_size)
    , queueingMode (qMode)
    , maxEntries (max_entries)
    , allowMultiplePayloads (allow_multiple_payloads)
    , generateTransmitPayloadConfirms (gen_pld_conf)
    , client_configuration (client_conf)
    {
      check_names (protName, shmAreaName);

      DcpStatus reg_status = register_with_bp(queueingMode, maxEntries, allowMultiplePayloads, generateTransmitPayloadConfirms);
      if (reg_status != BP_STATUS_OK)
	throw BPClientLibException (std::format("BPClientRuntime: registration failed, status code = {}", reg_status));
    };


    /**
     * @brief Constructor without registration
     *
     * @param pid: BP Protocol identifier of client protocol
     * @param protocol_name: textual name of BP client protocol
     * @param max_payload_size: maximum size of a payload generated by
     *        BP client protocol, BP instance checks every payload
     *        against this size
     * @param client_conf: configuration data for BP client protocol
     */
    BPClientRuntime (const BPProtocolIdT pid,
		     const std::string protocol_name,
		     const BPLengthT max_payload_size,
		     const BPClientConfiguration& client_conf)
    : BaseClientRuntime (client_conf.bp_cmdsock_conf.commandSocketFile.c_str(), client_conf.bp_cmdsock_conf.commandSocketTimeoutMS)
    , protId(pid)
    , protName (protocol_name)
    , shmAreaName (client_conf.bp_shm_conf.shmAreaName)
    , maxPayloadSize (max_payload_size)
    , client_configuration (client_conf)
    {
      check_names (protName, shmAreaName);
    };

    
    
    ~BPClientRuntime ()
    {
      if (_isRegistered)
	deregister_with_bp();
    };


    /********************************************************************************
     * Getters
     *******************************************************************************/
    
    /**
     * @brief Returns own protocol id
     */
    inline BPProtocolIdT    get_protocol_id () const { return protId; };

    
    /**
     * @brief Returns own protocol name
     */
    inline std::string      get_protocol_name () const { return protName; };

    
    /**
     * @brief Returns name of shared memory area towards BP demon
     */
    inline std::string      get_shared_memory_area_name () const { return shmAreaName; };


    /**
     * @brief Returns own node identifier
     */
    inline NodeIdentifierT  get_own_node_identifier () const { return ownNodeIdentifier; };


    /**
     * @brief Returns maximum payload size
     */
    inline BPLengthT        get_max_payload_size () const { return maxPayloadSize; };

    
    /**
     * @brief Returns queueing mode
     */
    inline BPQueueingMode   get_queueing_mode () const { return queueingMode; };


    /**
     * @brief Returns number of entries in BP interface queue
     */
    inline uint16_t         get_max_entries () const { return maxEntries; };


    /**
     * @brief Returns flag indicating whether multiple payloads for
     *        this client protocol in one beacon are allowed
     */
    inline bool             get_allow_multiple_payloads () const { return allowMultiplePayloads; };


    /**
     * @brief Returns flag indicating whether this client protocol
     *        expects and processes BP-TransmitPayload.confirm
     *        primitives
     */
    inline bool             get_generate_transmit_payload_confirms () const { return generateTransmitPayloadConfirms; };


    /********************************************************************************
     * Management services
     *******************************************************************************/
    

    /**
     * @brief Ask BP demon to exit
     */
    DcpStatus shutdown_bp ();


    /**
     * @brief Ask BP demon to activate itself (i.e. to resume
     *        processing payloads)
     */
    DcpStatus activate_bp ();


    /**
     * @brief Ask BP demon to deactivate itself (i.e. to stop
     *        processing payloads)
     *
     * The BP demon will continue to run even if de-activated.
     */
    DcpStatus deactivate_bp ();


    /**
     * @brief Ask BP demon to return a list of all registered
     *        protocols (service 'BP-ListRegisteredProtocols')
     *
     * @param descrs: output parameter containing a list of
     *        descriptions of registered protocols
     */
    DcpStatus list_registered_protocols (std::list<BPRegisteredProtocolDataDescription>& descrs);


    /**
     * @brief Ask BP demon for certain runtime statistics
     *
     * @param avg_inter_beacon_time: output parameter holding the
     *        average inter beacon reception time in milliseconds
     * @param avg_beacon_size: output parameter holding the average
     *        beacon size in bytes
     * @param number_received_payloads: output parameter holding the
     *        number of beacons received by BP (with or without errors)
     */
    DcpStatus get_runtime_statistics (double& avg_inter_beacon_time,
				      double& avg_beacon_size,
				      unsigned int& number_received_payloads);
    
    
    /********************************************************************************
     * Payload exchange and management services, only usable when
     * client protocol is registered with BP
     *******************************************************************************/


    /**
     * @brief BP client protocol hands over a payload to BP demon for
     *        transmission
     *
     * @param length: length of payload block
     * @param payload: pointer to payload block
     *
     * @return DcpStatus value
     *
     * Throws on processing errors (e.g. invalid or empty payload,
     * inability to access shared memory, or timeout upon shared
     * memory access)
     *
     * Note: this method can only be used when the client protocol is
     * registered with BP, otherwise it throws.
     */
    DcpStatus transmit_payload (BPLengthT length, byte* payload);


    /**
     * @brief Attempts to retrieve a received payload
     *
     * @param result_length: output value, length of received payload
     *        (or zero if nothing received within timeout)
     * @param result_buffer: point to buffer where any received payload
     *        is copied into. Buffer is left unmodified if no payload is
     *        received. The maximum number of bytes copied is given by
     *        the maximum payload length for this client protocol, the
     *        buffer should be large enough for that.
     * @param more_payloads: says whether there are more received
     *        payloads available
     *
     * @return DcpStatus value
     *
     * Throws upon processing errors (e.g. inability to access shared
     * memory area)
     *
     * Note: this method can only be used when the client protocol is
     * registered with BP, otherwise it throws.
     */
    DcpStatus receive_payload (BPLengthT& result_length, byte* result_buffer, bool& more_payloads);


    /**
     * @brief Delete all BP payloads for the client protocol
     *
     * @return DcpStatus value
     *
     * Note: this method can only be used when the client protocol is
     * registered with BP, otherwise it throws.
     */
    DcpStatus clear_buffer ();


    /**
     * @brief Query number of payloads buffered for client protocol
     *
     * @param num_payloads_buffered: output parameter containing
     *        number of buffered payloads
     *
     * @return DcpStatus value
     *
     * Note: this method can only be used when the client protocol is
     * registered with BP, otherwise it throws.
     */
    DcpStatus query_number_buffered_payloads (unsigned long& num_payloads_buffered);

    
  };
    
};  // namespace dcp
