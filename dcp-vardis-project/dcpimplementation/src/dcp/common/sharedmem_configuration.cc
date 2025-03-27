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

#include <dcp/common/exceptions.h>
#include <dcp/common/sharedmem_configuration.h>


namespace dcp {

  
  void SharedMemoryConfigurationBlock::add_options (po::options_description& cfgdesc, std::string default_area_name)
  {
    cfgdesc.add_options()
      (opt("areaName").c_str(),  po::value<std::string>(&shmAreaName)->default_value(default_area_name), txt("shared memory area name").c_str())
      ;
  }

  void SharedMemoryConfigurationBlock::validate ()
  {
    if (shmAreaName.empty()) throw ConfigurationException("no shared memory name given");
  }
  
};  // namespace dcp
