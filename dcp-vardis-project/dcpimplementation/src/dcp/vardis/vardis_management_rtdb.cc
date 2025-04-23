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


#include <chrono>
#include <thread>
#include <dcp/vardis/vardis_client_protocol_data.h>
#include <dcp/vardis/vardis_logging.h>
#include <dcp/vardis/vardis_management_rtdb.h>
#include <dcp/vardis/vardis_protocol_data.h>

namespace dcp::vardis {

  // ----------------------------------------------------------------------------

  template <typename RT, typename CT>
  void handle_request_queue (
			     VardisRuntimeData& runtime,
			     PayloadQueue& requestQueue,
			     ConfirmQueue& confirmQueue,
			     CT (*caller_handler) (VardisRuntimeData&, const RT&)
			     )
  {
    bool timed_out;

    std::function <void (byte*, size_t)> req_handler = [&] (byte* memaddr, size_t len)
    {
      DCPLOG_TRACE(log_mgmt_rtdb)
	<< "handle_request_queue: got buffer from request queue "
	<< requestQueue.get_queue_name ()
	<< " and confirm queue " << confirmQueue.get_queue_name ();
      
      MemoryChunkDisassemblyArea disass_area ("vd-hrq-dass", len, memaddr);
      
      RT theRequest;
      theRequest.deserialize (disass_area);
      
      DCPLOG_TRACE(log_mgmt_rtdb)
	<< "handle_request_queue: got request "
	<< theRequest;
      
      CT theConfirm = caller_handler (runtime, theRequest);
      bool conf_timed_out;
      bool conf_is_full;
      PushHandler conf_handler = [&] (byte* conf_memaddr, size_t)
      {
	MemoryChunkAssemblyArea ass_area ("vd-hrq-ass", sizeof(CT), conf_memaddr);
	theConfirm.serialize (ass_area);
	return ass_area.used();
      };
      
      DCPLOG_TRACE(log_mgmt_rtdb)
	<< "handle_request_queue: status code after processing = "
	<< vardis_status_to_string (theConfirm.status_code);
      
      confirmQueue.push_nowait (conf_handler, conf_timed_out, conf_is_full);
      
      if (conf_timed_out or conf_is_full)
	{
	  DCPLOG_FATAL(log_mgmt_rtdb) << "handle_request_queue: cannot place confirm in queue. Exiting.";
	  runtime.vardis_exitFlag = true;
	  return;
	}
    };

    requestQueue.popall_nowait (req_handler, timed_out);
    if (timed_out)
      {
	DCPLOG_FATAL(log_mgmt_rtdb)
	  << "handle_request_queue: shared memory timeout while processing request queue. Exiting.";
	runtime.vardis_exitFlag = true;
	return;
      }    
  }

  // ----------------------------------------------------------------------------

  void handle_client_shared_memory (VardisRuntimeData& runtime, VardisClientProtocolData& clientProt)
  {
    if (not runtime.protocol_data.vardis_store.get_vardis_isactive()) return;
    if (runtime.vardis_exitFlag) return;

    VardisShmControlSegment& CS = *(clientProt.pSCS);

    // --------------------
    
    {
      ScopedVariableStoreMutex pd_mtx (runtime);
      auto handler = [] (VardisRuntimeData& runtime, const RTDB_Create_Request& cr_req)
      {
	return runtime.protocol_data.handle_rtdb_create_request (cr_req);
      };
      handle_request_queue<RTDB_Create_Request, RTDB_Create_Confirm> (runtime,
								      CS.pqCreateRequest,
								      CS.pqCreateConfirm,
								      handler);
    }

    // --------------------
    
    {
      ScopedVariableStoreMutex pd_mtx (runtime);
      auto handler = [] (VardisRuntimeData& runtime, const RTDB_Delete_Request& del_req)
      {
	return runtime.protocol_data.handle_rtdb_delete_request (del_req);
      };
      handle_request_queue<RTDB_Delete_Request, RTDB_Delete_Confirm> (runtime,
								      CS.pqDeleteRequest,
								      CS.pqDeleteConfirm,
								      handler);
    }

    // --------------------

    {
      ScopedVariableStoreMutex pd_mtx (runtime);
      auto handler = [] (VardisRuntimeData& runtime, const RTDB_Update_Request& upd_req)
      {
	return runtime.protocol_data.handle_rtdb_update_request (upd_req);
      };
      handle_request_queue<RTDB_Update_Request, RTDB_Update_Confirm> (runtime,
								      CS.pqUpdateRequest,
								      CS.pqUpdateConfirm,
								      handler);
    }

  }
  
  // ----------------------------------------------------------------------------
  
  void management_thread_rtdb (VardisRuntimeData& runtime)
  {
    DCPLOG_INFO(log_mgmt_rtdb) << "Starting to interact with client via shared memory";

    try {
      while (not runtime.vardis_exitFlag)
	{
	  std::this_thread::sleep_for (std::chrono::milliseconds (runtime.vardis_config.vardis_conf.pollRTDBServiceIntervalMS));
	  
	  // run over all client protocols / applications, using lock
	  {
	    ScopedClientApplicationsMutex ca_mtx (runtime);
	    
	    for (auto& clapp : runtime.clientApplications)
	      {
		handle_client_shared_memory (runtime, clapp.second);
	      }
	  }
	}
    }
    catch (DcpException& e)
      {
	DCPLOG_FATAL(log_mgmt_rtdb)
	  << "Caught DCP exception in Vardis RTDB management main loop. "
	  << "Exception type: " << e.ename()
	  << ", module: " << e.modname()
	  << ", message: " << e.what()
	  << ". Exiting.";
	runtime.vardis_exitFlag = true;
      }
    catch (std::exception& e)
      {
	DCPLOG_FATAL(log_mgmt_rtdb)
	  << "Caught other exception in Vardis RTDB management main loop. "
	  << "Message: " << e.what()
	  << ". Exiting.";
	runtime.vardis_exitFlag = true;
      }

    DCPLOG_INFO(log_mgmt_rtdb) << "Stopping to interact with client via shared memory, cleanup";    
  }
  
};  // namespace dcp::vardis
