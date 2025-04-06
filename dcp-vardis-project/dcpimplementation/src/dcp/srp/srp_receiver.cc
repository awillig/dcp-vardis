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
#include <dcp/common/services_status.h>
#include <dcp/bp/bpclient_lib.h>
#include <dcp/srp/srp_logging.h>
#include <dcp/srp/srp_receiver.h>
#include <dcp/srp/srp_transmissible_types.h>



namespace dcp::srp {

  static const uint16_t rx_buffer_length = 1000;
  
  // -----------------------------------------------------------------
  
  void receiver_thread (SRPRuntimeData& runtime)
  {
    BOOST_LOG_SEV(log_rx, trivial::info) << "Starting receive thread.";

    try {
      while (not runtime.srp_exitFlag)
	{				     
	  if (not runtime.srp_store.get_srp_isactive())
	    {
	      std::this_thread::sleep_for (std::chrono::milliseconds (100));
	      continue;
	    }
	  
	  // check if we have received a payload
	  BPLengthT result_length = 0;
	  DcpStatus rx_stat;
	  byte rx_buffer [rx_buffer_length];
	  bool more_payloads = false;
	  
	  do {
	    rx_stat = runtime.receive_payload_wait (result_length, rx_buffer, more_payloads, runtime.srp_exitFlag);
	    
	    if ((rx_stat == BP_STATUS_OK) and (result_length == sizeof(ExtendedSafetyDataT)))
	      {
		BOOST_LOG_SEV(log_rx, trivial::trace) << "Processing payload of correct length";
		ExtendedSafetyDataT *pESD = (ExtendedSafetyDataT*) rx_buffer;
		
		if (pESD->nodeId == runtime.srp_store.get_own_node_identifier ())
		  continue;
		
		ScopedNeighbourTableMutex mtx (runtime);
		runtime.srp_store.insert_esd_entry (*pESD);	      
	      }
	    else
	      {
		if (rx_stat != BP_STATUS_OK)
		  {
		    BOOST_LOG_SEV(log_rx, trivial::info) << "Retrieving received payload issued error " << bp_status_to_string (rx_stat);
		  }
		else if (result_length != 0)
		  {
		    BOOST_LOG_SEV(log_rx, trivial::info) << "Retrieving received payload had wrong length " << result_length;
		  }
	      }
	  } while (more_payloads);
	}
    }
    catch (DcpException& e)
      {
	BOOST_LOG_SEV(log_rx, trivial::fatal)
	  << "Caught DCP exception in SRP receiver main loop. "
	  << "Exception type: " << e.ename()
	  << ", module: " << e.modname()
	  << ", message: " << e.what()
	  << "Exiting.";
	runtime.srp_exitFlag = true;
      }
    catch (std::exception& e)
      {
	BOOST_LOG_SEV(log_rx, trivial::fatal)
	  << "Caught other exception in SRP receiver main loop. "
	  << "Message: " << e.what()
	  << "Exiting.";
	runtime.srp_exitFlag = true;
      }

    
    BOOST_LOG_SEV(log_rx, trivial::info) << "Exiting receive thread.";
  }
  
};  // namespace dcp::srp
