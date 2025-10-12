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


#include <dcp/vardis/vardis_service_primitives.h>

namespace dcp::vardis {

  std::ostream& operator<<(std::ostream& os, const VardisRegister_Request& req)
  {
    os << "VardisRegister_Request{s_type=" << vardis_service_type_to_string(req.s_type)
       << ", shm_area_name = " << req.shm_area_name
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisRegister_Confirm& conf)
  {
    os << "VardisRegister_Confirm{s_type=" << vardis_service_type_to_string (conf.s_type)
       << ", status_code = " << vardis_status_to_string (conf.status_code)
       << ", own_node_identifier = " << conf.own_node_identifier
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisDeregister_Request& req)
  {
    os << "VardisDeregister_Request{s_type=" << vardis_service_type_to_string(req.s_type)
       << ", shm_area_name = " << req.shm_area_name
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisDeregister_Confirm& conf)
  {
    os << "VardisDeregister_Confirm{s_type=" << vardis_service_type_to_string (conf.s_type)
       << ", status_code = " << vardis_status_to_string (conf.status_code)
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisShutdown_Request& req)
  {
    os << "VardisShutdown_Request{s_type=" << vardis_service_type_to_string(req.s_type)
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisActivate_Request& req)
  {
    os << "VardisActivate_Request{s_type=" << vardis_service_type_to_string(req.s_type)
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisActivate_Confirm& conf)
  {
    os << "VardisActivate_Confirm{s_type=" << vardis_service_type_to_string (conf.s_type)
       << ", status_code = " << vardis_status_to_string (conf.status_code)
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisDeactivate_Request& req)
  {
    os << "VardisDeactivate_Request{s_type=" << vardis_service_type_to_string(req.s_type)
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisDeactivate_Confirm& conf)
  {
    os << "VardisDeactivate_Confirm{s_type=" << vardis_service_type_to_string (conf.s_type)
       << ", status_code = " << vardis_status_to_string (conf.status_code)
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisGetStatistics_Request& req)
  {
    os << "VardisGetStatistics_Request{s_type=" << vardis_service_type_to_string(req.s_type)
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisGetStatistics_Confirm& conf)
  {
    os << "VardisGetStatistics_Confirm{s_type=" << vardis_service_type_to_string (conf.s_type)
       << ", status_code = " << vardis_status_to_string (conf.status_code)
       << ", protocol_stats = " << conf.protocol_stats
       << " }";
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os, const VardisDescribeDatabase_Request& req)
  {
    os << "VardisDescribeDatabase_Request{s_type=" << vardis_service_type_to_string(req.s_type)
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisDescribeDatabase_Confirm& conf)
  {
    os << "VardisGetStatistics_Confirm{s_type=" << vardis_service_type_to_string (conf.s_type)
       << ", status_code = " << vardis_status_to_string (conf.status_code)
       << ", numberVariableDescriptions = " << conf.numberVariableDescriptions
       << " }";
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os, const DescribeDatabaseVariableDescription& descr)
  {
    os << "DescribeDatabaseVariableDescription{varId = " << (int) descr.varId.val
       << ", prodId = " << descr.prodId
       << ", repCnt = " << (int) descr.repCnt.val
       << ", description = " << descr.description
       << ", tStamp = " << descr.tStamp
       << ", isDeleted = " << descr.isDeleted
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisDescribeVariable_Request& req)
  {
    os << "VardisDescribeVariable_Request{s_type=" << vardis_service_type_to_string(req.s_type)
       << " }";
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os, const DescribeVariableDescription& descr)
  {
    os << "DescribeDatabaseVariableDescription{varId = " << (int) descr.varId.val
       << ", prodId = " << descr.prodId
       << ", repCnt = " << (int) descr.repCnt.val
       << ", description = " << descr.description
       << ", seqno = " << descr.seqno.val
       << ", tStamp = " << descr.tStamp
       << ", countUpdate = " << (int) descr.countUpdate.val
       << ", countCreate = " << (int) descr.countCreate.val
       << ", countDelete = " << (int) descr.countDelete.val
       << ", isDeleted = " << descr.isDeleted
       << ", value_length = " << (int) descr.value_length.val
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VardisDescribeVariable_Confirm& conf)
  {
    os << "VardisDescribeVariable_Confirm{s_type=" << vardis_service_type_to_string (conf.s_type)
       << ", status_code = " << vardis_status_to_string (conf.status_code)
       << ", var_description = " << conf.var_description
       << " }";
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os, const RTDB_Create_Request& req)
  {
    os << "RTDB_Create_Request{s_type=" << vardis_service_type_to_string(req.s_type)
       << ", spec = " << req.spec
       << ", value = " << req.value
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const RTDB_Create_Confirm& conf)
  {
    os << "RTDB_Create_Confirm{s_type=" << vardis_service_type_to_string (conf.s_type)
       << ", status_code = " << vardis_status_to_string (conf.status_code)
       << ", varId = " << (int) conf.varId.val
       << " }";
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os, const RTDB_Delete_Request& req)
  {
    os << "RTDB_Delete_Request{s_type=" << vardis_service_type_to_string(req.s_type)
       << ", varId = " << req.varId
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const RTDB_Delete_Confirm& conf)
  {
    os << "RTDB_Delete_Confirm{s_type=" << vardis_service_type_to_string (conf.s_type)
       << ", status_code = " << vardis_status_to_string (conf.status_code)
       << ", varId = " << (int) conf.varId.val
       << " }";
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os, const RTDB_Update_Request& req)
  {
    os << "RTDB_Update_Request{s_type=" << vardis_service_type_to_string(req.s_type)
       << ", varId = " << req.varId
       << ", value = " << req.value
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const RTDB_Update_Confirm& conf)
  {
    os << "RTDB_Update_Confirm{s_type=" << vardis_service_type_to_string (conf.s_type)
       << ", status_code = " << vardis_status_to_string (conf.status_code)
       << ", varId = " << (int) conf.varId.val
       << " }";
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os, const RTDB_Read_Request& req)
  {
    os << "RTDB_Read_Request{s_type=" << vardis_service_type_to_string(req.s_type)
       << ", varId = " << req.varId
       << " }";
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os, const RTDB_Read_Confirm& conf)
  {
    os << "RTDB_Read_Confirm{s_type=" << vardis_service_type_to_string (conf.s_type)
       << ", status_code = " << vardis_status_to_string (conf.status_code)
       << ", varId = " << (int) conf.varId.val
       << ", value = " << conf.value
       << ", tStamp = " << conf.tStamp
       << " }";
    return os;
  }

  
};  // namespace dcp::vardis
