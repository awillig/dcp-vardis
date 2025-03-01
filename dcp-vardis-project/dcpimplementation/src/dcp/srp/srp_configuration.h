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
#include <dcp/common/command_socket.h>
#include <dcp/common/shared_mem_area.h>
#include <dcp/bp/bpclient_configuration.h>
#include <dcp/srp/srp_constants.h>

namespace po = boost::program_options;

/**
 * @brief This module provides data types and operation concerning the
 *        configuration of the SRP demon
 */


namespace dcp::srp {

  
  const uint16_t    defaultValueSrpGenerationPeriodMS   = 100;
  const uint16_t    defaultValueSrpReceptionPeriodMS    = 100;
  const uint16_t    defaultValueSrpScrubbingPeriodMS    = 500;
  const uint16_t    defaultValueSrpKeepaliveTimeoutMS   = 5000;
  const uint16_t    defaultValueSrpScrubbingTimeoutMS   = 3000;
  

  /**
   * @brief This structure holds the configuration data for the core
   *        SRP demon
   */
  typedef struct SRPConfigurationBlock : public DcpConfigurationBlock {

    /**************************************************
     * Proper SRP options
     *************************************************/


    /**
     * @brief Period between submitting SRP payloads to BP in ms
     */
    uint16_t srpGenerationPeriodMS = defaultValueSrpGenerationPeriodMS;


    /**
     * @brief Period between attempting to retrieve received payloads in ms
     */
    uint16_t srpReceptionPeriodMS  = defaultValueSrpReceptionPeriodMS;
    

    /**
     * @brief Period between scrubbing runs
     */
    uint16_t srpScrubbingPeriodMS  = defaultValueSrpScrubbingPeriodMS;

    
    /**
     * @brief If the own SafetyDataT record has not been updated for
     *        this long, then payload generation is suppressed
     */
    uint16_t srpKeepaliveTimeoutMS = defaultValueSrpKeepaliveTimeoutMS;
    

    /**
     * @brief If a neighbours ExtendedSafetyDataT record has not been
     *        updated for this long, it is dropped from the neighbour
     *        table (scrubbing process)
     */
    uint16_t srpScrubbingTimeoutMS = defaultValueSrpScrubbingTimeoutMS;
    
    
    /**
     * @brief Constructors, mainly for setting section names in the
     *        config file
     */
    SRPConfigurationBlock () : DcpConfigurationBlock ("SRP") {};
    SRPConfigurationBlock (std::string bname) : DcpConfigurationBlock (bname) {};


    /**
     * @brief Add SRP option descriptions to config file reader
     */
    virtual void add_options (po::options_description& cfgdesc);


    /**
     * @brief Validate SRP configuration data. Throws if data is invalid.
     */
    virtual void validate ();

    
  } SRPConfigurationBlock;


  
  /**
   * @brief This struct contains the full configuration data for SRP
   *        to operate on.
   */
  typedef struct SRPConfiguration : public BPClientConfiguration {

    LoggingConfigurationBlock         logging_conf; /*!< Logging configuration */
    SRPConfigurationBlock             srp_conf;     /*!< Actual SRP configuration data */
    SharedMemoryConfigurationBlock    shm_conf;     /*!< Shared memory configuration for neighbour store */


    /**
     * @brief Constructor, setting section name in config file and
     *        default name for command socket towards BP
     */
    SRPConfiguration ()
      : BPClientConfiguration ("BPCommandSocket", "BPSharedMem"),
	logging_conf (),
	srp_conf (),
	shm_conf ("SRPNeighbourStoreShm", defaultSRPNeighbourStoreShmName)
    {
    };


    /**
     * @brief Build description of SRP demon configuration options for
     *        BOOST config file parser
     */
    virtual void build_description (po::options_description& cfgdesc)
    {
      BPClientConfiguration::build_description (cfgdesc);
      logging_conf.add_options (cfgdesc);
      srp_conf.add_options (cfgdesc);
      shm_conf.add_options (cfgdesc, defaultSRPNeighbourStoreShmName);
    };


    /**
     * @brief Validates parameter values, throws in case of invalid configuration
     */
    virtual void validate()
    {
      BPClientConfiguration::validate();
      logging_conf.validate();
      srp_conf.validate();
      shm_conf.validate();
    };
    
    friend std::ostream& operator<<(std::ostream& os, const dcp::srp::SRPConfiguration& cfg);
    
  } SRPConfiguration;
  
};  // namespace dcp::srp
