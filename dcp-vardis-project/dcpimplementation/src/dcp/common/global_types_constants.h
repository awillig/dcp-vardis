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
#include <cstring>
#include <cstdint>
#include <chrono>
#include <netinet/ether.h>
#include <boost/chrono/duration.hpp>
#include <dcp/common/foundation_types.h>
#include <dcp/common/transmissible_type.h>
#include <dcp/common/memblock.h>


using std::chrono::time_point;
using std::chrono::high_resolution_clock;


/**
 * @brief This module contains types that are relevant throughout the
 *        entire DCP implementation, plus some associated
 *        constants. All the types are transmissible data types.
 */


namespace dcp {

  /****************************************************************
   * DCP version information and description
   ***************************************************************/


  const std::string dcpVersionNumber        = "1.3";     /*!< Version number of DCP spec this implementation follows */
  const std::string dcpHighlevelDescription = "DCP (Drone Coordination Protocol)";


  /****************************************************************
   * Other global constants
   ***************************************************************/
  
  /**
   * @brief Maximum length of the filename for command sockets (Unix
   *        Domain Sockets)
   */
  const size_t     maxUnixDomainSocketPathLength = 108;


  /**
   * @brief Default timeout for a shared memory lock in ms, expiry
   *        suggests an error
   */
  const uint16_t   defaultLongSharedMemoryLockTimeoutMS = 1000;


  /**
   * @brief Default timeout for a shared memory lock in ms, expiry
   *        does not suggest error, but perhaps allows for checking
   *        exit conditions
   */
  const uint16_t   defaultShortSharedMemoryLockTimeoutMS = 20;

  
  /**
   * @brief Maximum length of a shared memory area name
   */
  const size_t maxShmAreaNameLength = 255;


  /**
   * @brief Timeout for packet sniffer in ms
   */
  const uint16_t defaultPacketSnifferTimeoutMS = 300;


  /**
   * @brief Timeout value for command sockets in ms
   */
  const uint16_t    defaultValueCommandSocketTimeoutMS   = 500;


  /**
   * @brief Maximum size of a beacon payload in bytes
   */
  const size_t maxBeaconPayloadSize  = 1500;
  

  /****************************************************************
   * Protocol identifier type for BP client protocols
   ***************************************************************/


  /**
   * @brief Type for protocol identifiers as used by BP for
   *        multiplexing its client protocols
   *
   * This type is derived from TransmissibleIntegral and therefore
   * serializable.
   */
  class BPProtocolIdT : public TransmissibleIntegral<uint16_t> {
  public:
    BPProtocolIdT  () : TransmissibleIntegral<uint16_t> () {};
    BPProtocolIdT (uint16_t v) : TransmissibleIntegral<uint16_t>(v) {};
    BPProtocolIdT (const BPProtocolIdT& other) : TransmissibleIntegral<uint16_t>(other) {};

    BPProtocolIdT& operator= (const BPProtocolIdT& other) { val = other.val; return *this; };
    
    friend std::ostream& operator<< (std::ostream&os, const BPProtocolIdT& protId);
  };

  
  /**
   * @brief Pre-defined BP client protocol id's for SRP and Vardis
   */
  const BPProtocolIdT BP_PROTID_SRP     =  0x0001;
  const BPProtocolIdT BP_PROTID_VARDIS  =  0x0002;
  
  
  /****************************************************************
   * NodeIdentifierT
   ***************************************************************/

  /**
   * @brief Transmissible type for DCP Node Identifiers
   *
   * In the current implementation 48-bit IEEE MAC addresses are used
   * as node identifiers. In particular, the intention is to use the
   * MAC address of a node's WLAN adapter as its node identifier, but
   * this is just a convention (and not enforced).
   */
  class NodeIdentifierT : public TransmissibleType<6> {
  private:
    
    static const size_t MAC_ADDRESS_SIZE = 6; /*!< Corresponding to the 48-bit size of IEEE 802.x MAC addresses */

  public:
    byte nodeId[MAC_ADDRESS_SIZE] = {0, 0, 0, 0, 0, 0};

    NodeIdentifierT () {};


    /**
     * @brief Constructor taking MAC address from memory (copies it)
     *
     * @param pb: Pointer to memory area storing MAC address
     *
     * Throws upon handed a null pointer.
     */
    NodeIdentifierT (byte* pb)
    {
      if (pb == nullptr) throw std::invalid_argument ("NodeIdentifierT: invalid buffer");
      std::memcpy (nodeId, pb, MAC_ADDRESS_SIZE);
    };


    /**
     * @brief Constructor taking the MAC address from a string given as argument
     *
     * @param addr: Pointer to zero-terminated string in memory
     *        containing the MAC address in its usual notation
     *        (hex-digits-and-colons notation)
     */
    NodeIdentifierT (const char* addr)
    {
      struct ether_addr *paddr = ether_aton (addr);
      if (paddr == nullptr) throw std::invalid_argument ("NodeIdentifierT: cannot convert address string");
      std::memcpy (nodeId, paddr->ether_addr_octet, MAC_ADDRESS_SIZE);
    };


    /**
     * @brief Making NodeIdentifierT a totally ordered type
     */
    friend inline bool operator== (const NodeIdentifierT& lhs, const NodeIdentifierT& rhs) { return (0 == std::memcmp(lhs.nodeId, rhs.nodeId, MAC_ADDRESS_SIZE)); };
    friend inline bool operator!= (const NodeIdentifierT& lhs, const NodeIdentifierT& rhs) { return not (rhs == lhs); };
    friend inline bool operator< (const NodeIdentifierT& lhs, const NodeIdentifierT& rhs)  { return (lhs.to_uint64_t() < rhs.to_uint64_t()); };
    friend inline bool operator> (const NodeIdentifierT& lhs, const NodeIdentifierT& rhs)  { return (rhs < lhs); };
    friend inline bool operator<= (const NodeIdentifierT& lhs, const NodeIdentifierT& rhs) { return not (lhs > rhs); };
    friend inline bool operator>= (const NodeIdentifierT& lhs, const NodeIdentifierT& rhs) { return not (lhs < rhs); };
    
    /**
     * @brief Returns string representation of MAC address (hex-digits-and-colons notation)
     */
    std::string to_str () const;


    /**
     * @brief Convert MAC address to uint64_t
     */
    inline uint64_t to_uint64_t () const {
      return
	(((uint64_t) nodeId[0]) << 40)
	+ (((uint64_t) nodeId[1]) << 32)
	+ (((uint64_t) nodeId[2]) << 24)
	+ (((uint64_t) nodeId[3]) << 16)
	+ (((uint64_t) nodeId[4]) << 8)
	+ ((uint64_t) nodeId[5]);
    };


    /**
     * @brief Friend for stream output
     */
    friend std::ostream& operator<<(std::ostream& os, const NodeIdentifierT& nodeid);



    /**
     * @brief Serialization methods
     */
    virtual void serialize (AssemblyArea& area) const { area.serialize_byte_block(MAC_ADDRESS_SIZE, nodeId); };
    virtual void deserialize (DisassemblyArea& area)  { area.deserialize_byte_block(MAC_ADDRESS_SIZE, nodeId); };
  };


  /**
   * @brief Representing an un-initialized node identifier
   */
  const NodeIdentifierT nullNodeIdentifier;


  /****************************************************************
   * TimeStampT
   ***************************************************************/


  /**
   * @brief Type for encapsulating timestamps
   *
   * Allows other code to be independent of timestamp representation
   */
  class TimeStampT : public TransmissibleType<sizeof(time_point<high_resolution_clock>)> {
  public:
    time_point<high_resolution_clock> tStamp;

    TimeStampT () {};
    TimeStampT (const TimeStampT& other) : tStamp (other.tStamp) {};
    
    friend std::ostream& operator<<(std::ostream& os, const TimeStampT& tstamp);
    TimeStampT& operator= (const TimeStampT& other) { tStamp = other.tStamp; return *this; };

    
    /**
     * @brief Serialization methods
     *
     * ISSUE: Be careful: these methods may fail if the involved nodes
     * run on different architectures with different endianness
     */
    virtual void serialize (AssemblyArea& area) const { area.serialize_byte_block(fixed_size(), (byte*) &tStamp); };
    virtual void deserialize (DisassemblyArea& area) { area.deserialize_byte_block(fixed_size(), (byte*) &tStamp); };


    /**
     * @brief Returns current system time in our chosen representation
     */
    static TimeStampT get_current_system_time ()
    {
      TimeStampT ts;
      ts.tStamp = std::chrono::high_resolution_clock::now();
      return ts;
    };


    /**
     * @brief Type shorthand for milliseconds
     */
    typedef boost::chrono::duration<long long, boost::milli> milliseconds;


    /**
     * @brief Returns number of whole milliseconds passed since reference time
     *
     * @param past_time: The reference time
     */
    inline uint32_t milliseconds_passed_since (const TimeStampT& past_time) const
    {
      auto duration = tStamp - past_time.tStamp;
      return (uint32_t) (duration.count() / 1000000);
    };
  };


  /****************************************************************
   * StringT
   ***************************************************************/


  /**
   * @brief Transmissible string data type
   *
   * Strings are represented by a length field (currently: a byte),
   * followed by as many bytes as indicated.
   */
  class StringT : public MemBlock, public TransmissibleType<sizeof(byte)> {
  public:
    StringT () : MemBlock() {};


    /**
     * @brief Initialize string from given zero-terminated C string
     */
    StringT (const char* str) : MemBlock()
    {
      size_t len = std::strlen(str);
      if (str == nullptr)  throw std::invalid_argument ("StringT: string is empty");
      if (len > UINT8_MAX) throw std::invalid_argument ("StringT: string is too long");
      length = len;
      set (len, (byte*) str);
    };


    /**
     * @brief Initialize string from given std::string
     */
    StringT (const std::string& str) : MemBlock()
    {
      const char*  cstr = str.c_str();
      size_t       len  = std::strlen(cstr);
      if (len > UINT8_MAX) throw std::invalid_argument ("StringT: string is too long");
      length = len;
      set (len, (byte*) cstr);
    };


    /**
     * @brief Maximum length of a StringT
     */
    static constexpr size_t max_length () { return UINT8_MAX; };


    /**
     * @brief Return std::string representation of this string
     */
    inline std::string to_str() const { return std::string((char*) data, length); };


    /**
     * @brief Return length of serialized representation of this string
     */
    virtual size_t total_size () const { return fixed_size() + length; };


    /**
     * @brief Serialize this string into given area
     */
    virtual void serialize (AssemblyArea& area) const
    {
      area.serialize_byte((byte) length);
      if (length > 0)
	area.serialize_byte_block(length, data);
    };


    /**
     * @brief Deserialize / initialize this string from given area
     */
    virtual void deserialize (DisassemblyArea& area)
    {
      length = area.deserialize_byte ();
      if (length > 0)
        {
	  if (data) throw std::invalid_argument ("StringT::deserialize: already contains data");
	  data = new byte [length];
	  area.deserialize_byte_block (length, data);
        }
    };

    
    friend std::ostream& operator<<(std::ostream& os, const StringT& str);
  };
  
  
};  // namespace dcp
