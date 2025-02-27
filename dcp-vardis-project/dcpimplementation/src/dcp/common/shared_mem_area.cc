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

#include <chrono>
#include <thread>
#include <format>
#include <sys/stat.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/shared_mem_area.h>


namespace dcp {

  std::ostream& operator<< (std::ostream& os, const SharedMemBuffer& buff)
  {
    os << "SharedMemBuffer{maxlen=" << buff._maxLength
       << ",usedlen=" << buff._usedLength
       << ",bufIndex=" << buff._bufIndex
       << ",data_offs=" << buff._data_offs << "}";
    return os;
  }


  ShmBufferPool::ShmBufferPool (const char*     area_name,
				bool            isCreator,
				const size_t    control_seg_size,
				const size_t    bufferSize,
				uint64_t        numberBuffers)
    : requestedBufferSize (bufferSize),
      requestedNumberBuffers (numberBuffers)
  {
    if (! area_name) throw ShmException ("ShmBufferPool: area_name undefined");
    if (isCreator and (bufferSize == 0)) throw ShmException ("ShmBufferPool: bufferSize = 0");
    if (isCreator and (numberBuffers == 0)) throw ShmException ("ShmBufferPool: maxToServerBuffers = 0");
    if (std::strlen(area_name) > maxShmAreaNameLength) throw ShmException (std::format("ShmBufferPool: area_name {} is too long", area_name));
    if (control_seg_size >= maximumControlSegmentSize) throw ShmException (std::format("ShmBufferPool: requested control segment size {} is too large (max = {})", control_seg_size, maximumControlSegmentSize));
    
    std::strcpy (areaName, area_name);
    
    if (isCreator)
      {
	try {
	  shm_obj = shared_memory_object (create_only, areaName, read_write);
	  shm_obj.truncate (get_total_area_size());
	  
	  // #####ISSUE: This is incredibly ugly and very probably
	  // #####absolutely not portable, but so far the only way I
	  // #####could think of to make sure client protocols do not
	  // #####need to run as sudo when they want to have r/w
	  // #####access to their shared memory segment. Possible
	  // #####refinement: put all dcp executables in the same
	  // #####group and limit access to group only
	  chmod (std::format("/dev/shm/{}", areaName).c_str(), 0666);  
	}
	catch (...) {
	  throw ShmException ("Attempt to create shared memory area failed");
	}
      }
    else
      {
	try {
	  shm_obj = shared_memory_object (open_only, areaName, read_write);
	}
	catch (...) {
	  throw ShmException ("Attempt to attach to shared memory area has failed");
	}
      }
    
    region = mapped_region (shm_obj, read_write);
    
    controlSegmentPtr = (ShmControlSegmentBase*) region.get_address();
    bufferSegmentPtr  = ((byte*) controlSegmentPtr) + get_actual_control_segment_size();
    
    
  }
  
  
  
  void SharedMemoryConfigurationBlock::add_options (po::options_description& cfgdesc, std::string default_area_name)
  {
    cfgdesc.add_options()
      (opt("areaName").c_str(),  po::value<std::string>(&shmAreaName)->default_value(default_area_name), txt("shared memory area name").c_str())
      ;
  }

  void SharedMemoryConfigurationBlock::validate ()
  {
    if (shmAreaName.empty()) throw ConfigurationException("no shared memory name given");
  }
  
};  // namespace dcp
