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


#include <dcp/bp/bp_shm_control_segment.h>


namespace dcp::bp {

  BPShmControlSegment::BPShmControlSegment (BPStaticClientInfo  static_ci, bool gen_pld_confirms)
    : pqTransmitPayloadConfirm ("transmit-payload-confirms", maxQueueLength),
      pqReceivePayloadIndication ("receive-payload-indications", maxQueueLength),
      queue ("payload-queue", std::max((uint16_t) 1, static_ci.maxEntries)),
      buffer ("payload-buffer", 1),
      generateTransmitPayloadConfirms (gen_pld_confirms),
      static_client_info (static_ci)
  {
  }

  std::string BPShmControlSegment::report_stored_buffers ()
  {
    std::stringstream ss;
    
    unsigned int queue_size, queue_free;
    unsigned int buffer_size, buffer_free;
    unsigned int conf_size, conf_free;
    unsigned int ind_size, ind_free;
    queue.report_sizes (queue_size, queue_free);
    buffer.report_sizes (buffer_size, buffer_free);
    pqTransmitPayloadConfirm.report_sizes (conf_size, conf_free);
    pqReceivePayloadIndication.report_sizes (ind_size, ind_free);
    
    ss << "BPShmControlSegment: queue.stored = " << queue_size
       << ", queue.free = " << queue_free
       << ", buffer.stored = " << buffer_size
       << ", buffer.free = " << buffer_free
       << ", payloadConfirm.stored = " << conf_size
       << ", payloadConfirm.free = " << conf_free
       << ", payloadIndication.stored = " << ind_size
       << ", payloadIndication.free = " << ind_free
      ;
    return ss.str();
  }
    
  DcpStatus BPShmControlSegment::transmit_payload (PushHandler handler)
  {
    bool timed_out;
    BPQueueingMode queueingMode = static_client_info.queueingMode;
    
    if (   (queueingMode == BP_QMODE_ONCE)
	   or (queueingMode == BP_QMODE_REPEAT))
      {
	buffer.reset ();
	buffer.push_wait (handler, timed_out);
	
	if (timed_out) 
	  return BP_STATUS_INTERNAL_SHARED_MEMORY_ERROR;	
	else 
	  return BP_STATUS_OK;    
      }
    
    
    if (   (queueingMode == BP_QMODE_QUEUE_DROPTAIL)
	   or (queueingMode == BP_QMODE_QUEUE_DROPHEAD))
      {
	
	if (queueingMode == BP_QMODE_QUEUE_DROPHEAD)
	  {
	    queue.push_wait_force (handler, timed_out);
	    if (timed_out)
	      return BP_STATUS_INTERNAL_SHARED_MEMORY_ERROR;
	    else
	      return BP_STATUS_OK;	      
	  }
	
	
	if (    (queueingMode == BP_QMODE_QUEUE_DROPTAIL)
		and (queue.stored_elements () >= static_client_info.maxEntries))
	  {
	    return BP_STATUS_OK;
	  }
	
	queue.push_wait (handler, timed_out);
	if (timed_out)
	  return BP_STATUS_INTERNAL_SHARED_MEMORY_ERROR;
	else
	  return BP_STATUS_OK;	      	  
      }
    
    return BP_STATUS_INTERNAL_ERROR;
  }
  
  

  DcpStatus BPShmControlSegment::transmit_payload (BPLengthT length, byte* payload)
  {
    if ((length == 0) or (!payload))
      return BP_STATUS_EMPTY_PAYLOAD;
    
    if (length > (static_client_info.maxPayloadSize + sizeof(BPTransmitPayload_Request)))
      return  BP_STATUS_PAYLOAD_TOO_LARGE;
    
    PushHandler handler = [&] (byte* memaddr, size_t)
    {
      std::memcpy (memaddr, payload, (size_t) length.val);
      return (size_t) length.val;
    };
    return transmit_payload (handler);      
  }
  
};  // namespace dcp::bp


  

