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



#include <format>
#include <fstream>
extern "C" {
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
}
#include <dcp/common/command_socket.h>
#include <dcp/common/debug_helpers.h>
#include <dcp/common/other_helpers.h>
#include <dcp/common/services_status.h>
#include <dcp/bp/bpclient_lib.h>
#include <dcp/bp/bp_service_primitives.h>

using dcp::ShmException;
using namespace dcp::bp;



namespace dcp {

  // -----------------------------------------------------------------------------------

  BPClientRuntime::BPClientRuntime (BPClientConfiguration client_conf,
				    BPStaticClientInfo static_client_info,
				    bool gen_pld_conf
				    )
    : BaseClientRuntime (client_conf.bp_cmdsock_conf.commandSocketFile.c_str(), client_conf.bp_cmdsock_conf.commandSocketTimeoutMS),
      static_client_info (static_client_info),
      shmAreaName (client_conf.bp_shm_conf.shmAreaName),
      generateTransmitPayloadConfirms (gen_pld_conf),
      client_configuration (client_conf)
  {
    if (gen_pld_conf)
      throw BPClientLibException ("BPClientRuntime",
				  "generation of payload confirms not supported");

    check_protocol_name (static_client_info.protocolName);
    check_shm_area_name (shmAreaName);
    
    DcpStatus reg_status = register_with_bp (generateTransmitPayloadConfirms);
    if (reg_status != BP_STATUS_OK)
      throw BPClientLibException ("BPClientRuntime, ",
				  std::format("registration failed, status code = {}", bp_status_to_string(reg_status)));
    
    pSSB = std::make_shared<ShmStructureBase> (shmAreaName.c_str(), 0, false);
    pSCS = (BPShmControlSegment*) pSSB->get_memory_address ();
    if (!pSCS)
      throw BPClientLibException ("BPClientRuntime",
				  "cannot attach to BPShmControlSegment");
  }
  
  // -----------------------------------------------------------------------------------
  
  BPClientRuntime::BPClientRuntime (const BPClientConfiguration& client_conf)
    : BaseClientRuntime (client_conf.bp_cmdsock_conf.commandSocketFile.c_str(), client_conf.bp_cmdsock_conf.commandSocketTimeoutMS),
      client_configuration (client_conf)
  {
    check_protocol_name (static_client_info.protocolName);
  }
  
  // -----------------------------------------------------------------------------------
  
  BPClientRuntime::~BPClientRuntime ()
  {
    if (_isRegistered)
      deregister_with_bp ();
  }
  
  // -----------------------------------------------------------------------------------
  
  void BPClientRuntime::check_protocol_name (const char* protName)
  {
    if (std::strlen(protName) == 0)
      throw BPClientLibException ("check_names",
				  "no protocol name given");
    
    if (std::strlen(protName) > dcp::bp::maximumProtocolNameLength - 1)
      throw BPClientLibException ("check_names",
				  std::format("protocol name {} is too long", protName));    
  };

    // -----------------------------------------------------------------------------------
  
  void BPClientRuntime::check_shm_area_name (std::string shmAreaName)
  {
    if (shmAreaName.empty())
      throw BPClientLibException ("check_names",
				  "no shared memory area name given");
    
    if (shmAreaName.capacity() > maxShmAreaNameLength - 1)
      throw BPClientLibException ("check_names",
				  "shared memory area name is too long");
  };
  
  // -----------------------------------------------------------------------------------
  
  DcpStatus BPClientRuntime::shutdown_bp ()
  {
    ScopedClientSocket cl_sock (commandSock);
    BPShutDown_Request sdReq;
    cl_sock.sendRequest<BPShutDown_Request> (sdReq);

    // This prevents the destructor from attempting to send a
    // deregistration service primitive to a Vardis demon that may
    // already have shut down
    _isRegistered = false;
    
    return BP_STATUS_OK;
  }
  

  // -----------------------------------------------------------------------------------

  DcpStatus BPClientRuntime::activate_bp ()
  {
    return simple_request_confirm_service<BPActivate_Request, BPActivate_Confirm> ("activate_bp");
  }
  
  
  // -----------------------------------------------------------------------------------
  
  DcpStatus BPClientRuntime::deactivate_bp ()
  {
    return simple_request_confirm_service<BPDeactivate_Request, BPDeactivate_Confirm> ("deactivate_bp");
  }

  // -----------------------------------------------------------------------------------

  DcpStatus BPClientRuntime::register_with_bp (const bool generateTransmitPayloadConfirms)
  {
    ScopedClientSocket cl_sock (commandSock);
        
    BPRegisterProtocol_Request rpReq;
    rpReq.static_info = static_client_info;
    rpReq.generateTransmitPayloadConfirms              = generateTransmitPayloadConfirms;
    
    std::strcpy (rpReq.shm_area_name, shmAreaName.c_str());

    byte buffer [command_sock_buffer_size];
    int nrcvd = cl_sock.sendRequestAndReadResponseBlock<BPRegisterProtocol_Request> (rpReq, buffer, command_sock_buffer_size);

    if (nrcvd != sizeof(BPRegisterProtocol_Confirm))
      {
	cl_sock.abort (std::format("register_with_bp: response has wrong size {}, expected was {}", nrcvd, sizeof(BPRegisterProtocol_Confirm)));
      }
    
    BPRegisterProtocol_Confirm *pConf = (BPRegisterProtocol_Confirm*) buffer;
    
    if (pConf->s_type != stBP_RegisterProtocol) cl_sock.abort ("register_with_bp: response has wrong service type");

    ownNodeIdentifier = pConf->ownNodeIdentifier;
    
    if (pConf->status_code != BP_STATUS_OK)
      throw BPClientLibException ("register_with_bp",
				  std::format("registration failed, status code = {}", bp_status_to_string (pConf->status_code)));
    
    _isRegistered = true;
    
    return pConf->status_code;
  }
  
  // -----------------------------------------------------------------------------------
  
  DcpStatus BPClientRuntime::deregister_with_bp ()
  {
    BPDeregisterProtocol_Confirm rpConf;
    DcpStatus rv = simple_bp_request_confirm_service <BPDeregisterProtocol_Request, BPDeregisterProtocol_Confirm> ("deregister_with_bp", rpConf);

    if (rv == BP_STATUS_OK)
      _isRegistered = false;
    
    return rv;
  }
  
  // -----------------------------------------------------------------------------------

  DcpStatus BPClientRuntime::clear_buffer ()
  {
    if (not _isRegistered)
      throw BPClientLibException ("clear_buffer",
				  "not registered with BP");
    
    return simple_request_confirm_service<BPClearBuffer_Request, BPClearBuffer_Confirm> ("clear_buffer");
  }

  // -----------------------------------------------------------------------------------
  
  DcpStatus BPClientRuntime::get_runtime_statistics (double& avg_inter_beacon_time,
						     double& avg_beacon_size,
						     unsigned int& number_received_payloads)
  {
    ScopedClientSocket cl_sock (commandSock);
    BPGetStatistics_Request gs_req;

    byte buffer [command_sock_buffer_size];
    int nrcvd = cl_sock.sendRequestAndReadResponseBlock<BPGetStatistics_Request> (gs_req, buffer, command_sock_buffer_size);

    if ((size_t) nrcvd < sizeof(BPGetStatistics_Confirm))
      cl_sock.abort ("get_runtime_statistics: too little or too much data");

    BPGetStatistics_Confirm* pConf = (BPGetStatistics_Confirm*) buffer;

    if (pConf->s_type != stBP_GetStatistics)
      cl_sock.abort ("get_runtime_statistics: response has wrong service type");

    if (pConf->status_code == BP_STATUS_OK)
      {
	avg_inter_beacon_time     = pConf->avg_inter_beacon_time;
	avg_beacon_size           = pConf->avg_beacon_size;
	number_received_payloads  = pConf->number_received_beacons;
      }
    
    return pConf->status_code;
  }

  
  // -----------------------------------------------------------------------------------

  DcpStatus BPClientRuntime::query_number_buffered_payloads (unsigned long& num_payloads_buffered)
  {
    if (not _isRegistered)
      throw BPClientLibException ("query_number_buffered_payloads",
				  "not registered with BP");
    
    BPQueryNumberBufferedPayloads_Confirm conf;
    DcpStatus stat = simple_bp_request_confirm_service<BPQueryNumberBufferedPayloads_Request, BPQueryNumberBufferedPayloads_Confirm> ("query_number_buffered_payloads", conf);
    num_payloads_buffered = conf.num_payloads_buffered;
    return stat;
  }
  

  // -----------------------------------------------------------------------------------

  DcpStatus BPClientRuntime::list_registered_protocols (std::list<BPRegisteredProtocolDataDescription>& descrs)
  {
    ScopedClientSocket cl_sock (commandSock);
    BPListRegisteredProtocols_Request lrpReq;
    
    byte buffer [command_sock_buffer_size];
    int nrcvd = cl_sock.sendRequestAndReadResponseBlock<BPListRegisteredProtocols_Request> (lrpReq, buffer, command_sock_buffer_size);

    if ((((size_t) nrcvd) < sizeof(BPListRegisteredProtocols_Confirm)) || (((size_t) nrcvd) >= command_sock_buffer_size - 1))
      cl_sock.abort ("list_registered_protocols: too little or too much data");

    BPListRegisteredProtocols_Confirm* pConf = (BPListRegisteredProtocols_Confirm*) buffer;

    if (pConf->s_type != stBP_ListRegisteredProtocols)
      cl_sock.abort (std::format("list_registered_protocols: response has wrong service type {}", pConf->s_type));

    if (pConf->status_code == BP_STATUS_OK)
      {
	int data_size = nrcvd - sizeof(BPListRegisteredProtocols_Confirm);
	if (data_size % sizeof(BPRegisteredProtocolDataDescription) != 0)
	  cl_sock.abort ("list_registered_protocols: response does not carry integral number of registered protocol description records");

	if (data_size / sizeof(BPRegisteredProtocolDataDescription) != pConf->numberProtocols)
	  cl_sock.abort (std::format("list_registered_protocols: response does not carry the right number of registered protocol description records, data_size is {}, expected are {}, actual are {}", data_size, pConf->numberProtocols, data_size / sizeof(BPRegisteredProtocolDataDescription)));
	
	BPRegisteredProtocolDataDescription* descrPtr = (BPRegisteredProtocolDataDescription*) (buffer + sizeof(BPListRegisteredProtocols_Confirm));
	
	for (uint64_t i = 0; i < pConf->numberProtocols; i++)
	  {
	    descrs.push_back(descrPtr[i]);
	  }
      }
    
    return pConf->status_code;
  }

  
  // -----------------------------------------------------------------------------------
  
  
};  // namespace dcp
