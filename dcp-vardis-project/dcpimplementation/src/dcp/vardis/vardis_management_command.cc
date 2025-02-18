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


#include <exception>
#include <list>
#include <dcp/common/services_status.h>
#include <dcp/vardis/vardis_constants.h>
#include <dcp/vardis/vardis_logging.h>
#include <dcp/vardis/vardis_management_command.h>
#include <dcp/vardis/vardis_service_primitives.h>


namespace dcp::vardis {

  // -------------------------------------------------------------------

  template <typename CT>
  void send_simple_confirmation (VardisRuntimeData& runtime, DcpStatus statcode)
  {
    runtime.vardisCommandSock.send_simple_confirmation<CT>(log_mgmt_command, statcode, runtime.vardis_exitFlag);
  }
  
  // -------------------------------------------------------------------

  template <typename RT, typename CT>
  inline bool wrong_request_size (VardisRuntimeData& runtime, const std::string& methname, size_t nbytes)
  {
    if (nbytes != sizeof(RT))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::fatal) << methname << ": request has wrong data size = " << nbytes << ", exiting.";
	runtime.vardis_exitFlag = true;
	send_simple_confirmation <CT> (runtime, VARDIS_STATUS_INTERNAL_ERROR);
	return true;
      }
    return false;
  }
  
  // -------------------------------------------------------------------

  void 	handleVardisRegisterRequest (VardisRuntimeData& runtime, byte* buffer, size_t nbytes)
  {
    if (wrong_request_size<VardisRegister_Request, VardisRegister_Confirm> (runtime, "handleVardisRegisterRequest", nbytes)) return;

    VardisRegister_Request*  pReq = (VardisRegister_Request*) buffer;
    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Processing VardisRegister request: shm name = " << pReq->shm_area_name;

    // check whether client application already exists -- to do this,
    // we check uniqueness of shared memory area name
    if (runtime.clientApplications.contains (std::string (pReq->shm_area_name)))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::info) << "Processing VardisRegister request: application already exists";
	send_simple_confirmation<VardisRegister_Confirm> (runtime, VARDIS_STATUS_APPLICATION_ALREADY_REGISTERED);
	return;
      }

    // Now create and initialize new client protocol data entry and add it to the list of registered protocols
    VardisClientProtocolData clientProt;
    
    // attempt to create the shared memory region
    uint64_t requestedBuffers = dcp::vardis::VardisShmControlSegment::get_minimum_number_buffers_required() + 20;
    size_t   realPayloadSize  = runtime.vardis_config.vardis_conf.maxValueLength + runtime.vardis_config.vardis_conf.maxDescriptionLength + 256;

    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Processing VardisRegister request: attempting to create shared memory area"
						    << ", requestedBuffers = " << requestedBuffers
						    << ", realPayloadSize = " << realPayloadSize
						    << ", sizeof(ControlSeg) = " << sizeof(VardisShmControlSegment);
    
    try {
      clientProt.sharedMemoryAreaPtr = std::make_shared<ShmBufferPool>(
								       pReq->shm_area_name,
								       true,
								       sizeof(VardisShmControlSegment),
								       realPayloadSize,
								       requestedBuffers
								       );
    }
    catch (ShmException& shme) {
      BOOST_LOG_SEV(log_mgmt_command, trivial::info) << "Processing VardisRegister request: cannot allocate shared memory block, reason = " << shme.what();
      send_simple_confirmation<VardisRegister_Confirm> (runtime, VARDIS_STATUS_INTERNAL_SHARED_MEMORY_ERROR);
      return;
    }

    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Processing VardisRegister request: registered shared memory"
						    << ", buffer payload size = " << realPayloadSize
						    << ", region size = " << clientProt.sharedMemoryAreaPtr->get_region().get_size()
						    << ", number buffers requested = " << requestedBuffers
						    << ", area name = " << clientProt.sharedMemoryAreaPtr->get_shm_area_name();
    
    // initialize control segment for shared memory segment    
    clientProt.controlSegmentPtr = new (clientProt.sharedMemoryAreaPtr->getControlSegmentPtr()) VardisShmControlSegment(*clientProt.sharedMemoryAreaPtr,
															requestedBuffers);
    
    // initialize client protocol entry
    clientProt.clientName                    =  std::string (pReq->shm_area_name);
    
    runtime.clientApplications[clientProt.clientName] = clientProt;

    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Processing VardisRegister request: completed successful registration";
    VardisRegister_Confirm conf (VARDIS_STATUS_OK, runtime.protocol_data.ownNodeIdentifier);
    runtime.vardisCommandSock.send_raw_data (log_mgmt_command, (byte*) &conf, sizeof(VardisRegister_Confirm), runtime.vardis_exitFlag);
  }

  // -------------------------------------------------------------------

  void 	handleVardisDeregisterRequest (VardisRuntimeData& runtime, byte* buffer, size_t nbytes)
  {
    if (wrong_request_size<VardisDeregister_Request, VardisDeregister_Confirm> (runtime, "handleVardisDeregisterRequest", nbytes)) return;

    VardisDeregister_Request*  pReq = (VardisDeregister_Request*) buffer;
    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Processing request: VardisDeregister, name = " << pReq->shm_area_name;

    // check whether client protocol exists
    if (not runtime.clientApplications.contains (std::string (pReq->shm_area_name)))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::info) << "handleVardisDeregisterRequest: vardis application / client with shmname " << pReq->shm_area_name << " is not registered";
	send_simple_confirmation<VardisDeregister_Confirm>(runtime, VARDIS_STATUS_UNKNOWN_APPLICATION);
	return;
      }
    
    runtime.clientApplications.erase(std::string(pReq->shm_area_name));

    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Processing VardisDeregister request: erased registered application";
    send_simple_confirmation<VardisDeregister_Confirm>(runtime, VARDIS_STATUS_OK);

  }
  
  // -------------------------------------------------------------------

  void handleVardisShutdownRequest (VardisRuntimeData& runtime, byte*, size_t)
  {
    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Processing VardisShutdown request. Exiting.";
    runtime.vardis_exitFlag = true;
  }
  
  // -------------------------------------------------------------------

  void handleVardisActivateRequest (VardisRuntimeData& runtime, byte*, size_t nbytes)
  {
    if (wrong_request_size<VardisActivate_Request, VardisActivate_Confirm> (runtime, "handleVardisActivateRequest", nbytes)) return;

    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Processing VardisActivate request.";
    runtime.protocol_data_mutex.lock ();
    runtime.protocol_data.vardis_isActive = true;
    runtime.protocol_data_mutex.unlock ();

    send_simple_confirmation<VardisActivate_Confirm>(runtime, VARDIS_STATUS_OK);
  }
  
  // -------------------------------------------------------------------

  void handleVardisDeactivateRequest (VardisRuntimeData& runtime, byte*, size_t nbytes)
  {
    if (wrong_request_size<VardisDeactivate_Request, VardisDeactivate_Confirm> (runtime, "handleVardisDeactivateRequest", nbytes)) return;

    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Processing VardisDeactivate request.";
    
    runtime.protocol_data_mutex.lock ();
    runtime.protocol_data.vardis_isActive = false;
    runtime.protocol_data_mutex.unlock ();

    send_simple_confirmation<VardisDeactivate_Confirm>(runtime, VARDIS_STATUS_OK);
  }

  // -------------------------------------------------------------------

  void handleVardisGetStatisticsRequest (VardisRuntimeData& runtime, byte*, size_t nbytes)
  {
    if (wrong_request_size<VardisGetStatistics_Request, VardisGetStatistics_Confirm> (runtime, "handleVardisGetStatisticsRequest", nbytes)) return;

    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Processing VardisGetStatistics request.";

    VardisGetStatistics_Confirm gsConf;
    
    runtime.protocol_data_mutex.lock();
    gsConf.protocol_stats = runtime.protocol_data.vardis_stats;
    runtime.protocol_data_mutex.unlock();

    runtime.vardisCommandSock.send_raw_data (log_mgmt_command, (byte*) &gsConf, sizeof(VardisGetStatistics_Confirm), runtime.vardis_exitFlag);
  }
  
  // -------------------------------------------------------------------

  void handleVardisRTDBDescribeDatabaseRequest (VardisRuntimeData& runtime, byte*, size_t nbytes)
  {
    if (wrong_request_size<VardisDescribeDatabase_Request, VardisDescribeDatabase_Confirm> (runtime, "handleVardisRTDBDescribeDatabaseRequest", nbytes)) return;

    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Processing VardisDescribeDatabase request.";

    std::list<DescribeDatabaseVariableDescription> var_descriptions;

    {
      ScopedProtocolDataMutex mtx (runtime);

      VardisProtocolData& PD = runtime.protocol_data;
    
      for (auto it = PD.theVariableDatabase.begin(); it != PD.theVariableDatabase.end(); ++it)
	{
	  DescribeDatabaseVariableDescription descr;
	  descr.varId          =  (it->second).spec.varId;
	  descr.prodId         =  (it->second).spec.prodId;
	  descr.repCnt         =  (it->second).spec.repCnt;
	  descr.tStamp         =  (it->second).tStamp;
	  descr.toBeDeleted    =  (it->second).toBeDeleted;
	  std::strcpy (descr.description, (it->second).spec.descr.to_str().c_str());
	  
	  var_descriptions.push_back (descr);
	}    
    }
    
    VardisDescribeDatabase_Confirm conf;
    conf.status_code                 = VARDIS_STATUS_OK;
    conf.numberVariableDescriptions  = var_descriptions.size();

    runtime.vardisCommandSock.send_raw_data (log_mgmt_command, (byte*) &conf, sizeof(VardisDescribeDatabase_Confirm), runtime.vardis_exitFlag);
    for (const auto& descr : var_descriptions)
      {
	runtime.vardisCommandSock.send_raw_data (log_mgmt_command, (byte*) &descr, sizeof(descr), runtime.vardis_exitFlag);
      }
  }


  // -------------------------------------------------------------------

  void handleVardisRTDBDescribeVariableRequest (VardisRuntimeData& runtime, byte* buffer, size_t nbytes)
  {
    if (wrong_request_size<VardisDescribeVariable_Request, VardisDescribeVariable_Confirm> (runtime, "handleVardisRTDBDescribeVariableRequest", nbytes)) return;

    VardisDescribeVariable_Request *pReq = (VardisDescribeVariable_Request*) buffer;
    VarIdT varId = pReq->varId;

    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Processing RTDBDescribeVariable request for varId " << varId;

    DescribeVariableDescription var_descr;
    byte val_buffer [MAX_maxValueLength + 1];
    bool found = false;
    {
      ScopedProtocolDataMutex mtx (runtime);

      VardisProtocolData& PD = runtime.protocol_data;

      if (PD.theVariableDatabase.contains (varId))
	{
	  found = true;
	  DBEntry& db_entry = PD.theVariableDatabase.at (varId);

	  var_descr.varId        =  db_entry.spec.varId;
	  var_descr.prodId       =  db_entry.spec.prodId;
	  var_descr.repCnt       =  db_entry.spec.repCnt;
	  std::strcpy (var_descr.description, db_entry.spec.descr.to_str().c_str());	  
	  var_descr.seqno        =  db_entry.seqno;
	  var_descr.tStamp       =  db_entry.tStamp;
	  var_descr.countUpdate  =  db_entry.countUpdate;
	  var_descr.countCreate  =  db_entry.countCreate;
	  var_descr.countDelete  =  db_entry.countDelete;
	  var_descr.toBeDeleted  =  db_entry.toBeDeleted;
	  var_descr.value_length =  db_entry.value.length;

	  std::memcpy (val_buffer, db_entry.value.data, db_entry.value.length);
	}
    }

    if (not found)
      {
	send_simple_confirmation<VardisDescribeVariable_Confirm>(runtime, VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST);
      }
    else
      {
	VardisDescribeVariable_Confirm dv_conf;
	dv_conf.var_description = var_descr;
	dv_conf.status_code     = VARDIS_STATUS_OK;
	
	runtime.vardisCommandSock.send_raw_data (log_mgmt_command, (byte*) &dv_conf, sizeof(VardisDescribeVariable_Confirm), runtime.vardis_exitFlag);
	runtime.vardisCommandSock.send_raw_data (log_mgmt_command, val_buffer, var_descr.value_length.val, runtime.vardis_exitFlag);
      }
  }


  
  // -------------------------------------------------------------------

  void handle_command_socket (VardisRuntimeData& runtime)
  {
    DcpServiceType serv_type;
    byte buffer [vardisCommandSocketBufferSize];
    int nbytes = runtime.vardisCommandSock.start_read_command (log_mgmt_command, buffer, vardisCommandSocketBufferSize, serv_type, runtime.vardis_exitFlag);

    if (nbytes <= 0) return;
    
    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Command loop: service type is " << vardis_service_type_to_string(serv_type);
    switch (serv_type)
      {
	
      case stVardis_Register:
	{
	  ScopedClientApplicationsMutex ca_mtx (runtime);
	  handleVardisRegisterRequest (runtime, buffer, nbytes);
	}
	break;

      case stVardis_Deregister:
	{
	  ScopedClientApplicationsMutex ca_mtx (runtime);
	  handleVardisDeregisterRequest (runtime, buffer, nbytes);
	}
	break;

      case stVardis_Shutdown:
	handleVardisShutdownRequest (runtime, buffer, nbytes);
	break;

      case stVardis_Activate:
	handleVardisActivateRequest (runtime, buffer, nbytes);
	break;

      case stVardis_Deactivate:
	handleVardisDeactivateRequest (runtime, buffer, nbytes);
	break;

      case stVardis_GetStatistics:
	handleVardisGetStatisticsRequest (runtime, buffer, nbytes);
	break;
	
      case stVardis_RTDB_DescribeDatabase:
	handleVardisRTDBDescribeDatabaseRequest (runtime, buffer, nbytes);
	break;

      case stVardis_RTDB_DescribeVariable:
	handleVardisRTDBDescribeVariableRequest (runtime, buffer, nbytes);
	break;
	
      default:
	BOOST_LOG_SEV(log_mgmt_command, trivial::fatal) << "Command loop: unknown or un-implemented service type, val = " << serv_type;
	runtime.vardis_exitFlag = true;
      }
    
    runtime.vardisCommandSock.stop_read_command (log_mgmt_command, runtime.vardis_exitFlag);
  }
  
  // -------------------------------------------------------------------
  
  void management_thread_command (VardisRuntimeData& runtime)
  {
    // open Vardis command socket

    try {
      runtime.vardisCommandSock.open_owner (log_mgmt_command);
    }
    catch (std::exception& e) {
      BOOST_LOG_SEV(log_mgmt_command, trivial::fatal) << "Could not establish Vardis command socket, exiting.";
      runtime.vardis_exitFlag = true;
      return;
    }

    BOOST_LOG_SEV(log_mgmt_command, trivial::trace)
      << "Established Vardis command socket "
      << runtime.vardisCommandSock.get_name()
      << ", starting to wait on commands";

    while (not runtime.vardis_exitFlag)
      {
	try {
	  handle_command_socket (runtime);
	}
	catch (std::exception& e) {
	  BOOST_LOG_SEV(log_mgmt_command, trivial::fatal) << "Caught exception " << e.what() << " while handling command, exiting.";
	  runtime.vardis_exitFlag = true;
	}
      }

    BOOST_LOG_SEV(log_mgmt_command, trivial::trace) << "Leaving command loop, cleanup";

    runtime.vardisCommandSock.close_owner ();
  }
  
};  // namespace dcp::vardis
