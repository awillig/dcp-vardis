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

/**
 * @brief Configuration required for a Vardis client application /
 *        protocol
 */

namespace dcp {


  /**
   * @brief Configuration for a Vardis client
   *
   * Contains a command socket (for exchanging service requests and
   * responses with Vardis demon) and a shared memory segment (for
   * exchanging RTDB services). For both of these the most important
   * features are their respective file names
   */
  typedef struct VardisClientConfiguration : public DcpConfiguration {

    CommandSocketConfigurationBlock   cmdsock_conf;
    SharedMemoryConfigurationBlock    shm_conf;

    /**
     * @brief Constructor, setting section names for both config
     *        blocks and setting command socket and shared memory
     *        names to given arguments
     */
    VardisClientConfiguration () :
      cmdsock_conf ("VardisCommandSocket"),
      shm_conf ("dcp-vardisclient-shm")
    {};
    

    /**
     * @brief Constructor, setting section names for both config
     *        blocks and setting command socket and shared memory
     *        names to given arguments
     */
    VardisClientConfiguration (const std::string& cmdsock_file, const std::string& shm_area_name) :
      cmdsock_conf ("VardisCommandSocket"),
      shm_conf ("dcp-vardisclient-shm")
    {
      cmdsock_conf.commandSocketFile = cmdsock_file;
      shm_conf.shmAreaName           = shm_area_name;
    };
    

    /**
     * @brief Build description of options for BOOST config file reader
     */
    virtual void build_description (po::options_description& cfgdesc)
    {
      cmdsock_conf.add_options (cfgdesc);
      shm_conf.add_options (cfgdesc);
    };


    /**
     * @brief Validate configuration, throws if invalid
     */
    virtual void validate ()
    {
      cmdsock_conf.validate ();
      shm_conf.validate ();
    };
        
  } VardisClientConfiguration;
  
};  // namespace dcp
