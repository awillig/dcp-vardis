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


#include <functional>
#include <dcp/srp/srp_constants.h>
#include <dcp/srp/srpclient_lib.h>

namespace dcp {

  // -----------------------------------------------------------------------------------

  SRPClientRuntime::SRPClientRuntime (const SRPClientConfiguration& client_conf)
    : srp_store (client_conf.shm_conf_store.shmAreaName.c_str(), false)
  {
  }

  // -----------------------------------------------------------------------------------

  NodeIdentifierT SRPClientRuntime::get_own_node_identifier () const
  {
    return srp_store.get_own_node_identifier ();
  }

  // -----------------------------------------------------------------------------------

  DcpStatus SRPClientRuntime::set_own_safety_data (const SafetyDataT& new_sd)
  {
    srp_store.lock_own_safety_data ();
    srp_store.set_own_safety_data (new_sd);
    srp_store.unlock_own_safety_data ();
    
    return SRP_STATUS_OK;
  }

  
  // -----------------------------------------------------------------------------------

  DcpStatus SRPClientRuntime::get_all_neighbours_esd (std::list<ExtendedSafetyDataT>& neighbour_list)
  {
    std::function<bool (const ExtendedSafetyDataT&)> all_predicate =
      [] (const srp::ExtendedSafetyDataT&)
      {
	return true;
      };
    
    srp_store.lock_neighbour_table ();
    neighbour_list = srp_store.list_matching_esd_records (all_predicate);
    srp_store.unlock_neighbour_table ();
    
    return SRP_STATUS_OK;
  }
  
  
  // -----------------------------------------------------------------------------------
  
    
};  // namespace dcp
