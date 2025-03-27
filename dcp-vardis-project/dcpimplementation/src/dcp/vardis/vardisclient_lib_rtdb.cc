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

using dcp::vardis::DBEntry;
using dcp::vardis::PayloadQueue;
using dcp::vardis::RTDB_Create_Confirm;
using dcp::vardis::RTDB_Create_Request;
using dcp::vardis::RTDB_Delete_Confirm;
using dcp::vardis::RTDB_Delete_Request;
using dcp::vardis::RTDB_Read_Confirm;
using dcp::vardis::RTDB_Read_Request;
using dcp::vardis::RTDB_Update_Confirm;
using dcp::vardis::RTDB_Update_Request;
using dcp::vardis::ConfirmQueue;


namespace dcp {

  // --------------------------------------

  template <typename CT>
  DcpStatus rtdb_helper (PayloadQueue& requestQueue,
			 ConfirmQueue& confirmQueue,
			 PushHandler req_handler)
  {
    confirmQueue.reset ();
    
    bool timed_out;
    requestQueue.push_wait (req_handler, timed_out);
    if (timed_out)
      return VARDIS_STATUS_INTERNAL_SHARED_MEMORY_ERROR;

    DcpStatus retval;
    bool      further_entries;

    PopHandler conf_handler = [&] (byte* memaddr, size_t len)
    {
      MemoryChunkDisassemblyArea area ("vdcl-dass", len, memaddr);
      
      CT conf;
      conf.deserialize (area);
      retval = conf.status_code;
    };
    
    confirmQueue.pop_wait (conf_handler, timed_out, further_entries);
    if (timed_out)
      return VARDIS_STATUS_INTERNAL_SHARED_MEMORY_ERROR;
    if (further_entries)
      return VARDIS_STATUS_INTERNAL_ERROR;
    
    return retval;

  }
  
  // --------------------------------------
  
  DcpStatus VardisClientRuntime::rtdb_create (const VarSpecT& spec, const VarValueT& value)
  {
    VardisShmControlSegment& CS = *pSCS;

    PushHandler req_handler = [&] (byte* memaddr, size_t bufferSize)
    {
      MemoryChunkAssemblyArea area ("vdcl-cr", bufferSize, memaddr);
      RTDB_Create_Request cr_req;
      cr_req.serialize (area, spec, value);
      return area.used ();
    };

    return rtdb_helper<RTDB_Create_Confirm> (CS.pqCreateRequest, CS.pqCreateConfirm, req_handler);    
  }

  // --------------------------------------

  DcpStatus VardisClientRuntime::rtdb_delete (VarIdT varId)
  {
    VardisShmControlSegment& CS = *pSCS;

    PushHandler req_handler = [&] (byte* memaddr, size_t bufferSize)
    {
      MemoryChunkAssemblyArea area ("vdcl-cr", bufferSize, memaddr);
      RTDB_Delete_Request del_req;
      del_req.varId = varId;
      del_req.serialize (area);
      return area.used ();
    };

    return rtdb_helper<RTDB_Delete_Confirm> (CS.pqDeleteRequest, CS.pqDeleteConfirm, req_handler);
  }

  // --------------------------------------

  DcpStatus VardisClientRuntime::rtdb_update (VarIdT varId, const VarValueT& value)
  {
    VardisShmControlSegment& CS = *pSCS;

    PushHandler req_handler = [&] (byte* memaddr, size_t bufferSize)
    {
      MemoryChunkAssemblyArea area ("vdcl-cr", bufferSize, memaddr);
      RTDB_Update_Request upd_req;
      upd_req.varId = varId;
      upd_req.serialize (area, value);
      return area.used ();
    };

    return rtdb_helper<RTDB_Update_Confirm> (CS.pqUpdateRequest, CS.pqUpdateConfirm, req_handler);
  }

  // --------------------------------------

  DcpStatus VardisClientRuntime::rtdb_read   (VarIdT varId,
					      VarIdT& responseVarId,
					      VarLenT& responseVarLen,
					      TimeStampT& responseTimeStamp,
					      size_t value_bufsize,
					      byte* value_buffer)
  {
    if ((value_buffer == nullptr) or (value_bufsize < dcp::vardis::MAX_maxValueLength))
      throw VardisClientLibException ("rtdb_read: illegal buffer information");

    variable_store.lock ();
    DBEntry& entry = variable_store.get_db_entry_ref (varId);
    variable_store.read_value (varId, value_buffer, responseVarLen);
    responseTimeStamp = entry.tStamp;
    responseVarId = entry.varId;
    variable_store.unlock ();

    return VARDIS_STATUS_OK;
    
  }

  // --------------------------------------
  
};  // namespace dcp
