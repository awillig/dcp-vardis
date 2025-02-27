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


#include <dcp/vardis/vardisclient_configuration.h>

using dcp::VardisClientConfiguration;

namespace dcp {
  
  std::ostream& operator<< (std::ostream& os, const VardisClientConfiguration& cfg)
  {
    os << "VardisClientConfiguration { "
       << "commandSocketFile[Vardis] = " << cfg.cmdsock_conf.commandSocketFile
       << " , commandSocketTimeoutMS[Vardis] = " << cfg.cmdsock_conf.commandSocketTimeoutMS
       << " , shmAreaName[Client] = " << cfg.shm_conf_client.shmAreaName
       << " , shmAreaNameVarStore = " << cfg.shm_conf_global.shmAreaName      
       << " }";
    return os;
  }
  
};  // namespace dcp::vardis
