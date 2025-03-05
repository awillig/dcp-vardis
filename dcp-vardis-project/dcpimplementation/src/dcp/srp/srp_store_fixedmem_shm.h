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
#include <dcp/srp/srp_store_fixedmem.h>


using namespace boost::interprocess;

/**
 * @brief This module provides a neighbour table / neighbour store
 *        that is stored in a shared-memory segment
 */


namespace dcp::srp {


  /**
   * @brief GlobalStateBase-derived class that additionally provides
   *        an interprocess mutex for sharing access to the variable
   *        store in shared memory
   */
  class GlobalStateShm : public GlobalStateBase {
  public:
    interprocess_mutex neighbour_table_mutex;
    interprocess_mutex own_sd_mutex;
  };


  /**
   * @brief Template class for an array-based variable store located
   *        in shared memory
   *
   * @tparam valueBufferSize: size of a memory buffer for storing variable value
   * @tparam descrBufferSize: size of a memory buffer for storing variable description
   */
  template <uint64_t maxNeighbours>
  class FixedMemSRPStoreShm : public FixedMemSRPStoreBase<GlobalStateShm, maxNeighbours> {
    
  protected:


    /**
     * @brief Shorthand type definitions
     */
    typedef FixedMemSRPStoreBase<GlobalStateShm, maxNeighbours>  ShmSRPStoreType;
    typedef FixedMemSRPStoreBase<GlobalStateShm, maxNeighbours>::FixedMemContents  ShmFixedMemContents;


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
    
    FixedMemSRPStoreShm () = delete;

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
    FixedMemSRPStoreShm (const char* area_name,
			 bool isServer,
			 double alpha_gapsize_ewma = 0.9,
			 NodeIdentifierT own_node_id = nullNodeIdentifier
			 )
      : isServer (isServer)
    {
      if (area_name == nullptr) throw SRPStoreException ("no area name");

      size_t store_contents_size = ShmSRPStoreType::get_fixedmem_contents_size();
      
      if (isServer)
	{
	  shm_obj = shared_memory_object (create_only, area_name, read_write);
	  shm_obj.truncate (store_contents_size);
	  region = mapped_region (shm_obj, read_write);
	  if (region.get_size() != store_contents_size)
	    throw SRPStoreException (std::format("wrong region size {} where {} is required", region.get_size(), store_contents_size));
	  ShmSRPStoreType::initialize_srp_store ((byte*) region.get_address(),
						 own_node_id,
						 alpha_gapsize_ewma);
	}
      else
	{
	  shm_obj = shared_memory_object (open_only, area_name, read_write);
	  region  = mapped_region (shm_obj, read_write);
	  if (region.get_size() != store_contents_size)
	    throw SRPStoreException (std::format("wrong region size {} where {} is required", region.get_size(), store_contents_size));

	  this->pContents = (ShmFixedMemContents*) region.get_address();
	  if (this->pContents == nullptr)
	    throw SRPStoreException ("illegal region pointer");

	}
    };
    

    /**
     * @brief Destructor, deallocates shared memory when server
     */
    ~FixedMemSRPStoreShm ()
    {
      if (isServer)
	{
	  shm_obj.remove(shm_obj.get_name());
	}
    };
    

    void lock_neighbour_table ()
    {
      ShmFixedMemContents&  FMC = *(this->pContents);
      FMC.global_state.neighbour_table_mutex.lock();
    };


    void unlock_neighbour_table ()
    {
      ShmFixedMemContents&  FMC = *(this->pContents);
      FMC.global_state.neighbour_table_mutex.unlock();
    };

    void lock_own_safety_data ()
    {
      ShmFixedMemContents&  FMC = *(this->pContents);
      FMC.global_state.own_sd_mutex.lock();
    };


    void unlock_own_safety_data ()
    {
      ShmFixedMemContents&  FMC = *(this->pContents);
      FMC.global_state.own_sd_mutex.unlock();
    };
    
  }; 

  typedef FixedMemSRPStoreShm<1000> DefaultSRPStoreType;

  
};  // namespace dcp::srp
