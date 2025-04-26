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

#include <exception>
#include <iostream>
#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <dcp/common/configuration.h>



namespace sources  = boost::log::sources;
namespace trivial  = boost::log::trivial;

typedef sources::severity_channel_logger<trivial::severity_level, std::string> logger_type;


/**
 * @brief This module contains some helper functions and classes supporting logging.
 *
 * This module uses BOOST's built-in support for logfile rotation,
 * automatic flushing, and choosing between logging to a file or to
 * console.
 */

namespace dcp {


  /**
   * @brief Contains the minimum severity level required for logging
   */
  extern trivial::severity_level   minimumSeverityLevel;
  

  /**
   * @brief Converts a string to boost::log::trivial::severity_level. Throws exception when unknown.
   */
  trivial::severity_level string_to_severity_level (const std::string& str);


  /**
   * @brief Converts a boost::log::trivial::severity_level to a string
   */
  std::string severity_level_to_string (const trivial::severity_level level);


  /**
   * @brief Type containing all the configuration values for logging
   *
   * The logging configuration data will have to be in its own section
     in a configuration file, with default block name 'logging'.
   */
  typedef struct LoggingConfigurationBlock : public DcpConfigurationBlock {

    /**
     * @brief Default values for the logging configuration values below
     */
    std::string defaultValueLogfileNamePrefix     = "dcp-log";
    bool        defaultValueLogAutoFlush          = true;
    std::string defaultValueMinimumSeverityLevel  = "warning";
    std::size_t defaultValueRotationSize          = 10485760;
    bool        defaultValueLoggingToConsole      = false;

    /**************************************************
     * Logging options
     *************************************************/

    /**
     * @brief Whether to use file or console logging
     */
    bool loggingToConsole  =  defaultValueLoggingToConsole;

    
    /**
     * @brief Prefix for the logfile name to be used
     */
    std::string    logfileNamePrefix = defaultValueLogfileNamePrefix;
    
    
    /**
     * @brief Whether or not logfile is flushed after each write
     *
     * Flushing comes with performance penalty, but ensures output
     * even in the case of crashes
     */
    bool           logAutoFlush = defaultValueLogAutoFlush;
    
    
    /**
     * @brief Minimum severity level required for logging (including)
     */
    std::string    minimumSeverityLevel  = defaultValueMinimumSeverityLevel;
    
    
    /**
     * @brief Maximum size one log file can reach before rotation
     */
    std::size_t    rotationSize = defaultValueRotationSize;
    
    /**************************************************
     * Logging options
     *************************************************/

    LoggingConfigurationBlock () : DcpConfigurationBlock ("logging") {} ;
    LoggingConfigurationBlock (std::string bname) : DcpConfigurationBlock (bname) {};


    /**
     * @brief Add logging configuration to description of configuration file format
     */
    virtual void add_options (po::options_description& cfgdesc);


    /**
     * @brief Validate logging configuration. Throws in case of invalid configuration
     */
    virtual void validate ();
    
    
  } LoggingConfigurationBlock;


  /**
   * @brief Initializes logging from the configuration data given in
   *        DcpConfiguration
   */
  void initialize_file_logging(const LoggingConfigurationBlock& cfg);

};  // namespace dcp
  

/**
 * @brief A set of macros to act as wrapper around logging calls for
 *        either BOOST logging (in the implementation) or OMNeT++
 *        logging (in the simulation)
 */
#ifdef __DCPSIMULATION__
#include <omnetpp.h>
using namespace omnetpp;
#define DCPLOG_TRACE(logstream) EV_TRACE
#define DCPLOG_INFO(logstream) EV_INFO
#define DCPLOG_WARNING(logstream) EV_WARN  
#define DCPLOG_ERROR(logstream) EV_ERROR
#define DCPLOG_FATAL(logstream) EV_FATAL
#else
#define DCPLOG_TRACE(logstream) BOOST_LOG_SEV(logstream,trivial::trace)
#define DCPLOG_INFO(logstream) BOOST_LOG_SEV(logstream,trivial::info)
#define DCPLOG_WARNING(logstream) BOOST_LOG_SEV(logstream,trivial::warning)
#define DCPLOG_ERROR(logstream) BOOST_LOG_SEV(logstream,trivial::error)
#define DCPLOG_FATAL(logstream) BOOST_LOG_SEV(logstream,trivial::fatal)
#endif
  

