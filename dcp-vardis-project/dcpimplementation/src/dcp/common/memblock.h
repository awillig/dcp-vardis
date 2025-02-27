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
   *
   * Through the 'do_delete' flag it is also possible to suppress
   * deletion of the memory block that an object points to
   */

  class MemBlock {
  public:
    size_t    length     = 0;          /*!< Length of memory block */
    byte*     data       = nullptr;    /*!< Location of memory block in memory */
    bool      do_delete  = true;       /*!< Whether or not the referenced memory block should be deleted */
    
  public:
    
    MemBlock () {};


    /**
     * @brief Copy constructor
     *
     * @param other: other memory block to copy contents from
     *
     * Allocates new memory and copies contents of given MemBlock
     */
    MemBlock (const MemBlock& other)
    {
      do_delete = other.do_delete;
      set (other.length, other.data);
    };


    /**
     * @brief Constructing MemBlock from memory
     *
     * @param len: length of memory block to copy
     * @param pdata: pointer to memory block to copy from
     *
     * Allocates new memory and copies given data into it
     */
    MemBlock (size_t len, byte* pdata)
    {
      length = len;
      do_delete = true;
      set (len, pdata);
    };


    /**
     * @brief Move constructor
     */
    MemBlock (MemBlock&& other)
      {
	length      = other.length;
	data        = other.data;
	do_delete   = other.do_delete;
	other.length     = 0;
	other.data       = nullptr;
	other.do_delete  = true;
      };


    /**
     * @brief Destructor, de-allocates memory if necessary
     */
    ~MemBlock ()
      {
	check_delete ();
      };


    /**
     * @brief Checks if we have valid memory and are requested to
     *        delete, then deletes memory
     */
    inline void check_delete ()
    {
      if ((data != nullptr) and do_delete)
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
      check_delete ();
      if (len > 0 && pdata)
	{
	  length    = len;
	  data      = new byte [length];
	  do_delete = true;
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
	    do_delete = other.do_delete;
	    check_delete ();
	    if (do_delete)
	      {
		set (other.length, other.data);
	      }
	    else
	      {
		length = other.length;
		data   = other.data;
	      }
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
	  do_delete = other.do_delete;
	  check_delete ();
	  data   = other.data;
	  length = other.length;
	  other.data       = nullptr;
	  other.length     = 0;
	  other.do_delete  = true;
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

      if (    (do_delete != other.do_delete)
           || (length != other.length)
	   || (data == nullptr)
	   || (other.data == nullptr))
	return false;
      
      return (0 == std::memcmp (data, other.data, length));
    };
    inline bool operator!= (const MemBlock& other) const { return (not (*this == other)); };
    
  };

  
}; // namespace dcp
