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
#include <dcp/common/shared_mem_area.h>

namespace dcp {

  /**
   * @brief Managing configuration data held by a BP client protocol
   *
   * The BP client configuration contains configurration blocks for a
   * command socket (for communicating with the BP demon) and a shared
   * memory block (for exchanging payloads).
   */
  typedef struct BPClientConfiguration : public DcpConfiguration {

    /**
     * @brief Configuration blocks included
     */
    CommandSocketConfigurationBlock  bp_cmdsock_conf;
    SharedMemoryConfigurationBlock   bp_shm_conf;
    

    /**
     * @brief Constructor, sets default section names for
     *        configuration file and default name for BP command
     *        socket
     */
    BPClientConfiguration () :
      bp_cmdsock_conf ("BPCommandSocket"),
      bp_shm_conf ("dcp-bpclient-shm")
    {
      bp_cmdsock_conf.commandSocketFile = "/tmp/dcp-bp-command-socket";
    };


    /**
     * @brief Constructor setting given section names for
     *        configuration file
     */
    BPClientConfiguration (std::string cmdsock_blockname, std::string shm_blockname) :
      bp_cmdsock_conf (cmdsock_blockname),
      bp_shm_conf (shm_blockname)
    {
      bp_cmdsock_conf.commandSocketFile = "/tmp/dcp-bp-command-socket";
    };
    
    /**************************************************
     * Methods
     *************************************************/


    /**
     * @brief Build options description for configuration file
     */
    virtual void build_description (po::options_description& cfgdesc)
    {
      bp_cmdsock_conf.add_options (cfgdesc);
      bp_shm_conf.add_options (cfgdesc);
    };


    /**
     * @brief Validate configuration data. Throws upon invalid
     *        configuration.
     */
    virtual void validate ()
    {
      bp_cmdsock_conf.validate();
      bp_shm_conf.validate();
    };

    
  } BPClientConfiguration;
  
};  // namespace dcp
