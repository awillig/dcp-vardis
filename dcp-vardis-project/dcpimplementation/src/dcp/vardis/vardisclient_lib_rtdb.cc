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


#include <dcp/common/area.h>
#include <dcp/common/exceptions.h>
#include <dcp/vardis/vardisclient_lib.h>
#include <dcp/vardis/vardis_constants.h>
#include <dcp/vardis/vardis_service_primitives.h>
#include <dcp/vardis/vardis_shm_control_segment.h>

using dcp::vardis::RTDB_Create_Confirm;
using dcp::vardis::RTDB_Create_Request;
using dcp::vardis::RTDB_Delete_Confirm;
using dcp::vardis::RTDB_Delete_Request;
using dcp::vardis::RTDB_Read_Confirm;
using dcp::vardis::RTDB_Read_Request;
using dcp::vardis::RTDB_Update_Confirm;
using dcp::vardis::RTDB_Update_Request;



namespace dcp {


  // --------------------------------------


  inline MemoryChunkAssemblyArea pop_buffer_and_setup_assembly_area (VardisShmControlSegment& CS,
								     byte *buffer_seg_ptr,
								     const std::string& methname,
								     RingBufferNormal& rbRequest,
								     const RingBufferNormal& rbConfirm,
								     SharedMemBuffer& buff)
  {
    if (CS.rbFree.isEmpty())      throw VardisClientLibException (methname + ": " + "no free block in shared memory");
    if (not rbRequest.isEmpty())  throw VardisClientLibException (methname + ": " + "shared memory request queue is not empty");
    if (not rbConfirm.isEmpty())  throw VardisClientLibException (methname + ": " + "shared memory confirm queue is not empty");
    
    buff = CS.rbFree.pop ();
    
    byte* data_ptr = buffer_seg_ptr + buff.data_offs ();
    return MemoryChunkAssemblyArea ("vdc-ass", buff.max_length(), data_ptr);
  }

  // --------------------------------------

  inline MemoryChunkDisassemblyArea await_confirmation_and_setup_disassembly_area (VardisShmControlSegment& CS,
										   byte* buffer_seg_ptr,
										   RingBufferNormal& rbConfirm,
										   SharedMemBuffer& buff)
  {
    buff = rbConfirm.wait_pop (CS);

    byte* data_ptr = buffer_seg_ptr + buff.data_offs ();
    return MemoryChunkDisassemblyArea ("vdc-dass", buff.used_length(), data_ptr);
  }
  
  // --------------------------------------

  inline void move_buffer_to_free (VardisShmControlSegment& CS,
				   const std::string& methname,
				   SharedMemBuffer& buff)
  {
    buff.clear();
    ScopedShmControlSegmentLock lock (CS);
    if (CS.rbFree.isFull())
      throw VardisClientLibException (methname + ": " + "cannot move buffer back into free list");
    CS.rbFree.push(buff);
  }
  
  // --------------------------------------

  template <typename CT>
  inline DcpStatus await_and_deserialize_simple_confirmation (VardisShmControlSegment& CS,
							      byte* buffer_seg_ptr,
							      RingBufferNormal& rbConfirm,
							      const std::string& methname)
  {
    SharedMemBuffer buff;
    MemoryChunkDisassemblyArea area = await_confirmation_and_setup_disassembly_area (CS,
										     buffer_seg_ptr,
										     rbConfirm,
										     buff);

    CT conf;
    conf.deserialize (area);
    move_buffer_to_free (CS, methname, buff); 
    return conf.status_code;
  }
						   
  
  // --------------------------------------
  
  DcpStatus VardisClientRuntime::rtdb_create (const VarSpecT& spec, const VarValueT& value)
  {
    if (not _isRegistered)
      throw VardisClientLibException ("rtdb_create: not registered with Vardis");
    
    byte*    buffer_seg_ptr     = nullptr;
    VardisShmControlSegment& CS = obtain_shm_refs (buffer_seg_ptr);

    // first block: submit create request primitive
    {
      ScopedShmControlSegmentLock lock (CS);
      
      SharedMemBuffer buff;
      MemoryChunkAssemblyArea area = pop_buffer_and_setup_assembly_area (CS,
									 buffer_seg_ptr,
									 "vardisclient_rtdb_create",
									 CS.rbCreateRequest,
									 CS.rbCreateConfirm,
									 buff);

      RTDB_Create_Request cr_req;
      cr_req.serialize (area, spec, value);

      buff.set_used_length (area.used());    
      CS.rbCreateRequest.push (buff);
    }

    return await_and_deserialize_simple_confirmation<RTDB_Create_Confirm> (CS, buffer_seg_ptr, CS.rbCreateConfirm, "vardisclient_rtdb_create");

  }

  // --------------------------------------

  DcpStatus VardisClientRuntime::rtdb_delete (VarIdT varId)
  {
    if (not _isRegistered)
      throw VardisClientLibException ("rtdb_delete: not registered with Vardis");
    
    byte*    buffer_seg_ptr     = nullptr;
    VardisShmControlSegment& CS = obtain_shm_refs (buffer_seg_ptr);
    
    // first block: submit delete request primitive
    {
      ScopedShmControlSegmentLock lock (CS);
      
      SharedMemBuffer buff;
      MemoryChunkAssemblyArea area = pop_buffer_and_setup_assembly_area (CS,
									 buffer_seg_ptr,
									 "vardisclient_rtdb_delete",
									 CS.rbDeleteRequest,
									 CS.rbDeleteConfirm,
									 buff);

      RTDB_Delete_Request del_req;
      del_req.varId = varId;
      del_req.serialize (area);

      buff.set_used_length (area.used());    
      CS.rbDeleteRequest.push (buff);
    }

    return await_and_deserialize_simple_confirmation<RTDB_Delete_Confirm> (CS, buffer_seg_ptr, CS.rbDeleteConfirm, "vardisclient_rtdb_delete");

  }

  // --------------------------------------

  DcpStatus VardisClientRuntime::rtdb_update (VarIdT varId, const VarValueT& value)
  {
    if (not _isRegistered)
      throw VardisClientLibException ("rtdb_update: not registered with Vardis");
    
    byte*    buffer_seg_ptr     = nullptr;
    VardisShmControlSegment& CS = obtain_shm_refs (buffer_seg_ptr);

    // first block: submit update request primitive
    {
      ScopedShmControlSegmentLock lock (CS);
      
      SharedMemBuffer buff;
      MemoryChunkAssemblyArea area = pop_buffer_and_setup_assembly_area (CS,
									 buffer_seg_ptr,
									 "vardisclient_rtdb_update",
									 CS.rbUpdateRequest,
									 CS.rbUpdateConfirm,
									 buff);

      RTDB_Update_Request upd_req;
      upd_req.varId = varId;
      upd_req.serialize (area, value);

      buff.set_used_length (area.used());    
      CS.rbUpdateRequest.push (buff);
    }

    return await_and_deserialize_simple_confirmation<RTDB_Update_Confirm> (CS, buffer_seg_ptr, CS.rbUpdateConfirm, "vardisclient_rtdb_update");

  }

  // --------------------------------------

  DcpStatus VardisClientRuntime::rtdb_read   (VarIdT varId,
					      VarIdT& responseVarId,
					      VarLenT& responseVarLen,
					      TimeStampT& responseTimeStamp,
					      size_t value_bufsize,
					      byte* value_buffer)
  {
    if (not _isRegistered)
      throw VardisClientLibException ("rtdb_read: not registered with Vardis");
    
    if ((value_buffer == nullptr) or (value_bufsize < dcp::vardis::MAX_maxValueLength))
      throw VardisClientLibException ("vardisclient_rtdb_read: illegal buffer information");
    
    byte*    buffer_seg_ptr     = nullptr;
    VardisShmControlSegment& CS = obtain_shm_refs (buffer_seg_ptr);

    //report_buffer_occupancy (CS);
    
    // first block: submit read request primitive
    {
      ScopedShmControlSegmentLock lock (CS);
      
      SharedMemBuffer buff;
      MemoryChunkAssemblyArea area = pop_buffer_and_setup_assembly_area (CS,
									 buffer_seg_ptr,
									 "vardisclient_rtdb_read",
									 CS.rbReadRequest,
									 CS.rbReadConfirm,
									 buff);

      RTDB_Read_Request read_req;
      read_req.varId = varId;
      read_req.serialize (area);

      buff.set_used_length (area.used());    
      CS.rbReadRequest.push (buff);
    }

    SharedMemBuffer buff;
    MemoryChunkDisassemblyArea area = await_confirmation_and_setup_disassembly_area (CS,
										     buffer_seg_ptr,
										     CS.rbReadConfirm,
										     buff);

    RTDB_Read_Confirm conf;
    conf.deserialize (area, responseVarLen, value_buffer);
    responseVarId      = conf.varId;
    responseTimeStamp  = conf.tStamp;
    

    move_buffer_to_free (CS, "vardisclient_rtdb_read", buff); 
    
    return conf.status_code;
  }

  // --------------------------------------
  
};  // namespace dcp
