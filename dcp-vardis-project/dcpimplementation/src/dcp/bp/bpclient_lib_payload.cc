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


#include <thread>
#include <chrono>
#include <dcp/bp/bpclient_lib.h>
#include <dcp/bp/bp_shm_control_segment.h>
#include <dcp/bp/bp_service_primitives.h>

using dcp::bp::BPTransmitPayload_Request;
using dcp::bp::BPTransmitPayload_Confirm;

using namespace std::chrono_literals;
using namespace dcp::bp;

namespace dcp {


  // ---------------------------------------------------------------

  
  DcpStatus BPClientRuntime::transmit_payload (BPLengthT length, byte* payload)
  {
    if (not _isRegistered)  throw BPClientLibException ("transmit_payload: not registered with BP");
    if (length == 0)        throw BPClientLibException ("transmit_payload: length is zero");
    if (!payload)           throw BPClientLibException ("transmit_payload: no payload given");

    BPShmControlSegment& CS = *pSCS;

    return CS.transmit_payload (length, payload);
  }


  // ---------------------------------------------------------------

  DcpStatus BPClientRuntime::receive_payload_helper (BPLengthT& result_length,
						     byte* result_buffer,
						     bool& more_payloads,
						     bool waiting,
						     bool& exitFlag)
  {
    result_length = 0;
    more_payloads = false;
    BPLengthT max_length = static_client_info.maxPayloadSize;
    DcpStatus retval     = BP_STATUS_OK;
    
    if (not _isRegistered) throw BPClientLibException ("receive_payload: not registered with BP");
    if (max_length == 0)   throw BPClientLibException ("receive_payload: max_length is zero");
    if (!result_buffer)    throw BPClientLibException ("receive_payload: no result buffer given");

    BPShmControlSegment& CS = *pSCS;
    bool timed_out;
    PopHandler handler = [&] (const byte* memaddr, size_t len)
    {
      if (len >= sizeof(bp::BPReceivePayload_Indication))
	{
	  BPReceivePayload_Indication* pInd         = (BPReceivePayload_Indication*) memaddr;
	  const byte*                  payload_ptr  = memaddr + sizeof(BPReceivePayload_Indication);              
	  
	  if (pInd->s_type != stBP_ReceivePayload) throw BPClientLibException ("receive_payload: incorrect service type");
	  if (pInd->length == 0)                   throw BPClientLibException ("receive_payload: got payload of zero length");
	  if (pInd->length > max_length)           { retval = BP_STATUS_PAYLOAD_TOO_LARGE; return; }
	  if (pInd->length != len - sizeof(bp::BPReceivePayload_Indication)) { retval = BP_STATUS_INTERNAL_ERROR; return; }

	  result_length = pInd->length;
	  std::memcpy (result_buffer, payload_ptr, result_length.val); 
	}
    };

    if (waiting)
      {
	do {
	  CS.pqReceivePayloadIndication.pop_wait (handler, timed_out, more_payloads, 10);
	} while ((not exitFlag) and timed_out);
      }
    else
      CS.pqReceivePayloadIndication.pop_nowait (handler, timed_out, more_payloads, 10);
    return retval;
    
  }

  
  // ---------------------------------------------------------------
  

  DcpStatus BPClientRuntime::receive_payload_nowait (BPLengthT& result_length, byte* result_buffer, bool& more_payloads)
  {
    bool dummy_exitFlag = false;
    return receive_payload_helper (result_length, result_buffer, more_payloads, false, dummy_exitFlag);    
  }

  // ---------------------------------------------------------------
  
  
  DcpStatus BPClientRuntime::receive_payload_wait (BPLengthT& result_length,
						   byte* result_buffer,
						   bool& more_payloads,
						   bool& exitFlag)
  {
    return receive_payload_helper (result_length, result_buffer, more_payloads, true, exitFlag);
  }
    
  // ---------------------------------------------------------------
  
};  // namespace dcp
