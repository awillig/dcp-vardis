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


#pragma once

#include <iostream>
#include <dcp/bp/bp_transmissible_types.h>


/**
 * @brief This module provides BP-specific header types
 *        (i.e. types providing serialization facilities)
 */


namespace dcp::bp {


  /**
   * @brief Header structure of BP protocol
   */
  class BPHeaderT : public TransmissibleType<sizeof(byte)
					     +sizeof(byte)
					     +sizeof(uint16_t)
					     +NodeIdentifierT::fixed_size()
					     +BPNetworkIdentifierT::fixed_size()
					     +BPLengthT::fixed_size()
					     +sizeof(byte)
					     +sizeof(uint32_t)>
  {
  public:
    
    uint8_t               versionMajor;    /*!< Major version field, fixed value */
    uint8_t               versionMinor;    /*!< Minor version field, fixed value */
    uint16_t              magicNo;         /*!< Magic number, fixed value */
    NodeIdentifierT       senderId;        /*!< Node id of sender */
    BPNetworkIdentifierT  networkId;       /*!< Network id of sender */
    BPLengthT             length;          /*!< Total length of beacon payload, not including this header */
    uint8_t               numPayloads;     /*!< Number of client protocol payloads */
    uint32_t              seqno;           /*!< Beacon sequence number */


    /**
     * @brief Constructor, initializes fixed value fields
     */
    BPHeaderT ();
    

    /**
     * @brief Serializing header into area
     */
    virtual void serialize (AssemblyArea& area) const;

    /**
     * @brief Deserializing header from area
     */
    virtual void deserialize (DisassemblyArea& area);


    /**
     * @brief Reserves space for this header in an assembly area
     *        without serializing any data into it
     */
    inline void reserve (AssemblyArea& area) const { area.incr(BPHeaderT::fixed_size()); };
    

    /**
     * @brief Validity checks on a header
     */
    bool isWellFormed (const NodeIdentifierT& ownNodeId, const BPNetworkIdentifierT netwId) const;

    
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
