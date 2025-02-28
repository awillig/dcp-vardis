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


#include <queue>
#include <thread>
#include <chrono>
#include <dcp/common/area.h>
#include <dcp/common/debug_helpers.h>
#include <dcp/common/services_status.h>
#include <dcp/bp/bpclient_lib.h>
#include <dcp/bp/bp_service_primitives.h>
#include <dcp/vardis/vardis_transmitter.h>
#include <dcp/vardis/vardis_logging.h>


using dcp::bp::BPTransmitPayload_Request;


namespace dcp::vardis {

  // -----------------------------------------------------------------

  void construct_payload (VardisRuntimeData& runtime, AssemblyArea& area, unsigned int& containers_added)
  {
    VardisProtocolData& pd         = runtime.protocol_data;
    try {
      if (runtime.vardis_config.vardis_conf.lockingForIndividualContainers)
	{
	  { ScopedVariableStoreMutex mtx (runtime);  pd.makeICTypeCreateVariables (area, containers_added); }
	  { ScopedVariableStoreMutex mtx (runtime);  pd.makeICTypeDeleteVariables (area, containers_added); }
	  { ScopedVariableStoreMutex mtx (runtime);  pd.makeICTypeRequestVarCreates (area, containers_added); }
	  { ScopedVariableStoreMutex mtx (runtime);  pd.makeICTypeSummaries (area, containers_added); }
	  { ScopedVariableStoreMutex mtx (runtime);  pd.makeICTypeUpdates (area, containers_added); }
	  { ScopedVariableStoreMutex mtx (runtime);  pd.makeICTypeRequestVarUpdates (area, containers_added); }
	}
      else
	{
	  ScopedVariableStoreMutex mtx (runtime);
	  pd.makeICTypeCreateVariables (area, containers_added);
	  pd.makeICTypeDeleteVariables (area, containers_added);
	  pd.makeICTypeRequestVarCreates (area, containers_added);
	  pd.makeICTypeSummaries (area, containers_added);
	  pd.makeICTypeUpdates (area, containers_added);
	  pd.makeICTypeRequestVarUpdates (area, containers_added);
	}
    }
    catch (std::exception& e)
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal) << "Caught exception during payload construction: " << e.what() << ". Exiting.";
	runtime.vardis_exitFlag = true;
	containers_added = 0;
      }
  }
  
  // -----------------------------------------------------------------
  
  void transmitter_thread (VardisRuntimeData& runtime)
  {
    BOOST_LOG_SEV(log_tx, trivial::info) << "Starting transmit thread.";

    if (runtime.bp_shm_area_ptr == nullptr)
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal) << "Invalid shared memory handle. Exiting.";
	runtime.vardis_exitFlag = true;
	return;
      }
	
    BPShmControlSegment* control_seg_ptr = (BPShmControlSegment*) runtime.bp_shm_area_ptr->getControlSegmentPtr();
    byte               * buffer_seg_ptr  = runtime.bp_shm_area_ptr->getBufferSegmentPtr();

    if ((control_seg_ptr == nullptr) or (buffer_seg_ptr == nullptr))
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal) << "Invalid shared memory area pointer(s). Exiting.";
	runtime.vardis_exitFlag = true;
	return;
      }
      
    BPShmControlSegment& CS = *control_seg_ptr;
    
    while (not runtime.vardis_exitFlag)
      {
	std::this_thread::sleep_for (std::chrono::milliseconds (runtime.vardis_config.vardis_conf.payloadGenerationIntervalMS));
				     
	if (not runtime.protocol_data.vardis_store.get_vardis_isactive())
	  continue;

	// Find a free buffer in the shared memory to BP, construct
	// the payload there in place, and either hand it back to the
	// freeList in shared memory if the payload is empty, or
	// submit a payload transmit request to BP

	SharedMemBuffer shmBuff;
	{
	  ScopedShmControlSegmentLock lock (CS);
	  if (CS.rbFree.isEmpty())
	    {
	      BOOST_LOG_SEV(log_tx, trivial::fatal) << "Cannot find free shared memory buffer. Exiting.";
	      runtime.vardis_exitFlag = true;
	      return;
	    }

	  if (CS.queue.stored_elements() >= CS.queue.get_max_capacity ())
	    {
	      BOOST_LOG_SEV(log_tx, trivial::trace) << "Queue towards BP demon is currently full. No payload generated.";
	      continue;
	    }
	  
	  shmBuff = CS.rbFree.pop();
	}

	byte* data_ptr = buffer_seg_ptr + shmBuff.data_offs ();

	BPTransmitPayload_Request*  pldReq_ptr = new (data_ptr) BPTransmitPayload_Request;
	byte* area_ptr = data_ptr + sizeof(BPTransmitPayload_Request);

	MemoryChunkAssemblyArea area ("vd-tx", runtime.vardis_config.vardis_conf.maxPayloadSize, area_ptr);
	unsigned int containers_added = 0;
	construct_payload (runtime, area, containers_added);
	
	if (containers_added > 0)
	  {
	    BOOST_LOG_SEV(log_tx, trivial::trace) << "preparing payload in buffer " << shmBuff;
	    
	    pldReq_ptr->protocolId = BP_PROTID_VARDIS;
	    pldReq_ptr->length     = area.used();

	    shmBuff.set_used_length (area.used() + sizeof(BPTransmitPayload_Request));

	    ScopedShmControlSegmentLock lock (CS);
	    if (CS.rbTransmitPayloadRequest.isFull())
	      {
		BOOST_LOG_SEV(log_tx, trivial::fatal) << "Shared memory buffer: transmit payload request buffer is full. Exiting.";
		runtime.vardis_exitFlag = true;
		return;
	      }
	    CS.rbTransmitPayloadRequest.push(shmBuff);
	  }
	else
	  {
	    ScopedShmControlSegmentLock lock (CS);
	    if (CS.rbFree.isFull())
	      {
		BOOST_LOG_SEV(log_tx, trivial::fatal) << "Shared memory buffer free list is full. Exiting.";
		runtime.vardis_exitFlag = true;
		return;
	      }
	    shmBuff.clear();
	    CS.rbFree.push(shmBuff);
	  }	
      }
    BOOST_LOG_SEV(log_tx, trivial::info) << "Exiting transmit thread.";
  }
  
};  // namespace dcp::vardis
