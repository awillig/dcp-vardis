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

namespace po = boost::program_options;

/**
 * @brief This module provides data types and operation concerning the
 *        configuration of the SRP demon
 */


namespace dcp::srp {

  
  const uint16_t    defaultValueSrpGenerationPeriodMS   = 100;


  /**
   * @brief This structure holds the configuration data for the core
   *        SRP demon
   */
  typedef struct SRPConfigurationBlock : public DcpConfigurationBlock {

    /**************************************************
     * Proper SRP options
     *************************************************/


    /**
     * @brief Period between submitting SRP payloads to BP
     */
    uint16_t srpGenerationPeriodMS = defaultValueSrpGenerationPeriodMS;


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

    CommandSocketConfigurationBlock   cmdsock_conf; /*!< Command socket towards BP */
    SRPConfigurationBlock             srp_conf;     /*!< Actual SRP configuration data */


    /**
     * @brief Constructor, setting section name in config file and
     *        default name for command socket towards BP
     */
    SRPConfiguration ()
      : BPClientConfiguration ("BPCommandSocket", "BPSharedMem")
      , srp_conf ("SRP")
    {
      bp_cmdsock_conf.commandSocketFile = "/tmp/dcp-bp-command-socket";
    };


    /**
     * @brief Build description of SRP demon configuration options for
     *        BOOST config file parser
     */
    virtual void build_description (po::options_description& cfgdesc)
    {
      BPClientConfiguration::build_description (cfgdesc);
      srp_conf.add_options (cfgdesc);
    };


    /**
     * @brief Validates parameter values, throws in case of invalid configuration
     */
    virtual void validate()
    {
      BPClientConfiguration::validate();
      srp_conf.validate();
    };


    
    friend std::ostream& operator<<(std::ostream& os, const dcp::srp::SRPConfiguration& cfg);
    
  } SRPConfiguration;
  
};  // namespace dcp::srp
