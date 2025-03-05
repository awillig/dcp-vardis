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
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <dcp/vardis/vardis_variable_store_array.h>


using namespace boost::interprocess;

/**
 * @brief This module provides an array-based variable store that is
 *        stored in a shared-memory segment
 */


namespace dcp::vardis {


  /**
   * @brief GlobalStateBase-derived class that additionally provides
   *        an interprocess mutex for sharing access to the variable
   *        store in shared memory
   */
  class GlobalStateShm : public GlobalStateBase {
  public:
    interprocess_mutex mutex;
  };


  /**
   * @brief Template class for an array-based variable store located
   *        in shared memory
   *
   * @tparam valueBufferSize: size of a memory buffer for storing variable value
   * @tparam descrBufferSize: size of a memory buffer for storing variable description
   */
  template <size_t valueBufferSize, size_t descrBufferSize>
  class ArrayVariableStoreShm : public ArrayVariableStoreBase<GlobalStateShm, valueBufferSize, descrBufferSize> {
    
  protected:


    /**
     * @brief Shorthand type definitions
     */
    typedef ArrayVariableStoreBase<GlobalStateShm, valueBufferSize, descrBufferSize>  ShmArrayType;
    typedef ArrayVariableStoreBase<GlobalStateShm, valueBufferSize, descrBufferSize>::ArrayContents  ShmArrayContents;


    /**
     * @brief Indicates whether the owner of current object is server of not.
     *
     * The server is responsible for allocating and de-allocating the
     * shared memory segment.
     */
    bool                 isServer  = false;


    shared_memory_object shm_obj;  /*!< The shared memory object */
    mapped_region        region;   /*!< Region information, e.g. containing memory address of shared memory segment */
    

  public:

    // ---------------------------------------
    
    ArrayVariableStoreShm () = delete;

    // ---------------------------------------


    /**
     * @brief Allocates shared memory segment and creates a variable
     *        store in there
     *
     * @param area_name: name of shared memory segment
     * @param isServer: indicates whether caller is server
     *        (Vardis demon) or not (Vardis client)
     * @param maxsumm: value of maxSummaries configuration parameter
     * @param maxdescrlen: value of maxDescriptionLength configuration parameter
     * @param maxvallen: value of maxValueLength configuration parameter
     * @param maxrep: value of maxRepetitions configuration parameter
     * @param own_node_id: value of ownNodeIdentifier parameter
     *
     * As a server, allocates shared memory object, and initializes
     * the array-based variable store there. As a client, attempts to
     * open and attach to the shared memory segment.
     *
     * Throws when shared memory allocation does not work.
     */
    ArrayVariableStoreShm (const char* area_name,
			   bool isServer,
			   uint16_t maxsumm = 0,
			   size_t maxdescrlen = 0,
			   size_t maxvallen = 0,
			   uint8_t maxrep = 0,
			   NodeIdentifierT own_node_id = nullNodeIdentifier
			   )
      : isServer (isServer)
    {
      if (area_name == nullptr)
	throw VardisStoreException ("no area name");

      size_t array_contents_size = ShmArrayType::get_array_contents_size();
      
      if (isServer)
	{
	  shm_obj = shared_memory_object (create_only, area_name, read_write);
	  shm_obj.truncate (array_contents_size);
	  region = mapped_region (shm_obj, read_write);
	  if (region.get_size() != array_contents_size)
	    throw VardisStoreException (std::format("wrong region size {} where {} is required", region.get_size(), array_contents_size));
	  ShmArrayType::initialize_array_store ((byte*) region.get_address(),
						maxsumm,
						maxdescrlen,
						maxvallen,
						maxrep,
						own_node_id);
	}
      else
	{
	  shm_obj = shared_memory_object (open_only, area_name, read_write);
	  region  = mapped_region (shm_obj, read_write);
	  if (region.get_size() != array_contents_size)
	    throw VardisStoreException (std::format("wrong region size {} where {} is required", region.get_size(), array_contents_size));

	  this->pContents = (ShmArrayContents*) region.get_address();
	  if (this->pContents == nullptr)
	    throw VardisStoreException ("illegal region pointer");

	}
    };
    

    /**
     * @brief Destructor, deallocates shared memory when server
     */
    ~ArrayVariableStoreShm ()
    {
      if (isServer)
	{
	  shm_obj.remove(shm_obj.get_name());
	}
    };
    

    /**
     * @brief Acquires mutex for variable store in shared memory
     */
    void lock ()
    {
      ShmArrayContents&  AC = *(this->pContents);
      AC.global_state.mutex.lock();
    };


    /**
     * @brief Releases mutex for variable store in shared memory
     */
    void unlock ()
    {
      ShmArrayContents&  AC = *(this->pContents);
      AC.global_state.mutex.unlock();
    };
    
  }; 


  /**
   * @brief Convenience type definition for client code
   */
  typedef ArrayVariableStoreShm<VarLenT::max_val()+1, MAX_maxDescriptionLength+1> VardisVariableStoreShm;
  
};  // namespace dcp::vardis
