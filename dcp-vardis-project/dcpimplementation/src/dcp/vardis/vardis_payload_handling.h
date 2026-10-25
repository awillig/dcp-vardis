/**
 * Copyright (C) 2026 Andreas Willig, University of Canterbury
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
#include <format>
#include <string>
#include <stdexcept>
#include <dcp/common/area.h>
#include <dcp/vardis/vardis_protocol_data.h>


/**
 * @brief This module provides top-level functions for Vardis payload
 *        processing, one to construct an outgoing payload, and one to
 *        process an incoming payload, available in a disassembly
 *        area. This module assumes that all non-protocol aspects like
 *        locking etc. are handled elsewhere.
 */


namespace dcp::vardis {

  /**
   * @brief Processes an incoming Vardis payload by executing all
   *        instruction records in the order given in the
   *        specification, with no regard for locking etc.
   *
   * @param  prot_data: Protocol data object containing methods to
   *         process individual instruction records
   * @param  area: Disassembly area containing the received payload
   *
   * Can throw upon processing errors.
   */

  void process_received_payload (VardisProtocolData& prot_data, DisassemblyArea& area); 


  /**
   * @brief Constructs an outgoing payload
   *
   * @param prot_data: Protocol data object containing methods to
   *        construct the different types of instruction containers
   * @param area: Assembly area in which to construct the payload
   * @param containers_added: output parameter counting how many
   *        Vardis instruction containers have been added to the
   *        payload
   *
   */
  void construct_payload (VardisProtocolData& prot_data,
			  AssemblyArea& area,
			  unsigned int& containers_added);
  
};  // namespace dcp::vardis
