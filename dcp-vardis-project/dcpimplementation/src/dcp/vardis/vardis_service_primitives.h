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
#include <iostream>
#include <dcp/common/area.h>
#include <dcp/common/services_status.h>
#include <dcp/common/transmissible_type.h>
#include <dcp/vardis/vardis_constants.h>
#include <dcp/vardis/vardis_protocol_statistics.h>
#include <dcp/vardis/vardis_transmissible_types.h>


/**
 * @brief This module defines the structure of Vardis services, both
 *        the ones defined in the specification and additional ones
 *
 * Most of the defined structures here have a fixed length (so that
 * sizeof() gives sensible results), and are exchanged via the command
 * socket. These are all the structures directly derived from classes
 * ServiceRequest, ServiceConfirm
 *
 * The structures related to RTDB CRUD services can have variable
 * length and are exchanged via shared memory, into which they are
 * serialized and deserialized. To support this, these services are
 * derived from the two classes RTDBServiceRequest and
 * RTDBserviceConfirm which are also defined in this module and which
 * include serialization methods.
 *
 */

namespace dcp::vardis {
  
  /*************************************************************************
   * Services exchanged via command interface with fixed-size primitives
   ************************************************************************/

  
  /**
   * @brief VardisRegister service primitives, exchanged via command socket
   *
   * The request primitive carries the name of the shared memory area
   * to be used between Vardis demon and Vardis client application
   */
  typedef struct VardisRegister_Request : ServiceRequest {
    char  shm_area_name [maxShmAreaNameLength+1];
    VardisRegister_Request () : ServiceRequest (stVardis_Register) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisRegister_Request& req);
  } VardisRegister_Request;

  
  typedef struct VardisRegister_Confirm : ServiceConfirm {
    NodeIdentifierT own_node_identifier = nullNodeIdentifier;
    
    VardisRegister_Confirm () : ServiceConfirm (stVardis_Register) {};
    VardisRegister_Confirm (DcpStatus statcode) : ServiceConfirm(stVardis_Register, statcode) {};
    VardisRegister_Confirm (DcpStatus statcode, NodeIdentifierT node_id)
    : ServiceConfirm(stVardis_Register, statcode),
    own_node_identifier (node_id)
    {};
    friend std::ostream& operator<<(std::ostream& os, const VardisRegister_Confirm& conf);
  } VardisRegister_Confirm;


  // -----------------------------------

  
  /**
   * @brief VardisDeregister service primitives, exchanged via command
   *        socket
   *
   * The request primitive carries the name of the shared memory area
   * used between Vardis demon and Vardis client application
   */
  typedef struct VardisDeregister_Request : ServiceRequest {
    char  shm_area_name [maxShmAreaNameLength+1];
    VardisDeregister_Request () : ServiceRequest (stVardis_Deregister) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisDeregister_Request& req);
  } VardisDeregister_Request;

  
  typedef struct VardisDeregister_Confirm : ServiceConfirm {
    VardisDeregister_Confirm () : ServiceConfirm (stVardis_Deregister) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisDeregister_Confirm& conf);
  } VardisDeregister_Confirm;


  // -----------------------------------

  
  /**
   * @brief VardisShutdown request service primitive, exchanged via
   *        command socket
   *
   * Note: There is *no* confirm primitive, as in response to
   *       receiving this primitive the Vardis demon will close the
   *       socket and there is no guarantee it will still be available
   *       when the client attempts to read an answer
   */
  typedef struct VardisShutdown_Request : ServiceRequest {
    VardisShutdown_Request () : ServiceRequest (stVardis_Shutdown) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisShutdown_Request& req);
  } VardisShutdown_Request;
  

  // -----------------------------------

  
  /**
   * @brief VardisActivate service primitives, exchanged via command
   *        socket
   */
  typedef struct VardisActivate_Request : ServiceRequest {
    VardisActivate_Request () : ServiceRequest (stVardis_Activate) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisActivate_Request& req);
  } VardisActivate_Request;

  
  typedef struct VardisActivate_Confirm : ServiceConfirm {
    VardisActivate_Confirm () : ServiceConfirm (stVardis_Activate) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisActivate_Confirm& confirm);
  } VardisActivate_Confirm;


  // -----------------------------------


  /**
   * @brief VardisDeactivate service primitives, exchanged via command
   *        socket
   */
  typedef struct VardisDeactivate_Request : ServiceRequest {
    VardisDeactivate_Request () : ServiceRequest (stVardis_Deactivate) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisDeactivate_Request& req);
  } VardisDeactivate_Request;

  
  typedef struct VardisDeactivate_Confirm : ServiceConfirm {
    VardisDeactivate_Confirm () : ServiceConfirm (stVardis_Deactivate) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisDeactivate_Confirm& conf);
  } VardisDectivate_Confirm;


  // -----------------------------------

  /**
   * @brief Service primitives to retrieve some statistics from
   *        Vardis, exchanged via command socket
   */
  
  typedef struct VardisGetStatistics_Request : ServiceRequest {
    VardisGetStatistics_Request () : ServiceRequest (stVardis_GetStatistics) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisGetStatistics_Request& req);
  } VardisGetStatistics_Request;

  
  typedef struct VardisGetStatistics_Confirm : ServiceConfirm {
    VardisProtocolStatistics protocol_stats;
    
    VardisGetStatistics_Confirm () : ServiceConfirm (stVardis_GetStatistics) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisGetStatistics_Confirm& conf);
  } VardisGetStatistics_Confirm;
  

  
  // -----------------------------------


  /**
   * @brief VardisDescribeDatabase request primitive, exchanged via command
   *        socket
   */
  typedef struct VardisDescribeDatabase_Request : ServiceRequest {
    VardisDescribeDatabase_Request () : ServiceRequest (stVardis_RTDB_DescribeDatabase) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisDescribeDatabase_Request& req);
  } VardisDescribeDatabase_Request;


  /**
   * @brief Contains all the metadata describing one variable in the
   *        variable database
   */
  typedef struct DescribeDatabaseVariableDescription {
    VarIdT             varId;
    NodeIdentifierT    prodId;
    VarRepCntT         repCnt;
    char               description [MAX_maxDescriptionLength + 1];
    TimeStampT         tStamp;
    bool               isDeleted;

    friend std::ostream& operator<<(std::ostream& os, const DescribeDatabaseVariableDescription& descr);
  } DescribeDatabaseVariableDescription;


  /**
   * @brief VardisDescribeDatabase confirm primitive, exchanged via
   *        command socket
   *
   * This primitive contains the number of variable descriptions, the
   * actual descriptions follow immediately after (written
   * contiguously)
   */
  typedef struct VardisDescribeDatabase_Confirm : ServiceConfirm {
    uint64_t           numberVariableDescriptions = 0;
    VardisDescribeDatabase_Confirm () : ServiceConfirm (stVardis_RTDB_DescribeDatabase) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisDescribeDatabase_Confirm& confirm);
  } VardisDescribeDatabase_Confirm;


  // -----------------------------------

  /**
   * @brief VardisDescribeVariable request primitive, exchanged via command
   *        socket
   */
  typedef struct VardisDescribeVariable_Request : ServiceRequest {
    VarIdT  varId;
    VardisDescribeVariable_Request () : ServiceRequest (stVardis_RTDB_DescribeVariable) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisDescribeVariable_Request& req);
  } VardisDescribeVariable_Request;


  /**
   * @brief Contains all the data describing one variable in the
   *        variable database
   *
   * Note: the actual value is not contained, but will immediately
   * follow this structure
   */
  typedef struct DescribeVariableDescription {
    VarIdT            varId;
    NodeIdentifierT   prodId;
    VarRepCntT        repCnt;
    char              description [MAX_maxDescriptionLength + 1];
    VarSeqnoT         seqno;
    TimeStampT        tStamp;
    VarRepCntT        countUpdate;
    VarRepCntT        countCreate;
    VarRepCntT        countDelete;
    bool              isDeleted;
    VarLenT           value_length;

    friend std::ostream& operator<<(std::ostream& os, const DescribeVariableDescription& descr);
  } DescribeVariableDescription;
  

  /**
   * @brief VardisDescribeVariable confirm primitive, exchanged via
   *        command socket
   *
   * This primitive contains the description of the requested
   * variable, followed by a variable number of bytes with the
   * variable value (written contiguously)
   */
  typedef struct VardisDescribeVariable_Confirm : ServiceConfirm {
    DescribeVariableDescription var_description;
    VardisDescribeVariable_Confirm () : ServiceConfirm (stVardis_RTDB_DescribeVariable) {};
    friend std::ostream& operator<<(std::ostream& os, const VardisDescribeVariable_Confirm& req);
  } VardisDescribeVariable_Confirm;

  
  /*************************************************************************
   * Base classes for RTDB CRUD service primitives -- these are also
   * transmissible types, but are exchanged via shared memory and make
   * use of serialization / deserialization facilities
   ************************************************************************/

  class RTDBServiceRequest : public ServiceRequest {
  public:
    RTDBServiceRequest () = delete;
    RTDBServiceRequest (DcpServiceType st) : ServiceRequest (st) {};
    void serialize    (AssemblyArea& area) const { area.serialize_uint16_n (s_type); };
    void deserialize  (DisassemblyArea& area) { area.deserialize_uint16_n (s_type); };
  };
  

  class RTDBServiceConfirm : public ServiceConfirm {
  public:
    RTDBServiceConfirm () = delete;
    RTDBServiceConfirm (DcpServiceType st) : ServiceConfirm (st) {};
    RTDBServiceConfirm (DcpServiceType st, DcpStatus stat) : ServiceConfirm (st, stat) {};
    void serialize    (AssemblyArea& area) const
    {
      area.serialize_uint16_n (s_type);
      area.serialize_uint16_n (status_code);
    };
    void deserialize  (DisassemblyArea& area)
    {
      area.deserialize_uint16_n (s_type);
      area.deserialize_uint16_n (status_code);
    };
  };
  


  
  /*************************************************************************
   * The RTDB CRUD services
   ************************************************************************/

  // -----------------------------------
  
  
  /**
   * @brief RTDB-Create request primitive, exchanged via shared memory
   */
  typedef struct RTDB_Create_Request : public RTDBServiceRequest {
    VarSpecT   spec;
    VarValueT  value;
    
    RTDB_Create_Request () : RTDBServiceRequest (stVardis_RTDB_Create) {};


    /**
     * @brief Serialization method, serializes own data members
     */
    void serialize (AssemblyArea& area) const
    {
      RTDBServiceRequest::serialize (area);
      spec.serialize (area);
      value.serialize (area);
    };


    /**
     * @brief Convenience serialization method, serializes given VarSpecT and VarValueT
     *
     * This avoids the need to first copy spec and value into this
     * service primitive before serializing
     */
    void serialize (AssemblyArea& area, const VarSpecT& vspec, const VarValueT& vval) const
    {
      RTDBServiceRequest::serialize (area);
      vspec.serialize (area);
      vval.serialize (area);
    };


    /**
     * @brief Deserialization
     */
    void deserialize (DisassemblyArea& area)
    {
      RTDBServiceRequest::deserialize (area);
      spec.deserialize (area);
      value.deserialize (area);
    };
    
    friend std::ostream& operator<<(std::ostream& os, const RTDB_Create_Request& req);
  } RTDB_Create_Request;


  /**
   * @brief RTDB-Create confirm primitive, exchanged via shared memory
   */
  typedef struct RTDB_Create_Confirm : RTDBServiceConfirm {
    VarIdT  varId;
    RTDB_Create_Confirm () : RTDBServiceConfirm(stVardis_RTDB_Create) {};
    RTDB_Create_Confirm (DcpStatus scode) : RTDBServiceConfirm(stVardis_RTDB_Create, scode) {};
    RTDB_Create_Confirm (DcpStatus scode, VarIdT vid) : RTDBServiceConfirm(stVardis_RTDB_Create, scode), varId (vid) {};

    void serialize (AssemblyArea& area) const
    {
      RTDBServiceConfirm::serialize (area);
      varId.serialize (area);
    };

    void deserialize (DisassemblyArea& area)
    {
      RTDBServiceConfirm::deserialize (area);
      varId.deserialize (area);
    };


    friend std::ostream& operator<<(std::ostream& os, const RTDB_Create_Confirm& conf);
  } RTDB_Create_Confirm;


  // -----------------------------------


  /**
   * @brief RTDB-Delete request and confirm primitives
   */
  

  typedef struct RTDB_Delete_Request : RTDBServiceRequest {
    VarIdT   varId;
    
    RTDB_Delete_Request () : RTDBServiceRequest (stVardis_RTDB_Delete) {};

    void serialize (AssemblyArea& area) const
    {
      RTDBServiceRequest::serialize (area);
      varId.serialize (area);
    };

    void deserialize (DisassemblyArea& area)
    {
      RTDBServiceRequest::deserialize (area);
      varId.deserialize (area);
    };
    
    friend std::ostream& operator<<(std::ostream& os, const RTDB_Delete_Request& req);
  } RTDB_Delete_Request;


  typedef struct RTDB_Delete_Confirm : RTDBServiceConfirm {
    VarIdT  varId;

    RTDB_Delete_Confirm () : RTDBServiceConfirm(stVardis_RTDB_Delete) {};
    RTDB_Delete_Confirm (DcpStatus scode) : RTDBServiceConfirm(stVardis_RTDB_Delete, scode) {};
    RTDB_Delete_Confirm (DcpStatus scode, VarIdT vid) : RTDBServiceConfirm(stVardis_RTDB_Delete, scode), varId (vid) {};

    void serialize (AssemblyArea& area) const
    {
      RTDBServiceConfirm::serialize (area);
      varId.serialize (area);
    };

    void deserialize (DisassemblyArea& area)
    {
      RTDBServiceConfirm::deserialize (area);
      varId.deserialize (area);
    };

    
    friend std::ostream& operator<<(std::ostream& os, const RTDB_Delete_Confirm& conf);
  } RTDB_Delete_Confirm;

  
  
  
  // -----------------------------------
  
  
  /**
   * @brief RTDB-Update request and confirm primitives, exchanged via shared memory
   */

  
  typedef struct RTDB_Update_Request : RTDBServiceRequest {
    VarIdT     varId;
    VarValueT  value;
    
    RTDB_Update_Request () : RTDBServiceRequest (stVardis_RTDB_Update) {};


    /**
     * @brief Serializes this object into given area
     */
    void serialize (AssemblyArea& area) const
    {
      RTDBServiceRequest::serialize (area);
      varId.serialize (area);
      value.serialize (area);
    };


    /**
     * @brief Convenience serialization method, serializes given VarValueT
     *
     * This avoids the need to first copy value into this service
     * primitive before serializing
     */
    void serialize (AssemblyArea& area, const VarValueT& vval) const
    {
      RTDBServiceRequest::serialize (area);
      varId.serialize (area);
      vval.serialize (area);
    };


    /**
     * @brief Deserialization
     */
    void deserialize (DisassemblyArea& area)
    {
      RTDBServiceRequest::deserialize (area);
      varId.deserialize (area);
      value.deserialize (area);
    };
    
    friend std::ostream& operator<<(std::ostream& os, const RTDB_Update_Request& req);
  } RTDB_Update_Request;


  typedef struct RTDB_Update_Confirm : RTDBServiceConfirm {
    VarIdT   varId;
    RTDB_Update_Confirm () : RTDBServiceConfirm(stVardis_RTDB_Update) {};
    RTDB_Update_Confirm (DcpStatus scode) : RTDBServiceConfirm(stVardis_RTDB_Update, scode) {};
    RTDB_Update_Confirm (DcpStatus scode, VarIdT vid) : RTDBServiceConfirm(stVardis_RTDB_Update, scode), varId(vid) {};

    void serialize (AssemblyArea& area) const
    {
      RTDBServiceConfirm::serialize (area);
      varId.serialize (area);
    };

    void deserialize (DisassemblyArea& area)
    {
      RTDBServiceConfirm::deserialize (area);
      varId.deserialize (area);
    };


    friend std::ostream& operator<<(std::ostream& os, const RTDB_Update_Confirm& conf);
  } RTDB_Update_Confirm;


  // -----------------------------------
  
  
  /**
   * @brief RTDB-Read request and confirm primitives, exchanged via shared memory
   */

  
  typedef struct RTDB_Read_Request : RTDBServiceRequest {
    VarIdT     varId;
    
    RTDB_Read_Request () : RTDBServiceRequest (stVardis_RTDB_Read) {};

    void serialize (AssemblyArea& area) const
    {
      RTDBServiceRequest::serialize (area);
      varId.serialize (area);
    };

    void deserialize (DisassemblyArea& area)
    {
      RTDBServiceRequest::deserialize (area);
      varId.deserialize (area);
    };
    
    friend std::ostream& operator<<(std::ostream& os, const RTDB_Read_Request& req);
  } RTDB_Read_Request;

  
  /**
   * @brief RTDB-Read confirm primitive
   */
  typedef struct RTDB_Read_Confirm : RTDBServiceConfirm {
    VarIdT      varId;
    VarValueT   value;
    TimeStampT  tStamp;


    /**
     * @brief Regular constructors
     */
    RTDB_Read_Confirm () : RTDBServiceConfirm(stVardis_RTDB_Read) {};
    RTDB_Read_Confirm (DcpStatus scode) : RTDBServiceConfirm(stVardis_RTDB_Read, scode) {};
    RTDB_Read_Confirm (DcpStatus scode, VarIdT vid)
      : RTDBServiceConfirm(stVardis_RTDB_Read, scode)
      , varId(vid)
    {};


    /**
     * @brief Constructor, initializes value without copying it from memory
     */
    RTDB_Read_Confirm (DcpStatus scode, VarIdT vid, VarLenT len, byte* pb)
      : RTDBServiceConfirm(stVardis_RTDB_Read, scode)
      , varId (vid)
      , value (len, pb)
    {};


    /**
     * @brief Serialization method
     */
    void serialize (AssemblyArea& area)
    {
      RTDBServiceConfirm::serialize (area);
      varId.serialize (area);
      value.serialize (area);
      tStamp.serialize (area);
    };


    /**
     * @brief Deserialization method storing the data in this object
     */
    void deserialize (DisassemblyArea& area)
    {
      RTDBServiceConfirm::deserialize (area);
      varId.deserialize (area);
      value.deserialize (area);
      tStamp.deserialize (area);
    };


    /**
     * @brief Deserialization method storing the data in a given buffer
     */
    void deserialize (DisassemblyArea& area, VarLenT& length, byte* data_buffer)
    {
      RTDBServiceConfirm::deserialize (area);
      varId.deserialize (area);
      value.deserialize (area, length, data_buffer);
      tStamp.deserialize (area);
    };


    friend std::ostream& operator<<(std::ostream& os, const RTDB_Read_Confirm& conf);
  } RTDB_Read_Confirm;

  
  
};  // namespace dcp::vardis
