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


#include <string>
#include <cstring>
#include <dcp/common/global_types_constants.h>

namespace dcp {

  std::ostream& operator<< (std::ostream&os, const BPProtocolIdT& protId)
  {
    os << protId.val;
    return os;
  }

  

  std::string NodeIdentifierT::to_str () const
  {
    std::stringstream ss;
    ss << std::hex;
    
    for (size_t i=0; i<MAC_ADDRESS_SIZE-1; i++)
      ss << std::setw(2) << std::setfill('0') << (int) nodeId[i] << ":";
    ss << std::setw(2) << std::setfill('0') << (int) nodeId[MAC_ADDRESS_SIZE-1];
    
    return ss.str();
  };
  
  
  std::ostream& operator<<(std::ostream& os, const NodeIdentifierT& nodeid)
  {
    os << nodeid.to_str();
    return os;
  }
  
  
  bool operator< (const NodeIdentifierT& left, const NodeIdentifierT& right)
  {
    return (left.to_uint64_t() < right.to_uint64_t());
  }
  
  
  std::ostream& operator<<(std::ostream& os, const TimeStampT& tstamp)
  {
    os << tstamp.tStamp;
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const StringT& str)
  {
    char buff [str.length + 1];
    std::memcpy (buff, str.data, str.length);
    buff [str.length] = 0;
      os << buff;
    return os;
  }

};  // namespace dcp



