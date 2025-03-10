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



#include <iostream>
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

  DcpStatus BPClientRuntime::register_with_bp (const BPQueueingMode  queueingMode,
					       const uint16_t        maxEntries,
					       const bool            allowMultiplePayloads,
					       const bool            generateTransmitPayloadConfirms)
  {
    ScopedClientSocket cl_sock (commandSock);
        
    BPRegisterProtocol_Request rpReq;
    std::strcpy (rpReq.name, protName.c_str());
    rpReq.protocolId                       = protId;
    rpReq.maxPayloadSize                   = get_max_payload_size();
    rpReq.queueingMode                     = queueingMode;
    rpReq.maxEntries                       = maxEntries;
    rpReq.allowMultiplePayloads            = allowMultiplePayloads;
    rpReq.generateTransmitPayloadConfirms  = generateTransmitPayloadConfirms;
    
    std::strcpy (rpReq.shm_area_name, shmAreaName.c_str());

    byte buffer [command_sock_buffer_size];
    int nrcvd = cl_sock.sendRequestAndReadResponseBlock<BPRegisterProtocol_Request> (rpReq, buffer, command_sock_buffer_size);

    if (nrcvd != sizeof(BPRegisterProtocol_Confirm))
      {
	std::cout << "register_with_bp: response has size " << nrcvd
		  << " but I expected size " << sizeof(BPRegisterProtocol_Confirm)
		  << std::endl;
	
	cl_sock.abort ("register_with_bp: response has wrong size");
      }
    
    BPRegisterProtocol_Confirm *pConf = (BPRegisterProtocol_Confirm*) buffer;
    
    if (pConf->s_type != stBP_RegisterProtocol) cl_sock.abort ("register_with_bp: response has wrong service type");

    ownNodeIdentifier = pConf->ownNodeIdentifier;

    // if response is ok, try to attach to shared memory block
    if (pConf->status_code == BP_STATUS_OK)
      {
	try {
	  bp_shm_area_ptr = std::make_shared<ShmBufferPool> (
							     shmAreaName.c_str(),
							     false,
							     sizeof(BPShmControlSegment),
							     0,
							     0
							     );
	}
	catch (ShmException& shme) {
	  cl_sock.abort ("register_with_bp: cannot attach to shared memory block");
	}
	_isRegistered = true;
      }
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
      throw BPClientLibException ("clear_buffer: not registered with BP");
    
    return simple_request_confirm_service<BPClearBuffer_Request, BPClearBuffer_Confirm> ("clear_buffer");
  }

  // -----------------------------------------------------------------------------------

  DcpStatus BPClientRuntime::query_number_buffered_payloads (unsigned long& num_payloads_buffered)
  {
    if (not _isRegistered)
      throw BPClientLibException ("query_number_buffered_payloads: not registered with BP");
    
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

    if ((((size_t) nrcvd) < sizeof(BPListRegisteredProtocols_Request)) || (((size_t) nrcvd) >= command_sock_buffer_size - 1))
      cl_sock.abort ("list_registered_protocols: too little or too much data");

    BPListRegisteredProtocols_Confirm* pConf = (BPListRegisteredProtocols_Confirm*) buffer;

    if (pConf->s_type != stBP_ListRegisteredProtocols)
      cl_sock.abort ("list_registered_protocols: response has wrong service type");

    if (pConf->status_code == BP_STATUS_OK)
      {	
	if ((nrcvd - sizeof(BPListRegisteredProtocols_Confirm)) % sizeof(BPRegisteredProtocolDataDescription) != 0)
	  cl_sock.abort ("list_registered_protocols: response does not carry integral number of registered protocol description records");

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
