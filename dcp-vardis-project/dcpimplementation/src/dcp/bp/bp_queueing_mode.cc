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



#include <exception>
#include <stdexcept>
#include <dcp/bp/bp_queueing_mode.h>

namespace dcp::bp {

  std::string bp_queueing_mode_to_string (BPQueueingMode qm)
  {
    switch (qm)
      {
      case BP_QMODE_ONCE             :  return "BP_QMODE_ONCE";
      case BP_QMODE_REPEAT           :  return "BP_QMODE_REPEAT";
      case BP_QMODE_QUEUE_DROPTAIL   :  return "BP_QMODE_QUEUE_DROPTAIL";
      case BP_QMODE_QUEUE_DROPHEAD   :  return "BP_QMODE_QUEUE_DROPHEAD";
      default:
	throw std::invalid_argument("queueing_mode_to_string: unknown queueing mode");
      }
    return "";
  }
  
};  // namespace dcp::bp
