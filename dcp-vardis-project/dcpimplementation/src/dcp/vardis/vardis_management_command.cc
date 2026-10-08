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
#include <dcp/vardis/vardis_constants.h>
#include <dcp/vardis/vardis_logging.h>
#include <dcp/vardis/vardis_management_command.h>
#include <dcp/vardis/vardis_service_primitives.h>


namespace dcp::vardis {

  // -------------------------------------------------------------------

  template <typename CT>
  void send_simple_confirmation (VardisRuntimeData& runtime, VardisStatus statcode)
  {
    runtime.vardisCommandSock.send_simple_confirmation<CT>(log_mgmt_command,
							   static_cast<uint16_t>(statcode),
							   runtime.vardis_exitFlag);
  }
  
  // -------------------------------------------------------------------

  template <typename RT, typename CT>
  inline bool wrong_request_size (VardisRuntimeData& runtime, const std::string& methname, size_t nbytes)
  {
    if (nbytes != sizeof(RT))
      {
	DCPLOG_FATAL(log_mgmt_command)
	  << methname
	  << ": request has wrong data size = "
	  << nbytes
	  << ". Exiting.";
	runtime.vardis_exitFlag = true;
	send_simple_confirmation <CT> (runtime, VardisStatus::Internal_Error);
	return true;
      }
    return false;
  }
  
  // -------------------------------------------------------------------

  void 	handleVardisRegisterRequest (VardisRuntimeData& runtime, byte* buffer, size_t nbytes)
  {
    if (wrong_request_size<VardisRegister_Request, VardisRegister_Confirm> (runtime, "handleVardisRegisterRequest", nbytes)) return;

    VardisRegister_Request*  pReq = (VardisRegister_Request*) buffer;
    DCPLOG_INFO(log_mgmt_command)
      << "Processing VardisRegister request: shm name = "
      << pReq->shm_area_name;

    // check whether client application already exists -- to do this,
    // we check uniqueness of shared memory area name
    if (runtime.clientApplications.contains (std::string (pReq->shm_area_name)))
      {
	DCPLOG_INFO (log_mgmt_command)
	  << "Processing VardisRegister request: application already exists.";
	if (not pReq->delete_old_registration)
	  {
	    send_simple_confirmation<VardisRegister_Confirm> (runtime, VardisStatus::Application_Already_Registered);
	    return;
	  }
	else
	  {
	    DCPLOG_INFO (log_mgmt_command)
	      << "Processing VardisRegister request: removing old application.";
	    runtime.clientApplications.erase(std::string(pReq->shm_area_name));	    
	  }
      }

    // Now create and initialize new client protocol data entry and add it to the list of registered protocols
    VardisClientProtocolData clientProt (pReq->shm_area_name);
															
    // initialize client protocol entry
    clientProt.clientName                    =  std::string (pReq->shm_area_name);
    
    runtime.clientApplications[clientProt.clientName] = clientProt;

    DCPLOG_INFO(log_mgmt_command)
      << "Processing VardisRegister request: completed successful registration";
    VardisRegister_Confirm conf (VardisStatus::Ok, runtime.protocol_data.ownNodeIdentifier);
    runtime.vardisCommandSock.send_raw_data (log_mgmt_command, (byte*) &conf, sizeof(VardisRegister_Confirm), runtime.vardis_exitFlag);
  }

  // -------------------------------------------------------------------

  void 	handleVardisDeregisterRequest (VardisRuntimeData& runtime, byte* buffer, size_t nbytes)
  {
    if (wrong_request_size<VardisDeregister_Request, VardisDeregister_Confirm> (runtime, "handleVardisDeregisterRequest", nbytes)) return;

    VardisDeregister_Request*  pReq = (VardisDeregister_Request*) buffer;
    DCPLOG_INFO(log_mgmt_command)
      << "Processing request: VardisDeregister, name = "
      << pReq->shm_area_name;

    // check whether client protocol exists
    if (not runtime.clientApplications.contains (std::string (pReq->shm_area_name)))
      {
	DCPLOG_INFO(log_mgmt_command)
	  << "handleVardisDeregisterRequest: vardis application / client with shmname "
	  << pReq->shm_area_name
	  << " is not registered";
	send_simple_confirmation<VardisDeregister_Confirm>(runtime, VardisStatus::Unknown_Application);
	return;
      }
    
    runtime.clientApplications.erase(std::string(pReq->shm_area_name));

    DCPLOG_INFO(log_mgmt_command)
      << "Processing VardisDeregister request: erased registered application";
    send_simple_confirmation<VardisDeregister_Confirm>(runtime, VardisStatus::Ok);

  }
  
  // -------------------------------------------------------------------

  void handleVardisShutdownRequest (VardisRuntimeData& runtime, byte*, size_t)
  {
    DCPLOG_INFO(log_mgmt_command) << "Processing VardisShutdown request. Exiting.";
    runtime.vardis_exitFlag = true;
  }
  
  // -------------------------------------------------------------------

  void handleVardisActivateRequest (VardisRuntimeData& runtime, byte*, size_t nbytes)
  {
    if (wrong_request_size<VardisActivate_Request, VardisActivate_Confirm> (runtime, "handleVardisActivateRequest", nbytes)) return;

    DCPLOG_INFO(log_mgmt_command) << "Processing VardisActivate request.";
    runtime.variable_store.set_vardis_isactive (true);

    send_simple_confirmation<VardisActivate_Confirm>(runtime, VardisStatus::Ok);
  }
  
  // -------------------------------------------------------------------

  void handleVardisDeactivateRequest (VardisRuntimeData& runtime, byte*, size_t nbytes)
  {
    if (wrong_request_size<VardisDeactivate_Request, VardisDeactivate_Confirm> (runtime, "handleVardisDeactivateRequest", nbytes)) return;

    DCPLOG_INFO(log_mgmt_command) << "Processing VardisDeactivate request.";
    
    runtime.variable_store.set_vardis_isactive (false);

    send_simple_confirmation<VardisDeactivate_Confirm>(runtime, VardisStatus::Ok);
  }

  // -------------------------------------------------------------------

  void handleVardisGetStatisticsRequest (VardisRuntimeData& runtime, byte*, size_t nbytes)
  {
    if (wrong_request_size<VardisGetStatistics_Request, VardisGetStatistics_Confirm> (runtime, "handleVardisGetStatisticsRequest", nbytes)) return;

    DCPLOG_TRACE(log_mgmt_command) << "Processing VardisGetStatistics request.";

    VardisGetStatistics_Confirm gsConf;
    
    runtime.variable_store.lock ();
    gsConf.protocol_stats = runtime.variable_store.get_vardis_protocol_statistics_ref();
    runtime.variable_store.unlock ();

    runtime.vardisCommandSock.send_raw_data (log_mgmt_command, (byte*) &gsConf, sizeof(VardisGetStatistics_Confirm), runtime.vardis_exitFlag);
  }
  
  // -------------------------------------------------------------------

  void handleVardisRTDBDescribeDatabaseRequest (VardisRuntimeData& runtime, byte*, size_t nbytes)
  {
    if (wrong_request_size<VardisDescribeDatabase_Request, VardisDescribeDatabase_Confirm> (runtime, "handleVardisRTDBDescribeDatabaseRequest", nbytes)) return;

    DCPLOG_TRACE(log_mgmt_command) << "Processing VardisDescribeDatabase request.";

    std::list<DescribeDatabaseVariableDescription> var_descriptions;

    {
      ScopedVariableStoreMutex mtx (runtime);

      VardisProtocolData& PD = runtime.protocol_data;
    
      for (auto varId : PD.active_variables)
	{
	  DBEntry& db_entry = PD.vardis_store.get_db_entry_ref (varId);
	  DescribeDatabaseVariableDescription descr;

          descr.varId          =  db_entry.varId;
	  descr.prodId         =  db_entry.prodId;
	  descr.repCnt         =  db_entry.repCnt;
	  descr.creationTime   =  db_entry.creationTime;
	  descr.timeout        =  db_entry.timeout;
	  descr.tStamp         =  db_entry.tStamp;
	  descr.isDeleted      =  db_entry.isDeleted;
	  PD.vardis_store.read_description (varId, descr.description);
	  
	  var_descriptions.push_back (descr);
	}    
    }
    
    VardisDescribeDatabase_Confirm conf (VardisStatus::Ok);
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

    DCPLOG_TRACE(log_mgmt_command)
      << "Processing RTDBDescribeVariable request for varId "
      << varId;

    static const size_t VAL_BUFFER_SIZE = MAX_maxValueLength + 1;
    DescribeVariableDescription var_descr;
    byte val_buffer [VAL_BUFFER_SIZE];
    bool found = false;
    {
      ScopedVariableStoreMutex mtx (runtime);

      VardisProtocolData& PD = runtime.protocol_data;

      if (PD.vardis_store.identifier_is_allocated (varId))
	{
	  found = true;
	  DBEntry& db_entry = PD.vardis_store.get_db_entry_ref (varId);
	  VarLenT  val_size;
	  
	  var_descr.varId        =  db_entry.varId;
	  var_descr.prodId       =  db_entry.prodId;
	  var_descr.repCnt       =  db_entry.repCnt;
	  var_descr.creationTime =  db_entry.creationTime;
	  var_descr.timeout      =  db_entry.timeout;
	  PD.vardis_store.read_description (varId, var_descr.description);
	  var_descr.seqno        =  db_entry.seqno;
	  var_descr.tStamp       =  db_entry.tStamp;
	  var_descr.countUpdate  =  db_entry.countUpdate;
	  var_descr.countCreate  =  db_entry.countCreate;
	  var_descr.countDelete  =  db_entry.countDelete;
	  var_descr.isDeleted    =  db_entry.isDeleted;
	  var_descr.value_length =  PD.vardis_store.size_of_value (varId);
	  PD.vardis_store.read_value (varId, VAL_BUFFER_SIZE , val_buffer, val_size);
	}
    }

    if (not found)
      {
	send_simple_confirmation<VardisDescribeVariable_Confirm>(runtime, VardisStatus::Variable_Does_Not_Exist);
      }
    else
      {
	VardisDescribeVariable_Confirm dv_conf (VardisStatus::Ok);
	dv_conf.var_description = var_descr;
	
	runtime.vardisCommandSock.send_raw_data (log_mgmt_command, (byte*) &dv_conf, sizeof(VardisDescribeVariable_Confirm), runtime.vardis_exitFlag);
	runtime.vardisCommandSock.send_raw_data (log_mgmt_command, val_buffer, var_descr.value_length.val, runtime.vardis_exitFlag);
      }
  }


  
  // -------------------------------------------------------------------

  void handle_command_socket (VardisRuntimeData& runtime)
  {
    std::uint16_t stype;
    byte buffer [vardisCommandSocketBufferSize];
    int nbytes = runtime.vardisCommandSock.start_read_command (log_mgmt_command, buffer, vardisCommandSocketBufferSize, stype, runtime.vardis_exitFlag);

    if (nbytes <= 0) return;

    VardisServiceType serv_type = static_cast<VardisServiceType>(stype);
    DCPLOG_TRACE(log_mgmt_command)
      << "Command loop: service type is "
      << to_string(serv_type);
    
    switch (serv_type)
      {
	
      case VardisServiceType::Register:
	{
	  ScopedClientApplicationsMutex ca_mtx (runtime);
	  handleVardisRegisterRequest (runtime, buffer, nbytes);
	}
	break;

      case VardisServiceType::Deregister:
	{
	  ScopedClientApplicationsMutex ca_mtx (runtime);
	  handleVardisDeregisterRequest (runtime, buffer, nbytes);
	}
	break;

      case VardisServiceType::ShutDown:
	handleVardisShutdownRequest (runtime, buffer, nbytes);
	break;

      case VardisServiceType::Activate:
	handleVardisActivateRequest (runtime, buffer, nbytes);
	break;

      case VardisServiceType::Deactivate:
	handleVardisDeactivateRequest (runtime, buffer, nbytes);
	break;

      case VardisServiceType::GetStatistics:
	handleVardisGetStatisticsRequest (runtime, buffer, nbytes);
	break;
	
      case VardisServiceType::RTDB_DescribeDatabase:
	handleVardisRTDBDescribeDatabaseRequest (runtime, buffer, nbytes);
	break;

      case VardisServiceType::RTDB_DescribeVariable:
	handleVardisRTDBDescribeVariableRequest (runtime, buffer, nbytes);
	break;
	
      default:
	DCPLOG_FATAL(log_mgmt_command)
	  << "Command loop: unknown or un-implemented service type, val = "
	  << stype;
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
    catch (DcpException& e) {
      DCPLOG_FATAL(log_mgmt_command)
	<< "Could not establish Vardis command socket. "
	<< "Exception type: " << e.ename()
	<< ", module: " << e.modname()
	<< ", message: " << e.what()
	<< ". Exiting.";
      runtime.vardis_exitFlag = true;
      return;
    }
    catch (std::exception& e) {
      DCPLOG_FATAL(log_mgmt_command)
	<< "Could not establish Vardis command socket. Exiting.";
      runtime.vardis_exitFlag = true;
      return;
    }

    DCPLOG_INFO(log_mgmt_command)
      << "Established Vardis command socket "
      << runtime.vardisCommandSock.get_name()
      << ", starting to wait on commands";

    while (not runtime.vardis_exitFlag)
      {
	try {
	  handle_command_socket (runtime);
	}
	catch (DcpException& e) {
	  DCPLOG_FATAL(log_mgmt_command)
	    << "Could not receive data from command socket. "
	    << "Exception type: " << e.ename()
	    << ", module: " << e.modname()
	    << ", message: " << e.what()
	    << ". Exiting.";
	  runtime.vardis_exitFlag = true;
	}
	catch (std::exception& e) {
	  DCPLOG_FATAL(log_mgmt_command)
	    << "Caught exception "
	    << e.what()
	    << " while handling command. Exiting.";
	  runtime.vardis_exitFlag = true;
	}
      }

    DCPLOG_INFO(log_mgmt_command) << "Leaving command loop, cleanup";

    runtime.vardisCommandSock.close_owner ();
  }
  
};  // namespace dcp::vardis
