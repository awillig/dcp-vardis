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


#include <dcp/vardis/vardis_protocol_statistics.h>

namespace dcp::vardis {

  std::ostream& operator<<(std::ostream& os, const VardisProtocolStatistics& stats)
  {
    os << "VardisProtocolStatistics{count_handle_rtdb_create = " << stats.count_handle_rtdb_create
       << " , count_handle_rtdb_delete = " << stats.count_handle_rtdb_delete
       << " , count_handle_rtdb_update = " << stats.count_handle_rtdb_update
       << " , count_handle_rtdb_read = " << stats.count_handle_rtdb_read
       << " , count_process_var_create = " << stats.count_process_var_create
       << " , count_process_var_delete = " << stats.count_process_var_delete
       << " , count_process_var_update = " << stats.count_process_var_update
       << " , count_process_var_summary = " << stats.count_process_var_summary
       << " , count_process_var_requpdate = " << stats.count_process_var_requpdate
       << " , count_process_var_reqcreate = " << stats.count_process_var_reqcreate
       << " }";
    return os;
  }

  
};  // namespace dcp::vardis
