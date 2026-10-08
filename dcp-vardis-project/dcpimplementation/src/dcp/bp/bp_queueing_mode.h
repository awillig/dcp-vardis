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


/**
 * @brief This module provides definition of the queueing mode type,
 *        a set of pre-defined queueing mode values, and a helper
 *        function to convert a queueing mode to a string
 */


namespace dcp::bp {

   enum class BPQueueingMode {Once, Repeat, Q_DropTail, Q_DropHead };
  
};  // namespace dcp::bp


/**
 * @brief Converts queueing mode to string
 *
 * Throws for unknown queueing mode.
 */
constexpr std::string_view to_string (dcp::bp::BPQueueingMode qm) {
  switch (qm)
    {
    case dcp::bp::BPQueueingMode::Once        :  return "BP_QMODE_ONCE";
    case dcp::bp::BPQueueingMode::Repeat      :  return "BP_QMODE_REPEAT";
    case dcp::bp::BPQueueingMode::Q_DropTail  :  return "BP_QMODE_QUEUE_DROPTAIL";
    case dcp::bp::BPQueueingMode::Q_DropHead  :  return "BP_QMODE_QUEUE_DROPHEAD";
    default:
      throw std::invalid_argument("to_string: unknown queueing mode");
    }
  return "";
}


std::ostream& operator<< (std::ostream& os, const dcp::bp::BPQueueingMode qm);
