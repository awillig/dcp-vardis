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

#include <cstdint>
#include <iostream>
#include <dcp/common/global_types_constants.h>
#include <dcp/bp/bp_queueing_mode.h>
#include <dcp/bp/bp_transmissible_types.h>


/**
 * @brief This module defines a data type describing the static /
 *        protocol configuration data that the BP demon holds about a
 *        client
 */


namespace dcp::bp {

  /*************************************************************************
   * Base classes and constants for primitives
   ************************************************************************/

  /**
   * @brief Maximum length of protocol name string buffer (excluding zero byte)
   */
  const size_t maximumProtocolNameLength = 127;

  /**
   * @brief All the static information the BP demon holds at runtime
   *        about a client protocol
   */
  typedef struct BPStaticClientInfo {
    BPProtocolIdT     protocolId = 0;       /*!< Protocol identifier of client protocol */
    char              protocolName[maximumProtocolNameLength+1] = "uninitialized";     /*!< Textual name of client protocol */
    BPLengthT         maxPayloadSize = 0;   /*!< Maximum payload size for this client protocol */
    BPQueueingMode    queueingMode;         /*!< Queueing mode for this client protocol */
    uint16_t          maxEntries = 0;       /*!< Maximum number of queue entries for one of the BP_QMODE_QUEUE_* queueing modes */
    bool              allowMultiplePayloads = false;   /*!< Can multiple payloads for this client protocol go into one beacon */

    friend std::ostream& operator<<(std::ostream& os, const BPStaticClientInfo& ci);
  } BPStaticClientInfo;    

};  // namespace dcp::bp
