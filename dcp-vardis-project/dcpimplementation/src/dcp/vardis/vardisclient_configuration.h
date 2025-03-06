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
#include <dcp/common/command_socket.h>
#include <dcp/common/configuration.h>
#include <dcp/common/shared_mem_area.h>
#include <dcp/vardis/vardis_constants.h>

/**
 * @brief Configuration required for a Vardis client application /
 *        protocol
 */

namespace dcp {


  /**
   * @brief Configuration for a Vardis client
   *
   * Contains a command socket (for exchanging service requests and
   * responses with Vardis demon), a client shared memory segment (for
   * exchanging RTDB services), and the Vardis global shared memory
   * segment (variable store). For all of these the most important
   * features are their respective file names
   */
  typedef struct VardisClientConfiguration : public DcpConfiguration {

    CommandSocketConfigurationBlock   cmdsock_conf;     /*!< Filename of Vardis command socket (UNIX Domain Socket) */
    SharedMemoryConfigurationBlock    shm_conf_client;  /*!< Name of client-specific shared memory area towards Vardis */
    SharedMemoryConfigurationBlock    shm_conf_global;  /*!< Name of Vardis global shared memory segment for variable store */

    /**
     * @brief Constructor, setting section names for all config
     *        blocks
     */
    VardisClientConfiguration () :
      cmdsock_conf ("VardisCommandSocket"),
      shm_conf_client ("dcp-vardisclient-shm"),
      shm_conf_global ("VardisVariableDatabaseShm")
    {};
    

    /**
     * @brief Constructor, setting section names for all config
     *        blocks and setting command socket and shared memory
     *        names to given arguments
     *
     * @param cmdsock_file: Filename of Vardis command socket (a UNIX Domain Socket)
     * @param client_area_name: shared memory name of client area towards Vardis
     * @param global_area_name: shared memory name of global Vardis variable store
     */
    VardisClientConfiguration (const std::string& cmdsock_file,
			       std::string client_area_name,
			       std::string global_area_name = dcp::vardis::defaultVardisStoreShmName) :
      cmdsock_conf ("VardisCommandSocket"),
      shm_conf_client ("dcp-vardisclient-shm"),
      shm_conf_global ("VardisVariableDatabaseShm")
    {
      cmdsock_conf.commandSocketFile  = cmdsock_file;
      shm_conf_client.shmAreaName     = client_area_name;
      shm_conf_global.shmAreaName     = global_area_name;
    };
    

    /**
     * @brief Build description of options for BOOST config file reader
     */
    virtual void build_description (po::options_description& cfgdesc)
    {
      cmdsock_conf.add_options (cfgdesc);
      shm_conf_client.add_options (cfgdesc);
      shm_conf_global.add_options (cfgdesc);
    };


    /**
     * @brief Validate configuration, throws if invalid
     */
    virtual void validate ()
    {
      cmdsock_conf.validate ();
      shm_conf_client.validate ();
      shm_conf_global.validate ();
    };

    /**
     * @brief Overloaded output operator
     *
     * @param os: output stream to use
     * @param cfg: the VardisClientConfiguration object to output 
     */
    friend std::ostream& operator<<(std::ostream& os, const VardisClientConfiguration& cfg);

    
  } VardisClientConfiguration;
  
};  // namespace dcp
