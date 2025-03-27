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

#include <dcp/common/configuration.h>
#include <dcp/common/sharedmem_configuration.h>
#include <dcp/srp/srp_constants.h>


/**
 * @brief Configuration required for a SRP client application /
 *        protocol
 */

namespace dcp {


  /**
   * @brief Configuration for a SRP client
   *
   * Contains the SRP global shared memory segment (SRP store). For
   * this the most important features is the shared memory segment
   * file name.
   */
  typedef struct SRPClientConfiguration : public DcpConfiguration {
    SharedMemoryConfigurationBlock    shm_conf_store;  /*!< Name of SRP store (contains neighbour table etc) */

    
    /**
     * @brief Constructor, setting SRP store file name
     *
     * @param store_area_name: shared memory name of global SRP store
     */
    SRPClientConfiguration (std::string store_area_name = dcp::srp::defaultSRPStoreShmName) :
      shm_conf_store ("SRPStoreShm")
    {
      shm_conf_store.shmAreaName     = store_area_name;
    };
    

    /**
     * @brief Build description of options for BOOST config file reader
     */
    virtual void build_description (po::options_description& cfgdesc)
    {
      shm_conf_store.add_options (cfgdesc);
    };


    /**
     * @brief Validate configuration, throws if invalid
     */
    virtual void validate ()
    {
      shm_conf_store.validate ();
    };

    /**
     * @brief Overloaded output operator
     *
     * @param os: output stream to use
     * @param cfg: the SRPClientConfiguration object to output 
     */
    friend std::ostream& operator<<(std::ostream& os, const SRPClientConfiguration& cfg);

    
  } SRPClientConfiguration;
  
};  // namespace dcp
