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


#include <dcp/bp/bp_client_protocol_data.h>


namespace dcp::bp {

  BPClientProtocolData::BPClientProtocolData (const char* area_name,
					      BPStaticClientInfo static_info,
					      bool gen_pld_confirms)
    : static_info (static_info)
  {
    pSSB = std::make_shared<ShmStructureBase> (area_name, sizeof(BPShmControlSegment), true);
    pSCS = (BPShmControlSegment*) pSSB->get_memory_address ();
    if (!pSCS)
      throw ShmException ("BPClientProtocolData",
			  "cannot allocate BPShmControlSegment");
    
    new (pSCS) BPShmControlSegment (static_info, gen_pld_confirms);
  }
  

  BPClientProtocolData::~BPClientProtocolData ()
  {
  }
  
  
};  // namespace dcp::bp


  

