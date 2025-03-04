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

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <dcp/vardis/vardis_configuration.h>

/**
 * @brief This module provides pre-defined logging channels for the Vardis demon
 */

namespace sources  = boost::log::sources;
namespace trivial  = boost::log::trivial;

typedef sources::severity_channel_logger<trivial::severity_level, std::string> logger_type;

namespace dcp::vardis {
  
  extern logger_type log_tx;             /*!< Logger for transmitter thread */
  extern logger_type log_rx;             /*!< Logger for receiver thread */
  extern logger_type log_mgmt_command;   /*!< Logger for thread handling command socket towards Vardis clients */
  extern logger_type log_mgmt_rtdb;      /*!< Logger for thread handling RTDB service invocations via shared memory */
  extern logger_type log_main;           /*!< Logger for main thread of Vardis demon */


  /**
   * @brief Initialize logging (set output format etc)
   */
  void initialize_logging (const VardisConfiguration& bpcfg);
  
};  // namespace dcp::vardis
