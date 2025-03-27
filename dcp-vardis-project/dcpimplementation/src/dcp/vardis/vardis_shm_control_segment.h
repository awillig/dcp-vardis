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

#include <dcp/common/sharedmem_finite_queue.h>
#include <dcp/vardis/vardis_constants.h>
#include <dcp/vardis/vardis_service_primitives.h>


namespace dcp::vardis {


  /**
   * @brief Maximum length of any finite queue for RTDB service
   * requests or confirms
   */
  const uint64_t maxServicePrimitiveQueueLength = 30;


  /**
   * @brief Maximum length of any possible RTDB service request (plus
   *        some safety margin)
   */
  static const size_t    maxRTDBServiceBufferSize   = MAX_maxValueLength + MAX_maxDescriptionLength + VarCreateT::fixed_size() + 16;


  /**
   * @brief Maximum length of any possible RTDB service confirm
   *        (outside RTDB-Read, which is handled elsewhere), plus some
   *        safety margin
   */
  static const size_t    maxRTDBConfirmBufferSize   = sizeof(RTDB_Create_Confirm) + 16;


  /**
   * @brief Type for a RTDB serivce request finite queue
   */
  typedef ShmFiniteQueue<maxServicePrimitiveQueueLength, maxRTDBServiceBufferSize>   PayloadQueue;


  /**
   * @brief Type for a RTDB service confirm finite queue
   */
  typedef ShmFiniteQueue<maxServicePrimitiveQueueLength, maxRTDBConfirmBufferSize>   ConfirmQueue;


  /**
   * @brief This type describes the shared memory structure used for
   *        exchanging most service primitives related to variables as
   *        such (RTDB-Create, -Update, -Delete). RTDB-Reads are
   *        handled through the VardisStore.
   *
   * For all of the four services two ring buffers / cyclic queues are
   * provided, the requests flow from the Vardis client to the Vardis
   * demon, the confirms flow in the opposite direction.
   *
   * There is a separate shared memory area between the Vardis demon
   * and each Vardis client.
   */
  typedef struct VardisShmControlSegment {

    PayloadQueue pqCreateRequest;   /*!< queue for create requests */
    PayloadQueue pqDeleteRequest;   /*!< queue for delete requests */
    PayloadQueue pqUpdateRequest;   /*!< queue for update requests */
    ConfirmQueue pqCreateConfirm;   /*!< queue for create confirms */
    ConfirmQueue pqDeleteConfirm;   /*!< queue for delete confirms */
    ConfirmQueue pqUpdateConfirm;   /*!< queue for update confirms */
    
    
    
    /**
     * @brief Constructor, initializes all the queues, and moves all
     *        the used SharedMemBuffers into the free list
     */
    VardisShmControlSegment ()
      : pqCreateRequest ("RTDB-Create request", maxServicePrimitiveQueueLength),
	pqDeleteRequest ("RTDB-Delete request", maxServicePrimitiveQueueLength),
	pqUpdateRequest ("RTDB-Update request", maxServicePrimitiveQueueLength),	
	pqCreateConfirm ("RTDB-Create confirm", maxServicePrimitiveQueueLength),
	pqDeleteConfirm ("RTDB-Delete confirm", maxServicePrimitiveQueueLength),
	pqUpdateConfirm ("RTDB-Update confirm", maxServicePrimitiveQueueLength)
    {
    };


    /**
     * @brief Returns string representation of the occupancy of all queues
     */
    std::string report_stored_buffers ()
    {
      std::stringstream ss;

      ss << "pqCreateRequest.stored = " << pqCreateRequest.stored_elements ()
	 << ", pqCreateConfirm.stored = " << pqCreateConfirm.stored_elements ()
	 << ", pqDeleteRequest.stored = " << pqDeleteRequest.stored_elements ()
	 << ", pqCeleteConfirm.stored = " << pqDeleteConfirm.stored_elements ()
	 << ", pqUpdateRequest.stored = " << pqUpdateRequest.stored_elements ()
	 << ", pqUpdateConfirm.stored = " << pqUpdateConfirm.stored_elements ()
	;
      
      return ss.str();
    };

  } VardisShmControlSegment;
  
};  // namespace dcp::vardis
