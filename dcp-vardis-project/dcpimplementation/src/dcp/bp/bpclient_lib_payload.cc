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

  
  void submit_transmit_payload_request (BPShmControlSegment& CS,
					byte* buffer_seg_ptr,
					BPProtocolIdT protId,
					BPLengthT length,
					byte* payload)
  {
    ScopedShmControlSegmentLock lock (CS);

    if (CS.rbTransmitPayloadRequest.isFull())      throw BPClientLibException ("submit_transmit_payload_request: rbTransmitPayloadRequest is full");    
    if (CS.rbFree.isEmpty())                       throw BPClientLibException ("submit_transmit_payload_request: rbFree is empty");    
    if (not CS.rbTransmitPayloadConfirm.isEmpty()) throw BPClientLibException ("submit_transmit_payload_request: rbTransmitPayloadConfirm is not empty");

    
    SharedMemBuffer shmBuff = CS.rbFree.pop();

    if (shmBuff.max_length() < length.val + sizeof(BPTransmitPayload_Request)) throw BPClientLibException ("submit_transmit_payload_request: payload does not fit into shared-memory buffer");

    byte* data_ptr = buffer_seg_ptr + shmBuff.data_offs ();

    BPTransmitPayload_Request pldReq;
    pldReq.protocolId = protId;
    pldReq.length     = length;

    std::memcpy (data_ptr, &pldReq, sizeof(BPTransmitPayload_Request));
    std::memcpy (data_ptr + sizeof(BPTransmitPayload_Request), payload, length.val);
    shmBuff.set_used_length (length.val + sizeof(BPTransmitPayload_Request));

    CS.rbTransmitPayloadRequest.push (shmBuff);
  }


  // ---------------------------------------------------------------

  
  DcpStatus BPClientRuntime::transmit_payload (BPLengthT length, byte* payload)
  {
    if (not _isRegistered)
      throw BPClientLibException ("transmit_payload: not registered with BP");
    
    if (length == 0)
      throw BPClientLibException ("transmit_payload: length is zero");
    if (payload == nullptr)
      throw BPClientLibException ("transmit_payload: no payload given");

    // get hold of the shared memory area
    if (bp_shm_area_ptr == nullptr)
      throw BPClientLibException ("transmit_payload: invalid shared memory area reference");

    BPShmControlSegment* control_seg_ptr = (BPShmControlSegment*) bp_shm_area_ptr->getControlSegmentPtr();
    byte               * buffer_seg_ptr  = bp_shm_area_ptr->getBufferSegmentPtr();

    if ((control_seg_ptr == nullptr) or (buffer_seg_ptr == nullptr))
      throw BPClientLibException ("transmit_payload: invalid shared memory area pointer(s)");

    BPShmControlSegment& CS = *control_seg_ptr;

    submit_transmit_payload_request (CS, buffer_seg_ptr, get_protocol_id(), length, payload);

    if (generateTransmitPayloadConfirms)
      {
	DcpStatus retval;
    
	auto process = [&retval] (SharedMemBuffer& buff, byte* data_ptr)
	{
	  BPTransmitPayload_Confirm* pConf = (BPTransmitPayload_Confirm*) data_ptr;
	  retval = pConf->status_code;
	  buff.clear();
	};
	
	CS.rbTransmitPayloadConfirm.wait_pop_process_push (CS, buffer_seg_ptr, CS.rbFree, process);
	
	return retval;
      }
    else
      {
	return BP_STATUS_OK;
      }
  }


  // ---------------------------------------------------------------
  

  DcpStatus BPClientRuntime::receive_payload (BPLengthT& result_length, byte* result_buffer, bool& more_payloads)
  {
    if (not _isRegistered)
      throw BPClientLibException ("receive_payload: not registered with BP");
    
    BPLengthT max_length = get_max_payload_size();
    
    if (max_length == 0)
      throw BPClientLibException ("receive_payload: max_length is zero");
    if (result_buffer == nullptr)
      throw BPClientLibException ("receive_payload: no result buffer given");

    result_length = 0;
    more_payloads = false;
    
    // get hold of the shared memory area
    if (bp_shm_area_ptr == nullptr)
      throw BPClientLibException ("receive_payload: invalid shared memory area reference");

    BPShmControlSegment* control_seg_ptr = (BPShmControlSegment*) bp_shm_area_ptr->getControlSegmentPtr();
    byte               * buffer_seg_ptr  = bp_shm_area_ptr->getBufferSegmentPtr();

    if ((control_seg_ptr == nullptr) or (buffer_seg_ptr == nullptr))
      throw BPClientLibException ("receive_payload: invalid shared memory area pointer(s)");

    BPShmControlSegment& CS = *control_seg_ptr;

    ScopedShmControlSegmentLock lock (CS);

    if (CS.rbReceivePayloadIndication.isEmpty())
      return BP_STATUS_OK;
    
    if (CS.rbFree.isFull())
      throw BPClientLibException ("receive_payload: free list is full");
    
    SharedMemBuffer shmBuff = CS.rbReceivePayloadIndication.pop();
    byte* data_ptr = buffer_seg_ptr + shmBuff.data_offs();

    BPReceivePayload_Indication* pInd         = (BPReceivePayload_Indication*) data_ptr;
    byte*                        payload_ptr  = data_ptr + sizeof(BPReceivePayload_Indication);              

    if (pInd->s_type != stBP_ReceivePayload)
      throw BPClientLibException ("receive_payload: incorrect service type");
    if (pInd->length == 0)
      throw BPClientLibException ("receive_payload: got payload of zero length");
    
    if (pInd->length > max_length) {
      shmBuff.clear();
      CS.rbFree.push (shmBuff);
      return BP_STATUS_PAYLOAD_TOO_LARGE;
    }

    result_length = pInd->length;
    std::memcpy (result_buffer, payload_ptr, result_length.val);

    shmBuff.clear();
    CS.rbFree.push (shmBuff);

    more_payloads = not CS.rbReceivePayloadIndication.isEmpty();
    
    return BP_STATUS_OK;
  }

  // ---------------------------------------------------------------
  
};  // namespace dcp
