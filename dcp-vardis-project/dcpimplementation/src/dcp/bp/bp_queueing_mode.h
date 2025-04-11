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
#include <string>

namespace dcp::bp {

  /**
   * @brief This module provides definition of the queueing mode type,
   *        a set of pre-defined queueing mode values, and a helper
   *        function to convert a queueing mode to a string
   */

  
  enum BPQueueingMode {
    BP_QMODE_ONCE             = 0,
    BP_QMODE_REPEAT           = 1,
    BP_QMODE_QUEUE_DROPTAIL   = 2,
    BP_QMODE_QUEUE_DROPHEAD   = 3
  };

  /**
   * @brief Converts queueing mode to string
   *
   * Throws for unknown queueing mode.
   */
  std::string bp_queueing_mode_to_string (BPQueueingMode qm);
  
};  // namespace dcp::bp
