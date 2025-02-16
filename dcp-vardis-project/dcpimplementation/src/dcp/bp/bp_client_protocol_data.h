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

#include <queue>
#include <iostream>
#include <memory>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/memblock.h>
#include <dcp/common/shared_mem_area.h>
#include <dcp/bp/bp_transmissible_types.h>
#include <dcp/bp/bp_queueing_mode.h>
#include <dcp/bp/bp_shm_control_segment.h>

using std::ostream;
using dcp::ShmBufferPool;


namespace dcp::bp {


  typedef MemBlock BPBufferEntry;


  /**
   * @brief This class contains the data that the BP demon keeps about
   *        a client protocol.
   *
   * Note that the buffer or queue associated with a client is not
   * stored here but in the referenced shared memory segment for the
   * client. Each client protocol has its separate shared memory
   * segment.
   */
  typedef struct BPClientProtocolData {

    /**************************************************************
     * Main entries required for core BP operation
     *************************************************************/
    
    BPProtocolIdT     protocolId;       /*!< Protocol identifier of client protocol */
    std::string       protocolName;     /*!< Textual name of client protocol */
    BPLengthT         maxPayloadSize;   /*!< Maximum payload size for this client protocol */
    BPQueueingMode    queueingMode;     /*!< Queueing mode for this client protocol */
    uint16_t          maxEntries;       /*!< Maximum number of queue entries for one of the BP_QMODE_QUEUE_* queueing modes */

    TimeStampT   timeStampRegistration;           /*!< Time at which protocol was registered */
    bool         bufferOccupied = false;          /*!< Buffer occupancy flag for BP_QMODE_ONCE and BP_QMODE_REPEAT */
    bool         allowMultiplePayloads = false;   /*!< Can multiple payloads for this client protocol go into one beacon */


    /**************************************************************
     * Entries for shared memory communication with client protocol
     *************************************************************/


    /**
     * @brief Pointer to shared memory area descriptor data structure
     */
    std::shared_ptr<ShmBufferPool>   sharedMemoryAreaPtr;


    /**
     * @brief Points to the start of the shared memory control segment
     *        in BP demon address space
     *
     * Initialized during successful protocol registration
     */
    BPShmControlSegment* controlSegmentPtr = nullptr;
    
    
    /**************************************************************
     * Statistics
     *************************************************************/

    /**
     * @brief Keeping track of outgoing and incoming payload transmissions
     *        as well as losses at the finite-size queue
     *
     * A payload is counted as outgoing when it has been transferred into
     * a beacon
     */
    unsigned int cntOutgoingPayloads         = 0;
    unsigned int cntReceivedPayloads         = 0;
    unsigned int cntDroppedOutgoingPayloads  = 0;
    unsigned int cntDroppedIncomingPayloads  = 0;
    
  } BPClientProtocolData;
  
};  // namespace dcp::bp
