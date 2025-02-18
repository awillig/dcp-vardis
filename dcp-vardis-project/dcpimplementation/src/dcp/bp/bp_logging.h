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
#include <dcp/common/logging_helpers.h>
#include <dcp/bp/bp_configuration.h>

/**
 * @brief This module provides the logging streams for the BP demon
 */

namespace dcp {

  namespace bp {
  
    /**
     * \brief Predefined loggers for BP
     */
    extern logger_type log_tx;            /*!< Logging stream for transmitter thread */
    extern logger_type log_rx;            /*!< Logging stream for receiver thread */
    extern logger_type log_mgmt_command;  /*!< Logging stream for thread handling command socket */
    extern logger_type log_mgmt_payload;  /*!< Logging stream for thread handling shared memory areas / payload exchange */
    extern logger_type log_main;          /*!< Logging stream for main BP demon program */


    /**
     * @brief Initializes all logging streams and sets logging output format
     */
    void initialize_logging (const LoggingConfigurationBlock& logcfg);

  };  // namespace bp
    
};  // namespace dcp
