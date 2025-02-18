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
#include <dcp/common/shared_mem_area.h>
#include <dcp/vardis/vardis_client_protocol_data.h>
#include <dcp/vardis/vardis_logging.h>
#include <dcp/vardis/vardis_management_rtdb.h>
#include <dcp/vardis/vardis_protocol_data.h>


namespace dcp::vardis {

  // ----------------------------------------------------------------------------

  template <typename RT, typename CT>
  void handle_request_queue (
			     VardisRuntimeData& runtime,
			     RingBufferNormal& requestQueue,
			     RingBufferNormal& confirmQueue,
			     byte* buffer_seg_ptr,
			     CT (*handler) (VardisRuntimeData&, const RT&)
			     )
  {
    while (not requestQueue.isEmpty())
      {
	SharedMemBuffer buff = requestQueue.pop ();

	BOOST_LOG_SEV(log_mgmt_rtdb, trivial::trace) << "handle_request_queue: got buffer " << buff
						     << " from request ringbuffer " << requestQueue.get_name()
						     << " and confirm ringbuffer " << confirmQueue.get_name()
	  ;
	
	byte* data_ptr       = buffer_seg_ptr + buff.data_offs ();
	MemoryChunkDisassemblyArea disass_area ("vd-hrq-dass", buff.used_length(), data_ptr);

	RT theRequest;
	theRequest.deserialize (disass_area);

	BOOST_LOG_SEV(log_mgmt_rtdb, trivial::trace) << "handle_request_queue: got request " << theRequest;
	
	CT theConfirm = handler (runtime, theRequest);

	BOOST_LOG_SEV(log_mgmt_rtdb, trivial::trace) << "handle_request_queue: status code after processing = "
						     << vardis_status_to_string (theConfirm.status_code);
	
	buff.clear();

	if (confirmQueue.isFull())
	  {
	    BOOST_LOG_SEV(log_mgmt_rtdb, trivial::fatal) << "handle_request_queue: confirm queue is full, exiting.";
	    runtime.vardis_exitFlag = true;
	    return;
	  }

	MemoryChunkAssemblyArea ass_area ("vd-hrq-ass", buff.max_length (), data_ptr);
	theConfirm.serialize (ass_area);
	buff.set_used_length (ass_area.used());
	
	confirmQueue.push (buff);
      }
  }

  // ----------------------------------------------------------------------------

  void handle_client_shared_memory (VardisRuntimeData& runtime, VardisClientProtocolData& clientProt)
  {
    if (not runtime.protocol_data.vardis_isActive) return;
    if (runtime.vardis_exitFlag) return;

    if ((clientProt.sharedMemoryAreaPtr == nullptr) || (clientProt.controlSegmentPtr == nullptr))
      {
	BOOST_LOG_SEV(log_mgmt_rtdb, trivial::fatal)
	  << "handle_client_shared_memory: handling memory area " << clientProt.clientName
	  << ": no valid shared memory area segment";
	runtime.vardis_exitFlag = true;
	return;
      }

    VardisShmControlSegment& CS = *(clientProt.controlSegmentPtr);
    byte* buffer_seg_ptr        = (byte*) clientProt.sharedMemoryAreaPtr->getBufferSegmentPtr();


    // --------------------
    
    {
      ScopedShmControlSegmentLock lock (CS);
      ScopedProtocolDataMutex pd_mtx (runtime);
      if (not CS.rbCreateRequest.isEmpty())
	{
	  auto handler = [] (VardisRuntimeData& runtime, const RTDB_Create_Request& cr_req)
                                  {
				    return runtime.protocol_data.handle_rtdb_create_request (cr_req);
				  };
	  handle_request_queue<RTDB_Create_Request, RTDB_Create_Confirm> (runtime,
									  CS.rbCreateRequest,
									  CS.rbCreateConfirm,
									  buffer_seg_ptr,
									  handler);
	}
    }

    // --------------------
    
    {
      ScopedShmControlSegmentLock lock (CS);
      ScopedProtocolDataMutex pd_mtx (runtime);
      if (not CS.rbDeleteRequest.isEmpty())
	{
	  auto handler = [] (VardisRuntimeData& runtime, const RTDB_Delete_Request& del_req)
                                  {
				    return runtime.protocol_data.handle_rtdb_delete_request (del_req);
				  };
	  handle_request_queue<RTDB_Delete_Request, RTDB_Delete_Confirm> (runtime,
									  CS.rbDeleteRequest,
									  CS.rbDeleteConfirm,
									  buffer_seg_ptr,
									  handler);

	}
    }

    // --------------------

    {
      ScopedShmControlSegmentLock lock (CS);
      ScopedProtocolDataMutex pd_mtx (runtime);
      if (not CS.rbUpdateRequest.isEmpty())
	{
	  auto handler = [] (VardisRuntimeData& runtime, const RTDB_Update_Request& upd_req)
                                  {
				    return runtime.protocol_data.handle_rtdb_update_request (upd_req);
				  };
	  handle_request_queue<RTDB_Update_Request, RTDB_Update_Confirm> (runtime,
									  CS.rbUpdateRequest,
									  CS.rbUpdateConfirm,
									  buffer_seg_ptr,
									  handler);

	}
    }


    // --------------------

    {
      ScopedShmControlSegmentLock lock (CS);
      ScopedProtocolDataMutex pd_mtx (runtime);
      if (not CS.rbReadRequest.isEmpty())
	{
	  auto handler = [] (VardisRuntimeData& runtime, const RTDB_Read_Request& read_req)
                                  {
				    return runtime.protocol_data.handle_rtdb_read_request (read_req);
				  };
	  handle_request_queue<RTDB_Read_Request, RTDB_Read_Confirm> (runtime,
								      CS.rbReadRequest,
								      CS.rbReadConfirm,
								      buffer_seg_ptr,
								      handler);

	}
    }
    

    // --------------------
  }
  
  // ----------------------------------------------------------------------------
  
  void management_thread_rtdb (VardisRuntimeData& runtime)
  {
    BOOST_LOG_SEV(log_mgmt_rtdb, trivial::info) << "Starting to interact with client via shared memory";

    while (not runtime.vardis_exitFlag)
      {
	std::this_thread::sleep_for (std::chrono::milliseconds (runtime.vardis_config.vardis_conf.pollRTDBServiceIntervalMS));

	// run over all client protocols / applications, using lock
	{
	  ScopedClientApplicationsMutex ca_mtx (runtime);

	  for (auto it = runtime.clientApplications.begin(); it != runtime.clientApplications.end(); ++it)
	    {
	      handle_client_shared_memory (runtime, it->second);
	    }
	}
      }

    BOOST_LOG_SEV(log_mgmt_rtdb, trivial::info) << "Stopping to interact with client via shared memory, cleanup";    
  }
  
};  // namespace dcp::vardis
