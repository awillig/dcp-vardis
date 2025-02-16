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
#include <dcp/common/foundation_types.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/transmissible_type.h>


/**
 * @brief This module provides BP-specific transmissible types
 *        (i.e. types providing serialization facilities)
 */


namespace dcp::bp {

  /**
   * @brief Type for a BP length field (length of payloads)
   */
  class BPLengthT : public TransmissibleIntegral<uint16_t> {
  public:
    BPLengthT  () : TransmissibleIntegral<uint16_t> () {};
    BPLengthT (uint16_t v) : TransmissibleIntegral<uint16_t>(v) {};
    BPLengthT (const BPLengthT& other) : TransmissibleIntegral<uint16_t>(other) {};

    BPLengthT& operator= (const BPLengthT& other) { val = other.val; return *this; };
    
    friend std::ostream& operator<< (std::ostream&os, const BPLengthT& protId);
  };

  
  /**
   * @brief Constant fields of BPHeader
   */
  const uint8_t         bpHeaderVersion = 1;
  const uint16_t        bpMagicNo = 0x497E;
  

  /**
   * @brief Header structure of BP protocol
   */
  class BPHeaderT : public TransmissibleType<sizeof(byte)
					     +sizeof(uint16_t)
					     +NodeIdentifierT::fixed_size()
					     +BPLengthT::fixed_size()
					     +sizeof(byte)
					     +sizeof(uint32_t)>
  {
  public:
    uint8_t          version         = bpHeaderVersion;  /*!< Version field, fixed value */
    uint16_t         magicNo         = bpMagicNo;        /*!< Magic number, fixed value */
    NodeIdentifierT  senderId;                           /*!< Node id of sender */
    BPLengthT        length;                             /*!< Total length of beacon payload, not including this header */
    uint8_t          numPayloads;                        /*!< Number of client protocol payloads */
    uint32_t         seqno;                              /*!< Beacon sequence number */


    /**
     * @brief Serializing header into area
     */
    virtual void serialize (AssemblyArea& area) const
    {
      area.serialize_byte (version);
      area.serialize_uint16_n (magicNo);
      senderId.serialize (area);
      length.serialize (area);
      area.serialize_byte (numPayloads);
      area.serialize_uint32_n (seqno);
    };


    /**
     * @brief Deserializing header from area
     */
    virtual void deserialize (DisassemblyArea& area)
    {
      version = area.deserialize_byte ();
      area.deserialize_uint16_n (magicNo);
      senderId.deserialize (area);
      length.deserialize (area);
      numPayloads = area.deserialize_byte ();
      area.deserialize_uint32_n (seqno);
    };


    /**
     * @brief Validity checks on a header
     */
    inline bool isWellFormed (const NodeIdentifierT& ownNodeId) const
    {
      return (    (version == bpHeaderVersion)
	       && (magicNo == bpMagicNo)
	       && (senderId != ownNodeId)
  	       && (numPayloads > 0)
	       && (length.val > 0)
	     );
    };

    
    friend std::ostream& operator<<(std::ostream& os, const BPHeaderT& hdr);
  };



  /**
   * @brief Header preceding an individual payload
   */
  class BPPayloadHeaderT : public TransmissibleType<BPProtocolIdT::fixed_size() + BPLengthT::fixed_size()> {
  public:
    BPProtocolIdT   protocolId;   /*!< Protocol id of client protocol that generated payload */
    BPLengthT       length;       /*!< Length of client protocol payload, not including this header */


    /**
     * @brief Serialize header into area
     */
    virtual void serialize (AssemblyArea& area) const
    {
      protocolId.serialize (area);
      length.serialize (area);
    };


    /**
     * @brief Deserialize this header from area
     */
    virtual void deserialize (DisassemblyArea& area)
    {
      protocolId.deserialize (area);
      length.deserialize (area);
    };

    friend std::ostream& operator<<(std::ostream& os, const BPPayloadHeaderT& phdr);
  };
  
};  // namespace dcp
