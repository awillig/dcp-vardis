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
#include <dcp/srp/srpclient_configuration.h>
#include <dcp/srp/srp_store_fixedmem_shm.h>
#include <dcp/srp/srp_transmissible_types.h>


/**
 * @brief This module / class provides the runtime data that a SRP
 *        client application / protocol needs, and offers a range of
 *        data access and management services.
 *
 * Note that the SRP demon does not offer a command socket a a
 * per-client shared memory segment, only one global shared memory
 * segment (the SRP store) containing all relevant data like the
 * neighbour table, a buffer for the own safety data and others. An
 * SRP client attaches to this shared memory segment as a client.
 *
 * This module provides the 'official API' available to a SRP client.
 */

using dcp::srp::SafetyDataT;
using dcp::srp::ExtendedSafetyDataT;
using dcp::srp::DefaultSRPStoreType;
using dcp::srp::NodeInformation;

namespace dcp {

  /**
   * @brief Class providing all the runtime data a SRP client needs
   */
  class SRPClientRuntime {
  protected:


    /**
     * @brief This member holds access to the SRP store (in shared
     *        memory)
     */
    DefaultSRPStoreType srp_store;

    
  public:

    
    SRPClientRuntime () = delete;


    /**
     * @brief Constructor, initializes access to the SRP store
     *
     * @param client_conf: Configuration data for SRP client (contains
     *        in particular the name of the SRP store)
     */
    SRPClientRuntime (const SRPClientConfiguration& client_conf);


    /****************************************************************
     * Queries
     ***************************************************************/

    /**
     * @brief Returns the DCP node identifier of the present node
     */
    NodeIdentifierT get_own_node_identifier () const;


    /****************************************************************
     * Management services
     ***************************************************************/


    /**
     * @brief Request SRP demon to activate, i.e. resume processing of
     *        received payloads and generating payloads
     */
    DcpStatus activate_srp ();


    /**
     * @brief Request SRP demon to deactivate, i.e. stop processing
     *        received payloads and stop generating own payloads
     */
    DcpStatus deactivate_srp ();

    
    /****************************************************************
     * Service for sending own safety data
     ***************************************************************/


    /**
     * @brief Writes new safety data into buffer for periodic
     *        transmission
     *
     * Note: After this operation the safety data is sent only for a
     * limited amount of time (cf configuration parameter
     * keepaliveTimeoutMS) before ceasing. Therefore, this must be
     * regularly refreshed to keep transmission of the own safety data
     * going.
     */
    DcpStatus set_own_safety_data (const SafetyDataT& new_sd);


    /****************************************************************
     * Services for accessing neighbour table
     ***************************************************************/
    

    /**
     * @brief Return list of NodeInformation records for all currently
     *        registered neighbours
     *
     * @param neighbour_list: output value containing list of
     *        NodeInformation records of all current neighbours
     */
    DcpStatus get_all_neighbours_node_information (std::list<NodeInformation>& neighbour_list);
    
  };
  
};  // namespace dcp
