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


#include <cstring>
#include <format>
#include <iostream>
#include <dcp/common/exceptions.h>
#include <dcp/common/foundation_types.h>

namespace dcp {

  // -------------------------------------------------------------

  /**
   * Provide simple serialization and deserialization support for data
   * blocks and header fields with a size that is an integral number of
   * bytes.
   *
   * An assembly area is an abstraction for an area of memory into which
   * outgoing packets are serialized, a disassembly area is an area of
   * memory from which a received packet is deserialized.
   *
   * After describing the abstract operations we define two specific
   * types of areas, one just given by a memory block, another one being
   * a C++ byte vector.
   *
   */
  
  
  // =============================================================================
  // =============================================================================
  // General area, abstract base class
  // =============================================================================
  // =============================================================================
  

  /**
   * @brief Abstract base class for an area.
   *
   * Provides data members for counting available and consumed bytes
   * from an area
   */
  
  class Area {

  protected:
    std::string _name; /*!< Name of area, mainly for logging */
      
  private:
    size_t bytes_available;     /*!< bytes that can still be written / retrieved */
    size_t initial_available;   /*!< bytes initially available in the area */
    size_t bytes_used;          /*!< bytes already written / retrieved */

  public:
    
    Area () = delete;

    
    /**
     * @brief Initialize area from given number of available bytes
     *
     * @param name: textual name of area, for logging purposes
     * @param available: number of bytes available in the area
     *
     * Initializes the internal counters
     */
    Area (std::string name, size_t available) :
      _name (name),
      bytes_available(available),
      initial_available(available), bytes_used(0)
    {};

    
    /**
     * @brief Getter for number of used bytes in area
     */
    inline size_t used() const { return bytes_used; };
    
    
    /**
     * @brief Getter for remaining number of bytes available in area
     */
    inline size_t available() const { return bytes_available; };

    
    /**
     * @brief Getter for initial number of bytes available
     */
    inline size_t initial() const { return initial_available; };

    
    /**
     * @brief updates used / available variables and throws exception when insufficient availability
     */
    inline void incr (size_t by = 1)
    {
      if (bytes_available < by)
	throw AreaException (std::format ("{}.incr", _name),
			     std::format ("insufficient bytes available, by = {}, available = {}", by, bytes_available));

      bytes_used       += by;
      bytes_available  -= by;
    };
    
    
    /**
     * @brief Checks for serializing or deserializing a block of bytes
     */
    inline void assert_block (size_t size, const byte* pb) const
    {
      if ((size == 0) || (pb == nullptr))
	throw AreaException(std::format ("{}.assert_block", _name),
			    std::format ("size or buffer are null, size is {}", size));
      if (bytes_available < size)
	throw AreaException(std::format ("{}.assert_block", _name),
			    std::format ("not enough space available: requested = {}, available = {}", size, bytes_available));
    }

    
    /*
     * @brief Re-sets used / available information to their initial values
     */
    inline void reset ()
    {
      bytes_available = initial_available;
      bytes_used      = 0;
    };

    
    /**
     * @brief Modifies upper bound of the area towards a new initial size, but keeping
     *        bytes used untouched.
     */
    inline void resize (size_t new_initial)
    {
      if ((new_initial == 0) || (bytes_used > new_initial))
	throw AreaException(std::format ("{}.resize", _name),
			    std::format ("new_initial {} is zero or larger than number of already used bytes", new_initial));
      
      initial_available  =  new_initial;
      bytes_available    =  new_initial - bytes_used;
    }
  };
  
  
  // =============================================================================
  // =============================================================================
  // Assembly and disassembly area base classes
  // =============================================================================
  // =============================================================================
  

  /**
   * @brief Abstract base class for an area used for serialization,
   *        called an assembly area.
   */
  
  class AssemblyArea : public Area {
  public:

    AssemblyArea () = delete;

    
    AssemblyArea (std::string name, size_t size) : Area (name, size) {};


    /**
     * @brief Abstract method for serializing a single byte.
     */
    virtual void serialize_byte (byte b) = 0;


    /**
     * @brief Default method for serializing a byte block, assumed to
     *        be slow.
     *
     * This method should be overloaded when more efficient
     * implementations are available.
     */
    virtual void serialize_byte_block (size_t size, const byte* pb)
    {
      assert_block (size, pb);
      for (size_t i = 0; i < size; i++)
	{
	  serialize_byte(pb[i]);
	}
    };


    /**
     * @brief Default method for serializing a 16-bit value in network byte order
     */
    virtual void serialize_uint16_n (uint16_t val)
    {
      if (available() < sizeof(uint16_t))
	throw AssemblyAreaException(std::format ("{}.serialize_uint16_n", _name),
				    std::format ("insufficient space, available = {}", available()));
      serialize_byte ((byte) (val >> 8));
      serialize_byte ((byte) (val & 0x00FF));
    };


    /**
     * @brief Default method for serializing a 32-bit value in network byte order
     */
    virtual void serialize_uint32_n (uint32_t val)
    {
      if (available() < sizeof(uint32_t))
	throw AssemblyAreaException(std::format ("{}.serialize_uint32_n", _name),
				    std::format ("insufficient space, available = {}", available()));
      serialize_byte ((byte) (val >> 24));
      serialize_byte ((byte) ((val & 0x00FF0000)>>16));
      serialize_byte ((byte) ((val & 0x0000FF00)>>8));
      serialize_byte ((byte) (val & 0x000000FF));
    };

    
    /**
     * @brief Default method for serializing a 64-bit value in network byte order
     */
    virtual void serialize_uint64_n (uint64_t val)
    {
      if (available() < sizeof(uint32_t))
	throw AssemblyAreaException(std::format ("{}.serialize_uint64_n", _name),
				    std::format ("insufficient space, available = {}", available()));      
      serialize_byte ((byte) (val >> 56));
      serialize_byte ((byte) ((val & 0x00FF000000000000) >> 48));
      serialize_byte ((byte) ((val & 0x0000FF0000000000) >> 40));
      serialize_byte ((byte) ((val & 0x000000FF00000000) >> 32));
      serialize_byte ((byte) ((val & 0x00000000FF000000) >> 24));
      serialize_byte ((byte) ((val & 0x0000000000FF0000) >> 16));
      serialize_byte ((byte) ((val & 0x000000000000FF00) >> 8));
      serialize_byte ((byte) (val  & 0x00000000000000FF));
    };
    
  };
  
  // -------------------------------------------------------------


  /**
   * @brief Abstract base class for an area used for deserialization,
   *        called a disassembly area.
   */
  class DisassemblyArea : public Area {
  public:
    
    DisassemblyArea () = delete;

    
    DisassemblyArea (std::string name, size_t size) : Area (name, size) {};

    
    /**
     * @brief Abstract method for deserializing one byte.
     */
    virtual byte deserialize_byte () = 0;

    
    /**
     * @brief Abstract method for peeking a byte without deserializing it
     */
    virtual byte peek_byte () = 0;

    
    /**
     * @brief Default method for deserializing a byte block, assumed
     *        to be slow.
     *
     * This method should be overloaded when more efficient
     * implementations are available.
     */
    virtual void deserialize_byte_block (size_t size, byte* pb)
    {
      assert_block (size, pb);
      for (size_t i = 0; i < size; i++)
        {
	  *pb = deserialize_byte ();
	  pb++;
        }
    };


    /**
     * @brief Default method for deserializing a 16-bit value in network byte order
     */
    virtual void deserialize_uint16_n (uint16_t& val)
    {
      if (available() < sizeof(uint16_t))
	throw DisassemblyAreaException(std::format ("{}.deserialize_uint16_n", _name),
				       std::format ("insufficient space, available = {}", available()));
      byte b1 = deserialize_byte();
      byte b2 = deserialize_byte();
      val = (((uint16_t) b1) << 8) + ((uint16_t) b2);
    };


    /**
     * @brief Default method for deserializing a 32-bit value in network byte order
     */
    virtual void deserialize_uint32_n (uint32_t& val)
    {
      if (available() < sizeof(uint16_t))
	throw DisassemblyAreaException(std::format ("{}.deserialize_uint32_n", _name),
				       std::format ("insufficient space, available = {}", available()));
      byte b1 = deserialize_byte();
      byte b2 = deserialize_byte();
      byte b3 = deserialize_byte();
      byte b4 = deserialize_byte();
      
      val =   (((uint32_t) b1) << 24)
	+ (((uint32_t) b2) << 16)
	+ (((uint32_t) b3) << 8)
	+ ((uint32_t) b4);
    };


    /**
     * @brief Default method for deserializing a 64-bit value in network byte order
     */
    virtual void deserialize_uint64_n (uint64_t& val)
    {
      if (available() < sizeof(uint16_t))
	throw DisassemblyAreaException(std::format ("{}.deserialize_uint64_n", _name),
				       std::format ("insufficient space, available = {}", available()));
      byte b1 = deserialize_byte();
      byte b2 = deserialize_byte();
      byte b3 = deserialize_byte();
      byte b4 = deserialize_byte();
      byte b5 = deserialize_byte();
      byte b6 = deserialize_byte();
      byte b7 = deserialize_byte();
      byte b8 = deserialize_byte();
      
      val =   (((uint64_t) b1) << 56)
	+ (((uint64_t) b2) << 48)
	+ (((uint64_t) b3) << 40)
	+ (((uint64_t) b4) << 32)
	+ (((uint64_t) b5) << 24)
	+ (((uint64_t) b6) << 16)
	+ (((uint64_t) b7) << 8)
	+ ((uint64_t) b8);
    };
    
  };
  
  
  
  // =============================================================================
  // =============================================================================
  // Assembly and disassembly areas working with a memory chunk
  // =============================================================================
  // =============================================================================
  
  /**
   * @brief Assembly area using an in-memory chunk of bytes.
   *
   * Depending on constructor used, memory is either allocated and
   * managed here or by the caller.
   */
  class MemoryChunkAssemblyArea : public AssemblyArea {
  private:
    bool    deallocate  = false;     /*!< Should destructor deallocate memory area? */
    size_t  bufferSize  = 0;         /*!< Size of buffer */
    byte*   buffer      = nullptr;   /*!< Points to buffer allocated by this class, if any */
    byte*   pointer     = nullptr;   /*!< Points to where the next serialized byte will be stored */
    
  public:

    /**
     * @brief Constructor, allocates own buffer.
     *
     * @param name: Name of area, used for logging purposes
     * @param size: Size of memory area to be allocated
     */
    MemoryChunkAssemblyArea (std::string name, size_t size)
      : AssemblyArea (name, size)
    {
      if (size == 0)
	throw AssemblyAreaException(std::format ("{}.MemoryChunkAssemblyArea", _name),
				    std::format ("zero buffer size"));
      
      buffer      = new byte [size];
      pointer     = buffer;
      deallocate  = true;
      
      std::memset(pointer, 0, size);
    }


    /**
     * @brief Constructor, using caller-provided memory block
     */
    MemoryChunkAssemblyArea (std::string name, size_t size, byte* memblock)
      : AssemblyArea (name, size)
      , buffer (memblock)
      , pointer (memblock)
    {
      if (size == 0)
	throw AssemblyAreaException(std::format ("{}.MemoryChunkAssemblyArea", _name),
				    std::format ("zero buffer size"));
      if (!memblock)
	throw AssemblyAreaException(std::format ("{}.MemoryChunkAssemblyArea", _name),
				    std::format ("memblock is null"));
      
      deallocate = false;
    };


    /**
     * @brief Destructor, de-allocates buffer if we allocated it ourselves
     */ 
    virtual ~MemoryChunkAssemblyArea ()
    {
      if (deallocate && buffer) delete [] buffer;
    };


    /**
     * @brief Getter for buffer pointer
     */
    byte* getBufferPtr () const { return buffer; };


    /**
     * @brief Serializes a single byte, writing it to current address
     */
    virtual void serialize_byte (byte b)
    {
      if (available() == 0)
	throw AssemblyAreaException(std::format ("{}.MemoryChunkAssemblyArea.serialize_byte", _name),
				    std::format ("no byte available"));
      *pointer = b;
      pointer++;
      incr();
    };


    /**
     * @brief Serializes entire byte block using memcpy
     */
    virtual void serialize_byte_block (size_t size, const byte* pb) 
    {
      assert_block (size, pb);
      std::memcpy (pointer, pb, size);
      incr (size);
      pointer += size;
    };


    /**
     * @brief Re-set area to start serializing at the beginning again
     */
    virtual void reset ()
    {
      AssemblyArea::reset();
      pointer = buffer;
    }
  };
  
  // -------------------------------------------------------------

  
  /**
   * @brief Disassembly area using an in-memory chunk of bytes
   *        supplied and managed by the calling code.
   */
  class MemoryChunkDisassemblyArea : public DisassemblyArea {
    
  private:
    byte*   buffer      = nullptr;  /*!< Pointer to buffer area from which to deserialize */
    byte*   pointer     = nullptr;  /*!< Points to byte that is deserialized next */
    
  public:
    
    MemoryChunkDisassemblyArea () = delete;


    /**
     * @brief Constructor, sets deserialization pointer to supplied memblock
     *
     * @param name: Name of disassembly area, for logging purposes
     * @param size: Size of memory area from which to deserialize
     * @param memblock: the memory area from which to deserialize
     */
    MemoryChunkDisassemblyArea (std::string name, size_t size, byte* memblock)
      : DisassemblyArea (name, size)
      , buffer(memblock)
      , pointer(memblock)
    {};


    /**
     * @brief Returns one deserialized byte
     */
    virtual byte deserialize_byte ()
    {
      if (available() == 0)
	throw DisassemblyAreaException(std::format ("{}.MemoryChunkDisassemblyArea.deserialize_byte", _name),
				       std::format ("no byte available"));
      byte rv = *pointer;
      pointer++;
      incr();
      return rv;
    };


    /**
     * @brief Returns next byte to deserialize, without actually deserializing it
     */
    virtual byte peek_byte ()
    {
      if (available() == 0)
	throw DisassemblyAreaException(std::format ("{}.MemoryChunkDisassemblyArea.peek_byte", _name),
				       std::format ("no byte available"));

      return *pointer;
    };


    /**
     * @brief Deserialize entire byte block, using memcpy
     */
    virtual void deserialize_byte_block (size_t size, byte* pb)
    {
      assert_block (size, pb);
      std::memcpy (pb, pointer, size);
      incr (size);
      pointer += size;
    };


    /**
     * @brief Re-set area to start serializing at the beginning again
     */
    virtual void reset ()
    {
      Area::reset();
      pointer = buffer;
    }
    
  };
  
  
  // =============================================================================
  // =============================================================================
  // Assembly and disassembly areas working with a byte vector
  // =============================================================================
  // =============================================================================
  
  /**
   * @brief Assembly area using byte vector.
   *
   * Allocation and de-allocation of the byte vector can be done
   * inside and outside this class. If an outside byte vector is used,
   * then the calling code must make sure that its lifetime exceeds
   * the lifetime of the assembly area object.
   */
  class ByteVectorAssemblyArea : public AssemblyArea {
  private:
    std::vector<byte>*  pvector = nullptr;   /*!< Pointer to the byte vector object to be used */
    bool deallocate = false;                 /*!< If we allocate the byte vector ourselves it needs to be deallocated */
    
  public:

    /**
     * @brief Constructor which actively allocates the byte vector
     *
     * @param name: Name of assembly area, for logging purposes
     * @param size: size of byte vector (number of byte elements) to be allocated
     */
    ByteVectorAssemblyArea (std::string name, size_t size)
      : AssemblyArea (name, size)
    {
      if (size == 0)
	throw AssemblyAreaException(std::format ("{}.ByteVectorAssemblyArea", _name),
				    std::format ("zero buffer size"));
      pvector = new std::vector<byte> (size);
      deallocate = true;
    }


    /**
     * @brief Constructor taking user-supplied byte vector
     *
     * @param name: Name of assembly area, for logging purposes
     * @param size: size of byte vector (number of byte elements)
     * @param vect: The actual byte vector, must have a lifetime exceeding the one of this object
     */
    ByteVectorAssemblyArea (std::string name, size_t size, std::vector<byte>& vect)
      : AssemblyArea (name, size)
    {
      if (size == 0)
	throw AssemblyAreaException(std::format ("{}.ByteVectorAssemblyArea", _name),
				    std::format ("zero buffer size"));
      pvector    = &vect;
      deallocate = false;
    };


    /**
     * @brief Destructor, de-allocates byte vector if we allocated it ourselves
     */
    ~ByteVectorAssemblyArea ()
    {
      if (deallocate) delete pvector;
    };


    /**
     * @brief Returns a pointer to the current byte vector in use
     */
    const std::vector<byte>* get_vector_ptr () const { return pvector; };


    /**
     * @brief Serialize single byte
     */
    virtual void serialize_byte (byte b)
    {
      if (available() == 0)
	throw AssemblyAreaException(std::format ("{}.ByteVectorAssemblyArea.serialize_byte", _name),
				    std::format ("no space left"));
      (*pvector)[used()] = b;
      incr();
    };


    /**
     * @brief Serialize byte block
     */
    virtual void serialize_byte_block (size_t size, const byte* pb)
    {
      assert_block (size, pb);
      std::memcpy (& ((*pvector)[used()]), pb, size);
      incr (size);
    };
    
  };
  
  // ---------------------------------------------------------------


  /**
   * @brief Disassembly area using byte vector.
   *
   * The byte vector is supplied by the calling code. Its lifetime
   * must exceed the lifetime of this object.
   */
  class ByteVectorDisassemblyArea : public DisassemblyArea {
    
  private:
    const std::vector<byte>*   pvector = nullptr;  /*!< Pointer to the byte vector being used */
    
  public:

    
    ByteVectorDisassemblyArea () = delete;


    /**
     * @brief Constructor
     *
     * @param name: Name of disassembly area, for logging purposes
     * @param vect: Reference to the actual byte vector being used, must have lifetime exceeding the one of this object
     */
    ByteVectorDisassemblyArea (std::string name, const std::vector<byte>& vect)
      : DisassemblyArea(name, vect.size())
      , pvector(&vect)
    {};


    /**
     * @brief Deserialize single byte
     */
    virtual byte deserialize_byte ()
    {
      if (available() == 0)
	throw DisassemblyAreaException(std::format ("{}.ByteVectorDisassemblyArea.deserialize_byte", _name),
				    std::format ("no space left"));
      byte rv = (*pvector)[used()];
      incr();
      return rv;
    };


    /**
     * @brief Returns next byte to be deserialized, without deserializing it
     */
    virtual byte peek_byte ()
    {
      if (available() == 0)
	throw DisassemblyAreaException(std::format ("{}.ByteVectorDisassemblyArea.peek_byte", _name),
				    std::format ("no space left"));
      byte rv = (*pvector)[used()];
      return rv;
    };
    

    /**
     * @brief Deserialize byte block
     */
    virtual void deserialize_byte_block (size_t size, byte* pb)
    {
      assert_block (size, pb);
      std::memcpy (pb, & ((*pvector)[used()]), size);
      incr (size);
    };
    
  };
  
  
  // -------------------------------------------------------------
  
  
};  // namespace dcp

