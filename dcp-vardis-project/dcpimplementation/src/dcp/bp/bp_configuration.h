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
#include <dcp/common/configuration.h>
#include <dcp/common/logging_helpers.h>
#include <dcp/bp/bp_configuration.h>

namespace po = boost::program_options;

using std::ostream;

namespace dcp::bp {
      
  const size_t minimumRequiredMTUSize  = 256;
  
  const std::string defaultValueInterfaceName               = "wlan0";
  const size_t      defaultValueMtuSize                     = 1492;
  const uint16_t    defaultValueEtherType                   = 0x4953;
  const size_t      defaultValueMaxBeaconSize               = 1000;
  const double      defaultValueAvgBeaconPeriodMS           = 100.0;
  const double      defaultValueJitterFactor                = 0.1;
  const double      defaultValueInterBeaconTimeEWMAAlpha    = 0.975;
  const double      defaultValueBeaconSizeEWMAAlpha         = 0.975;
  
    /**
     * @brief This struct contains the configuration data for BP to operate on.
     *
     * It is assumed that the WLAN interface is already configured and running.
     */
    typedef struct BPConfigurationBlock : public DcpConfigurationBlock {
      
      /**************************************************
       * Options describing the (wireless) interface
       *************************************************/
      
      /**
       * @brief Name of interface to use
       *
       * Must refer to an existing interface, e.g. "wlan0"
       */
      std::string    interfaceName = defaultValueInterfaceName;
      
      
      /**
       * @brief Maximum Transfer Unit (MTU) of underlying wireless bearer
       *
       * Must be no smaller than 256
       */
      std::size_t    mtuSize = defaultValueMtuSize;
      
      
      /**
       * @brief Protocol type field written into the ether_type of EthernetII header
       *
       * Must be at least as large as 0x0800
       */
      uint16_t       etherType = defaultValueEtherType;
      
      
     /**************************************************
       * Proper Beaconing Protocol options
       *************************************************/
      
      /**
       * @brief Maximum size of generated beacons.
       *
       * Must not exceed mtuSize.
       */
      std::size_t  maxBeaconSize = defaultValueMaxBeaconSize;
      
      
      /**
       * @brief Average beaconing period (in milliseconds)
       */
      double      avgBeaconPeriodMS = defaultValueAvgBeaconPeriodMS;
      
      
      /**
       * @brief Fraction of jitter
       * 
       * Inter-beacon generation periods follow a uniform distribution from
       * the interval [(1-jitterFactor)*avgBeaconPeriod, (1+jitterFactor)*avgBeaconPeriod].
       * This must be a number strictly between zero and one.
       */ 
      double      jitterFactor    = defaultValueJitterFactor;
            

      /**************************************************
       * Other options (e.g. runtime statistics)
       *************************************************/


      /**
       * @brief Alpha value for the EWMA estimator of the average
       *        inter-beacon reception time (in ms)
       */
      double  interBeaconTimeEWMAAlpha = defaultValueInterBeaconTimeEWMAAlpha;


      /**
       * @brief Alpha value for the EWMA estimator of the average
       *        received beacon size (in bytes)
       */
      double  beaconSizeEWMAAlpha      = defaultValueBeaconSizeEWMAAlpha;
      
            
      /**************************************************
       * Methods
       *************************************************/


      /**
       * @brief Constructors, setting the section name for the config file
       */
      BPConfigurationBlock () : DcpConfigurationBlock ("BP") {} ;
      BPConfigurationBlock (std::string bname) : DcpConfigurationBlock (bname) {};


      /**
       * @brief Add BP option descriptions to config file reader
       */
      virtual void add_options (po::options_description& cfgdesc);


      /**
       * @brief Validate BP configuration data. Throws if data invalid.
       */
      virtual void validate ();
      
    } BPConfigurationBlock;




  /**
   * @brief This structure holds the entire configuration of the BP demon.
   *
   * It consists of configuration blocks for logging, a command socket
   * (for interacting with client protocols) and the BP configuration
   * data
   */
  typedef struct BPConfiguration : public DcpConfiguration {

    /**
     * @brief The required configuration blocks
     */
    LoggingConfigurationBlock          logging_conf;
    CommandSocketConfigurationBlock    cmdsock_conf;
    BPConfigurationBlock               bp_conf;


    /**
     * @brief Constructor, setting section names in config file and
     *        default names for log file prefix and command socket
     *        name
     */
    BPConfiguration ()
      : logging_conf ("logging")
      , cmdsock_conf ("BPCommandSocket")
      , bp_conf ("BP")
    {
      logging_conf.logfileNamePrefix = "dcp-bp-log";
      cmdsock_conf.commandSocketFile = "/tmp/dcp-bp-command-socket";
    }


    /**
     * @brief Build description structure for BP configuration, adding
     *        all the options of the contained configuration blocks
     */
    virtual void build_description (po::options_description& cfgdesc)
    {
      logging_conf.add_options (cfgdesc);
      cmdsock_conf.add_options (cfgdesc);
      bp_conf.add_options (cfgdesc);
    };


    /**
     * @brief Validate configuration, throws if invalid configuration
     *        data
     */
    virtual void validate ()
    {
      logging_conf.validate();
      cmdsock_conf.validate();
      bp_conf.validate();
    };

    
    friend ostream& operator<<(ostream& os, const dcp::bp::BPConfiguration& cfg);
    
  } BPConfiguration;
      
};  // namespace dcp::bp
