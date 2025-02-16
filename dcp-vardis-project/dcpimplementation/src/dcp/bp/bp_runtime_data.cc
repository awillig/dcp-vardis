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


#include <tins/tins.h>
#include <dcp/common/command_socket.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/bp/bp_runtime_data.h>
#include <dcp/bp/bp_logging.h>

using namespace Tins;

namespace dcp::bp {

  BPRuntimeData::BPRuntimeData (BPConfiguration& cfg)
    : bp_config (cfg),
      bp_isActive (true),
      bp_exitFlag (false),
      commandSocket(bp_config.cmdsock_conf.commandSocketFile, bp_config.cmdsock_conf.commandSocketTimeoutMS)
  {
    // retrieve own node identifier (aka: MAC address)
    nw_if_info = NetworkInterface(cfg.bp_conf.interfaceName).addresses();
    for (size_t i=0; i<NodeIdentifierT::fixed_size(); i++)
      ownNodeIdentifier.nodeId[i] = nw_if_info.hw_addr[i];
  }
      

  
};  // namespace dcp::bp
