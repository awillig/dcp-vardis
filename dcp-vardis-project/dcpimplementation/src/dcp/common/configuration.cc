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
#include <dcp/common/configuration.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/logging_helpers.h>

using std::cerr;
using std::endl;

namespace dcp {

  void DcpConfiguration::read_from_config_file (const std::string& cfgfilename, bool allow_unregistered)
  {
    po::options_description cfgdesc = construct_options_description();
    try {
      po::variables_map vm;
      po::store(po::parse_config_file(cfgfilename.c_str(), cfgdesc, allow_unregistered), vm);
      po::notify(vm);
    }
    catch (std::exception& e) {
      cerr << "read_from_config_file: " << e.what() << endl;
      cerr << cfgdesc << endl;
      throw ConfigurationException (e.what());
    }
  }

};  // namespace dcp
