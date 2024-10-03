// Copyright (C) 2024 Andreas Willig, University of Canterbury
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#pragma once

#include <dcp/common/FoundationTypes.h>

namespace dcp {

// -------------------------------------------------------------

/*
 * An assembly area is an abstraction for an area of memory into which
 * outgoing packets are serialized, or from which received packets are
 * deserialized. After describing its abstract operations we define two
 * specific types of areas, one just given by a memory block, another
 * one being an OMNeT++ byte vector.
 *
 */

// =============================================================================
// =============================================================================
// Exceptions
// =============================================================================
// =============================================================================


class AreaException : public std::exception {
 public:
  AreaException (const std::string& message) : message_(message) {};
  const char* what() const throw() { return message_.c_str(); };
 private:
  std::string message_;
};

class AssemblyAreaException : public std::exception {
 public:
  AssemblyAreaException (const std::string& message) : message_(message) {};
  const char* what() const throw() { return message_.c_str(); };
 private:
  std::string message_;
};

class DisassemblyAreaException : public std::exception {
 public:
  DisassemblyAreaException (const std::string& message) : message_(message) {};
  const char* what() const throw() { return message_.c_str(); };
 private:
  std::string message_;
};


// =============================================================================
// =============================================================================
// General area, abstract base class
// =============================================================================
// =============================================================================


class AreaBase {

 private:
  size_t bytes_available;     // bytes that can still be written / retrieved
  size_t initial_available;   // bytes initially available in the area
  size_t bytes_used;          // bytes already written / retrieved

 public:

  AreaBase () = delete;
  AreaBase (size_t available) : bytes_available(available), initial_available(available), bytes_used(0) {};

  inline size_t used() const { return bytes_used; };
  inline size_t available() const { return bytes_available; };
  inline size_t initial() const { return initial_available; };

  /*
   * updates used / available variables and throws exception when insufficient availability
   */
  inline void incr (size_t by = 1)
  {
      if (bytes_available < by) throw AreaException("Area::incr: insufficient bytes available");
      bytes_used       = bytes_used + by;
      bytes_available  = bytes_available - by;
  };


  /*
   * Performs certain checks when attempting to serialize a block of bytes
   */
  inline void block_prechecks (size_t size, byte* pb)
  {
      if ((size == 0) || (pb == nullptr)) throw AssemblyAreaException("AssemblyArea::block_prechecks: illegal parameters");
      if (bytes_available < size) throw AssemblyAreaException("AssemblyArea::block_prechecks: not enough space available");
  }

  /*
   * Re-sets used / available information to their initial values
   */
  inline void reset ()
  {
    bytes_available = initial_available;
    bytes_used      = 0;
  };
};


// =============================================================================
// =============================================================================
// Assembly and disassembly area base classes
// =============================================================================
// =============================================================================


class AssemblyArea : public AreaBase {
public:
    AssemblyArea () = delete;
    AssemblyArea (size_t size) : AreaBase (size) {};

    // abstract method for serializing one byte
    virtual void serialize_byte (byte b) = 0;

    // Default method for serialization of a byte block. Should be
    // overloaded with more efficient methods where possible.
    virtual void serialize_byte_block (size_t size, byte* pb)
    {
        block_prechecks (size, pb);
        for (size_t i = 0; i < size; i++)
        {
            serialize_byte(pb[i]);
        }
    };

};

// -------------------------------------------------------------

class DisassemblyArea : public AreaBase {
public:
    DisassemblyArea () = delete;
    DisassemblyArea (size_t size) : AreaBase (size) {};

    /*
     * Abstract method for deserializing one byte
     */
    virtual byte deserialize_byte () = 0;

    /*
     * Abstract method for peeking a byte without deserializing it
     */
    virtual byte peek_byte () = 0;

    // Default method for deserialization of a byte block. Should be
    // overloaded with more efficient methods where possible.
    virtual void deserialize_byte_block (size_t size, byte* pb)
    {
        block_prechecks (size, pb);
        for (size_t i = 0; i < size; i++)
        {
            *pb = deserialize_byte ();
            pb++;
        }
    };

};

// =============================================================================
// =============================================================================
// Assembly and disassembly areas working with a memory chunk
// =============================================================================
// =============================================================================

// -------------------------------------------------------------
/*
 * Assembly area using an in-memory chunk of bytes. Depending on constructor
 * used, memory is either allocated and managed here or by calling code.
 */

class MemoryChunkAssemblyArea : public AssemblyArea {
 private:
    bool    deallocate  = false;
    size_t  bufferSize  = 0;
    byte*   buffer      = nullptr;
    byte*   pointer     = nullptr;

 public:

  MemoryChunkAssemblyArea (size_t size)
     : AssemblyArea (size)
     , buffer (nullptr)
     , pointer (nullptr)
  {
      if (size == 0) throw AssemblyAreaException ("MemoryChunkAssemblyArea: zero buffer size");

      buffer      = new byte [size];
      pointer     = buffer;
      deallocate  = true;

      std::memset(pointer, 0, size);
  }

  MemoryChunkAssemblyArea (size_t size, byte* memblock)
     : AssemblyArea (size)
     , buffer (memblock)
     , pointer (memblock)
  {
      if (size == 0) throw AssemblyAreaException ("MemoryChunkAssemblyArea: zero buffer size");
      if (!memblock) throw AssemblyAreaException ("MemoryChunkAssemblyArea: memblock is null");

      deallocate = false;
  };

  ~MemoryChunkAssemblyArea ()
  {
      if (deallocate && buffer) delete [] buffer;
  };

  byte* getBufferPtr () const { return buffer; };

  virtual void serialize_byte (byte b)
  {
      if (available() == 0) throw AssemblyAreaException ("MemoryChunkAssemblyArea::serialize_byte: no byte available");
      *pointer = b;
      pointer++;
      incr();
  };

  virtual void serialize_byte_block (size_t size, byte* pb)
  {
      block_prechecks (size, pb);
      std::memcpy (pointer, pb, size);
      incr (size);
      pointer += size;
  };

  virtual void reset ()
  {
    AssemblyArea::reset();
    pointer = buffer;
  }
};

// -------------------------------------------------------------
/*
 * Disassembly area using an in-memory chunk of bytes supplied and
 * managed by the calling code.
 */


class MemoryChunkDisassemblyArea : public DisassemblyArea {

private:
    byte*   buffer      = nullptr;
    byte*   pointer     = nullptr;

public:

    MemoryChunkDisassemblyArea () = delete;
    MemoryChunkDisassemblyArea (size_t size, byte* memblock)
       : DisassemblyArea (size)
       , buffer(memblock)
       , pointer(memblock)
    {};

    virtual byte deserialize_byte ()
    {
        if (available() == 0) throw DisassemblyAreaException ("MemoryChunkDisassemblyArea::deserialize_byte: no byte available");
        byte rv = *pointer;
        pointer++;
        incr();
        return rv;
    };

    virtual byte peek_byte ()
    {
        if (available() == 0) throw DisassemblyAreaException ("MemoryChunkDisassemblyArea::deserialize_byte: no byte available");
        return *pointer;
    };

    virtual void deserialize_byte_block (size_t size, byte* pb)
    {
        block_prechecks (size, pb);
        std::memcpy (pb, pointer, size);
        incr (size);
        pointer += size;
    };

    virtual void reset ()
    {
      AreaBase::reset();
      pointer = buffer;
    }

};


// -------------------------------------------------------------
/*
 * Assembly area using a std::vector<byte> (as required by OMNeT++/INET).
 * Depending on choice of constructor, either the byte vector is created
 * and managed here, or by the calling code.
 */

class ByteVectorAssemblyArea : public AssemblyArea {
 private:
    std::vector<byte>*  pvector = nullptr;
    bool deallocate = false;

 public:

  ByteVectorAssemblyArea (size_t size)
     : AssemblyArea (size)
  {
      if (size == 0) throw AssemblyAreaException ("ByteVectorAssemblyArea: zero buffer size");
      pvector = new std::vector<byte> (size);
      deallocate = true;
  }

  ByteVectorAssemblyArea (size_t size, std::vector<byte>& vect)
     : AssemblyArea (size)
  {
      pvector    = &vect;
      deallocate = false;
  };

  ~ByteVectorAssemblyArea ()
  {
      if (deallocate) delete pvector;
  };

  const std::vector<byte>* getVectorPtr () const { return pvector; };

  virtual void serialize_byte (byte b)
  {
      if (available() == 0) throw AssemblyAreaException ("ByteVectorAssemblyArea: no space left");
      (*pvector)[used()] = b;
      incr();
  };

  virtual void serialize_byte_block (size_t size, byte* pb)
  {
      block_prechecks (size, pb);
      std::memcpy (& ((*pvector)[used()]), pb, size);
      incr (size);
  };

};


class ByteVectorDisassemblyArea : public DisassemblyArea {

private:
    const std::vector<byte>*   pvector = nullptr;

public:

    ByteVectorDisassemblyArea () = delete;

    ByteVectorDisassemblyArea (const std::vector<byte>& vect)
       : DisassemblyArea(vect.size())
       , pvector(&vect)
    {};

    virtual byte deserialize_byte ()
    {
        if (available() == 0) throw DisassemblyAreaException ("ByteVectorAssemblyArea::deserialize_byte: no space left");
        byte rv = (*pvector)[used()];
        incr();
        return rv;
    };

    virtual byte peek_byte ()
    {
        if (available() == 0) throw DisassemblyAreaException ("ByteVectorAssemblyArea::peek_byte: no space left");
        byte rv = (*pvector)[used()];
        return rv;
    };


    virtual void deserialize_byte_block (size_t size, byte* pb)
    {
        block_prechecks (size, pb);
        std::memcpy (pb, & ((*pvector)[used()]), size);
        incr (size);
    };

};


// -------------------------------------------------------------


};  // namespace dcp

