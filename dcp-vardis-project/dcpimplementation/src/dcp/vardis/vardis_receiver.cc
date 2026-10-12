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
#include <dcp/bp/bpclient_lib.h>
#include <dcp/vardis/vardis_configuration.h>
#include <dcp/vardis/vardis_logging.h>
#include <dcp/vardis/vardis_process_payload.h>
#include <dcp/vardis/vardis_receiver.h>

using dcp::bp::BPLengthT;
using dcp::bp::BPStatus;

namespace dcp::vardis {

  static const uint16_t rx_buffer_length = 4000;
  
  // -----------------------------------------------------------------
  
  void receiver_thread (VardisRuntimeData& runtime)
  {
    DCPLOG_INFO(log_rx) << "Starting receive thread.";

    try {
      while (not runtime.vardis_exitFlag)
	{				     
	  if (not runtime.protocol_data.vardis_store.get_vardis_isactive())
	    {
	      std::this_thread::sleep_for (std::chrono::milliseconds (100));
	      continue;
	    }
	  
	  // check if we have received a payload
	  BPLengthT result_length = 0;
	  BPStatus rx_stat;
	  byte rx_buffer [rx_buffer_length];
	  bool more_payloads = false;
	  
	  do {
	    rx_stat = runtime.receive_payload_wait (result_length, rx_buffer, more_payloads, runtime.vardis_exitFlag);
	    
	    if ((result_length > 0) && (rx_stat == BPStatus::Ok))
	      {
		DCPLOG_TRACE(log_rx)
		  << "Processing payload of length "
		  << result_length;
		MemoryChunkDisassemblyArea area ("vd-rx", (size_t) result_length.val, rx_buffer);
		ScopedVariableStoreMutex mtx (runtime);
		process_received_payload (runtime.protocol_data, area);
	      }
	    else
	      {
		if (rx_stat != BPStatus::Ok)
		  DCPLOG_INFO(log_rx)
		    << "Retrieving received payload issued error "
		    << to_string (rx_stat);
	      }
	  } while (more_payloads);
	}
    }
    catch (DcpException& e)
      {
	DCPLOG_FATAL(log_rx)
	  << "Caught DCP exception in Vardis receiver main loop. "
	  << "Exception type: " << e.ename()
	  << ", module: " << e.modname()
	  << ", message: " << e.what()
	  << ". Exiting.";
	runtime.vardis_exitFlag = true;
      }
    catch (std::exception& e)
      {
	DCPLOG_FATAL(log_rx)
	  << "Caught other exception in Vardis receiver main loop. "
	  << "Message: " << e.what()
	  << ". Exiting.";
	runtime.vardis_exitFlag = true;
      }

    
    DCPLOG_INFO(log_rx) << "Exiting receive thread.";
  }
    
};  // namespace dcp::vardis
