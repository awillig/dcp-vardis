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
      (opt("generationPeriodMS").c_str(),   po::value<uint16_t>(&srpGenerationPeriodMS)->default_value(defaultValueSrpGenerationPeriodMS), txt("generation period for values (in MS)").c_str())
      
      ;
    
  }
  
  
  void SRPConfigurationBlock::validate ()
  {
    /***********************************
     * checks for SRP options
     **********************************/
    if (srpGenerationPeriodMS <= 0) throw ConfigurationException("generation period (in ms) must be strictly positive");
  }

  std::ostream& operator<< (std::ostream& os, const dcp::srp::SRPConfiguration& cfg)
  {
    os << "SRPConfiguration {"
       << "srpGenerationPeriodMS = " << cfg.srp_conf.srpGenerationPeriodMS
       << " , commandSocketFile = " << cfg.cmdsock_conf.commandSocketFile
       << " , commandSocketTimeoutMS = " << cfg.cmdsock_conf.commandSocketTimeoutMS
       << " }";
    return os;
  }
  
};  // namespace dcp::srp


  

