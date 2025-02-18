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

#include <iostream>
#include <boost/program_options.hpp>
#include <dcp/bp/bpclient_configuration.h>

namespace po = boost::program_options;

using std::iostream;

namespace dcp::vardis {

  /**
   * @brief Default values for configuration options
   */
  const size_t     defaultValueMaxValueLength                   =  32;
  const size_t     defaultValueMaxDescriptionLength             =  32;
  const uint8_t    defaultValueMaxRepetitions                   =  1;
  const uint16_t   defaultValueMaxPayloadSize                   =  1000;
  const uint16_t   defaultValueMaxSummaries                     =  20;
  const uint16_t   defaultValuePollReceivedPayloadMS            =  25;
  const uint16_t   defaultValueQueueMaxEntries                  =  20;
  const uint16_t   defaultValuePayloadGenerationIntervalMS      =  30;
  const uint16_t   defaultValuePollRTDBServiceIntervalMS        =  25;
  const bool       defaultValueLockingForIndividualContainers   =  false;
  
  /**
   * @brief This struct contains the Vardis protocol configuration
   *        data, both those mandated by the spec and additional ones
   */
  typedef struct VardisConfigurationBlock : public DcpConfigurationBlock {
    
    /**************************************************
     * Proper Vardis options
     *************************************************/
    
    /**
     * @brief Maximum length of a variable value
     */
    size_t  maxValueLength         =  defaultValueMaxValueLength;


    /**
     * @brief Maximum length of a variable description
     */
    size_t  maxDescriptionLength   =  defaultValueMaxDescriptionLength;


    /**
     * @brief Maximum number of repetitions of a variable
     */
    uint8_t maxRepetitions         =  defaultValueMaxRepetitions;
      

    /**
     * @brief Maximum size of a Vardis payload.
     */
    uint16_t maxPayloadSize        =  defaultValueMaxPayloadSize;


    /**
     * @brief Maximum number of summaries in an outgoing payload.
     */
    uint16_t maxSummaries          =  defaultValueMaxSummaries;



    /**************************************************
     * Implementation options
     *************************************************/

    /**
     * @brief Time between polls of the shared memory towards BP to
     *        check for received payloads (in ms)
     */
    uint16_t pollReceivePayloadMS  =  defaultValuePollReceivedPayloadMS;


    /**
     * @brief Time between attempts to generate a Vardis payload and
     *        hand it over to BP
     */
    uint16_t payloadGenerationIntervalMS =  defaultValuePayloadGenerationIntervalMS;


    /**
     * @brief Time between checking shared memory towards client
     *        applications / protocols for new RTDB service primitives
     */
    uint16_t pollRTDBServiceIntervalMS   =  defaultValuePollRTDBServiceIntervalMS;
    
    
    /**
     * @brief Number of entries in BP queue for VarDis
     */
    uint16_t queueMaxEntries       =  defaultValueQueueMaxEntries;


    /**
     * @brief Process received payload in one go or with one locking
     *        operation per container
     */
    bool lockingForIndividualContainers = defaultValueLockingForIndividualContainers;
    
    
    /**************************************************
     * Methods
     *************************************************/

    /**
     * @brief Constructors, mainly setting section name in the config file
     */
    VardisConfigurationBlock () : DcpConfigurationBlock ("Vardis") {};
    VardisConfigurationBlock (std::string bname) : DcpConfigurationBlock (bname) {};


    /**
     * @brief Add Vardis configuration information to configuration options record
     */
    virtual void add_options (po::options_description& cfgdesc);


    /**
     * @brief Validate Vardis configuration data, throws if invalid
     */
    virtual void validate ();

  } VardisConfigurationBlock;
    

  // -------------------------------------------------------------------------------------


  /**
   * @brief This holds the entire configuration for the Vardis demon
   *
   * It consists of configuration blocks for logging, the actual
   * Vardis protocol configuration and for the command socket between
   * the Vardis demon and its client applications / protocols
   */
  typedef struct VardisConfiguration : public BPClientConfiguration {

    LoggingConfigurationBlock         logging_conf;
    VardisConfigurationBlock          vardis_conf;
    CommandSocketConfigurationBlock   vardis_cmdsock_conf;


    /**
     * @brief Constructor, setting section name in config file and
     *        default names for logfile, command socket towards BP,
     *        command socket towards Vardis clients, and the shared
     *        memory area towards BP
     */
    VardisConfiguration ()
      : BPClientConfiguration ("BPCommandSocket", "BPSharedMem")
      , logging_conf ()
      , vardis_conf ("Vardis")
      , vardis_cmdsock_conf ("VardisCommandSocket")
    {
      logging_conf.logfileNamePrefix         = "dcp-vardis-log";
      bp_cmdsock_conf.commandSocketFile      = "/tmp/dcp-bp-command-socket";
      bp_shm_conf.shmAreaName                = "shm-bpclient-vardis";
      vardis_cmdsock_conf.commandSocketFile  = "/tmp/dcp-vardis-command-socket";
    };
    

    /**
     * @brief Build the description of all demon configuration options
     *        for the BOOST configuration file parser
     */
    virtual void build_description (po::options_description& cfgdesc)
    {
      BPClientConfiguration::build_description (cfgdesc);
      logging_conf.add_options (cfgdesc);
      vardis_conf.add_options (cfgdesc);
      vardis_cmdsock_conf.add_options (cfgdesc);
    };

    
    /**
     * @brief Validates parameter values, throws exceptions in case of invalid values.
     */
    virtual void validate()
    {
      BPClientConfiguration::validate ();
      logging_conf.validate ();
      vardis_conf.validate ();
      vardis_cmdsock_conf.validate ();
    };
      
      
    /**
     * @brief Overloaded output operator
     * @param os: output stream to use
     * @param cfg: the VardisConfiguration object to output 
     */
    friend std::ostream& operator<<(std::ostream& os, const dcp::vardis::VardisConfiguration& cfg);
    
  } VardisConfiguration;

  
};  // namespace dcp::vardis
