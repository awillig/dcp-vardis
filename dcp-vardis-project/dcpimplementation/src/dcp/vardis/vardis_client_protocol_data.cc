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
#include <dcp/vardis/vardis_client_protocol_data.h>

namespace dcp::vardis {


  VardisClientProtocolData::VardisClientProtocolData (const char* area_name)
  {
    pSSB = std::make_shared<ShmStructureBase> (area_name, sizeof(VardisShmControlSegment), true);
    pSCS = (VardisShmControlSegment*) pSSB->get_memory_address ();
    if (!pSCS)
      throw ShmException ("VardisClientProtocolData", "cannot allocate BPShmControlSegment");
    new (pSCS) VardisShmControlSegment ();
  }
  

  
  std::ostream& operator<<(std::ostream& os, const VardisClientProtocolData& cpd)
  {
    os << "VardisClientProtocolData{clientName = " << cpd.clientName
       << " }";
    return os;
  }

  
};  // namespace dcp::vardis
