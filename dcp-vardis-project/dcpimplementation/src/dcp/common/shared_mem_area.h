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
#include <format>
#include <iostream>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/thread/thread_time.hpp>
#include <dcp/common/configuration.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/foundation_types.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/ring_buffer.h>


using namespace boost::interprocess;
using namespace std::chrono_literals;

/**
 * @brief This module contains support for shared-memory based
 *        inter-process communications between different processes
 *
 * For communicating between two processes, a named shared memory
 * segment is used, which is mapped into the memory of each
 * participating process (at different addresses usually). This shared
 * memory segment is split into two parts: a control segment and a
 * buffer segment.
 *
 * The buffer segment is simply a contiguous array of fixed-size
 * memory buffers (type SharedMemBuffer), into which processes can
 * write or from which they can read.
 *
 * The control segment contains the data relevant for controlling
 * access to the buffers in the buffer segment. Aside from a mutex
 * controlling access to the control segment, the control segment will
 * typically contain so-called ring buffers (another term are cyclic
 * queues), into which the memory buffers are organised. The ring
 * buffers themselves are a management structure which contain
 * descriptors for the SharedMemBuffer buffers belonging to the ring
 * buffer.
 *
 * An important design constraint is that any data stored in the
 * control segment must be of fixed length, dynamically allocated
 * memory structures like lists cannot be used as they in general will
 * have different addresses and layouts in the participating
 * processes. In particular, the control segment should not contain
 * any pointers.
 *
 * The control segment is generally stored at the start of the shared
 * memory area, then the buffer segment starts following some gap
 * space.
 *
 * In addition to the control segment and the buffer segment (which
 * both are stored in shared memory), there is a class ShmBufferPool,
 * which holds all the information that one process has about the
 * shared memory segment (including pointers to the start of the
 * control and buffer segments).
 *
 * Shared memory segments are used for example between BP and one of
 * its client protocols, and between Vardis and one of its client
 * applications.
 */



namespace dcp {


  /***************************************************************************
   * Base class and access support class for control segments
   **************************************************************************/

  /**
   * @brief Magic number stored at the start of a control segment, can be used
   *        for mild integrity checking
   */
  const uint64_t controlSegmentMagicNo = 0x4711497E01020304;


  /**
   * @brief Base class for control segments, all actual control
   *        segments must be derived from this
   *
   * Note that an object of this (or a derived) type must be stored in
   * the control segment of the shared memory (at its start), so that
   * the mutex is shared by all participating processes
   */
  class ShmControlSegmentBase {
  public:

    uint64_t  magicNo = controlSegmentMagicNo; /*!< Magic number at the start of control segment */

    
    /**
     * @brief Mutex to be used by participating processes to ensure
     *        mutual exclusion when accessing the control segment
     */
    boost::interprocess::interprocess_mutex mutex;


    /**
     * @brief Checks that the magicno at the start of the control
     *        segment has not been overwritten. Throws if it has.
     */
    void assert_magicno () const {
      if (magicNo != controlSegmentMagicNo )
	throw ShmException ("ShmControlSegmentBase::assert_magicno failed");
    };
    
  };


  // ------------------------------------------------------------

  
  /**
   * @brief Capture the mutex for a control segment and release it at
   *        the end of object lifetime
   *
   * Note: an object of this class maintains a pointer to an object of
   * class ShmControlSegmentBase or derived. This other object must
   * have a lifetime at least as long as the lifetime of this locking
   * object, otherwise undefined behaviour results.
   */
  class ScopedShmControlSegmentLock {

    /**
     * @brief Pointer to the start of the control segment in shared memory area
     */
    ShmControlSegmentBase* pseg = nullptr; 

  public:

    ScopedShmControlSegmentLock () = delete;


    /**
     * @brief Constructor taking an existing control segment and a
     *        timeout. Attempts to acquire the lock within the time
     *        given by the timeout. Throws if lock could not be
     *        acquired.
     *
     * @param cs: The existing control segment object, containing the mutex
     * @param timeoutMS: timeout for obtaining the lock
     */
    ScopedShmControlSegmentLock (ShmControlSegmentBase& cs, uint16_t timeoutMS = defaultSharedMemoryLockTimeoutMS)
    {
      pseg = & cs;
      cs.assert_magicno ();
      const boost::posix_time::ptime timeout(boost::get_system_time() + boost::posix_time::milliseconds(timeoutMS));
      if (not cs.mutex.timed_lock (timeout))
	throw ShmException ("ScopedShmControlSegmentLock: timeout on lock");
    };


    /**
     * @brief Destructor, releases the lock
     */
    ~ScopedShmControlSegmentLock ()
    {
      if (pseg)
	{
	  pseg->assert_magicno ();
	  pseg->mutex.unlock();
	}
      pseg = nullptr;
    };
  };
  

  /***************************************************************************
   * Class containing the data that a process is holding for shared memory segment
   **************************************************************************/


  /**
   * @brief This is the main class that participating processes use to
   *        work with shared memory.
   *
   * It distinguishes between the 'server' (which actually creates the
   * shared memory segment) and the 'client' (which just attaches
   * itself to it). It allows both to query data describing the
   * segment, in particular pointers to the start of the control and
   * buffer segments, respectively.
   */
  class ShmBufferPool {
  private:

    /**
     * @brief Maximum size that an actual control segment type should
     *        have in the shared memory
     */
    static constexpr size_t maximumControlSegmentSize = (1 << 16) - (1 << 12);


    /**
     * @brief Actual size of control segment in shared memory, we add
     *        some additional gap space between the end of the actual
     *        control segment and the start of the buffer segment
     */
    static constexpr size_t actualControlSegmentSize  = (1 << 16);


    /**
     * @brief The BOOST shared memory object describing the shared memory area
     */
    shared_memory_object  shm_obj;
    

    /**
     * @brief User-assigned name of the shared memory area
     *
     * Stored as zero-terminated string
     */
    char                  areaName[maxShmAreaNameLength+1];


    /**
     * @brief Auxiliary Boost structure to get a valid pointer to the shared
     *        memory area
     */
    mapped_region         region;


    /**
     * @brief Pointers to the start of the control segment and buffer segment in memory
     */
    ShmControlSegmentBase* controlSegmentPtr = nullptr;
    byte*                  bufferSegmentPtr  = nullptr;


    /**
     * @brief Storing requested buffer size and number of buffers
     */
    size_t              requestedBufferSize        = 0;
    uint64_t            requestedNumberBuffers     = 0;

    
  public:

    ShmBufferPool () = delete;


    /**
     * @brief Constructor, creates shared memory area ('server') or
     *        attaches to existing area ('client')
     *
     * @param area_name: user-supplied name of shared memory area, its
     *        length is checked against maxShmAreaNameLength
     * @param isCreator: if true, constructor actually creates the
     *        area (which must not yet exist). This will be set to
     *        true by the server, clients should use false.
     * @param control_seg_size: specifies the size of the control
     *        segment to be created
     * @param bufferSize: size of an individual buffer, will be
     *        rounded up to create some safety margin. Value
     *        does not matter for client     
     * @param numberBuffers: Number of buffers in the buffer
     *        segment. Value does not matter for client.
     *
     * If called on server (isCreator=true), then the shared memory
     * area will be created, and a ControlSegmentType structure (plus
     * some safety margin) will be initialized at its start. If called
     * by client protocol (isCreator=false), we will attempt to attach
     * to the given area. Throws exceptions when either of these
     * operations fails.
     */
    ShmBufferPool (const char*     area_name,
		   bool            isCreator,
		   const size_t    control_seg_size,
		   const size_t    bufferSize,
		   uint64_t        numberBuffers);
      

    /**
     * @brief Destructor, releases the shared memory area.
     */
    virtual ~ShmBufferPool ()
    {
      shm_obj.remove (shm_obj.get_name());
    };
    

    /**
     * @brief Returns pointer to start of the control segment (also at
     *        the start of the shared memory area).
     */
    inline ShmControlSegmentBase* getControlSegmentPtr () const { return controlSegmentPtr; };


    /**
     * @brief Returns pointer to the start of the buffer segment
     */
    inline byte* getBufferSegmentPtr () const
    {
      return ((byte*) controlSegmentPtr) + get_actual_control_segment_size();
    };


    /**
     * @brief Getter for the BOOST region object
     */
    inline const mapped_region& get_region () const { return region; };


    /**
     * @brief Getter for requested number of buffers
     */
    inline size_t get_requested_number_buffers () const { return requestedNumberBuffers; };


    /**
     * @brief Getter for requested buffer size
     */
    inline size_t get_requested_buffer_size () const { return requestedBufferSize; };


    /**
     * @brief Calculates actual buffer size to be used
     *
     * The actual buffer size is larger than the requested buffer size
     * to have some safety gap. At least the space for one uint64_t is
     * added.
     */
    inline size_t get_actual_buffer_size () const
    {
      return sizeof(uint64_t) * ((requestedBufferSize + 2*sizeof(uint64_t)) / sizeof(uint64_t));
    };


    /**
     * @brief Getter for actual size of control segment
     */
    static size_t get_actual_control_segment_size () { return actualControlSegmentSize; };


    /**
     * @brief Queries the total size of the shared memory segment
     *        (including control segment, buffer segments and gaps)
     */
    inline size_t get_total_area_size () const
    {
      return get_actual_control_segment_size() + requestedNumberBuffers * get_actual_buffer_size ();
    };


    /**
     * @brief Getter for name of shared memory area
     */
    inline const char* get_shm_area_name () const { return shm_obj.get_name(); };
  };



  

  /***************************************************************************
   * An individual buffer in shared memory, for holding payloads
   **************************************************************************/
  
  /**
   * @brief This class represents a single buffer in shared memory,
   *        for exchange of payloads between participating processes.
   *
   * Note that an object of this class is _not_ stored in the buffer
   * segment, but will be stored in the control segment.
   *
   * Note also that this object is not concerned with inter-process
   * synchronisation
   */
  class SharedMemBuffer {
  private:
    
    /**
     * @brief Maximum amount of user data that can be stored in this
     *        buffer
     */
    size_t     _maxLength  = 0;


    /**
     * @brief Actual amount of user data stored in this buffer
     */
    size_t     _usedLength = 0;

    
    /**
     * @brief Index of this buffer in the buffer segment (which is
     *        organised as an array of buffers)
     */
    uint64_t   _bufIndex   = 0;
    
    
    /**
     * @brief Byte offset within the shared memory area where the data
     *        of this buffer starts.  relative to the start of the
     *        buffer segment
     */
    size_t     _data_offs  = 0; 


  public:
    SharedMemBuffer () {};


    /**
     * @brief Constructor taking given values for maxLength, bufIndex, data_offs
     */
    SharedMemBuffer (size_t maxlen, uint64_t bufidx, size_t dt_offs) :
      _maxLength(maxlen),
      _usedLength(0),
      _bufIndex(bufidx),
      _data_offs (dt_offs)
    {
      if (_maxLength == 0) throw ShmException ("SharedMemBuffer: maxLength is zero");
    };


    /**
     * @brief Getters for max_length, usedLength, bufIndex, data_offs
     */
    inline size_t    max_length () const { return _maxLength; };
    inline size_t    used_length () const { return _usedLength; };
    inline uint64_t  index () const { return _bufIndex; };
    inline size_t    data_offs () const { return _data_offs; };


    /**
     * @brief Checks if the SharedMemBuffer is currently empty (no data)
     *
     * Note that the empty/nonempty property depends only on usedLength
     */
    inline bool isEmpty () const { return _usedLength == 0; };


    /**
     * @brief Mark the buffer as empty
     */
    inline void clear () { _usedLength = 0; };


    /**
     * @brief Setter for usedLength
     */ 
    inline void set_used_length (size_t len) { _usedLength = len; };


    /**
     * @brief Writes data into the buffer and updates usedLength accordingly
     *
     * @param buffer_seg_ptr: pointer to start of buffer segment in
     *        the memory of the calling process. This pointer together
     *        with the data_offs gives the actual memory address of
     *        the stored data
     * @param data: data to be written / copied into the buffer
     * @param len: amount of data to be written / copied into the buffer
     * @param check_empty: if true check that the buffer is empty and throw if it is not
     *
     * Throws upon processing error
     */
    void write_to (byte* buffer_seg_ptr, byte* data, size_t len, bool check_empty = true)
    {
      if (check_empty and (not isEmpty())) throw ShmException ("SharedMemBuffer::write_to: non-empty buffer");
      if (buffer_seg_ptr == nullptr)       throw ShmException ("SharedMemBuffer::write_to: empty buffer segment pointer");
      if ((data == nullptr) or (len == 0)) throw ShmException ("SharedMemBuffer::write_to: invalid data");
      if (len > _maxLength)                throw ShmException ("SharedMemBuffer::write_to: data too long");

      std::memcpy (buffer_seg_ptr + _data_offs, data, len);
      _usedLength = len;
    };

    
    friend std::ostream& operator<<(std::ostream&os, const SharedMemBuffer& buff);

  };


  /***************************************************************************
   * A ring buffer, holding a number of SharedMemBuffer instances.
   **************************************************************************/

  /**
   * @brief A ring buffer (or cyclic queue) of SharedMemBuffer instances
   *
   * A ring buffer is stored in the control segment. It is an array of
   * SharedMemBuffer entries and two index pointers used for
   * maintaining a queue over the array. The size of the array is
   * given as a template parameter.
   *
   * The class offers a number of access methods, some which are
   * unprotected, and some of which acquire the mutex of the control
   * segment.
   */

  template <size_t maxRingBufferElements>
  class ShmRingBuffer : public RingBufferBase<SharedMemBuffer, maxRingBufferElements> {
        
  public:

    ShmRingBuffer () = delete;

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
    ShmRingBuffer (const char* name, uint64_t maxCap)
      : RingBufferBase<SharedMemBuffer, maxRingBufferElements> (name, maxCap)
      {
      };


    
    /**
     * @brief Pops/removes oldest element/buffer from the ring buffer
     *        and returns the buffer. Throws exception if ring buffer
     *        is empty for longer than given timeout.
     *
     * @param CS is the Shared memory control segment that contains
     *        the mutex for which to obtain a scoped lock.
     * @param timeoutMS is the timeout (in milliseconds)
     *
     * Locks the shared memory area itself, and waits for a ring
     * buffer to become non-empty, then pops. The waiting time is
     * limited by the timeout value. Throws if timeout occurs.
     *
     * #####ISSUE: There should probably also be a non-throwing
     *      version?? At least it does not throw while having the lock
     */
    SharedMemBuffer wait_pop (ShmControlSegmentBase& CS, uint16_t timeoutMS = 200)
    {
      if (timeoutMS <= 0) throw RingBufferException ("wait_pop: timeout is not strictly positive");
      
      TimeStampT start_time = TimeStampT::get_current_system_time ();
      
      while (true)
	{
	  {
	    ScopedShmControlSegmentLock lock (CS);
	    if (not RingBufferBase<SharedMemBuffer, maxRingBufferElements>::isEmpty())
		return RingBufferBase<SharedMemBuffer, maxRingBufferElements>::pop ();
	  }
	  TimeStampT current_time = TimeStampT::get_current_system_time();
	  if (current_time.milliseconds_passed_since (start_time) > timeoutMS)
	    throw RingBufferException ("wait_pop: timeout");

	  // #####ISSUE: make sleep time configurable/selectable
	  std::this_thread::sleep_for (10ms);
	}
    };


    /**
     * @brief Pushes/adds new element/buffer to the ring buffer.
     *        Throws exception if ring buffer is full for longer than
     *        given timeout.
     *
     * @param CS: is the Shared memory control segment that contains
     *        the mutex for which to obtain a scoped lock.
     * @param buf: the shared memory buffer to push
     * @param timeoutMS: is the timeout (in milliseconds)
     *
     * Locks the shared memory area itself, and waits for a ring
     * buffer to become non-full, then pushes the new buffer onto
     * it. The waiting time is limited by the timeout value. Throws if
     * timeout occurs.
     *
     * #####ISSUE: There should probably also be a non-throwing version??
     */
    void wait_push (ShmControlSegmentBase& CS, SharedMemBuffer buf, uint16_t timeoutMS = 200)
    {
      if (timeoutMS <= 0) throw RingBufferException ("wait_push: timeout is not strictly positive");
      
      TimeStampT start_time = TimeStampT::get_current_system_time ();
      
      while (true)
	{
	  {
	    ScopedShmControlSegmentLock lock (CS);
	    if (not RingBufferBase<SharedMemBuffer, maxRingBufferElements>::isFull())
	      {
		RingBufferBase<SharedMemBuffer, maxRingBufferElements>::push (buf);
		return;
	      }

	  }
	  TimeStampT current_time = TimeStampT::get_current_system_time();
	  if (current_time.milliseconds_passed_since (start_time) > timeoutMS)
	    throw RingBufferException ("wait_push: timeout");

	  // #####ISSUE: make sleep time configurable/selectable
	  std::this_thread::sleep_for (10ms);
	}

    };


    /**
     * @brief Pops a buffer from this ring buffer (subject to
     *        timeout), then invokes processing of this buffer, and
     *        then pushes it onto another ring buffer after processing
     *        finished.
     *
     * @tparam RBTargetMaxElements: size of the target ring buffer
     *         (so that source and target ring buffers can have different
     *         sizes)
     * @param CS is the Shared memory control segment that contains
     *        the mutex for which to obtain a scoped lock.
     * @param buffer_seg_ptr is a byte pointer denoting the start of
     *        the actual buffer area in shared memory
     * @param rbTarget is the ring buffer onto which the processed
     *        buffer will be pushed.
     * @param process is the function to be called to process the
     *        buffer. It is called with the buffer itself and with a
     *        pointer to the data stored in the buffer.
     * @param timeoutMS is the timeout (in milliseconds)
     *
     * Locks the shared memory itself, and waits for this ring-buffer
     * to become non-empty. Then retrieves one buffer, invokes
     * processing and pushes the processed buffer onto target ring
     * buffer. Throws when either the buffer_seg_ptr is nullptr, the
     * timeout is zero, or when pushing the processed element onto
     * target buffer fails. The waiting time is limited by the timeout
     * value. Throws if timeout occurs.
     *
     * The main point of this method is to do the pop() and push()
     * with just one acquisition of a lock instead of two separate
     * ones.
     *
     * #####ISSUE: There should probably also be a non-throwing version??
     */
    template <size_t RBTargetMaxElements>
    void wait_pop_process_push (ShmControlSegmentBase& CS,
				byte* buffer_seg_ptr,
				ShmRingBuffer<RBTargetMaxElements>& rbTarget,
				std::function<void (SharedMemBuffer&, byte*)> process,
				uint16_t timeoutMS = 200)
    {
      if (buffer_seg_ptr == nullptr) throw RingBufferException ("wait_pop_process_push: buffer_seg_ptr is null");
      if (timeoutMS <= 0) throw RingBufferException ("wait_pop_process_push: timeout is not strictly positive");
      
      TimeStampT start_time = TimeStampT::get_current_system_time ();

      while (true)
	{
	  {
	    ScopedShmControlSegmentLock lock (CS);
	    if (not RingBufferBase<SharedMemBuffer, maxRingBufferElements>::isEmpty())
	      {
		SharedMemBuffer buff = RingBufferBase<SharedMemBuffer, maxRingBufferElements>::pop ();
		byte* data_ptr = buffer_seg_ptr + buff.data_offs ();
		process (buff, data_ptr);
		rbTarget.push (buff);
		return;
	      }
	  }
	  TimeStampT current_time = TimeStampT::get_current_system_time();
	  if (current_time.milliseconds_passed_since (start_time) > timeoutMS)
	    throw RingBufferException ("wait_pop_process_push: timeout");

	  std::this_thread::sleep_for (5ms);
	}

    }

  };


  /**
   * @brief Here we pre-define two types of ring buffers (with their
   *        respective maximum sizes), one for 'normal' ring buffers
   *        (of which there can be multiple in a control segment) and
   *        one for the free list (which must be able to hold all
   *        existing SharedMemBuffers)
   */
  
  const size_t maxRingBufferElements_Normal   =  64;
  const size_t maxRingBufferElements_Free     =  512;  
  typedef ShmRingBuffer<maxRingBufferElements_Free>    RingBufferFree;
  typedef ShmRingBuffer<maxRingBufferElements_Normal>  RingBufferNormal;
  

  
  /***************************************************************************
   * Configuration block for shared memory area
   **************************************************************************/


  /**
   * @brief Default name for a shared memory block
   */
  const std::string defaultValueShmAreaName    = "dcp-shm";


  /**
   * @brief This class holds all the configuration data for a shared
   *        memory block
   *
   * As a default, the shared memory configuration is a separate
   * section in a config file with section name 'sharedmem'
   */
  class SharedMemoryConfigurationBlock : public DcpConfigurationBlock {
  public:

    std::string shmAreaName; /*!< Name of shared memory area, must be systemwide unique at the time of creation */


    /**
     * @brief Constructors, setting the section name for config file
     */
    SharedMemoryConfigurationBlock ()
      : DcpConfigurationBlock ("sharedmem")
    {};

    SharedMemoryConfigurationBlock (std::string bname)
      : DcpConfigurationBlock (bname)
    {};

    SharedMemoryConfigurationBlock (std::string bname, std::string shmname)
      : DcpConfigurationBlock (bname),
	shmAreaName (shmname)
    {};

    /**
     * @brief Add description of configuration options for config file, also taking default area name
     */
    virtual void add_options (po::options_description& cfgdesc)
    {
      add_options (cfgdesc, defaultValueShmAreaName);
    };
    
    
    /**
     * @brief Add description of configuration options for config file, also taking default area name
     */
    virtual void add_options (po::options_description& cfgdesc, std::string default_area_name);


    /**
     * @brief Validates configuration values. Throws exception if invalid.
     */
    virtual void validate ();
    
  };


  
  
  
};  // namespace dcp
