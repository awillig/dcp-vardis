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
#include <functional>
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
#include <dcp/bp/bp_transmissible_types.h>
#include <dcp/bp/bp_service_primitives.h>
#include <dcp/bp/bp_logging.h>
#include <dcp/bp/bp_client_protocol_data.h>
#include <dcp/bp/bp_shm_control_segment.h>
#include <dcp/bp/bp_management_payload.h>


using std::size_t;
using dcp::ShmException;

using namespace std::chrono_literals;

namespace dcp::bp {

  // ------------------------------------------------------------------
  
  void send_transmit_payload_confirmation (BPRuntimeData& runtime,
					   BPShmControlSegment& CS,
					   DcpStatus status_code)
  {
    // Check first whether generation of confirms is suppressed
    if (not CS.generateTransmitPayloadConfirms)
      return;

    bool timed_out = false;
    PushHandler handler = [&] (byte* memaddr, size_t max_buffer_size)
    {
      if (sizeof(BPTransmitPayload_Confirm) > max_buffer_size)
	{
	  BOOST_LOG_SEV(log_mgmt_payload, trivial::fatal) << "send_transmit_payload_confirmation: buffer too small for payload. Exiting.";
	  runtime.bp_exitFlag = true;
	  return (size_t) 0;
	}
      
      BPTransmitPayload_Confirm* pConf = new (memaddr) BPTransmitPayload_Confirm;
      pConf->status_code = status_code;
      return sizeof(BPTransmitPayload_Confirm);
    };

    
    CS.pqTransmitPayloadConfirm.push_wait (handler, timed_out);

    if (timed_out)
      {
	BOOST_LOG_SEV(log_mgmt_payload, trivial::fatal) << "send_transmit_payload_confirmation: timeout for confirm queue. Exiting.";
	runtime.bp_exitFlag = true;
	return;
      }
  }
  
  // ------------------------------------------------------------------

  void handle_payload_from_client (BPRuntimeData& runtime)
  {
    // iterate over all registered protocols to see whether a new
    // payload is waiting in their respective shared memory area. The
    // payloads are then transferred into newly allocated memory
    // blocks and the shared memory buffers are returned into the pool.

    for (auto it = runtime.clientProtocols.begin(); it != runtime.clientProtocols.end(); ++it)
      {
	BPClientProtocolData& clProt = it->second;

	// when we are exiting or inactive, simply delete all payloads
	// that came from higher layers
	// #####ISSUE: This code needs to go elsewhere
	if ((not runtime.bp_isActive) or runtime.bp_exitFlag)
	  {
	    clProt.pSCS->queue.reset ();
	    clProt.pSCS->buffer.reset ();
	    continue;
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
