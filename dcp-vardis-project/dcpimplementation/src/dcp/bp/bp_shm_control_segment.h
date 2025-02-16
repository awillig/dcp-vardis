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


#pragma once

#include <iostream>
#include <dcp/common/shared_mem_area.h>

using dcp::RingBuffer;
using dcp::SharedMemBuffer;


/**
 * @brief This module defines the structure of a shared memory control
 *        segment between the BP demon and a BP client protocol
 *
 * The control segment contains a list of free SharedMemBuffer buffers
 * ('free list'), an input queue (from client protocol to BP demon),
 * an output queue (from BP demon to client protocol) and the actual
 * payload queue / buffer associated to a client protocol.
 *
 */


namespace dcp::bp {

  /**
   * @brief Maximum length of input and output queues
   */
  const uint64_t maxServicePrimitiveQueueLength = 10;
  
  typedef struct BPShmControlSegment : public ShmControlSegmentBase {
    
    RingBufferFree     rbFree;                        /*!< free list of shared mem buffers */
    RingBufferNormal   rbTransmitPayloadRequest;      /*!< Input queue of BPTransmitPayload requests from client protocol */
    RingBufferNormal   rbTransmitPayloadConfirm;      /*!< Output queue of BPTransmitPayload confirms */
    RingBufferNormal   rbReceivePayloadIndication;    /*!< Output queue with BPReceivePayload indications */

    
    RingBufferNormal queue;    /*!< This is where buffers for the queue-based queueing modes are stored */
    SharedMemBuffer buffer;    /*!< This is the buffer for QMODE_ONCE and QMODE_REPEAT */

    bool generateTransmitPayloadConfirms; /*!< Indicate whether payload confirmations should be generated */

    
    BPShmControlSegment () = delete;


    /**
     * @brief Constructor, initializes control segment in shared memory area
     *
     * @param shmBuffPool: contains description of shared memory area,
     *        including pointers to control and buffer segment
     * @param numberBuffers: number of SharedMemBuffers to allocate
     * @param maxEntries: number of entries in the payload queue
     * @param gen_pld_confirms: specify whether or not the client
     *        protocol wants BP instance to generate payload confirm
     *        primitives
     *
     * This assumes that the shared memory area is already available
     */
    BPShmControlSegment (ShmBufferPool& shmBuffPool,
			 uint64_t  numberBuffers,
			 uint16_t  maxEntries,
			 bool      gen_pld_confirms)
      : rbFree ("free list", numberBuffers),
	rbTransmitPayloadRequest ("transmit-payload-requests", maxServicePrimitiveQueueLength),
	rbTransmitPayloadConfirm ("transmit-payload-confirms", maxServicePrimitiveQueueLength),
	rbReceivePayloadIndication ("receive-payload-indications", maxServicePrimitiveQueueLength),
	queue("payload-queue", std::max((uint16_t) 1, maxEntries)),
	generateTransmitPayloadConfirms (gen_pld_confirms)
    {
      // set up all buffers and put them into free list
      size_t   act_buffer_size   = shmBuffPool.get_actual_buffer_size ();
      byte*    buffer_start      = shmBuffPool.getBufferSegmentPtr ();
      uint64_t needed_buffers    = get_minimum_number_buffers_required ((uint64_t) maxEntries);

      if (act_buffer_size == 0)             throw ShmException ("BPShmControlSegment: actual buffer size is zero");
      if (buffer_start    == nullptr)       throw ShmException ("BPShmControlSegment: invalid buffer start pointer");
      if (needed_buffers  > numberBuffers)  throw ShmException ("BPShmControlSegment: insufficient number of buffers");
      if (maxEntries > maxRingBufferElements_Normal - 1) throw ShmException ("BPShmControlSegment: maxEntries is too large");

      // put all but one buffer into the free list
      for (uint64_t i = 0; i<numberBuffers-1; i++)
	{
	  SharedMemBuffer buff (act_buffer_size, i, i * act_buffer_size);
	  rbFree.push (buff);
	}

      // and reserve the last ShmBuffer for, well, the buffer
      // (according to the BP specification)
      buffer = SharedMemBuffer (act_buffer_size, numberBuffers-1, (numberBuffers-1) * act_buffer_size);
    };


    /**
     * @brief Calculates minimum number of SharedMemBuffers required
     */
    static uint64_t get_minimum_number_buffers_required (uint64_t queue_len)
    {
      return queue_len + 1 + 3*maxServicePrimitiveQueueLength;
    };


    /**
     * @brief Report current state of control segment as a string (for
     *        logging/debugging purposes)
     */
    std::string report_stored_buffers ()
    {
      std::stringstream ss;

      ss << "BPShmControlSegment: rbFree.stored = " << rbFree.stored_elements ()
	 << ", rbTransmitPayloadRequest.stored = " << rbTransmitPayloadRequest.stored_elements ()
	 << ", rbTransmitPayloadConfirm.stored = " << rbTransmitPayloadConfirm.stored_elements ()
	 << ", rbReceivePayloadIndication.stored = " << rbReceivePayloadIndication.stored_elements ()
	 << ", queue.stored = " << queue.stored_elements ()
	 << ", buffer.used_length = " << buffer.used_length ();
      
      return ss.str();
    };
    
  } BPShmControlSegment;
  
}
