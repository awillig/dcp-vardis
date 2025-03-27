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

#include <format>
#include <dcp/common/exceptions.h>
#include <dcp/common/global_types_constants.h>


namespace dcp {

  /**
   * @brief This template class provides a ring buffer organised in a
   *        fixed region of memory, with a fixed number of elements
   *        (of parameterized type)
   *
   * @tparam ElemT: type of elements of a ring buffer. Should have
   *         a copy constructor
   * @tparam maxRingBufferElements: number of array elements in the
   *         ring buffer
   *
   * Note that besides the the number of array elements, the ring
   * buffer can have a maximum capacity, which is the maximum number
   * of elements that user code can store in the ring buffer. This
   * capacity must be strictly smaller than maxRingBufferElements.
   */

  template <typename ElemT, size_t maxRingBufferElements>
  class FixedMemRingBuffer {

    static_assert (maxRingBufferElements >= 2, "FixedMemRingBuffer: maxRingBufferElements must be at least two");
    
  private:

    /**
     * @brief Maximum length of the human-readable name of a ring buffer.
     *
     * The name is only relevant for logging / debugging purposes
     */
    static constexpr size_t maxRingBufferNameLength =  64;

    
    uint64_t         in                    = 0;  /*!< Index where next element will be stored */
    uint64_t         out                   = 0;  /*!< Index from where next element will be removed */
    uint64_t         maxCapacity           = 0;  /*!< Maximum number of elements that can be stored in the queue */
    uint64_t         currentNumberElements = 0;  /*!< Current number of elements in the queue */
    char             rbName[maxRingBufferNameLength];  /*!< Name of the ring buffer */
    ElemT            the_ring[maxRingBufferElements];  /*!< The actual array containing the elements */

    
  public:

    FixedMemRingBuffer () = delete;

    /**
     * @brief Constructor
     *
     * @param name: descriptive name of ring buffer, must be non-null
     * @param maxCap: maximum number of elements that can be stored in ring buffer
     *
     * Initializes a ring buffer. Throws when name is nullptr or too
     * long, or when maxCap is either zero or too large (larger than
     * maxRingBufferElements minus one)
     */
    FixedMemRingBuffer (const char* name, uint64_t maxCap)
      : maxCapacity (maxCap)
      {
	if (!name)
	  throw RingBufferException("invalid name");

	if (std::strlen(name) > maxRingBufferNameLength-1)
	  throw RingBufferException(std::format("name is too long at {} bytes ({} bytes allowed)", std::strlen(name), maxRingBufferNameLength-1));

	if ((maxCap < 1) || (maxCap >= maxRingBufferElements))
	  throw RingBufferException("illegal value for maxCapacity");

	std::strcpy(rbName, name);
      };

    
    /**
     * @brief Returns descriptive name.
     */
    inline const char* get_name () const { return rbName; };


    /**
     * @brief Returns number of buffers to use for the ringbuffer
     */
    static size_t get_max_ring_buffer_elements () {return maxRingBufferElements; };

    
    /**
     * @brief Indicates whether ring buffer is currently empty (true) or not (false).
     */
    inline bool isEmpty () const { return currentNumberElements == 0; };


    /**
     * @brief Indicates whether ring buffer is currently full (true)
     *        or not (false).
     */
    inline bool isFull () const { return currentNumberElements == maxCapacity; };

    
    /**
     * @brief Returns the current number of elements in the ring buffer
     */
    inline unsigned int stored_elements () const { return currentNumberElements; };


    /**
     * @brief Returns the maximum number of elements that is allowed
     *        to be stored in the ringbuffer
     */
    inline uint64_t get_max_capacity () const { return maxCapacity; };
    

    /**
     * @brief Pops/removes oldest element/buffer from the ring buffer
     *        and returns the buffer.
     */
    inline ElemT pop () {
      if (isEmpty()) throw RingBufferException ("pop(): trying to pop from empty ring buffer");
      uint64_t rvidx = out;
      out = (out + 1) % maxRingBufferElements;
      currentNumberElements--;
      return the_ring[rvidx];
    };


    /**
     * @brief Returns the oldest element in the ring buffer without
     *        removing it.
     */
    inline ElemT peek () const {
      if (isEmpty()) throw RingBufferException ("peek(): trying to peek from empty ring buffer");
      return the_ring[out];
    };


    /**
     * @brief Pushes/adds new element / buffer into ring buffer.
     */
    inline void push(const ElemT& buf) {
      if (isFull()) throw RingBufferException ("push(): trying to push onto full ring buffer");
      the_ring[in] = buf;
      in = (in + 1) % maxRingBufferElements;
      currentNumberElements++;
    };


    /**
     * @brief Re-sets the ring buffer into an empty state
     */
    inline void reset ()
    {
      in   = 0;
      out  = 0;
      currentNumberElements = 0;
    };
    

    /**
     * @brief Outputting ring buffer description
     */
    std::ostream& operator<<(std::ostream& os)
    {
      os << "RingBuffer{name=" << rbName
	 << ",in=" << in
	 << ",out=" << out
	 << ",maxCap=" << maxCapacity
	 << ",numElem=" << currentNumberElements
	 << ",maxElements=" << maxRingBufferElements
	 << "}";
      return os;
    };


  };  
    
};  // namespace dcp
