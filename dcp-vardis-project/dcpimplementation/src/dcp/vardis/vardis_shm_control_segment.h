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

namespace dcp::vardis {


  const uint64_t maxServicePrimitiveQueueLength = 30;


  /**
   * @brief This type describes the shared memory structure used for
   *        exchanging all service primitives related to variables as
   *        such (RTDB-Create, -Read, -Update, -Delete).
   *
   * For all of the four services two ring buffers / cyclic queues are
   * provided, the requests flow from the Vardis client to the Vardis
   * demon, the confirms flow in the opposite direction.
   *
   * There is a separate shared memory area between the Vardis demon
   * and each Vardis client.
   */
  typedef struct VardisShmControlSegment : public ShmControlSegmentBase {
    
    RingBufferFree rbFree;                               /*!< List of free buffers */
    RingBufferNormal rbCreateRequest, rbCreateConfirm;   /*!< Request and confirm queues for RTDB-Create */
    RingBufferNormal rbDeleteRequest, rbDeleteConfirm;   /*!< Request and confirm queues for RTDB-Delete */
    RingBufferNormal rbUpdateRequest, rbUpdateConfirm;   /*!< Request and confirm queues for RTDB-Update */
    RingBufferNormal rbReadRequest, rbReadConfirm;       /*!< Request and confirm queues for RTDB-Read */
    
    
    
    
    VardisShmControlSegment () = delete;
    

    /**
     * @brief Constructor, initializes all the queues, and moves all
     *        the used SharedMemBuffers into the free list
     */
    VardisShmControlSegment (ShmBufferPool& shmBuffPool,
			     uint64_t numberBuffers)
      : rbFree ("free list", numberBuffers),
	rbCreateRequest ("RTDB-Create request", maxServicePrimitiveQueueLength),
	rbCreateConfirm ("RTDB-Create confirm", maxServicePrimitiveQueueLength),
	rbDeleteRequest ("RTDB-Delete request", maxServicePrimitiveQueueLength),
	rbDeleteConfirm ("RTDB-Delete confirm", maxServicePrimitiveQueueLength),
	rbUpdateRequest ("RTDB-Update request", maxServicePrimitiveQueueLength),
	rbUpdateConfirm ("RTDB-Update confirm", maxServicePrimitiveQueueLength),
	rbReadRequest ("RTDB-Create request", maxServicePrimitiveQueueLength),
	rbReadConfirm ("RTDB-Create confirm", maxServicePrimitiveQueueLength)      
    {
      // set up all buffers and put them into free list
      size_t act_buffer_size    =  shmBuffPool.get_actual_buffer_size ();
      byte*  buffer_start       =  shmBuffPool.getBufferSegmentPtr ();
      uint64_t needed_buffers   =  get_minimum_number_buffers_required ();

      if (act_buffer_size == 0)             throw ShmException ("VardisShmControlSegment::ctor: actual buffer size is zero");
      if (buffer_start    == nullptr)       throw ShmException ("VardisShmControlSegment::ctor: invalid buffer start pointer");
      if (needed_buffers  > numberBuffers)  throw ShmException ("VardisShmControlSegment::ctor: insufficient number of buffers");

      // put all but one buffer into the free list
      for (uint64_t i = 0; i<numberBuffers-1; i++)
	{
	  SharedMemBuffer buff (act_buffer_size, i, i * act_buffer_size);
	  rbFree.push (buff);
	}
    };


    /**
     * @brief Computes the minimum number of SharedMemBuffers required
     *        (including some slack)
     */
    static uint64_t get_minimum_number_buffers_required ()
    {
      return 9*maxServicePrimitiveQueueLength + 10;
    };


    /**
     * @brief Returns string representation of the occupancy of all queues
     */
    std::string report_stored_buffers ()
    {
      std::stringstream ss;

      ss << "VardisShmControlSegment: rbFree.stored = " << rbFree.stored_elements ()

	 << ", rbCreateRequest.stored = " << rbCreateRequest.stored_elements ()
	 << ", rbCreateConfirm.stored = " << rbCreateConfirm.stored_elements ()

	 << ", rbDeleteRequest.stored = " << rbDeleteRequest.stored_elements ()
	 << ", rbCeleteConfirm.stored = " << rbDeleteConfirm.stored_elements ()

	 << ", rbUpdateRequest.stored = " << rbUpdateRequest.stored_elements ()
	 << ", rbUpdateConfirm.stored = " << rbUpdateConfirm.stored_elements ()

	 << ", rbReadRequest.stored = " << rbReadRequest.stored_elements ()
	 << ", rbReadConfirm.stored = " << rbReadConfirm.stored_elements ()
 
	;
      
      return ss.str();
    };

  } VardisShmControlSegment;
  
};  // namespace dcp::vardis
