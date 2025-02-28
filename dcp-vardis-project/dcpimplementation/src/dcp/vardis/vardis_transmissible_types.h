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
#include <dcp/common/area.h>
#include <dcp/common/foundation_types.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/transmissible_type.h>

namespace dcp::vardis {

  // -----------------------------------------

  /**
   * @brief This header declares the various data types defined in the
   *        VarDis specification and used by the VarDis protocol
   *        implementation, in particular all the transmissible data
   *        types.
   */
  
  
  // -----------------------------------------
  

  /**
   * @brief Type for variable identifiers
   */
  class VarIdT : public TransmissibleIntegral<byte> {
  public:
    
    static constexpr uint8_t   max_val () { return UINT8_MAX; };
    static constexpr uint64_t  max_number_identifiers () { return UINT8_MAX + 1; };
    
    VarIdT () : TransmissibleIntegral<byte>(0) {};
    VarIdT (const VarIdT& other) : TransmissibleIntegral<byte> (other) {};
    VarIdT (uint8_t vid) : TransmissibleIntegral<byte> (vid) {};

    VarIdT& operator= (const VarIdT& other) { val = other.val; return *this; };
    
    friend std::ostream& operator<< (std::ostream& os, const VarIdT& vid);
  };



  /**
   * @brief Type for variable length values
   */
  class VarLenT : public TransmissibleIntegral<byte> {
  public:

    static constexpr uint8_t max_val () { return UINT8_MAX; };
    
    VarLenT () : TransmissibleIntegral<byte>(0) {};
    VarLenT (const VarLenT& other) : TransmissibleIntegral<byte> (other) {};
    VarLenT (uint8_t vlen) : TransmissibleIntegral<byte>(vlen) {};

    VarLenT& operator= (const VarLenT& other) { val = other.val; return *this; };
    
    friend std::ostream& operator<< (std::ostream& os, const VarLenT& vlen);
  };



  /**
   * @brief Variable repetition counter
   */
  class VarRepCntT : public TransmissibleIntegral<byte> {
  public:

    static constexpr uint8_t max_val () { return 15; };
    
    VarRepCntT () : TransmissibleIntegral<byte>(0) {};
    VarRepCntT (const VarRepCntT& other) : TransmissibleIntegral<byte> (other) {};
    VarRepCntT (uint8_t rc) : TransmissibleIntegral<byte>(rc) {};

    VarRepCntT& operator= (const VarRepCntT& other) { val = other.val; return *this; };
    
    friend std::ostream& operator<< (std::ostream& os, const VarRepCntT& vrc);
  };


  /**
   * @brief Variable sequence number
   *
   * Sequence numbers are circular.
   */
  class VarSeqnoT : public TransmissibleIntegral<byte> {
  public:

    static constexpr uint8_t max_val () { return UINT8_MAX; };
    static constexpr unsigned int modulus () { return max_val() + 1; };
    
    VarSeqnoT () : TransmissibleIntegral<byte>(0) {};
    VarSeqnoT (const VarSeqnoT& other) : TransmissibleIntegral<byte> (other) {};
    VarSeqnoT (uint8_t s) : TransmissibleIntegral<byte>(s) {};

    VarSeqnoT& operator= (const VarSeqnoT& other) { val = other.val; return *this; };
    
    friend std::ostream& operator<< (std::ostream& os, const VarSeqnoT& vs);
  };
  
  
  // -----------------------------------------
  
  /**
   * @brief Checks if the first seqno is more recent than the second one
   */ 

  inline bool more_recent_seqno (const VarSeqnoT& a, const VarSeqnoT& b)
  {
    return ((((a) > (b)) && (((a)-(b)) <= ((int)(VarSeqnoT::max_val()/2)))) || (((a) < (b)) && (((b)-(a)) > ((int)(VarSeqnoT::max_val()/2) + 1))));
  };
  

  // -----------------------------------------

  /*
   * @brief Type representing a VarDis value, made up of one field
   *        indicating the length, and a byte array of that exact
   *        length
   */
  class VarValueT : public MemBlock, public TransmissibleType<VarLenT::fixed_size()> {
  public:

    /**
     * @brief Separate field representing the length
     *
     * This field is introduced in parallel to the length field of the
     * MemBlock base class to avoid direct calls to serialize_byte
     * when serializing the length. Rather, in the
     * serialization/deserialization methods we can call the generic
     * serialize/deserialize methods of VarLenT. This obviates the
     * need for changes in this class should the length of VarLenT
     * change.
     */
    VarLenT len;


    
    VarValueT () : MemBlock() {};


    /**
     * @brief Constructor, initializing variable value from given
     *        memory block (which is copied)
     *
     * @param size: length of value
     * @param data: points to the data to be copied into this variable value
     */
    VarValueT (VarLenT size, byte* data)
    {
      set (size.val, data);
      len = size;
    };


    /**
     * @brief Returns total serialized size of this variable value
     */
    virtual size_t total_size () const { return fixed_size() + length; };


    /**
     * @brief Serialization into given area
     */
    virtual void serialize (AssemblyArea& area) const
    {
      len.serialize (area);
      if (len > 0)
	area.serialize_byte_block(length, data);
    };


    /**
     * @brief Deserialization from given area into this variable value
     *
     * Throws if attempting to deserialize into a variable that
     * already has data, otherwise allocates new memory and
     * deserializes into that memory.
     */
    virtual void deserialize (DisassemblyArea& area)
    {
      len.deserialize (area);
      length = len.val;
      if (length > 0)
        {
	  if (data) throw DisassemblyAreaException ("VarValueT::deserialize: already contains data");
	  
	  data = new byte [length];
	  area.deserialize_byte_block (length, data);
        }
    };


    /**
     * @brief Deserialization, but value is written into user-provided buffer
     *
     * @param area: reference parameter for disassembly area from which to deserialize
     * @param len: output parameter giving the length of deserialized value
     * @param data_buffer: points to the buffer into which to deserialize. The buffer
     *        size must be sufficient to hold a variable value
     */
    virtual void deserialize (DisassemblyArea& area, VarLenT& len, byte* data_buffer)
    {
      len.deserialize (area);
      length = len.val;
      if (data_buffer == nullptr) throw DisassemblyAreaException ("VarValueT::deserialize[buffer]: invalid data buffer");
      if (data)                   throw DisassemblyAreaException ("VarValueT::deserialize[buffer]: already contains data");

      if (len > 0)
	area.deserialize_byte_block (len.val, data_buffer);
    };

    
    friend std::ostream& operator<<(std::ostream& os, const VarValueT& vv);
  };
  



  // -----------------------------------------


  /**
   * @brief Type representing a Vardis summary instruction
   *
   * A summary is made up of a variable identifier and a variable
   * sequence number
   */
  class VarSummT : public TransmissibleType<VarIdT::fixed_size() + VarSeqnoT::fixed_size()> {
  public:
    VarIdT     varId;
    VarSeqnoT  seqno;


    /**
     * @brief Equality test, both variable identifier and seqno must be the same
     */
    inline bool operator== (const VarSummT& other) const { return ((varId == other.varId) and (seqno == other.seqno)); };

    
    /**
     * @brief Serialization into given area
     */
    virtual void serialize (AssemblyArea& area) const
    {
      varId.serialize (area);
      seqno.serialize (area);
    };


    /**
     * @brief Deserialization from given area
     */
    virtual void deserialize (DisassemblyArea& area)
    {
      varId.deserialize (area);
      seqno.deserialize (area);
    };

    
    friend std::ostream& operator<<(std::ostream& os, const VarSummT& vs);
  };
  
  

  // -----------------------------------------


  /**
   * @brief Type representing a variable update instruction
   *
   * A variable update instruction consists of a variable identifier,
   * a sequence number and a variable value.
   */
  class VarUpdateT : public TransmissibleType<VarIdT::fixed_size()
					      + VarSeqnoT::fixed_size()
					      + VarValueT::fixed_size()> {
  public:
    VarIdT     varId;
    VarSeqnoT  seqno;
    VarValueT  value;
    
    virtual size_t total_size () const { return fixed_size() + value.length; };


    /**
     * @brief Equality test, all three of variable id, seqno and value
     *        must be the same
     */
    inline bool operator== (const VarUpdateT& other) const
    {
      return ((varId == other.varId) and (seqno == other.seqno) and (value == other.value));
    };


    /**
     * @brief Serialization into given area
     */
    virtual void serialize (AssemblyArea& area) const
    {
      varId.serialize (area);
      seqno.serialize (area);;
      value.serialize (area);
    };


    /**
     * @brief Deserialization from given area
     */
    virtual void deserialize (DisassemblyArea& area)
    {
      varId.deserialize (area);
      seqno.deserialize (area);
      value.deserialize (area);
    };

    
    friend std::ostream& operator<<(std::ostream& os, const VarUpdateT& vu);
  };
  
  
  
  // -----------------------------------------
  
  
  /**
   * @brief Type representing a variable specification
   *
   * A variable specification consists of a variable identifier, a
   * node identifier, a repetition counter and a descriptive string
   */
  class VarSpecT : public TransmissibleType<VarIdT::fixed_size()
					    + NodeIdentifierT::fixed_size()
					    + VarRepCntT::fixed_size()
					    + StringT::fixed_size()> {
  public:
    VarIdT            varId;
    NodeIdentifierT   prodId;
    VarRepCntT        repCnt;
    StringT           descr;
    
    virtual size_t total_size () const { return fixed_size() + descr.length; };


    /**
     * @brief Equality test, all fields must agree
     */
    inline bool operator== (const VarSpecT& other) const
    {
      return ((varId == other.varId)
	      and (prodId == other.prodId)
	      and (repCnt == other.repCnt)
	      and (descr == other.descr)
	      );
    };


    /**
     * @brief Serialization into given area
     */
    virtual void serialize (AssemblyArea& area) const
    {
      varId.serialize (area);
      prodId.serialize (area);
      repCnt.serialize (area);
      descr.serialize (area);
    };


    /**
     * @brief Deserialization from given area
     */
    virtual void deserialize (DisassemblyArea& area)
    {
      varId.deserialize (area);
      prodId.deserialize (area);
      repCnt.deserialize (area);
      descr.deserialize (area);
    };

    friend std::ostream& operator<<(std::ostream& os, const VarSpecT& vs);
  };
  
  
  
  // -----------------------------------------
  

  /**
   * @brief Type representing a VarCreate instruction
   *
   * A VarCreate instruction contains a variable specification and an
   * initial value (provided as variable update)
   */
  class VarCreateT : public TransmissibleType<VarSpecT::fixed_size() + VarUpdateT::fixed_size()> {
  public:
    VarSpecT    spec;
    VarUpdateT  update;
    
    virtual size_t total_size () const { return fixed_size() + spec.descr.length + update.value.length; };


    /**
     * @brief Equality test, both spec and value must agree
     */
    inline bool operator== (const VarCreateT& other) const
    {
      return ((spec == other.spec) and (update == other.update));
    };

    
    /**
     * @brief Serialization into given area
     */
    virtual void serialize (AssemblyArea& area) const
    {
      spec.serialize (area);
      update.serialize (area);
    };


    /**
     * @brief Deserialization from given area
     */
    virtual void deserialize (DisassemblyArea& area)
    {
      spec.deserialize (area);
      update.deserialize (area);
    };

    
    friend std::ostream& operator<<(std::ostream& os, const VarCreateT& vc);
  };
  
  
  // -----------------------------------------
  

  /**
   * @brief Type representing a VarDelete instruction
   *
   * It contains just a variable identifier
   */
  class VarDeleteT : public TransmissibleType<VarIdT::fixed_size()> {
  public:
    VarIdT  varId;

    inline bool operator== (const VarDeleteT& other) const { return varId == other.varId; };
    
    virtual void serialize (AssemblyArea& area) const { varId.serialize (area); };
    virtual void deserialize (DisassemblyArea& area) { varId.deserialize (area); };
    friend std::ostream& operator<<(std::ostream& os, const VarDeleteT& vd);
  };
  
  // -----------------------------------------
  

  /**
   * @brief Type representing a VarReqUpdate instruction
   *
   * It contains a variable summary
   */
  class VarReqUpdateT : public TransmissibleType<VarSummT::fixed_size()> {
  public:
    VarSummT updSpec;

    inline bool operator== (const VarReqUpdateT& other) const { return updSpec == other.updSpec; };
    
    virtual void serialize (AssemblyArea& area) const { updSpec.serialize (area); };
    virtual void deserialize (DisassemblyArea& area) { updSpec.deserialize (area); };
    friend std::ostream& operator<<(std::ostream& os, const VarReqUpdateT& vru);
  };
  
  
  // -----------------------------------------
  

  /**
   * @brief Type representing a VarReqCreate instruction
   */
  class VarReqCreateT : public TransmissibleType<VarIdT::fixed_size()> {
  public:
    VarIdT  varId;

    inline bool operator== (const VarReqCreateT& other) const { return varId == other.varId; };
    
    virtual void serialize (AssemblyArea& area) const { varId.serialize (area); };
    virtual void deserialize (DisassemblyArea& area) { varId.deserialize (area); };
    friend std::ostream& operator<<(std::ostream& os, const VarReqCreateT& vrc);
  };
  
  
  
  // -----------------------------------------

  /**
   * @brief Transmissible type encoding the type of an instruction
   *        container
   */
  class InstructionContainerT : public TransmissibleIntegral<byte> {
  public:

    /**
     * @brief Constructors
     */
    InstructionContainerT () : TransmissibleIntegral<byte> (0) {};
    InstructionContainerT (const InstructionContainerT& other) : TransmissibleIntegral<byte> (other) {};
    InstructionContainerT (byte icv) : TransmissibleIntegral<byte> (icv) {};


    /**
     * @brief Assignment operator
     */
    InstructionContainerT& operator= (const InstructionContainerT& other) { val = other.val; return *this; };

    
    /**
     * @brief Serialization and deserialization methods
     */
    virtual void serialize (AssemblyArea& area) const  { area.serialize_byte (val); };
    virtual void deserialize (DisassemblyArea& area)   { val = area.deserialize_byte (); };

    friend std::ostream& operator<< (std::ostream& os, const InstructionContainerT ic);
  };


  /**
   * @brief The known types of instruction containers
   */
  const byte  ICTYPE_SUMMARIES           =  1;
  const byte  ICTYPE_UPDATES             =  2;
  const byte  ICTYPE_REQUEST_VARUPDATES  =  3;
  const byte  ICTYPE_REQUEST_VARCREATES  =  4;
  const byte  ICTYPE_CREATE_VARIABLES    =  5;
  const byte  ICTYPE_DELETE_VARIABLES    =  6;


  /**
   * @brief Convert instruction container type to string
   */
  std::string vardis_instruction_container_to_string (InstructionContainerT ic);
  

  // -----------------------------------------
    

  /**
   * @brief Type representing the header of an instruction container
   *
   * It just indicates the type of instruction container and the
   * number of instruction records contained.
   */
  class ICHeaderT : public TransmissibleType<InstructionContainerT::fixed_size()+sizeof(byte)> {
  public:
    InstructionContainerT icType;          // note that this right now is one byte
    byte                  icNumRecords;

    
    static byte max_records() { return std::numeric_limits<byte>::max(); };

    
    inline bool operator== (const ICHeaderT& other) const
    {
      return ((icType == other.icType) and (icNumRecords == other.icNumRecords));
    };

    
    virtual void serialize (AssemblyArea& area) const
    {
      icType.serialize (area);
      area.serialize_byte (icNumRecords);
    };

    
    virtual void deserialize (DisassemblyArea& area)
    {
      icType.deserialize(area);
      icNumRecords  = area.deserialize_byte();
    };

    
    friend std::ostream& operator<<(std::ostream& os, const ICHeaderT& ich);
  };
  

  
  // -----------------------------------------

  
};  // namespace dcp::vardis
