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
#include <list>
#include <memory>
#include <sys/time.h>
#include <dcp/common/command_socket.h>
#include <dcp/common/foundation_types.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/services_status.h>
#include <dcp/common/sharedmem_structure_base.h>
#include <dcp/bp/bp_queueing_mode.h>
#include <dcp/bp/bp_service_primitives.h>
#include <dcp/bp/bp_shm_control_segment.h>
#include <dcp/bp/bp_transmissible_types.h>
#include <dcp/bp/bpclient_configuration.h>




using dcp::BP_STATUS_OK;
using dcp::bp_status_to_string;
using dcp::DcpStatus;
using dcp::ShmException;
using dcp::ShmStructureBase;
using dcp::bp::BPDeregisterProtocol_Confirm;
using dcp::bp::BPDeregisterProtocol_Request;
using dcp::bp::BPLengthT;
using dcp::bp::BPQueueingMode;
using dcp::bp::BPRegisteredProtocolDataDescription;
using dcp::bp::BPRegisterProtocol_Confirm;
using dcp::bp::BPRegisterProtocol_Request;
using dcp::bp::BPShmControlSegment;
using dcp::bp::BPStaticClientInfo;

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


    
    bp::BPStaticClientInfo  static_client_info;
    std::string shmAreaName;
    
    /**
     * @brief DCP Node identifier of this node / station.
     *
     * Valid after successful call to register_with_bp()
     */
    NodeIdentifierT ownNodeIdentifier;



    /**
     * @brief Indicates whether client protocol expect confirms for
     *        BP-TransmitPayload.request primitives.
     *
     * ISSUE: Generation of confirms is currently not supported!
     */
    bool generateTransmitPayloadConfirms;

    
    /**
     * @brief The BPClientConfiguration structure
     */
    BPClientConfiguration client_configuration;


    /**
     * @brief Register BP client protocol with BP (service
     *        'BP-RegisterProtocol'), using the stored
     *        static_client_info for BP-related configuration
     *
     * @param generateTransmitPayloadConfirms: specify whether client
     *        protocol expects BP demon to generate
     *        BP-TransmitPayload.confirm primitives
     *
     * After successful registration, BP can process payloads related
       to this client protocol if it is active.
     */
    DcpStatus register_with_bp (const bool generateTransmitPayloadConfirms);


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
    void check_names (const char* protName, std::string shmAreaName);


    /**
     * @brief Helper function for a BP client protocol to retrieve
     *        (copy) a received payload from the BP demon. Can be
     *        waiting/blocking or non-waiting/blocking.
     *
     * @param result_length: output parameter giving the size of the
     *        received payload in bytes. If this is zero, then no
     *        payload has been received.
     * @param result_buffer: memory address of the buffer into which
     *        to copy the received payload
     * @param more_payloads: output parameter indicating whether more
     *        payloads are waiting
     * @param waiting: indicates whether we should wait/block until a
     *        payload arrives or not.
     * @param exitFlag: if we are waiting to receive a payload, then
     *        this flag is checked regularly and waiting is aborted if
     *        it becomes true.
     */
    DcpStatus receive_payload_helper (BPLengthT& result_length,
				      byte* result_buffer,
				      bool& more_payloads,
				      bool waiting,
				      bool& exitFlag);
    
    
  public:

    /**
     * @brief Pointer to the shared memory area descriptor
     *
     * The shared memory area is constructed in the constructor of
     * this class and used for payload exchange between client
     * protocol and BP
     */
    std::shared_ptr<ShmStructureBase>  pSSB;


    /**
     * @brief Pointer to the actual shared memory area. Set after
     *        shared memory area access has been achieved.
     */
    BPShmControlSegment*               pSCS = nullptr;

    

    BPClientRuntime () = delete;


    /**
     * @brief Constructor, which includes registration of a client protocol
     *
     * @param client_conf: configuration data for BP client protocol
     * @param static_client_info: contains all the static information about
     *        a BP client protocol (e.g. protocol name, queueing mode etc)
     * @param gen_pld_conf: specify whether client protocol expects BP
     *        demon to generate BP-TransmitPayload.confirm primitives
     */
    BPClientRuntime (BPClientConfiguration client_conf, BPStaticClientInfo static_client_info, bool gen_pld_conf);


    /**
     * @brief Constructor without registration
     *
     * @param client_conf: configuration data for BP client protocol
     *
     * This is useful when a client protocol or application only needs
     * the command socket
     */
    BPClientRuntime (const BPClientConfiguration& client_conf);


    ~BPClientRuntime ();
    
    
    /********************************************************************************
     * Getters
     *******************************************************************************/

    /**
     * @brief Returns static BP client information
     */
    inline bp::BPStaticClientInfo get_static_client_info () const { return static_client_info; };
        
    /**
     * @brief Returns name of shared memory area towards BP demon
     */
    inline std::string      get_shared_memory_area_name () const { return shmAreaName; };


    /**
     * @brief Returns own node identifier
     */
    inline NodeIdentifierT  get_own_node_identifier () const { return ownNodeIdentifier; };


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
     * This method does not block to wait for an incoming payload when
     * no payload is available
     *
     * Throws upon processing errors (e.g. inability to access shared
     * memory area)
     *
     * Note: this method can only be used when the client protocol is
     * registered with BP, otherwise it throws.
     */
    DcpStatus receive_payload_nowait (BPLengthT& result_length, byte* result_buffer, bool& more_payloads);


    /**
     * @brief Like receive_payload_nowait() but blocks caller until a
     *        payload is available or the exitFlag is true
     */
    DcpStatus receive_payload_wait (BPLengthT& result_length, byte* result_buffer, bool& more_payloads, bool& exitFlag);

    
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


    /*********************************************************************************
     * End of public interface
     ********************************************************************************/

    
  protected:
    
    /**
     * @brief Support method to send a command request over a command
     *        socket and retrieve a corresponding (fixed-size)
     *        confirmation primitive
     */
    template <class RT, class CT>
    DcpStatus simple_bp_request_confirm_service (const std::string& methname, CT& conf)
    {
      ScopedClientSocket cl_sock (commandSock);
      RT req;
      req.protocolId = static_client_info.protocolId;
      
      byte buffer [command_sock_buffer_size];
      int nrcvd = cl_sock.sendRequestAndReadResponseBlock<RT> (req, buffer, command_sock_buffer_size);
      
	if (nrcvd != sizeof(CT)) cl_sock.abort (methname + ": response has wrong size");
	
	CT *pConf = (CT*) buffer;
	conf = *pConf;
	
	if (pConf->s_type != req.s_type)
	  cl_sock.abort (methname + ": response has wrong service type");
	
	return pConf->status_code;
    }
    
  };
    
};  // namespace dcp
