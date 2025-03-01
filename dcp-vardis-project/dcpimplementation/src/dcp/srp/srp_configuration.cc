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
#include <dcp/common/logging_helpers.h>
#include <dcp/common/exceptions.h>
#include <dcp/srp/srp_configuration.h>

using std::cerr;
using std::endl;

namespace dcp::srp {

  void SRPConfigurationBlock::add_options (po::options_description& cfgdesc)
  {
    cfgdesc.add_options()
      
      // SRP parameters
      (opt("generationPeriodMS").c_str(),   po::value<uint16_t>(&srpGenerationPeriodMS)->default_value(defaultValueSrpGenerationPeriodMS), txt("generation period for SRP payloads (in ms)").c_str())

      (opt("receptionPeriodMS").c_str(),   po::value<uint16_t>(&srpReceptionPeriodMS)->default_value(defaultValueSrpReceptionPeriodMS), txt("reception period for retrieving SRP payloads (in ms)").c_str())

      (opt("scrubbingPeriodMS").c_str(),   po::value<uint16_t>(&srpScrubbingPeriodMS)->default_value(defaultValueSrpScrubbingPeriodMS), txt("scrubbing period for the neighbour table (in ms)").c_str())

      (opt("keepaliveTimeoutMS").c_str(),   po::value<uint16_t>(&srpKeepaliveTimeoutMS)->default_value(defaultValueSrpKeepaliveTimeoutMS), txt("timeout for generating own payloads (in ms)").c_str())

      (opt("scrubbingTimeoutMS").c_str(),   po::value<uint16_t>(&srpScrubbingTimeoutMS)->default_value(defaultValueSrpScrubbingTimeoutMS), txt("timeout for neighbour entries in the scrubbing process (in ms)").c_str())
      
      ;
    
  }
  
  void SRPConfigurationBlock::validate ()
  {
    /***********************************
     * checks for SRP options
     **********************************/
    if (srpGenerationPeriodMS <= 0) throw ConfigurationException("generation period (in ms) must be strictly positive");
    if (srpReceptionPeriodMS <= 0) throw ConfigurationException("reception period (in ms) must be strictly positive");
    if (srpScrubbingPeriodMS <= 0) throw ConfigurationException("scrubbing period (in ms) must be strictly positive");
    if (srpKeepaliveTimeoutMS <= 0) throw ConfigurationException("keepalive timeout (in ms) must be strictly positive");
    if (srpScrubbingTimeoutMS <= 0) throw ConfigurationException("scrubbing timeout (in ms) must be strictly positive");
  }

  std::ostream& operator<< (std::ostream& os, const dcp::srp::SRPConfiguration& cfg)
  {
    os << "SRPConfiguration {"

       << "loggingToConsole = " << cfg.logging_conf.loggingToConsole
       << " , logfileNamePrefix = " << cfg.logging_conf.logfileNamePrefix
       << " , logAutoFlush = " << cfg.logging_conf.logAutoFlush
       << " , minimumSeverityLevel = " << cfg.logging_conf.minimumSeverityLevel
       << " , rotationSize = " << cfg.logging_conf.rotationSize
       << " , commandSocketFile[BP] = " << cfg.bp_cmdsock_conf.commandSocketFile
       << " , commandSocketTimeoutMS[BP] = " << cfg.bp_cmdsock_conf.commandSocketTimeoutMS
       << " , shmAreaNameBP = " << cfg.bp_shm_conf.shmAreaName
       << " , shmAreaNameNeighbourStore = " << cfg.shm_conf.shmAreaName
      
       << " , generationPeriodMS = " << cfg.srp_conf.srpGenerationPeriodMS
       << " , receptionPeriodMS = " << cfg.srp_conf.srpReceptionPeriodMS
       << " , scrubbingPeriodMS = " << cfg.srp_conf.srpScrubbingPeriodMS
       << " , keepaliveTimeoutMS = " << cfg.srp_conf.srpKeepaliveTimeoutMS
       << " , scrubbingTimeoutMS = " << cfg.srp_conf.srpScrubbingTimeoutMS
       << " }";
    return os;
  }
  
};  // namespace dcp::srp


  

