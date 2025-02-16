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

/**
 * @brief This module provides a class for collecting runtime Vardis
 *        protocol statistics
 */


namespace dcp::vardis {

  /**
   * @brief Collecting statistics for Vardis operation
   */
  class VardisProtocolStatistics {
  public:
    /**
     * @brief Statistics counters for RTDB operations
     *
     * These only count *successful* operations, not the ones that had errors
     */
    unsigned long count_handle_rtdb_create = 0;   
    unsigned long count_handle_rtdb_delete = 0;
    unsigned long count_handle_rtdb_update = 0;
    unsigned long count_handle_rtdb_read   = 0;


    /**
     * @brief Statistics counters for received instructions
     *
     * These only count successfully processed received instructions
     */
    unsigned long count_process_var_create    = 0;
    unsigned long count_process_var_delete    = 0;
    unsigned long count_process_var_update    = 0;
    unsigned long count_process_var_summary   = 0;
    unsigned long count_process_var_requpdate = 0;
    unsigned long count_process_var_reqcreate = 0;



    friend std::ostream& operator<<(std::ostream& os, const VardisProtocolStatistics& stats);
  };
  
};  // namespace dcp::vardis
