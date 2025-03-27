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


#include <exception>
#include <boost/program_options.hpp>
#include <tins/tins.h>
#include <dcp/common/logging_helpers.h>
#include <dcp/bp/bp_transmissible_types.h>
#include <dcp/bp/bp_configuration.h>

using std::cerr;
using std::endl;

namespace dcp::bp {

  void BPConfigurationBlock::add_options (po::options_description& cfgdesc)
  {
    cfgdesc.add_options()
      
      // interface parameters
      (opt("interface_name").c_str(),       po::value<std::string>(&interfaceName)->default_value(defaultValueInterfaceName), txt("Wireless interface: interface name").c_str())
      (opt("interface_mtuSize").c_str(),    po::value<size_t>(&mtuSize)->default_value(defaultValueMtuSize), txt("Wireless interface: MTU size (bytes)").c_str())
      (opt("interface_etherType").c_str(),  po::value<uint16_t>(&etherType)->default_value(defaultValueEtherType), txt("Wireless interface: ether_type value (protocol field)").c_str())
      
      // BP parameters
      (opt("maxBeaconSize").c_str(),     po::value<size_t>(&maxBeaconSize)->default_value(defaultValueMaxBeaconSize), txt("BP: maximum beacon size (bytes)").c_str())
      (opt("avgBeaconPeriodMS").c_str(), po::value<double>(&avgBeaconPeriodMS)->default_value(defaultValueAvgBeaconPeriodMS), txt("BP: average beacon period (ms)").c_str())
      (opt("jitterFactor").c_str(),      po::value<double>(&jitterFactor)->default_value(defaultValueJitterFactor), txt("BP: jitter factor (strictly between 0 and 1)").c_str())

      // Other parameters (e.g. run-time statistics)
      (opt("interBeaconTimeEWMAAlpha").c_str(),      po::value<double>(&interBeaconTimeEWMAAlpha)->default_value(defaultValueInterBeaconTimeEWMAAlpha), txt("BP: alpha value for EWMA estimator of inter-beacon reception time in ms (between 0 and 1)").c_str())
      (opt("beaconSizeEWMAAlpha").c_str(),      po::value<double>(&beaconSizeEWMAAlpha)->default_value(defaultValueBeaconSizeEWMAAlpha), txt("BP: alpha value for EWMA estimator of beacon size in bytes (between 0 and 1)").c_str())
      
      ;
  }
  
  
  void BPConfigurationBlock::validate ()
  {
    /***********************************
     * checks for interface data
     **********************************/
    if (interfaceName.empty()) throw ConfigurationException ("BP: no interface name given");
    if (mtuSize < minimumRequiredMTUSize) throw ConfigurationException ("BP: MTU size too small");
    if (mtuSize > maxBeaconPayloadSize) throw ConfigurationException ("BP: MTU size too large");
    if (etherType < 0x0800) throw ConfigurationException ("BP: ether_type must be at least 0x0800");
    try {
      Tins::NetworkInterface iface (interfaceName);
    }
    catch (...) {
      throw ConfigurationException("BP: invalid or unknown interface name given");
    }
    
    /***********************************
     * checks for BP options
     **********************************/
    if (maxBeaconSize <= dcp::bp::BPHeaderT::fixed_size() + dcp::bp::BPPayloadHeaderT::fixed_size()) throw ConfigurationException ("BP: maximum beacon size is too small");
    if (maxBeaconSize > mtuSize) throw ConfigurationException ("BP: maximum beacon size exceeds MTU size");
    if (avgBeaconPeriodMS <= 0) throw ConfigurationException ("BP: beacon period must be strictly positive");
    if ((jitterFactor <= 0) || (jitterFactor >= 1)) throw ConfigurationException ("BP: jitter factor must be strictly between zero and one");

    /***********************************
     * checks for other options
     **********************************/

    if (interBeaconTimeEWMAAlpha < 0) throw ConfigurationException ("BP: alpha value for EWMA inter beacon time estimator must be non-negative");
    if (interBeaconTimeEWMAAlpha > 1) throw ConfigurationException ("BP: alpha value for EWMA inter beacon time estimator must not exceed one");
    if (beaconSizeEWMAAlpha < 0) throw ConfigurationException ("BP: alpha value for EWMA beacon size estimator must be non-negative");
    if (beaconSizeEWMAAlpha > 1) throw ConfigurationException ("BP: alpha value for EWMA beacon size estimator must not exceed one");

    
  }

  std::ostream& operator<< (std::ostream& os, const dcp::bp::BPConfiguration& cfg)
  {
    os << "BPConfiguration { interfaceName = " << cfg.bp_conf.interfaceName
       << " , mtuSize = " << cfg.bp_conf.mtuSize
       << " , etherType = " << cfg.bp_conf.etherType
       << " , maxBeaconSize = " << cfg.bp_conf.maxBeaconSize
       << " , avgBeaconPeriodMS = " << cfg.bp_conf.avgBeaconPeriodMS
       << " , jitterFactor = " << cfg.bp_conf.jitterFactor
       << " , interBeaconTimeEWMAAlpha = " << cfg.bp_conf.interBeaconTimeEWMAAlpha
       << " , beaconSizeEWMAAlpha = " << cfg.bp_conf.beaconSizeEWMAAlpha
      
       << " , loggingToConsole = " << cfg.logging_conf.loggingToConsole
       << " , logfileNamePrefix = " << cfg.logging_conf.logfileNamePrefix
       << " , logAutoFlush = " << cfg.logging_conf.logAutoFlush
       << " , minimumSeverityLevel = " << cfg.logging_conf.minimumSeverityLevel
       << " , rotationSize = " << cfg.logging_conf.rotationSize
      
       << " , commandSocketFile = " << cfg.cmdsock_conf.commandSocketFile
       << " , commandSocketTimeoutMS = " << cfg.cmdsock_conf.commandSocketTimeoutMS
       << " }";
    return os;
  }
  
};  // namespace dcp::bp


  

