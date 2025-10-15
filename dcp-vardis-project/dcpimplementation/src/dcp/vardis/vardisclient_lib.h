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

#include <list>
#include <dcp/common/command_socket.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/services_status.h>
#include <dcp/common/sharedmem_structure_base.h>
#include <dcp/vardis/vardisclient_configuration.h>
#include <dcp/vardis/vardis_constants.h>
#include <dcp/vardis/vardis_protocol_statistics.h>
#include <dcp/vardis/vardis_service_primitives.h>
#include <dcp/vardis/vardis_shm_control_segment.h>
#include <dcp/vardis/vardis_transmissible_types.h>
#include <dcp/vardis/vardis_store_array_shm.h>


using dcp::vardis::DescribeDatabaseVariableDescription;
using dcp::vardis::vardisCommandSocketBufferSize;
using dcp::vardis::VardisProtocolStatistics;
using dcp::vardis::VardisShmControlSegment;
using dcp::vardis::VardisVariableStoreShm;
using dcp::vardis::VariableStoreI;
using dcp::vardis::DescribeVariableDescription;
using dcp::vardis::VarIdT;
using dcp::vardis::VarLenT;
using dcp::vardis::VarSpecT;
using dcp::vardis::VarValueT;




/**
 * @brief This module / class provides the runtime data that a Vardis
 *        client application / protocol needs, and offers a range of
 *        management services.
 *
 * This module profides the 'official API' available to a Vardis client.
 */


namespace dcp {

  class VardisClientRuntime : public BaseClientRuntime {
  protected:

    std::string shmSegmentName;  /*!< Name of shared segment between client and Vardis demon */

    /**
     * @brief Register client with Vardis demon.
     *
     * @param delete_old_registration: specifies whether an old registration
     *        (if any) should be simply deleted instead of raising an error
     *
     * If registration is successful this establishes access to the
     * shared memory segment.
     */
    DcpStatus register_with_vardis (bool delete_old_registration = false);

    
    /**
     * @brief Deregister client with Vardis demon
     *
     * The Vardis demon will detach from the shared memory
     * segment. The client will detach when the lifetime of this
     * object ends
     */
    DcpStatus deregister_with_vardis ();


    /**
     * @brief Pointer to the per-client shared memory segment descriptor
     */
    std::shared_ptr<ShmStructureBase> pSSB;


    /**
     * @brief Pointer to the actual shared memory region. Valid after
     *        successful registration
     */
    VardisShmControlSegment*          pSCS = nullptr;


    /**
     * @brief Own node identifier, results from successful registration
     */
    NodeIdentifierT ownNodeIdentifier;
    

    /**
     * @brief The Vardis variable store -- contains all variables and
     *        their descriptions
     */
    VardisVariableStoreShm variable_store;


    /**
     * @brief Configuration data of Vardis client
     */
    VardisClientConfiguration client_configuration;
    
    
  public:

    
    VardisClientRuntime () = delete;


    /**
     * @brief Constructor, checks validity of command socket and
     *        shared memory segment names, and then registers with
     *        Vardis. Throws if invalid or registration failed
     *
     * @param client_conf: Configuration data for Vardis client
     * @param do_register: specifies whether client should
     *        register with Vardis demon
     * @param delete_old_registration: specifies whether an old registration
     *        (if any) should be simply deleted instead of raising an error
     */
    VardisClientRuntime (const VardisClientConfiguration& client_conf,
			 bool do_register = true,
			 bool delete_old_registration = false
			 );

    
    /**
     * @brief Destructor, deregisters with Vardis if previously registered
     */
    ~VardisClientRuntime ();


    /****************************************************************
     * Queries
     ***************************************************************/

    /**
     * @brief Returns the DCP node identifier of the present node
     */
    NodeIdentifierT get_own_node_identifier () const { return ownNodeIdentifier; };


    /**
     * @brief Returns shared memory segment name towards Vardis demon
     */
    std::string get_shm_segment_name () const { return shmSegmentName; };
    

    /****************************************************************
     * Management services
     ***************************************************************/

    /**
     * @brief Request the Vardis demon to exit
     *
     * After calling this no further service requests should be sent
     * to the Vardis demon, otherwise undefined behaviour might result
     */
    DcpStatus shutdown_vardis ();


    /**
     * @brief Request Vardis demon to activate, i.e. resume processing
     *        of received payloads, generating payloads and processing
     *        of RTDB service requests
     */
    DcpStatus activate_vardis ();


    /**
     * @brief Request Vardis demon to deactivate, i.e. stop processing
     *        of received payloads, stop generating own payloads, and stop
     *        processing RTDB service requests
     */
    DcpStatus deactivate_vardis ();


    /**
     * @brief Queries Vardis demon for a database description (list of
     *        existing variables, each reported with relevant metadata)
     *
     * @param db_descr: output parameter containing the list of
     *        retrieved variable descriptions. Only valid if this
     *        returns VARDIS_STATUS_OK
     */
    DcpStatus describe_database (std::list<DescribeDatabaseVariableDescription>& db_descr);


    /**
     * @brief Queries Vardis demon for a complete description of a
     *        variable
     *
     * @param varId: the variable identifier of the variable to be
     *        queried
     * @param var_descr: output parameter containing all information
     *        about the retrieved variable
     * @param buffer: pre-allocated buffer into which the variable
     *        value will be written. The length of the written
     *        variable is contained in the var_descr output
     *        parameter. The buffer must be large enough to
     *        accommodate a maximum sized variable.
     */
    DcpStatus describe_variable (VarIdT varId,
				 DescribeVariableDescription& var_descr,
				 byte* buffer);
    

    
    /**
     * @brief Queries certain runtime statistics from the Vardis demon
     *
     * @param stats: output parameter containing the retrieved
     *        statistics object, only valid if this returns
     *        VARDIS_STATUS_OK
     */
    DcpStatus retrieve_statistics (VardisProtocolStatistics& stats);

    
    /****************************************************************
     * Synchronous RTDB services
     ***************************************************************/


    /**
     * @brief Create a variable
     *
     * @param spec: specification for new variable (all fields except
     *        creationTime need to be valid)
     * @param value: initial value of variable
     */
    DcpStatus rtdb_create (const VarSpecT& spec, const VarValueT& value);


    /**
     * @brief Delete a variable
     *
     * @param varId: Identifier of variable to be deleted
     */
    DcpStatus rtdb_delete (VarIdT varId);


    /**
     * @brief Update a variable
     */
    DcpStatus rtdb_update (VarIdT varId, const VarValueT& value);


    /**
     * @brief Read a variable.
     *
     * Note: This function does not allocate memory, caller must
     *       provide sufficient buffer space. Returns VARDIS_STATUS_OK
     *       if variable exists and is not deleted, and only in this
     *       case are the buffer contents and other return values
     *       valid.
     *
     * @param varId: variable identifier of variable to be read
     * @param responseVarId: output: variable identifier stored in the
     *        variable database entry
     * @param responseVarLen: output: length of variable value returned
     * @param responseTimeStamp: output: local timestamp of last create
     *        or update operation on this variable
     * @param value_bufsize: size of the application-provided buffer
     *        into which the variable value is to be copied
     * @param value_buffer: application-provided buffer into which the
     *        variable value is being copied.
     *
     * @return If the variable is not allocated then the value
     *         VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST is returned. If the
     *         variable exists but is deleted then VARDIS_STATUS_VARIABLE_IS_DELETED
     *         is returned, otherwise VARDIS_STATUS_OK.
     */
    DcpStatus rtdb_read   (VarIdT varId,
			   VarIdT& responseVarId,
			   VarLenT& responseVarLen,
			   TimeStampT& responseTimeStamp,
			   size_t value_bufsize,
			   byte* value_buffer);
    
  };
  
};  // namespace dcp
