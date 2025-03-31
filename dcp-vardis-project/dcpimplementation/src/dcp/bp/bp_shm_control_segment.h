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

#include <dcp/common/global_types_constants.h>
#include <dcp/common/sharedmem_finite_queue.h>
#include <dcp/bp/bp_service_primitives.h>



/**
 * @brief This module defines the structure of a shared memory control
 *        segment between the BP demon and a BP client protocol
 *
 * The control segment contains some finite queues: one for the BP
 * queue associated with a client, one for the buffer associated with
 * a client, and two queues for transmit payload confirmations and
 * receive payload indications.
 *
 * All the queues are realized as shared memory finite queues.
 *
 */


using dcp::PushHandler;

namespace dcp::bp {

  
  typedef struct BPShmControlSegment {

    /**
     * @brief Maximum length of any of the queues in this class
     */
    static const uint64_t maxQueueLength     = 10;


    /**
     * @brief Maximum size of a payload buffer, given as maximum
     *        payload size plus some margin
     */
    static const size_t   maxBufferSize      = maxBeaconPayloadSize + 128;


    /**
     * @brief Maximum size of a confirmation buffer, given by
     *        confirmation size plus some safety margin
     */
    static const size_t   confirmBufferSize  = sizeof(BPTransmitPayload_Confirm) + 128;


    /**
     * @brief Type representing finite queue holding a payload
     */
    typedef ShmFiniteQueue<maxQueueLength, maxBufferSize>      PayloadQueue;


    /**
     * @brief Type representing a finite queue holding a confirmation
     */
    typedef ShmFiniteQueue<maxQueueLength, confirmBufferSize>  ConfirmQueue;

    
    ConfirmQueue  pqTransmitPayloadConfirm;      /*!< Output queue of BPTransmitPayload confirms */
    PayloadQueue  pqReceivePayloadIndication;    /*!< Output queue with BPReceivePayload indications */
    
    PayloadQueue  queue;     /*!< This is where buffers for the queue-based queueing modes are stored */
    PayloadQueue  buffer;    /*!< This is the buffer for QMODE_ONCE and QMODE_REPEAT */

    bool generateTransmitPayloadConfirms; /*!< Indicate whether payload confirmations should be generated */

    
    BPStaticClientInfo static_client_info; /*!< Static information about BP client protocol (e.g. name, queueing mode) */

    
    BPShmControlSegment () = delete;


    /**
     * @brief Constructor, initializes control segment in shared memory area
     *
     * @param static_ci: contains static BP client protocol parameters
     *        (e.g. name, queueing mode etc)
     * @param gen_pld_confirms: specify whether or not the client
     *        protocol wants BP instance to generate payload confirm
     *        primitives
     *
     * This assumes that the shared memory area is already available
     */
    BPShmControlSegment (BPStaticClientInfo  static_ci, bool gen_pld_confirms);


    /**
     * @brief Report current state of control segment as a string (for
     *        logging/debugging purposes)
     */
    std::string report_stored_buffers ();



    /**
     * @brief Method for transmitting a payload by providing a
     *        PushHandler (from the finite queue module)
     *
     * @param handler: push handler constructing the payload in
     *        place. When the push handler returns zero, no payload is
     *        placed.
     *
     * This is useful for a BP client protocol to request transmission
     * of a payload from the BP, it essentially implements the
     * BP-TransmitPayload service.
     */
    DcpStatus transmit_payload (PushHandler handler);
      

    /**
     * @brief Method for transmitting a payload by providing the
     *        payload directly as a buffer
     *
     * @param length: size of the payload to be transmitted
     * @param payload: pointer to the actual payload
     *
     * This is useful for a BP client protocol to request transmission
     * of a payload from the BP, it essentially implements the
     * BP-TransmitPayload service.
     */
    DcpStatus transmit_payload (BPLengthT length, byte* payload);

    
    
  } BPShmControlSegment;  
}
