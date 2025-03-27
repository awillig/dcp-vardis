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


#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <dcp/common/global_types_constants.h>

using namespace boost::interprocess;

/**
 * @brief Module providing base support for creating and attaching to
 *        shared memory areas
 */

namespace dcp {

  /**
   * @brief Provides support for creating a shared memory segment, and
   *        for attaching to an existing shared memory segment.
   */
  class ShmStructureBase {
  protected:
    
    shared_memory_object shm_obj;      /*!< Boost shared memory object */
    mapped_region region;              /*!< Boost shared memory region, holds information about the region (address, size) */
    bool    isCreator      = false;    /*!< Indicates whether user of this object is the one creating the shared memory segmeng */
    byte*   memory_address = nullptr;  /*!< Memory address of shared memory segment */
    size_t  structure_size = 0;        /*!< Size of shared memory segment */

  public:

    /**
     * @brief Empty constructor, does nothing
     */
    ShmStructureBase ();


    /**
     * @brief Constructor, either creating area or attaching to existing area
     *
     * @param area_name: name of area to create or attach to
     * @param struct_size: size of area to be created. Irrelevant when
     *        attaching to an existing area.
     * @param isCreator: indicates whether to create new area or attach to
     *        existing area
     */
    ShmStructureBase (const char* area_name, size_t struct_size, bool isCreator);


    /**
     * @brief Destructor, removes the area when I have created it.
     */
    ~ShmStructureBase ();


    /**
     * @brief Move assignment operator
     */
    ShmStructureBase& operator= (ShmStructureBase&& other);
    

    /**
     * @brief Attempts to create the shared memory area
     *
     * @param area_name: name of shared memory area to create
     * @param struct_size: requested size of shared memory area
     *
     * Throws if shared memory area cannot be created (e.g. when it
     * already exists). WARNING: currently the shared memory area is
     * made world-readable and write-able.
     */
    void create_shm_area (const char* area_name, size_t struct_size);


    /**
     * @brief Attempts to attach to an existing shared memory
     *        area. Throws if this is not possible (e.g. area does not
     *        exist)
     *
     * @param area_name: name of shared memory area to attach to.
     */
    void attach_to_shm_area (const char* area_name);


    /**
     * @brief Returns the address of shared memory area
     */
    inline byte*       get_memory_address () const { return memory_address; };


    /**
     * @brief Returns whether object owner has created area
     */
    inline bool        get_is_creator () const { return isCreator; };


    /**
     * @brief Returns size of shared memory area
     */
    inline size_t      get_structure_size () const { return structure_size; };


    /**
     * @brief Returns area name
     */
    inline const char* get_name () const { return shm_obj.get_name(); };


    /**
     * @brief Checks whether we have a valid shared memory area
     */
    inline bool        has_valid_memory () const { return memory_address != nullptr; };
  };

};  // namespace dcp
