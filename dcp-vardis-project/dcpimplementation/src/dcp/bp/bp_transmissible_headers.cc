/**
 * Copyright (C) 2026 Andreas Willig, University of Canterbury
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


#include <dcp/bp/bp_transmissible_headers.h>
#include "bp_version.h"

namespace dcp::bp {

  /**
   * @brief Constant fields of BPHeader
   */
  const uint16_t        bpMagicNo = 0x497E;
    

  BPHeaderT::BPHeaderT ()
  {
    versionMajor   = BP_VERSION_NUMBER_MAJOR;
    versionMinor   = BP_VERSION_NUMBER_MINOR;
    magicNo        = bpMagicNo;
  }

  
  void BPHeaderT::serialize (AssemblyArea& area) const
  {
    area.serialize_byte (versionMajor);
    area.serialize_byte (versionMinor);
    area.serialize_uint16_n (magicNo);
    senderId.serialize (area);
    networkId.serialize (area);
    length.serialize (area);
    area.serialize_byte (numPayloads);
    area.serialize_uint32_n (seqno);
  }

  
  void BPHeaderT::deserialize (DisassemblyArea& area)
  {
    versionMajor = area.deserialize_byte ();
    versionMinor = area.deserialize_byte ();
    area.deserialize_uint16_n (magicNo);
    senderId.deserialize (area);
    networkId.deserialize (area);
    length.deserialize (area);
    numPayloads = area.deserialize_byte ();
    area.deserialize_uint32_n (seqno);
  }


  bool BPHeaderT::isWellFormed (const NodeIdentifierT& ownNodeId, const BPNetworkIdentifierT netwId) const
  {
    return (    (versionMajor == BP_VERSION_NUMBER_MAJOR)
	     && (magicNo == bpMagicNo)
	     && (senderId != ownNodeId)
	     && (networkId == netwId)
	     && (numPayloads > 0)
	     && (length.val > 0)
	   );
  }

    
  
  std::ostream& operator<<(std::ostream& os, const BPHeaderT& hdr)
  {
    os << "BPHeaderT { versionMajor = " << (int) hdr.versionMajor
       << " , versionMinor = " << (int) hdr.versionMinor
       << " , magicNo = " << (int) hdr.magicNo
       << " , senderId = " << hdr.senderId
       << " , networkId = " << hdr.networkId
       << " , length = " << hdr.length
       << " , numPayloads = " << (int) hdr.numPayloads
       << " , seqno = " << hdr.seqno
       << " }";
    return os;
  }


  std::ostream& operator<<(std::ostream& os, const BPPayloadHeaderT& phdr)
  {
      os << "BPPayloadHeaderT { protocolId = " << phdr.protocolId
	 << " , length = " << phdr.length
	 << " }";
      return os;
  }
  
}
