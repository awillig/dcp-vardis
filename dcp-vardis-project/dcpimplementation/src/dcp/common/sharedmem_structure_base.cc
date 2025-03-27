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



#include <dcp/common/exceptions.h>
#include <dcp/common/sharedmem_structure_base.h>

namespace dcp {

  ShmStructureBase::ShmStructureBase ()
    : isCreator (false),
      memory_address (nullptr),
      structure_size (0)
  {
  }
  
  ShmStructureBase::ShmStructureBase (const char* area_name, size_t struct_size, bool isCreator)
    : isCreator (isCreator),
      memory_address (nullptr),
      structure_size (struct_size)
  {
    if (isCreator)      create_shm_area (area_name, struct_size);
    if (not isCreator)  attach_to_shm_area (area_name);
  }
  

  ShmStructureBase::~ShmStructureBase ()
  {
    if (has_valid_memory() and get_is_creator())
      {
	shm_obj.remove (shm_obj.get_name());
      }
  }

  
  ShmStructureBase& ShmStructureBase::operator= (ShmStructureBase&& other)
  {
    if (this == &other) return *this;
    
    shm_obj         = std::move(other.shm_obj);
    region          = std::move(other.region);
    isCreator       = other.isCreator;
    memory_address  = other.memory_address;
    structure_size  = other.structure_size;
    
    other.isCreator       = false;
    other.memory_address  = nullptr;
    other.structure_size  = 0;
    
    return *this;
  }
  
  
  void ShmStructureBase::create_shm_area (const char* area_name, size_t struct_size)
  {
    if (!area_name)
      throw ShmException  (std::format("create_shm_area: No area name"));
    if (std::strlen(area_name) > maxShmAreaNameLength)
      throw ShmException  (std::format("create_shm_area: Area {}: name is too long", area_name));
    if (structure_size == 0)
      throw ShmException (std::format("create_shm_area: Area {}: structure size is zero", area_name));
        
    shm_obj = shared_memory_object (create_only, area_name, read_write);
    shm_obj.truncate (structure_size);
    
    // #####ISSUE: This is incredibly ugly and very probably
    // #####absolutely not portable, but so far the only way I
    // #####could think of to make sure client protocols do not
    // #####need to run as sudo when they want to have r/w
    // #####access to their shared memory segment. Possible
    // #####refinement: put all dcp executables in the same
    // #####group and limit access to group only
    chmod (std::format("/dev/shm/{}", area_name).c_str(), 0666);  
    
    region = mapped_region (shm_obj, read_write);
    if (region.get_size() != structure_size)
      throw ShmException (std::format("create_shm_area: Area {}: wrong region size {} where {} is required", area_name, region.get_size(), structure_size));
    memory_address = (byte*) region.get_address();
    if (!memory_address)
      throw ShmException (std::format("create_shm_area: Area {}: illegal region pointer for creator", area_name));
    structure_size = struct_size;    
  }
  

  void ShmStructureBase::attach_to_shm_area (const char* area_name)
  {
    if (!area_name)
      throw ShmException  (std::format("attach_to_shm_area: No area name"));
    if (std::strlen(area_name) > maxShmAreaNameLength)
      throw ShmException  (std::format("attach_to_shm_area: Area {}: name is too long", area_name));
    
    try {
      shm_obj = shared_memory_object (open_only, area_name, read_write);
      region  = mapped_region (shm_obj, read_write);
      memory_address = (byte*) region.get_address();
      if (!memory_address)
	throw ShmException (std::format("attach_to_shm_area: Area {}: illegal region pointer for client", area_name));
      structure_size = region.get_size();
    }
    catch (...)
      {
	throw ShmException (std::format("attach_to_shm_area: cannot open shared memory region {}", area_name));
      }    
  } 
};  // namespace dcp
