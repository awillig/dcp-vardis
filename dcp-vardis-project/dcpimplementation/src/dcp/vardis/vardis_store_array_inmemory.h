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
#include <dcp/vardis/vardis_constants.h>
#include <dcp/vardis/vardis_store_array.h>


/**
 * @brief This module provides an array-based variable store that is
 *        stored in a simple memory region without any locking
 */


namespace dcp::vardis {


  /**
   * @brief GlobalStateBase-derived class
   */
  class GlobalStateInMemory : public GlobalStateBase {
  };


  /**
   * @brief Template class for an array-based variable store located
   *        in simple unlocked memory region
   *
   * @tparam valueBufferSize: size of a memory buffer for storing variable value
   * @tparam descrBufferSize: size of a memory buffer for storing variable description
   */
  template <size_t valueBufferSize, size_t descrBufferSize>
  class ArrayVariableStoreInMemory : public ArrayVariableStoreBase<GlobalStateInMemory, valueBufferSize, descrBufferSize> {
    
  protected:


    /**
     * @brief Shorthand type definitions
     */
    typedef ArrayVariableStoreBase<GlobalStateInMemory, valueBufferSize, descrBufferSize>  InMemoryArrayType;
    typedef ArrayVariableStoreBase<GlobalStateInMemory, valueBufferSize, descrBufferSize>::ArrayContents  InMemoryArrayContents;


    /**
     * @brief Indicates whether the owner of current object is creator or not.
     *
     * The creator is responsible for allocating and de-allocating the
     * memory segment.
     */
    bool  isCreator  = false;


    /**
     * @brief Contains the memory address or the memory segment
     */
    byte* memory_address = nullptr;
    

  public:

    // ---------------------------------------

    inline byte* get_memory_address () const { return memory_address; };
    
    // ---------------------------------------
    
    ArrayVariableStoreInMemory () = delete;

    // ---------------------------------------


    /**
     * @brief Allocates memory segment and creates a variable store in
     *        there
     *
     * @param isCreator: indicates whether caller is creating the
     *        memory area for the Vardis store (Vardis demon)
     *        or not (Vardis client)
     * @param maxsumm: value of maxSummaries configuration parameter
     * @param maxdescrlen: value of maxDescriptionLength configuration parameter
     * @param maxvallen: value of maxValueLength configuration parameter
     * @param maxrep: value of maxRepetitions configuration parameter
     * @param own_node_id: value of ownNodeIdentifier parameter
     *
     * As a creator, allocates memory area, and initializes the
     * array-based variable store there. As a client, just retrieves
     * the pointer to the memory segment.
     *
     * Throws when memory allocation does not work.
     */
    ArrayVariableStoreInMemory (bool isCreator,
				uint16_t maxsumm = 0,
				size_t maxdescrlen = 0,
				size_t maxvallen = 0,
				uint8_t maxrep = 0,
				NodeIdentifierT own_node_id = nullNodeIdentifier
				)
      : isCreator (isCreator)
    {
      if (isCreator)
	{
	  memory_address = new byte [sizeof(InMemoryArrayContents)];
	  InMemoryArrayType::initialize_array_store (get_memory_address (),
						     maxsumm,
						     maxdescrlen,
						     maxvallen,
						     maxrep,
						     own_node_id);
	}
      else
	{
	  this->pContents = (InMemoryArrayContents*) get_memory_address();
	  if (this->pContents == nullptr)
	    throw VardisStoreException ("ArrayVariableStoreInMemory",
					"illegal region pointer");

	}
    };
    
    ~ArrayVariableStoreInMemory ()
    {
      if (memory_address)
	delete [] memory_address;
    };
    
    
  }; 


  /**
   * @brief Convenience type definition for client code
   */
  typedef ArrayVariableStoreInMemory<VarLenT::max_val()+1, MAX_maxDescriptionLength+1> VardisVariableStoreInMemory;
  
};  // namespace dcp::vardis
