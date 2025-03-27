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
//#include <dcp/common/shared_mem_area.h>
#include <dcp/bp/bp_transmissible_types.h>
#include <dcp/bp/bp_queueing_mode.h>
#include <dcp/bp/bp_shm_control_segment.h>

using std::ostream;


namespace dcp::bp {



  /**
   * @brief This class contains the data that the BP demon keeps about
   *        a client protocol.
   *
   * Note that the buffer or queue associated with a client is not
   * stored here but in the referenced shared memory segment for the
   * client. Each client protocol has its separate shared memory
   * segment.
   */
  class BPClientProtocolData {

  public:
    
    /**************************************************************
     * Main entries required for core BP operation
     *************************************************************/

    /**
     * @brief Contains static information about a BP client protocol
     *        (e.g. name, queueing mode etc).
     */
    BPStaticClientInfo static_info;


    /**
     * @brief Time stamp for registration
     */
    TimeStampT   timeStampRegistration;


    /**************************************************************
     * Entries for shared memory communication with client protocol
     *************************************************************/


    /**
     * @brief Pointer to shared memory area descriptor data structure
     */
    std::shared_ptr<ShmStructureBase>    pSSB;


    /**
     * @brief Pointer to the memory address of shared memory
     *        area. Valid after successful registration of client
     *        protocol
     */
    BPShmControlSegment*                 pSCS = nullptr;

    
    
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


    BPClientProtocolData () {};

    BPClientProtocolData (const char* area_name, BPStaticClientInfo static_info, bool gen_pld_confirms);


    ~BPClientProtocolData ();

  };
  
};  // namespace dcp::bp
