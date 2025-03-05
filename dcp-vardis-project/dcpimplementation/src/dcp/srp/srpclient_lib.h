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
#include <list>
#include <dcp/common/command_socket.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/services_status.h>
#include <dcp/srp/srpclient_configuration.h>
#include <dcp/srp/srp_store_fixedmem_shm.h>
#include <dcp/srp/srp_transmissible_types.h>


/**
 * @brief This module / class provides the runtime data that a Vardis
 *        client application / protocol needs, and offers a range of
 *        management services.
 */

using dcp::srp::SafetyDataT;
using dcp::srp::DefaultSRPStoreType;

namespace dcp {

  //class SRPClientRuntime : public BaseClientRuntime {
  class SRPClientRuntime {
  protected:
    
    DefaultSRPStoreType srp_store;

    
  public:

    
    SRPClientRuntime () = delete;


    /**
     * @brief Constructor, checks validity of command socket and
     *        shared memory segment names, and then registers with
     *        Vardis. Throws if invalid or registration failed
     *
     * @param client_conf: Configuration data for Vardis client
     * @param do_register: specifies whether client should
     *        register with Vardis demon
     */
    SRPClientRuntime (const SRPClientConfiguration& client_conf);

    
    /**
     * @brief Destructor, deregisters with Vardis if previously registered
     */
    ~SRPClientRuntime ();


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
     * @brief Request the SRP demon to exit
     *
     * After calling this no further service requests should be sent
     * to the Vardis demon, otherwise undefined behaviour might result
     */
    DcpStatus shutdown_srp ();


    /**
     * @brief Request Vardis demon to activate, i.e. resume processing
     *        of received payloads, generating payloads and processing
     *        of RTDB service requests
     */
    DcpStatus activate_srp ();


    /**
     * @brief Request Vardis demon to deactivate, i.e. stop processing
     *        of received payloads, stop generating own payloads, and stop
     *        processing RTDB service requests
     */
    DcpStatus deactivate_srp ();

    
    /****************************************************************
     * Synchronous RTDB services
     ***************************************************************/

    DcpStatus set_own_safety_data (const SafetyDataT& new_sd);

    
    
  };
  
};  // namespace dcp
