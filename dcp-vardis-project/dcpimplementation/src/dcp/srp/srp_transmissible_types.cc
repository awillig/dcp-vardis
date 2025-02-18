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


#include <dcp/srp/srp_transmissible_types.h>

namespace dcp::srp {

  std::ostream& operator<<(std::ostream& os, const SafetyDataT& sd)
  {
    os << "SafetyDataT { x = " << sd.position_x
       << " , y = " << sd.position_y
       << " , z = " << sd.position_z
       << " }";
    return os;
  }


  std::ostream& operator<<(std::ostream& os, const ExtendedSafetyDataT& esd)
  {
      os << "ExtendedSafetyDataT { safetyData = " << esd.safetyData
	 << " , nodeId = " << esd.nodeId
	 << " , timeStamp = " << esd.timeStamp
	 << " , seqno = " << esd.seqno
	 << " }";
      return os;
  }
  
}
