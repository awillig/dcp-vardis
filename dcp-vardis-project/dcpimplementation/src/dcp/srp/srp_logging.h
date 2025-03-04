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
#include <dcp/srp/srp_configuration.h>

/**
 * @brief This module provides pre-defined logging channels for the SRP demon
 */

namespace sources  = boost::log::sources;
namespace trivial  = boost::log::trivial;

typedef sources::severity_channel_logger<trivial::severity_level, std::string> logger_type;

namespace dcp::srp {
  
  extern logger_type log_tx;             /*!< Logger for transmitter thread */
  extern logger_type log_rx;             /*!< Logger for receiver thread */
  extern logger_type log_scrub;          /*!< Logger for scrubbing thread */
  extern logger_type log_main;           /*!< Logger for main thread of SRP demon */


  /**
   * @brief Initialize logging (set output format etc)
   */
  void initialize_logging (const SRPConfiguration& bpcfg);
  
};  // namespace dcp::srp
