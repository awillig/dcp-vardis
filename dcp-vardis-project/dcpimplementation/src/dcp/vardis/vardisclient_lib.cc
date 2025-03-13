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
#include <dcp/vardis/vardis_service_primitives.h>
#include <dcp/vardis/vardis_constants.h>
#include <dcp/vardis/vardisclient_lib.h>


using dcp::vardis::VardisActivate_Confirm;
using dcp::vardis::VardisActivate_Request;
using dcp::vardis::VardisDeactivate_Confirm;
using dcp::vardis::VardisDeactivate_Request;
using dcp::vardis::VardisDescribeDatabase_Confirm;
using dcp::vardis::VardisDescribeDatabase_Request;
using dcp::vardis::VardisDescribeVariable_Confirm;
using dcp::vardis::VardisDescribeVariable_Request;
using dcp::vardis::VardisGetStatistics_Request;
using dcp::vardis::VardisGetStatistics_Confirm;
using dcp::vardis::VardisRegister_Request;
using dcp::vardis::VardisRegister_Confirm;
using dcp::vardis::VardisDeregister_Request;
using dcp::vardis::VardisDeregister_Confirm;
using dcp::vardis::VardisShutdown_Request;
using dcp::vardis::vardisCommandSocketBufferSize;

using dcp::vardis::MAX_maxValueLength;
using dcp::vardis::MAX_maxDescriptionLength;

namespace dcp {

  // -----------------------------------------------------------------------------------

  VardisClientRuntime::VardisClientRuntime (const VardisClientConfiguration& client_conf,
					    bool do_register)
    : BaseClientRuntime (client_conf.cmdsock_conf.commandSocketFile, client_conf.cmdsock_conf.commandSocketTimeoutMS),
      variable_store (client_conf.shm_conf_global.shmAreaName.c_str(), false)
		      
  {
    shmSegmentName = client_conf.shm_conf_client.shmAreaName;

    if (shmSegmentName.empty())
      throw VardisClientLibException ("Shared memory name is empty");

    if (std::strlen (shmSegmentName.c_str()) > maxShmAreaNameLength-1)
      throw VardisClientLibException (std::format("Shared memory name {} is too long", shmSegmentName));
    
    const std::string& cmdsock_name = client_conf.cmdsock_conf.commandSocketFile;

    if (cmdsock_name.empty())
      throw VardisClientLibException ("Command socket name is empty");

    if (std::strlen(cmdsock_name.c_str()) > CommandSocket::max_command_socket_name_length())
      throw VardisClientLibException (std::format ("Command socket name {} is too long", cmdsock_name));

    if (do_register)
      {
	DcpStatus reg_response = register_with_vardis();
	if (reg_response != VARDIS_STATUS_OK)
	  throw VardisClientLibException (std::format("Registration with Vardis failed, status code = {}", vardis_status_to_string(reg_response)));
      }
  }

  // -----------------------------------------------------------------------------------

  VardisClientRuntime::~VardisClientRuntime ()
  {
    if (_isRegistered)
      {
        deregister_with_vardis();
      }
  }

  // -----------------------------------------------------------------------------------


  /**
   * @brief Send command to shut down to Vardis demon
   *
   * Note that this just sends a command and does not expect a
   * response. The reason is that in response the Vardis demon will
   * shut down its command socket, and there is no guarantee that this
   * client will be fast enough to retrieve the response
   * (confirmation) data from that socket before it is removed.
   */
  DcpStatus VardisClientRuntime::shutdown_vardis ()
  {
    ScopedClientSocket cl_sock (commandSock);
    VardisShutdown_Request sdReq;
    cl_sock.sendRequest<VardisShutdown_Request>(sdReq);

    // This prevents the destructor from attempting to send a
    // deregistration service primitive to a Vardis demon that may
    // already have shut down
    _isRegistered = false;
    
    return VARDIS_STATUS_OK;
  }
  

  // -----------------------------------------------------------------------------------

  DcpStatus VardisClientRuntime::activate_vardis ()
  {
    return simple_request_confirm_service<VardisActivate_Request, VardisActivate_Confirm> ("activate_vardis");
  }
  
  
  // -----------------------------------------------------------------------------------
  
  DcpStatus VardisClientRuntime::deactivate_vardis ()
  {
    return simple_request_confirm_service<VardisDeactivate_Request, VardisDeactivate_Confirm> ("deactivate_vardis");
  }

  // -----------------------------------------------------------------------------------

  DcpStatus VardisClientRuntime::register_with_vardis ()
  {
    ScopedClientSocket cl_sock (commandSock);
        
    VardisRegister_Request rpReq;
    std::strcpy (rpReq.shm_area_name, shmSegmentName.c_str());
    byte buffer [vardisCommandSocketBufferSize];
    int nrcvd = cl_sock.sendRequestAndReadResponseBlock<VardisRegister_Request> (rpReq, buffer, vardisCommandSocketBufferSize);

    if (nrcvd != sizeof(VardisRegister_Confirm))
      {	
	cl_sock.abort (std::format("register_with_vardis: response has wrong size {} (expected: {})",
				   nrcvd,
				   sizeof(VardisRegister_Confirm)));
      }
    
    VardisRegister_Confirm *pConf = (VardisRegister_Confirm*) buffer;
    
    if (pConf->s_type != stVardis_Register)
      cl_sock.abort (std::format("register_with_vardis: response has wrong service type {}", pConf->s_type));

    // if response is ok, try to attach to shared memory block
    try {
      vardis_shm_area_ptr = std::make_shared<ShmBufferPool> (
							     shmSegmentName.c_str(),
							     false,
							     sizeof(VardisShmControlSegment),
							     0,
							     0
							     );
    }
    catch (ShmException& shme) {
      cl_sock.abort (std::format("register_with_vardis: cannot attach to shared memory block {}", shmSegmentName));
    }

    if (pConf->status_code == VARDIS_STATUS_OK)
      {
	_isRegistered     =  true;
	ownNodeIdentifier =  pConf->own_node_identifier;
      }
    
    return pConf->status_code;
  }
  
  // -----------------------------------------------------------------------------------
  
  DcpStatus VardisClientRuntime::deregister_with_vardis ()
  {
    ScopedClientSocket cl_sock (commandSock);
    VardisDeregister_Request drpReq;
    std::strcpy (drpReq.shm_area_name, shmSegmentName.c_str());
    
    byte buffer [vardisCommandSocketBufferSize];
    int nrcvd = cl_sock.sendRequestAndReadResponseBlock<VardisDeregister_Request> (drpReq, buffer, vardisCommandSocketBufferSize);
    
    if (nrcvd != sizeof(VardisDeregister_Confirm))
      cl_sock.abort (std::format("deregister_with_vardis: response has wrong size {} (expected: {})",
				 nrcvd,
				 sizeof(VardisDeregister_Confirm)));
    
    VardisDeregister_Confirm *pConf = (VardisDeregister_Confirm*) buffer;
    
    if (pConf->s_type != stVardis_Deregister)
      cl_sock.abort (std::format("deregister_with_vardis: response has wrong service type {}", pConf->s_type));

    if (pConf->status_code == VARDIS_STATUS_OK)
      _isRegistered = false;
    
    return pConf->status_code;
  }
    

  // -----------------------------------------------------------------------------------

  DcpStatus VardisClientRuntime::describe_database (std::list<DescribeDatabaseVariableDescription>& db_descr_list)
  {
    ScopedClientSocket cl_sock (commandSock);
    VardisDescribeDatabase_Request dd_req;

    const size_t buffer_size = dcp::vardis::VarIdT::max_val() * (sizeof(DescribeDatabaseVariableDescription) + 16);
    byte buffer [buffer_size];
    int nrcvd = cl_sock.sendRequestAndReadResponseBlock<VardisDescribeDatabase_Request> (dd_req, buffer, sizeof(VardisDescribeDatabase_Confirm));

    if (nrcvd < (int) sizeof(VardisDescribeDatabase_Confirm))
      cl_sock.abort (std::format ("describe_database: response has insufficient size {}", nrcvd));

    VardisDescribeDatabase_Confirm* pConf = (VardisDescribeDatabase_Confirm*) buffer;

    for (uint64_t i=0; i < pConf->numberVariableDescriptions; i++)
      {
	byte descbuffer [sizeof(DescribeDatabaseVariableDescription)];
	int nread = cl_sock.read_whole_response (descbuffer, sizeof(DescribeDatabaseVariableDescription));

	if ((size_t) nread < sizeof(DescribeDatabaseVariableDescription))
	  cl_sock.abort (std::format ("describe_database: response for single entry has insufficient size {}", nread));
	
	db_descr_list.push_back (*((DescribeDatabaseVariableDescription*) descbuffer));
      }

    return VARDIS_STATUS_OK;
  }

  // -----------------------------------------------------------------------------------

  DcpStatus VardisClientRuntime::describe_variable (VarIdT varId,
						    vardis::DescribeVariableDescription& var_descr,
						    byte* buffer)
  {
    ScopedClientSocket cl_sock (commandSock);
    vardis::VardisDescribeVariable_Request dv_req;
    dv_req.varId = varId;
    
    if (buffer == nullptr)
      cl_sock.abort ("describe_variable: no buffer given");

    byte conf_buffer [sizeof(DescribeVariableDescription) + 128];
    int nrcvd = cl_sock.sendRequestAndReadResponseBlock<VardisDescribeVariable_Request> (dv_req, conf_buffer, sizeof(VardisDescribeVariable_Confirm));

    if (nrcvd < (int) sizeof(VardisDescribeVariable_Confirm))
      cl_sock.abort (std::format ("describe_variable: response has insufficient size {}", nrcvd));

    VardisDescribeVariable_Confirm* pConf = (VardisDescribeVariable_Confirm*) conf_buffer;
    var_descr = pConf->var_description;

    int nread = cl_sock.read_whole_response (buffer, var_descr.value_length.val);
    
    if ((size_t) nread < var_descr.value_length.val)
      cl_sock.abort (std::format ("describe_variable: response for variable value has insufficient size {}", nread));

    return VARDIS_STATUS_OK;
  }

  // -----------------------------------------------------------------------------------

  DcpStatus VardisClientRuntime::retrieve_statistics (VardisProtocolStatistics& stats)
  {
    ScopedClientSocket cl_sock (commandSock);
    vardis::VardisGetStatistics_Request gs_req;

    byte buffer [2*sizeof(VardisProtocolStatistics)];
    int nrcvd = cl_sock.sendRequestAndReadResponseBlock<VardisGetStatistics_Request> (gs_req, buffer, sizeof(VardisGetStatistics_Confirm));

    if (nrcvd < (int) sizeof(VardisGetStatistics_Confirm))
      cl_sock.abort (std::format ("retrieve_statistics: response has wrong size {}", nrcvd));

    VardisGetStatistics_Confirm* pConf = (VardisGetStatistics_Confirm*) buffer;

    stats = pConf->protocol_stats;
    
    return VARDIS_STATUS_OK;
  }
  
  // -----------------------------------------------------------------------------------
  
    
};  // namespace dcp
