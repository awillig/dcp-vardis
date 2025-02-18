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
#include <list>
#include <thread>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <tins/tins.h>
#include <dcp/common/area.h>
#include <dcp/common/memblock.h>
#include <dcp/common/shared_mem_area.h>
#include <dcp/bp/bp_client_protocol_data.h>
#include <dcp/bp/bp_logging.h>
#include <dcp/bp/bp_service_primitives.h>
#include <dcp/bp/bp_transmissible_types.h>
#include <dcp/bp/bp_transmitter.h>





using namespace Tins;


namespace dcp::bp {

  // ------------------------------------------------------------------

  void serialize_payload (BPClientProtocolData&  protEntry,
			  AssemblyArea&          area,
			  byte*                  buffer_seg_ptr,
			  SharedMemBuffer&       fromBuff,
			  unsigned int&          numPayloadsAdded)
  {
    BPTransmitPayload_Request* pldReq_ptr   = (BPTransmitPayload_Request*) (buffer_seg_ptr + fromBuff.data_offs());
    byte*                      payload_ptr  = buffer_seg_ptr + fromBuff.data_offs() + sizeof(BPTransmitPayload_Request);
    
    BOOST_LOG_SEV(log_tx, trivial::trace) << "serialize_payload: serializing payload for protocolId " << protEntry.protocolId
					  << " of length " << pldReq_ptr->length;
    
    BPPayloadHeaderT pldHdr;
    pldHdr.protocolId = protEntry.protocolId;
    pldHdr.length     = pldReq_ptr->length;
    pldHdr.serialize (area);
    
    area.serialize_byte_block (pldReq_ptr->length.val, payload_ptr);

    numPayloadsAdded++;
    protEntry.cntOutgoingPayloads++;
  }
  
  
  // ------------------------------------------------------------------

  void transfer_and_free_payload_from_queue (BPRuntimeData&          runtime,
					     BPClientProtocolData&   protEntry,
					     BPShmControlSegment&    CS,
					     byte*                   buffer_seg_ptr,
					     AssemblyArea&           area,
					     unsigned int&           numPayloadsAdded)
  {
    ScopedShmControlSegmentLock lock (CS);
    if ((not CS.queue.isEmpty()) and (not CS.rbFree.isFull()))
      {
	SharedMemBuffer tmpShmBuff = CS.queue.peek ();

	if (dcp::bp::BPPayloadHeaderT::fixed_size() + tmpShmBuff.used_length() - sizeof(BPTransmitPayload_Request)  <= area.available())
	  {		
	    SharedMemBuffer shmBuff = CS.queue.pop ();
	    serialize_payload (protEntry, area, buffer_seg_ptr, shmBuff, numPayloadsAdded);
	    shmBuff.clear();
	    CS.rbFree.push(shmBuff);
	  }
	else
	  {
	    BOOST_LOG_SEV(log_tx, trivial::fatal) << "transfer_and_free_payload_from_queue: payload is too large, dropping it";
	    runtime.bp_exitFlag = true;
	  }
      }
  }


  // ------------------------------------------------------------------

  void transfer_and_free_payload_from_buffer (BPRuntimeData&           runtime,
					      BPClientProtocolData&    protEntry,
					      BPShmControlSegment&     CS,
					      byte*                    buffer_seg_ptr,
					      AssemblyArea&            area,
					      unsigned int&            numPayloadsAdded)
  {
    ScopedShmControlSegmentLock lock (CS);
    if (CS.buffer.used_length () > 0)
      {
	if (dcp::bp::BPPayloadHeaderT::fixed_size() + CS.buffer.used_length() - sizeof(BPTransmitPayload_Request)  <= area.available())
	  {		
	    serialize_payload (protEntry, area, buffer_seg_ptr, CS.buffer, numPayloadsAdded);
	    CS.buffer.clear ();
	  }
	else
	  {
	    BOOST_LOG_SEV(log_tx, trivial::fatal) << "transfer_and_free_payload_from_buffer: payload is too large, dropping it";
	    CS.buffer.clear();
	    runtime.bp_exitFlag = true;
	  }
      }
  }


  // ------------------------------------------------------------------

  void transfer_and_leave_payload_from_buffer (BPRuntimeData&                         runtime,
					       BPClientProtocolData&                  protEntry,
					       BPShmControlSegment&                   CS,
					       byte*                                  buffer_seg_ptr,
					       AssemblyArea&                          area,
					       unsigned int&                          numPayloadsAdded)
  {
    ScopedShmControlSegmentLock lock (CS);
    if (CS.buffer.used_length () > 0)
      {
	if (dcp::bp::BPPayloadHeaderT::fixed_size() + CS.buffer.used_length() - sizeof(BPTransmitPayload_Request)  <= area.available())
	  {
	    serialize_payload (protEntry, area, buffer_seg_ptr, CS.buffer, numPayloadsAdded);
	  }
	else
	  {
	    BOOST_LOG_SEV(log_tx, trivial::fatal) << "transfer_and_leave_payload_from_buffer: payload is too large, dropping it";
	    CS.buffer.clear();
	    runtime.bp_exitFlag = true;
	  }
      }
  }
  
  // ------------------------------------------------------------------

  
  void attempt_add_payload (BPRuntimeData&          runtime,
			    BPClientProtocolData&   protEntry,
			    AssemblyArea&           area,
			    unsigned int&           numPayloadsAdded)
  {
    if (protEntry.sharedMemoryAreaPtr == nullptr)
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal) << "attempt_add_payload: shared memory area not accessible";
	runtime.bp_exitFlag = true;
	return;
      }

    BPShmControlSegment* control_seg_ptr = (BPShmControlSegment*) protEntry.sharedMemoryAreaPtr->getControlSegmentPtr();
    byte*                buffer_seg_ptr  = protEntry.sharedMemoryAreaPtr->getBufferSegmentPtr();
    if ((control_seg_ptr == nullptr) or (buffer_seg_ptr == nullptr))
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal) << "attempt_add_payload: invalid shared memory area references";
	runtime.bp_exitFlag = true;
	return;
      }

    BPShmControlSegment& CS = *control_seg_ptr;
    
    if ((protEntry.queueingMode == BP_QMODE_QUEUE_DROPTAIL) || (protEntry.queueingMode == BP_QMODE_QUEUE_DROPHEAD))
      {
	transfer_and_free_payload_from_queue (runtime, protEntry, CS, buffer_seg_ptr, area, numPayloadsAdded);
	return;
      }

    if (not protEntry.bufferOccupied)
      return;
    
    if (protEntry.queueingMode == BP_QMODE_ONCE)
      {
	transfer_and_free_payload_from_buffer (runtime, protEntry, CS, buffer_seg_ptr, area, numPayloadsAdded);
	protEntry.bufferOccupied = false;
	return;
      }
    
    if (protEntry.queueingMode == BP_QMODE_REPEAT)
      {
	transfer_and_leave_payload_from_buffer (runtime, protEntry, CS, buffer_seg_ptr, area, numPayloadsAdded);
	return;
      }

    BOOST_LOG_SEV(log_tx, trivial::fatal) << "attempt_add_payload: unknown queueing mode";
    runtime.bp_exitFlag = true;
    return;
  }
			    
  
  // ------------------------------------------------------------------

  void generate_beacon (BPRuntimeData& runtime)
  {
    if (not runtime.bp_isActive)
      return;
    
    // first determine number and total size of available payloads without
    // yet moving them into a beacon. We use a very simple method allowing
    // only one payload from each client protocol.
    bytevect               bv_payload (runtime.bp_config.bp_conf.maxBeaconSize);
    ByteVectorAssemblyArea area ("bp-tx", runtime.bp_config.bp_conf.maxBeaconSize, bv_payload);
    unsigned int           numPayloadsAdded  = 0;

    // first serialize a dummy version of the BPHeaderT. We will re-do
    // this once we know the total amount of data
    BPHeaderT tmpBPHdr;
    tmpBPHdr.serialize (area);
    
    for (auto it = runtime.clientProtocols.begin(); it != runtime.clientProtocols.end(); ++it)
      {
	BPClientProtocolData& protEntry = it->second;
	attempt_add_payload (runtime, protEntry, area, numPayloadsAdded);

	if (runtime.bp_exitFlag)
	  return;
      }

    if (numPayloadsAdded > 0)
      {
	// resize byte vector to reflect actual size, and prepend
	// BPHeaderT (can to this only at the end since we know the
	// total length only now)
	bv_payload.resize(area.used());
	BPHeaderT bpHdr;
	bpHdr.version      =   bpHeaderVersion;
	bpHdr.magicNo      =   bpMagicNo;
	bpHdr.senderId     =   runtime.ownNodeIdentifier;
	bpHdr.length       =   area.used() - dcp::bp::BPHeaderT::fixed_size();
	bpHdr.numPayloads  =   numPayloadsAdded;
	bpHdr.seqno        =   runtime.bpSequenceNumber++;

	ByteVectorAssemblyArea tmpArea ("bp-tx-tmp", dcp::bp::BPHeaderT::fixed_size(), bv_payload);
	bpHdr.serialize (tmpArea);

	RawPDU payload_pdu (bv_payload);
	EthernetII ethpacket = EthernetII(EthernetII::BROADCAST, runtime.nw_if_info.hw_addr);
	ethpacket.payload_type (runtime.bp_config.bp_conf.etherType);
	ethpacket = ethpacket / payload_pdu;

	runtime.pktSender.send (ethpacket, runtime.bp_config.bp_conf.interfaceName);	  
      }
  }
  
  // ------------------------------------------------------------------

  void transmitter_thread (BPRuntimeData& runtime)
  {
    BOOST_LOG_SEV(log_tx, trivial::info) << "Starting transmit thread.";

    boost::random::mt19937 randgen;
    int lower_bound_int = (int) floor (runtime.bp_config.bp_conf.avgBeaconPeriodMS * (1 - runtime.bp_config.bp_conf.jitterFactor));
    int upper_bound_int = (int) floor (runtime.bp_config.bp_conf.avgBeaconPeriodMS * (1 + runtime.bp_config.bp_conf.jitterFactor));

    if (lower_bound_int <= 0)
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal) << "Average beacon period (ms) has been chosen too small ("
					      << runtime.bp_config.bp_conf.avgBeaconPeriodMS
					      << "), leaving.";

	runtime.bp_exitFlag = true;
	return;
      }

    unsigned int lower_bound = (unsigned int) lower_bound_int;
    unsigned int upper_bound = (unsigned int) upper_bound_int;

    boost::random::uniform_int_distribution<> dist (lower_bound, upper_bound); 
    
    while (not runtime.bp_exitFlag)
      {
	unsigned int wait_time_ms = dist (randgen);
	std::this_thread::sleep_for (std::chrono::milliseconds (wait_time_ms));

	runtime.clientProtocols_mutex.lock();
	generate_beacon (runtime);
	runtime.clientProtocols_mutex.unlock();
      }
    BOOST_LOG_SEV(log_tx, trivial::info) << "Stopping transmit thread.";
  }

  // ------------------------------------------------------------------
  
};  // namespace dcp::bp
