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
#include <format>
#include <functional>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/thread/thread_time.hpp>
#include <dcp/common/exceptions.h>
#include <dcp/common/fixedmem_ring_buffer.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/sharedmem_structure_base.h>

/**
 * @brief This module provides an implementation of a finite queue
 *        (based on a ring buffer), which is placed in shared memory
 *        and provides access primitives incorporating interprocess
 *        synchronization
 *
 */


namespace dcp {


  /**
   * @brief Type definition for handler functions for 'push_*()'
   *        methods for writing data into the buffer
   *
   * The byte* parameter points to the start of the buffer area to
   * write into, the size_t parameter indicates the maximum number of
   * bytes that can be written into the buffer.
   *
   * The return value has to be the actual number of bytes written
   * into the buffer.
   */
  typedef std::function<size_t (byte*, size_t)> PushHandler;


  /**
   * @brief Type definition for hander functions for 'pop_*()' methods
   *        for reading data from a buffer
   *
   * The byte* parameter points to the start of the buffer area to
   * read from, the size_t parameter gives the number of bytes stored
   * in that buffer.
   */
  typedef std::function<void (byte*, size_t)> PopHandler;    // arguments: memory address to read from, amount of data available


  
  /**
   * @brief Shared memory finite queue of buffers
   *
   * The queue elements are simply memory blocks, referenced by an
   * offset relative to the start of the buffer memory. The finite
   * queue also contains mutexes and condition variables for proper
   * synchronization across processes.
   *
   * The buffers themselves are just byte blocks which user code can
   * tinker without further checks.
   *
   * @tparam numberBuffers: number of buffers in the queue, the queue
   *         can hold only this many elements
   * @tparam bufferSize: size of a buffer
   */
  
  template <uint64_t numberBuffers, size_t bufferSize>
  class ShmFiniteQueue {
  protected:

    
    /**********************************************************************
     * Protected part
     *********************************************************************/
    
    static const size_t maxQueueNameLength = 255;                 /*!< maximum length of the name of the queue */
    static const uint64_t defaultMagicNo   = 0x497E471112349876;  /*!< magicno at start of segment */

    /**
     * @brief Calculates the actual size of a buffer in memory
     *
     * This adds a little extra space to one buffer, just as a small
     * safety margin for PushHandlers that write beyond their limit.
     */
    static constexpr size_t   get_actual_buffer_size ()
    {
      const size_t s = sizeof (uint64_t);
      return s * ( (bufferSize + 4*s) / s);
    };


    /**
     * @brief Type for buffer descriptor
     *
     * Buffer descriptors are stored in the ring buffers for the free
     * list (list of free buffers) and the ring buffer for the actual
     * queue
     */
    typedef struct DescrT {
      uint64_t offs;    /*!< byte offset relative to start of 'buffer_space' where this buffer begins */
      size_t   len;     /*!< number of user data bytes stored in this buffer */
    } DescrT;


    uint64_t  magicNo = defaultMagicNo;
    char queue_name [maxQueueNameLength+1];                          /*!< storing the user-given name of the finite queue */
    FixedMemRingBuffer<DescrT, numberBuffers+1>  queue;              /*!< ring buffer with current queue elements / buffers */
    FixedMemRingBuffer<DescrT, numberBuffers+1>  freeList;           /*!< ring buffer with list of free elements / buffers */
    byte buffer_space [numberBuffers * get_actual_buffer_size()];    /*!< the actual buffer space storing user data */

    
    interprocess_mutex      mutex;               /*!< mutex protecting access to the finite queue */
    interprocess_condition  cond_empty;          /*!< condition variable telling whether queue is empty or not */
    interprocess_condition  cond_full;           /*!< condition variable telling whether queue is full or not */
    bool                    has_data = false;    /*!< flag indicating whether queue has data or not */


    /**
     * @brief Method to initialize the finite queue
     *
     * Clears queue and free list, and puts all buffers into free list
     */
    inline void initialize_queue_and_freelist ()
    {
      queue.reset ();
      freeList.reset ();
      for (uint64_t i=0; i<numberBuffers; i++)
	{
	  DescrT descr;
	  descr.offs = i * get_actual_buffer_size();
	  descr.len  = 0;
	  freeList.push (descr);
	}
    };


    /**
     * @brief Checks that the magicno has still the right value, throws if not
     *
     * @param modname: module name to include in exception 
     */
    inline void assert_magicno (std::string modname)
    {
      if (magicNo != defaultMagicNo)
	throw ShmException (std::format("{}.{}", get_queue_name (), modname), "check for magic number failed");
    };
    
    
  public:

    /**********************************************************************
     * Public interface
     *********************************************************************/
    
    ShmFiniteQueue () = delete;

    /**
     * @brief Constructor, copies queue name, initializes queue and free list
     */
    ShmFiniteQueue (const char* qname, uint64_t maxcap) :
      queue ("queue", maxcap),
      freeList ("freeList", numberBuffers)
    {
      if (!qname)
	throw ShmException ("ShmFiniteQueue", "no valid queue name");
      if (std::strlen(qname) > maxQueueNameLength)
	throw ShmException ("ShmFiniteQueue",
			    std::format("queue name {} is too long", qname));
      std::strcpy (queue_name, qname);

      initialize_queue_and_freelist ();
    };


    /**
     * @brief Returns maximum number buffers that users can put into the queue
     */
    static constexpr uint64_t get_number_buffers () { return numberBuffers; };


    /**
     * @brief Returns user-provided buffer size
     */
    static constexpr size_t   get_buffer_size () { return bufferSize; };


    /**
     * @brief Returns name of the queue
     */
    const char*               get_queue_name () const { return queue_name; };


    /**
     * @brief Reports number of buffers in queue and free list, respectively
     *
     * @param queue_size: output parameter for queue size
     * @param free_size: output parameter for free list size
     */
    void report_sizes (unsigned int& queue_size, unsigned int& free_size)
    {
      scoped_lock<interprocess_mutex> lock (mutex);
      queue_size = queue.stored_elements ();
      free_size  = freeList.stored_elements ();
    };
    

    
    // -----------------------------------------------


    /**
     * @brief Reset the queue to initial status (queue empty, free
     *        list holds all buffers)
     */
    inline void reset ()
    {
      scoped_lock<interprocess_mutex> lock (mutex);
      initialize_queue_and_freelist();
    };

    // -----------------------------------------------


    /**
     * @brief Returns number of elements / buffers in the queue
     */
    inline unsigned int stored_elements ()
    {
      scoped_lock<interprocess_mutex> lock (mutex);
      return queue.stored_elements ();
    };
    
    // -----------------------------------------------


    
    /**
     * @brief Pushes data to the end of the queue. If the queue is
     *        full, caller is put into wait state until queue empties
     *        again and the data can be pushed, or a timeout occurs
     *
     * @param handler: a push handler that is called when data can be
     *        written into the queue. When the push handler returns
     *        zero, then no data is written and the queue is left
     *        unchanged
     * @param timed_out: output parameter indicating whether any of
     *        the involved locking operations (for the mutex or the
     *        condition variable) timed out
     * @param timeoutMS: timeout value to use for locking operations
     *        in milliseconds
     *
     * Throws if the handler reports back that more data has been
     * written than fits into the buffer.
     */
    void push_wait (PushHandler handler,
		    bool& timed_out,
		    uint16_t timeoutMS = defaultLongSharedMemoryLockTimeoutMS)
    {
      if (timeoutMS==0)
	throw ShmException (std::format("{}.push_wait", get_queue_name()), "timeout is zero");
      
      timed_out        = false;
      
      const boost::posix_time::ptime timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
      
      scoped_lock<interprocess_mutex> lock (mutex, timeout);
      
      if (!lock.owns())
	{
	  timed_out = true;
	  return;
	}
      
      if (has_data)
	{
	  const boost::posix_time::ptime cv_timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
	  if (not cond_full.timed_wait (lock, cv_timeout))
	    {
	      timed_out = true;
	      return;
	    }
	}
      
      DescrT descr = freeList.pop ();
      byte* effective_address = buffer_space + descr.offs;
      descr.len = handler (effective_address, bufferSize);
      if (descr.len == 0)
	{
	  freeList.push (descr);
	  return;
	}
      if (descr.len > bufferSize)
	throw ShmException (std::format("{}.push_wait", get_queue_name()),
			    "handler exceeded buffer length");
      queue.push (descr);
      cond_empty.notify_all ();
      has_data = true;
    };
    
    // -----------------------------------------

    
    /**
     * @brief Pushes data to the end of the queue. If the queue is
     *        full, caller will not be made to wait until queue is
     *        nonempty
     *
     * @param handler: a push handler that is called when data can be
     *        written into the queue. When the push handler returns
     *        zero, then no data is written and the queue is left
     *        unchanged     
     * @param timed_out: output parameter indicating whether any of
     *        the involved locking operations (for the mutex) timed out
     * @param is_full: output parameter indicating that the queue was
     *        full and no data was written
     * @param timeoutMS: timeout value to use for locking operations
     *        in milliseconds
     *
     * Throws if the handler reports back that more data has been
     * written than fits into the buffer.     
     */
    void push_nowait (PushHandler handler,
		      bool& timed_out,
		      bool& is_full,
		      uint16_t timeoutMS = defaultLongSharedMemoryLockTimeoutMS)
    {
      if (timeoutMS==0)
	throw ShmException (std::format("{}.push_nowait", get_queue_name()), "timeout is zero");
      
      timed_out  = false;
      is_full   = false;
      
      const boost::posix_time::ptime timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
      
      scoped_lock<interprocess_mutex> lock (mutex, timeout);
      
      if (!lock.owns())
	{
	  timed_out = true;
	  return;
	}

      if (queue.isFull())
	{
	  is_full = true;
	  return;
	}
            
      DescrT descr = freeList.pop ();
      byte* effective_address = buffer_space + descr.offs;
      descr.len = handler (effective_address, bufferSize);
      if (descr.len == 0)
	{
	  freeList.push (descr);
	  return;
	}
      if (descr.len > bufferSize)
	throw ShmException (std::format("{}.push_nowait", get_queue_name()), "handler exceeded buffer length");
      queue.push (descr);
      cond_empty.notify_all ();
      has_data = true;
    };

    // -----------------------------------------

    /**
     * @brief Pushes data to the end of the queue. If the queue is
     *        full, the oldest element is pushed out and the new
     *        element is added.
     *
     * @param handler: a push handler that is called when data can be
     *        written into the queue. When the push handler returns
     *        zero, then no data is written, but the oldest element of
     *        the queue will have been removed.
     * @param timed_out: output parameter indicating whether any of
     *        the involved locking operations (for the mutex) timed out
     * @param timeoutMS: timeout value to use for locking operations
     *        in milliseconds
     *
     * Throws if the handler reports back that more data has been
     * written than fits into the buffer.

     */
    void push_wait_force (PushHandler handler,
			  bool& timed_out,
			  uint16_t timeoutMS = defaultLongSharedMemoryLockTimeoutMS)
    {
      if (timeoutMS==0)
	throw ShmException (std::format("{}.push_wait_force", get_queue_name()), "timeout is zero");
      
      timed_out        = false;
      
      const boost::posix_time::ptime timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
      
      scoped_lock<interprocess_mutex> lock (mutex, timeout);
      
      if (!lock.owns())
	{
	  timed_out = true;
	  return;
	}
            
      DescrT descr = queue.isFull () ? queue.pop () : freeList.pop ();
      byte* effective_address = buffer_space + descr.offs;
      descr.len = handler (effective_address, bufferSize);
      if (descr.len == 0)
	{
	  freeList.push (descr);
	  return;
	}
      if (descr.len > bufferSize)
	throw ShmException (std::format("{}.push_wait_force", get_queue_name()), "handler exceeded buffer length");
      queue.push (descr);
      cond_empty.notify_all ();
      has_data = true;
    };
    

    // -----------------------------------------

    /**
     * @brief Retrieves the head-of-line element of the queue, lets
     *        the user process it and then removes it from the
     *        queue. Caller is put into waiting state until the queue
     *        becomes nonempty of a timeout occurs.
     *
     * @param handler: a pop handler that is called immediately before
     *        the buffer is removed. The handler can copy the data or
     *        process it otherwise.
     * @param timed_out: output parameter indicating whether any of
     *        the involved locking operations (for the mutex or the
     *        condition variable) timed out
     * @param further_entries: output parameter indicating whether
     *        after removing the head-of-line entry there are
     *        further entries available in the queue
     * @param timeoutMS: timeout value to use for locking operations
     *        in milliseconds
     */
    
    void pop_wait (PopHandler handler,
		   bool& timed_out,
		   bool& further_entries,
		   uint16_t timeoutMS = defaultLongSharedMemoryLockTimeoutMS)
    {
      if (timeoutMS==0)
	throw ShmException (std::format("{}.pop_wait", get_queue_name()), "timeout is zero");
      
      timed_out       = false;
      further_entries = false;
      
      const boost::posix_time::ptime timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
      
      scoped_lock<interprocess_mutex> lock (mutex, timeout);
      
      if (!lock.owns())
	{
	  timed_out = true;
	  return;
	}
      
      if (!has_data)
	{
	  const boost::posix_time::ptime cv_timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
	  
	  if (not cond_empty.timed_wait (lock, cv_timeout))
	    {
	      timed_out = true;
	      return;
	    }
	}

      if (queue.isEmpty())
	throw ShmException (std::format("{}.pop_wait", get_queue_name()), "queue is empty, but has_data is true");
      
      DescrT descr = queue.pop ();
      byte* effective_address = buffer_space + descr.offs;
      handler (effective_address, descr.len);
      descr.len = 0;
      freeList.push (descr);
      
      if (queue.isEmpty())
	has_data = false;
      else
	further_entries = true;
      
      cond_full.notify_all ();
    };

    // -----------------------------------------

    /**
     * @brief Retrieves the head-of-line element of the queue, lets
     *        the user process it and then removes it from the
     *        queue. If queue is empty, returns
     *
     * @param handler: a pop handler that is called immediately before
     *        the buffer is removed. The handler can copy the data or
     *        process it otherwise.
     * @param timed_out: output parameter indicating whether any of
     *        the involved locking operations (for the mutex) timed out
     * @param further_entries: output parameter indicating whether
     *        after removing the head-of-line entry there are
     *        further entries available in the queue
     * @param timeoutMS: timeout value to use for locking operations
     *        in milliseconds
     */
    
    void pop_nowait (PopHandler handler,
		     bool& timed_out,
		     bool& further_entries,
		     uint16_t timeoutMS = defaultLongSharedMemoryLockTimeoutMS)
    {
      //assert_magicno ("pop_nowait");
      if (timeoutMS==0)
	throw ShmException (std::format("{}.pop_nowait", get_queue_name()), "timeout is zero");
      
      timed_out       = false;
      further_entries = false;
      
      const boost::posix_time::ptime timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
      
      scoped_lock<interprocess_mutex> lock (mutex, timeout);
      
      if (!lock.owns())
	{
	  timed_out = true;
	  return;
	}
      
      if (!has_data)
	{
	  return;
	}

      if (queue.isEmpty())
	throw ShmException (std::format("{}.pop_nowait", get_queue_name()), "queue is empty, but has_data is true");
      
      DescrT descr = queue.pop ();
      byte* effective_address = buffer_space + descr.offs;
      handler (effective_address, descr.len);
      descr.len = 0;
      freeList.push (descr);
      
      if (queue.isEmpty())
	has_data = false;
      else
	further_entries = true;
      
      cond_full.notify_all ();
    };

    
    // -----------------------------------------

    /**
     * @brief Retrieves all elements in the queue, processing them in
     *        order. Caller is put into waiting state until the queue
     *        becomes nonempty of a timeout occurs.
     *
     * @param handler: a pop handler that is called immediately before
     *        the buffer is removed. The handler can copy the data or
     *        process it otherwise.
     * @param timed_out: output parameter indicating whether any of
     *        the involved locking operations (for the mutex or the
     *        condition variable) timed out
     * @param timeoutMS: timeout value to use for locking operations
     *        in milliseconds
     */
    void popall_wait (PopHandler handler,
		      bool& timed_out,
		      uint16_t timeoutMS = defaultLongSharedMemoryLockTimeoutMS)
    {
      if (timeoutMS==0)
	throw ShmException (std::format("{}.popall_wait", get_queue_name()), "timeout is zero");
      
      timed_out       = false;
      
      const boost::posix_time::ptime timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
      
      scoped_lock<interprocess_mutex> lock (mutex, timeout);
      
      if (!lock.owns())
	{
	  timed_out = true;
	  return;
	}
      
      if (!has_data)
	{
	  const boost::posix_time::ptime cv_timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
	  
	  if (not cond_empty.timed_wait (lock, cv_timeout))
	    {
	      timed_out = true;
	      return;
	    }
	}

      while (not queue.isEmpty())
	{
	  DescrT descr = queue.pop ();
	  byte* effective_address = buffer_space + descr.offs;
	  handler (effective_address, descr.len);
	  descr.len = 0;
	  freeList.push (descr);
	}
      
      has_data = false;
      
      cond_full.notify_all ();
    };


    // -----------------------------------------


    /**
     * @brief Retrieves all elements in the queue, processing them in
     *        order. If the queue is empty, the method returns without
     *        further action or waiting.
     *
     * @param handler: a pop handler that is called immediately before
     *        the buffer is removed. The handler can copy the data or
     *        process it otherwise.
     * @param timed_out: output parameter indicating whether any of
     *        the involved locking operations (for the mutex or the
     *        condition variable) timed out
     * @param timeoutMS: timeout value to use for locking operations
     *        in milliseconds
     */

    void popall_nowait (PopHandler handler,
			bool& timed_out,
			uint16_t timeoutMS = defaultLongSharedMemoryLockTimeoutMS)
    {
      if (timeoutMS==0)
	throw ShmException (std::format("{}.popall_nowait", get_queue_name()), "timeout is zero");
      
      timed_out       = false;
      
      const boost::posix_time::ptime timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
      
      scoped_lock<interprocess_mutex> lock (mutex, timeout);
      
      if (!lock.owns())
	{
	  timed_out = true;
	  return;
	}
      
      if (!has_data)
	{
	  return;
	}

      while (not queue.isEmpty())
	{
	  DescrT descr = queue.pop ();
	  byte* effective_address = buffer_space + descr.offs;
	  handler (effective_address, descr.len);
	  descr.len = 0;
	  freeList.push (descr);
	}
      
      has_data = false;
      
      cond_full.notify_all ();
    };
    
    
    
    // -----------------------------------------

    /**
     * @brief Retrieves the head-of-line element of the queue, lets
     *        the user process it and leaves it in the queue without
     *        modifying it. Caller is put into waiting state until the
     *        queue becomes nonempty of a timeout occurs.
     *
     * @param handler: a pop handler that is called immediately before
     *        the buffer is removed. The handler can copy the data or
     *        process it otherwise.
     * @param timed_out: output parameter indicating whether any of
     *        the involved locking operations (for the mutex or the
     *        condition variable) timed out
     * @param timeoutMS: timeout value to use for locking operations
     *        in milliseconds
     */    
    void peek_wait (PopHandler handler,
		    bool& timed_out,
		    uint16_t timeoutMS = defaultLongSharedMemoryLockTimeoutMS)
    {
      if (timeoutMS==0)
	throw ShmException (std::format("{}.peek_wait", get_queue_name()), "timeout is zero");
      
      timed_out       = false;
      
      const boost::posix_time::ptime timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
      
      scoped_lock<interprocess_mutex> lock (mutex, timeout);
      
      if (!lock.owns())
	{
	  timed_out = true;
	  return;
	}
      
      if (!has_data)
	{
	  const boost::posix_time::ptime cv_timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
	  
	  if (not cond_empty.timed_wait (lock, cv_timeout))
	    {
	      timed_out = true;
	      return;
	    }
	}
      
      DescrT descr = queue.peek ();
      byte* effective_address = buffer_space + descr.offs;
      handler (effective_address, descr.len);
    };


    // -----------------------------------------

    /**
     * @brief Lets you do stuff with the oldest element in the queue
     *        without removing it. Does not wait until a packet is in the queue
     */
    /**
     * @brief Retrieves the head-of-line element of the queue, lets
     *        the user process it and leaves it in the queue without
     *        modifying it. Returns immediately without any further
     *        action if the queue is empty
     *
     * @param handler: a pop handler that is called immediately before
     *        the buffer is removed. The handler can copy the data or
     *        process it otherwise.
     * @param timed_out: output parameter indicating whether any of
     *        the involved locking operations (for the mutex or the
     *        condition variable) timed out
     * @param timeoutMS: timeout value to use for locking operations
     *        in milliseconds
     */    
    void peek_nowait (PopHandler handler,
		      bool& timed_out,
		      uint16_t timeoutMS = defaultLongSharedMemoryLockTimeoutMS)
    {
      if (timeoutMS==0)
	throw ShmException (std::format("{}.peek_nowait", get_queue_name()), "timeout is zero");
      
      timed_out       = false;
      
      const boost::posix_time::ptime timeout (boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
      
      scoped_lock<interprocess_mutex> lock (mutex, timeout);
      
      if (!lock.owns())
	{
	  timed_out = true;
	  return;
	}
      
      if (!has_data)
	{
	  return;
	}
      
      DescrT descr = queue.peek ();
      byte* effective_address = buffer_space + descr.offs;
      handler (effective_address, descr.len);
    };


    // -----------------------------------------
    
  };
  
};  // namespace dcp
