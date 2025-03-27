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
#include <dcp/bp/bpclient_lib.h>
#include <dcp/bp/bp_service_primitives.h>
#include <dcp/srp/srp_transmitter.h>
#include <dcp/srp/srp_logging.h>


using dcp::bp::BPTransmitPayload_Request;


namespace dcp::srp {

  // -----------------------------------------------------------------
  
  void transmitter_thread (SRPRuntimeData& runtime)
  {
    BOOST_LOG_SEV(log_tx, trivial::info) << "Starting transmit thread.";

    BPShmControlSegment& CS = *runtime.pSCS;
    uint16_t          sleep_time         = runtime.srp_config.srp_conf.srpGenerationPeriodMS;
    uint16_t          keepalive_timeout  = runtime.srp_config.srp_conf.srpKeepaliveTimeoutMS;
    NodeIdentifierT   own_node_id        = runtime.srp_store.get_own_node_identifier ();
    
    while (not runtime.srp_exitFlag)
      {
	std::this_thread::sleep_for (std::chrono::milliseconds (sleep_time));
				     
	if (not runtime.srp_store.get_srp_isactive())
	  continue;

	ScopedOwnSDMutex own_sd_lock (runtime);

	if (not runtime.srp_store.get_own_safety_data_written_flag ())
	  continue;
	
	TimeStampT curr_time = TimeStampT::get_current_system_time();
	TimeStampT past_time = runtime.srp_store.get_own_safety_data_timestamp();

	// do not generate payload if there has been no new safety
	// data for a while
	if (curr_time.milliseconds_passed_since(past_time) >= keepalive_timeout)
	  {
	    if (runtime.srp_store.get_own_safety_data_written_flag ())
	      BOOST_LOG_SEV(log_tx, trivial::info) << "Stop sending own safety data after not being updated for a while.";
	    runtime.srp_store.set_own_safety_data_written_flag (false);
	    continue;
	  }

	byte pld_buffer [sizeof(BPTransmitPayload_Request) + sizeof(ExtendedSafetyDataT) + 16];
	BPTransmitPayload_Request*  pldReq_ptr = new (pld_buffer) BPTransmitPayload_Request;
	byte* pld_ptr = pld_buffer + sizeof(BPTransmitPayload_Request);
	pldReq_ptr->protocolId = BP_PROTID_SRP;
	pldReq_ptr->length     = sizeof(ExtendedSafetyDataT);
	ExtendedSafetyDataT* pESD = new (pld_ptr) ExtendedSafetyDataT;
	pESD->safetyData = runtime.srp_store.get_own_safety_data ();
	pESD->nodeId     = own_node_id;
	pESD->timeStamp  = TimeStampT::get_current_system_time();
	pESD->seqno      = runtime.srp_store.get_own_sequence_number ();
	runtime.srp_store.set_own_sequence_number (pESD->seqno + 1);

	DcpStatus retval = CS.transmit_payload (BPLengthT(sizeof(BPTransmitPayload_Request) + sizeof(ExtendedSafetyDataT)), pld_buffer);
	if (retval != BP_STATUS_OK)
	  {
	    BOOST_LOG_SEV(log_tx, trivial::fatal) << "transmit payload request failed, status = "
						  << bp_status_to_string (retval)
						  << ". Exiting.";
	    runtime.srp_exitFlag = true;
	    return;
	  }	
      }
    BOOST_LOG_SEV(log_tx, trivial::info) << "Exiting transmit thread.";
  }
  
};  // namespace dcp::srp
