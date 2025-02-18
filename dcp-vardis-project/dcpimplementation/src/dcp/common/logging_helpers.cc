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



#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>
#include <dcp/common/logging_helpers.h>
#include <dcp/common/exceptions.h>


namespace logging  = boost::log;
namespace sinks    = boost::log::sinks;
namespace sources  = boost::log::sources;
namespace expr     = boost::log::expressions;
namespace attrs    = boost::log::attributes;
namespace keywords = boost::log::keywords;
namespace trivial  = boost::log::trivial;

namespace dcp {

  trivial::severity_level   minimumSeverityLevel;
  
  trivial::severity_level string_to_severity_level (const std::string& str)
  {
    if (str == "trace")   return trivial::trace;
    if (str == "debug")   return trivial::debug;
    if (str == "info")    return trivial::info;
    if (str == "warning") return trivial::warning;
    if (str == "error")   return trivial::error;
    if (str == "fatal")   return trivial::fatal;
    
    throw dcp::LoggingException ("unknown logging severity_level string");
    
    return trivial::fatal;
  }


  std::string severity_level_to_string (const trivial::severity_level level)
  {
    if (level == trivial::trace)    return "trace";
    if (level == trivial::debug)    return "debug";
    if (level == trivial::info)     return "info";
    if (level == trivial::warning)  return "warning";
    if (level == trivial::error)    return "error";
    if (level == trivial::fatal)    return "fatal";
    
    throw dcp::LoggingException ("unknown logging severity_level value");
    
    return "";
  }
  
  
  void initialize_file_logging(const dcp::LoggingConfigurationBlock& cfg)
  {
    minimumSeverityLevel = dcp::string_to_severity_level (cfg.minimumSeverityLevel);
    
    logging::add_common_attributes();

    if (not cfg.loggingToConsole)
      {
	logging::add_file_log
	  (
	   keywords::file_name = cfg.logfileNamePrefix + "_%N.log",
	   keywords::rotation_size = cfg.rotationSize,
	   keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
	   keywords::auto_flush = true,
	   keywords::format =
	   (
	    expr::stream
	    << std::setw(8) << std::setfill('0') 
	    << expr::attr< unsigned int >("LineID")
	    << " -- ["
	    << expr::format_date_time<boost::posix_time::ptime>("TimeStamp","%H:%M:%S.%f")
	    << "] [" << expr::attr < std::string > ("Channel")
	    << "] [" << trivial::severity
	    << "]: " << expr::smessage
	    )
	   );
      }
    else
      {
	logging::add_console_log (
				  std::cout,
				  keywords::format =
				  (
				   expr::stream
				   << std::setw(8) << std::setfill('0') 
				   << expr::attr< unsigned int >("LineID")
				   << " -- ["
				   << expr::format_date_time<boost::posix_time::ptime>("TimeStamp","%H:%M:%S.%f")
				   << "] [" << expr::attr < std::string > ("Channel")
				   << "] [" << trivial::severity
				   << "]: " << expr::smessage
				   )
				  );
				  
      }
  }


  void LoggingConfigurationBlock::add_options (po::options_description& cfgdesc)
  {
    cfgdesc.add_options()
      
      // logging parameters

      (opt("loggingToConsole").c_str(),  po::value<bool>(&loggingToConsole)->default_value(defaultValueLoggingToConsole), txt("Whether to redirect logging output to console").c_str())
      (opt("filenamePrefix").c_str(),    po::value<std::string>(&logfileNamePrefix)->default_value(defaultValueLogfileNamePrefix), txt("prefix for log file names").c_str())
      (opt("autoFlush").c_str(),         po::value<bool>(&logAutoFlush)->default_value(defaultValueLogAutoFlush), txt("whether or not to flush buffer after each write to log").c_str())
      (opt("severityLevel").c_str(),     po::value<std::string>(&minimumSeverityLevel)->default_value(defaultValueMinimumSeverityLevel), txt("minimum severity level for logging").c_str())
      (opt("rotationSize").c_str(),      po::value<size_t>(&rotationSize)->default_value(defaultValueRotationSize), txt("maximum size of one log file before rotation").c_str())
      ;
  }

  
  void LoggingConfigurationBlock::validate ()
  {
    /***********************************
     * checks for logging options
     **********************************/
    
    if (logfileNamePrefix.empty()) throw ConfigurationException("no log file name prefix given");
    try {
      string_to_severity_level (minimumSeverityLevel);
    }
    catch (...) {
      throw ConfigurationException ("unknown logging severity level");
    }
    if (rotationSize < 1024*1024) throw ConfigurationException("minimum rotation size of 1 MB expected");
  }

  


  
};  // namespace dcp
