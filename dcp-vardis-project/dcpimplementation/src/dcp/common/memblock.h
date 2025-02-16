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

#include <cstdint>
#include <cstring>
#include <dcp/common/foundation_types.h>

namespace dcp {

  /**
   * @brief Class describing and providing management of a memory block.
   *
   * A memory block is described by its length and a pointer to a
   * memory area. This manages allocation of the memory, and supports
   * operations such as copying, moving etc. The allocated memory is
   * freed when a MemBlock object ceases to exist.
   */

  class MemBlock {
  public:
    size_t    length     = 0;
    byte*     data       = nullptr;
    
  public:
    
    MemBlock () : length(0), data(nullptr) {};


    /**
     * @brief Copy constructor
     *
     * @param other: other memory block to copy contents from
     *
     * Allocates new memory and copies contents of given MemBlock
     */
    MemBlock (const MemBlock& other) : length(0), data(nullptr) {set (other.length, other.data); };


    /**
     * @brief Constructing MemBlock from memory
     *
     * @param len: length of memory block to copy
     * @param pdata: pointer to memory block to copy from
     *
     * Allocates new memory and copies given data into it
     */
    MemBlock (size_t len, byte* pdata) : length(0), data(nullptr) { length = len; set (len, pdata); };


    /**
     * @brief Move constructor
     */
    MemBlock (MemBlock&& other)
      {
	length      = other.length;
	data        = other.data;
	other.length     = 0;
	other.data       = nullptr;
      };


    /**
     * @brief Destructor, de-allocates memory if necessary
     */
    ~MemBlock ()
      {
	if (data)
	  {
	    delete [] data;
	  }
      };


    /**
     * @brief Deletes current memory block if necessary, allocates new
     *        one and copies given data block into it (if one is
     *        provided)
     *
     * @param len: length of memory block to copy
     * @param pdata: pointer to memory block to copy from
     */
    inline void set (size_t len, byte* pdata)
    {
      if (data)
	{
	  delete [] data;
	}
      if (len > 0 && pdata)
	{
	  length = len;
	  data = new byte [length];
	  std::memcpy(data, pdata, length);
	}
      else
	{
	  length  = 0;
	  data    = nullptr;
	}
    };


    /**
     * @brief Assignment operator, deleting current memory block if
     *        necessary, copying memory block from other MemBlock
     */
    inline MemBlock& operator= (const MemBlock& other)
      {
	if (this != &other)
	  {
	    if (data)
	      {
		delete [] data;
	      }
	    length = 0;
	    data   = nullptr;
	    set (other.length, other.data);
	  }
	return *this;
      };


    /**
     * @brief Move assignment, deletes current memory block if
     *        necessary, moves contents of other MemBlock into this
     *        one
     */
    inline MemBlock& operator= (MemBlock&& other)
    {
      if (this != &other)
	{
	  if (data)
	    {
	      delete [] data;
	    }
	  length = 0;
	  data   = nullptr;
	  
	  if (other.data)
	    {
	      data   = other.data;
	      length = other.length;
	      other.data   = nullptr;
	      other.length = 0;
	    }
	}
      return *this;
    };


    /**
     * @brief Check equality / inequality of two MemBlock's
     *
     * ISSUE (possible): Currently when the object has len>0 but no
     * valid pointer to a memory block then this is not raised as a
     * critical error, but rather as simple inequality. Not sure this
     * is the right choice.
     */
    inline bool operator== (const MemBlock& other) const
    {
      if (length == 0)
	return (other.length == 0);

      if (length != other.length)
	return false;

      if ((data == nullptr) or (other.data == nullptr))
	return false;
      
      return (0 == std::memcmp (data, other.data, length));
    };
    inline bool operator!= (const MemBlock& other) const { return (not (*this == other)); };
    
  };

  
}; // namespace dcp
