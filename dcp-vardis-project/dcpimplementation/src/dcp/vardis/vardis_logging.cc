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


#include <iostream>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>
#include <dcp/common/logging_helpers.h>
#include <dcp/vardis/vardis_logging.h>


namespace logging  = boost::log;
namespace sinks    = boost::log::sinks;
namespace sources  = boost::log::sources;
namespace expr     = boost::log::expressions;
namespace attrs    = boost::log::attributes;
namespace keywords = boost::log::keywords;
namespace trivial  = boost::log::trivial;


using std::cout;
using std::endl;


BOOST_LOG_ATTRIBUTE_KEYWORD(a_severity, "Severity", trivial::severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(a_channel, "Channel", std::string)


namespace dcp::vardis {

  logger_type log_tx             (keywords::channel = "TX");
  logger_type log_rx             (keywords::channel = "RX");
  logger_type log_mgmt_command   (keywords::channel = "MGMT-COMMAND");
  logger_type log_mgmt_rtdb      (keywords::channel = "MGMT-RTDB");
  logger_type log_main           (keywords::channel = "MAIN");
  
  void initialize_logging(const VardisConfiguration& vdcfg)
  {
    initialize_file_logging (vdcfg.logging_conf);
    
    logging::core::get()->set_filter
      (
       (a_channel == "TX" && a_severity >= minimumSeverityLevel) ||
       (a_channel == "RX" && a_severity >= minimumSeverityLevel) ||
       (a_channel == "MGMT-COMMAND" && a_severity >= minimumSeverityLevel) ||
       (a_channel == "MGMT-RTDB" && a_severity >= minimumSeverityLevel) ||
       (a_channel == "MAIN" && a_severity >= minimumSeverityLevel)
       ); 
  }
  

};  // namespace dcp
