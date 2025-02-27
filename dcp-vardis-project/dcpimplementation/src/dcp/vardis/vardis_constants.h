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

#include <cstdint>
#include <dcp/vardis/vardis_transmissible_types.h>

/**
 * @brief Provides some constants used in Vardis
 */

namespace dcp::vardis {

  const size_t        vardisCommandSocketBufferSize = 8192;
  
  const size_t        MAX_maxValueLength            = VarLenT::max_val();
  const size_t        MAX_maxDescriptionLength      = StringT::max_length();

  const std::string   defaultVardisVariableStoreShmName   = "shm-vardis-global-database";
  const std::string   defaultVardisCommandSocketFileName  = "/tmp/dcp-vardis-command-socket";
  
};  // namespace dcp::vardis
