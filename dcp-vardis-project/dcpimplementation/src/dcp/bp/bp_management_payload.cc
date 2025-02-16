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
#include <chrono>
#include <thread>
extern "C" {
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
}
#include <dcp/common/debug_helpers.h>
#include <dcp/common/memblock.h>
#include <dcp/common/services_status.h>
#include <dcp/common/shared_mem_area.h>
#include <dcp/bp/bp_transmissible_types.h>
#include <dcp/bp/bp_service_primitives.h>
#include <dcp/bp/bp_logging.h>
#include <dcp/bp/bp_client_protocol_data.h>
#include <dcp/bp/bp_shm_control_segment.h>
#include <dcp/bp/bp_management_payload.h>


using std::size_t;
using dcp::ShmException;
using dcp::SharedMemBuffer;

using namespace std::chrono_literals;

namespace dcp::bp {

  // ------------------------------------------------------------------

  void send_transmit_payload_confirmation (BPRuntimeData& runtime,
					   BPShmControlSegment& CS,
					   byte* buffer_seg_ptr,
					   DcpStatus status_code)
  {
    // in this function we assume we have the shared memory lock

    // Check first whether generation of confirms is suppressed
    if (not CS.generateTransmitPayloadConfirms)
      return;
    
    if (CS.rbFree.isEmpty())
      {
	BOOST_LOG_SEV(log_mgmt_payload, trivial::fatal) << "send_transmit_payload_confirmation: no free buffer available in shared memory.";
	runtime.bp_exitFlag = true;
	return;
      }

    if (CS.rbTransmitPayloadConfirm.isFull())
      {
	BOOST_LOG_SEV(log_mgmt_payload, trivial::fatal) << "send_transmit_payload_confirmation: no indication buffer available in shared memory.";
	runtime.bp_exitFlag = true;
	return;
      }
    
    SharedMemBuffer buff = CS.rbFree.pop();
    BPTransmitPayload_Confirm conf;
    conf.status_code = status_code;
    buff.write_to (buffer_seg_ptr, (byte*) &conf, sizeof(conf));
    CS.rbTransmitPayloadConfirm.push (buff);
  }
  
  // ------------------------------------------------------------------

  void handle_payload_from_client (BPRuntimeData& runtime)
  {
    if (not runtime.bp_isActive) return;
    if (runtime.bp_exitFlag) return;

    // iterate over all registered protocols to see whether a new
    // payload is waiting in their respective shared memory area. The
    // payloads are then transferred into newly allocated memory
    // blocks and the shared memory buffers are returned into the pool.

    for (auto it = runtime.clientProtocols.begin(); it != runtime.clientProtocols.end(); ++it)
      {
	BPClientProtocolData& clProt = it->second;

	if ((clProt.sharedMemoryAreaPtr == nullptr) or (clProt.controlSegmentPtr == nullptr))
	  {
	    BOOST_LOG_SEV(log_mgmt_payload, trivial::fatal) << "handle_payload_from_client: protocol has no valid shared memory area"
						    << ", protocolId = " << clProt.protocolId
						    << ", protocolName = " << clProt.protocolName;
	    runtime.bp_exitFlag = true;
	    return;
	  }



	BPShmControlSegment& CS = *(clProt.controlSegmentPtr);
	byte* buffer_seg_ptr    = (byte*) clProt.sharedMemoryAreaPtr->getBufferSegmentPtr();

	ScopedShmControlSegmentLock lock (CS);
	
	while (not (CS.rbTransmitPayloadRequest.isEmpty()))
	  {	
	    SharedMemBuffer buff = CS.rbTransmitPayloadRequest.pop();
	    byte*  data_ptr = buffer_seg_ptr + buff.data_offs();
	
	    BOOST_LOG_SEV(log_mgmt_payload, trivial::trace) << "handle_payload_from_client: got payload" << ", protocolId = " << clProt.protocolId << ", protocolName = " << clProt.protocolName << " in buffer " << buff << " and buffer_seg_ptr = " << (void*) buffer_seg_ptr;
	    
	    BPTransmitPayload_Request* pRequest = (BPTransmitPayload_Request*) data_ptr;

	    if (pRequest == nullptr)
	      {
		BOOST_LOG_SEV(log_mgmt_payload, trivial::fatal) << "handle_payload_from_client: protocol has no valid address for buffer, protocolId = " << clProt.protocolId;
		runtime.bp_exitFlag = true;
		return;
	      }

	    if (pRequest->s_type != stBPTransmitPayload)
	      {
		BOOST_LOG_SEV(log_mgmt_payload, trivial::info) << "handle_payload_from_client: service request has wrong type " << pRequest->s_type
							       << ", dropping it. ProtocolId = " << clProt.protocolId
							       << ", first bytes = " << byte_array_to_string (data_ptr, sizeof(BPTransmitPayload_Request))
		  ;
		send_transmit_payload_confirmation (runtime, CS, buffer_seg_ptr, BP_STATUS_ILLEGAL_SERVICE_TYPE);
		buff.clear();
		CS.rbFree.push (buff);
		continue;
	      }

	    if (pRequest->protocolId != clProt.protocolId)
	      {
		BOOST_LOG_SEV(log_mgmt_payload, trivial::info) << "handle_payload_from_client: service request has wrong protocol id, dropping it. "
						       << "Received protocol id = " << pRequest->protocolId
						       << ", expected protocol id = " << clProt.protocolId;
		send_transmit_payload_confirmation (runtime, CS, buffer_seg_ptr, BP_STATUS_WRONG_PROTOCOL_TYPE);
		buff.clear();
		CS.rbFree.push (buff);
		continue;
	      }

	    if (    (pRequest->length > clProt.maxPayloadSize)
		 || (pRequest->length.val > (runtime.bp_config.bp_conf.mtuSize - (dcp::bp::BPHeaderT::fixed_size() + dcp::bp::BPPayloadHeaderT::fixed_size()))))
	      {
		BOOST_LOG_SEV(log_mgmt_payload, trivial::info) << "handle_payload_from_client: service request has too much data, dropping it. "
						       << "Received data length = " << pRequest->length
						       << ", maxPayloadSize = " << clProt.maxPayloadSize;
		send_transmit_payload_confirmation (runtime, CS, buffer_seg_ptr, BP_STATUS_PAYLOAD_TOO_LARGE);
		buff.clear();
		CS.rbFree.push (buff);
		continue;
	      }

	    if (pRequest->length == 0)
	      {
		BOOST_LOG_SEV(log_mgmt_payload, trivial::info) << "handle_payload_from_client: service request has no data, dropping it.";
		send_transmit_payload_confirmation (runtime, CS, buffer_seg_ptr, BP_STATUS_EMPTY_PAYLOAD);
		buff.clear();
		CS.rbFree.push (buff);
		continue;
	      }

	    if ((clProt.queueingMode == BP_QMODE_ONCE) or (clProt.queueingMode == BP_QMODE_REPEAT))
	      {
		BOOST_LOG_SEV(log_mgmt_payload, trivial::trace) << "handle_payload_from_client: payload for QMODE_ONCE or QMODE_REPEAT";

		if (clProt.bufferOccupied)
		  {
		    CS.buffer.clear();
		    CS.rbFree.push (CS.buffer);
		  }

		CS.buffer = buff;
		clProt.bufferOccupied = true;

		send_transmit_payload_confirmation (runtime, CS, buffer_seg_ptr, BP_STATUS_OK);
		
		continue;
	      }

	    if ((clProt.queueingMode == BP_QMODE_QUEUE_DROPTAIL) or (clProt.queueingMode == BP_QMODE_QUEUE_DROPHEAD))
	      {
		BOOST_LOG_SEV(log_mgmt_payload, trivial::trace) << "handle_payload_from_client: payload for QMODE_QUEUE_DROPTAIL or QMODE_QUEUE_DROPHEAD";
		
		if ((clProt.queueingMode == BP_QMODE_QUEUE_DROPTAIL) and (CS.queue.stored_elements () >= clProt.maxEntries))
		  {
		    BOOST_LOG_SEV(log_mgmt_payload, trivial::trace) << "handle_payload_from_client: dropping new payload in QMODE_QUEUE_DROPTAIL";
		    buff.clear();
		    CS.rbFree.push (buff);
		    send_transmit_payload_confirmation (runtime, CS, buffer_seg_ptr, BP_STATUS_OK);
		    continue;
		  }

		if ((clProt.queueingMode == BP_QMODE_QUEUE_DROPHEAD) and (CS.queue.stored_elements () >= clProt.maxEntries))
		  {
		    SharedMemBuffer tmpbuff = CS.queue.pop();
		    tmpbuff.clear();
		    CS.rbFree.push (tmpbuff);
		  }

		CS.queue.push (buff);
		send_transmit_payload_confirmation (runtime, CS, buffer_seg_ptr, BP_STATUS_OK);
		continue;
	      }
	    

	    // unknown condition
	    BOOST_LOG_SEV(log_mgmt_payload, trivial::fatal) << "handle_payload_from_client: I should not have reached this point, exiting. Protocol id = " << clProt.protocolId;
	    runtime.bp_exitFlag = true;
	    return;
	  }
      }
  }

  // ------------------------------------------------------------------

  void management_thread_payload (BPRuntimeData& runtime)
  {
    BOOST_LOG_SEV(log_mgmt_payload, trivial::info) << "Starting payload management thread.";
    
    while (not runtime.bp_exitFlag)
      {
	std::this_thread::sleep_for (20ms);
	runtime.clientProtocols_mutex.lock();
	handle_payload_from_client (runtime);
	runtime.clientProtocols_mutex.unlock();
      }
    
    BOOST_LOG_SEV(log_mgmt_payload, trivial::info) << "Stopping payload management thread.";
  }
  
};  // namespace dcp::bp
