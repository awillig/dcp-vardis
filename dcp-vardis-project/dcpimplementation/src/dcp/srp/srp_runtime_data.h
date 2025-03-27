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

#include <map>
#include <queue>
#include <mutex>
#include <dcp/common/command_socket.h>
#include <dcp/bp/bpclient_lib.h>
#include <dcp/srp/srp_configuration.h>
#include <dcp/srp/srp_store_fixedmem_shm.h>
#include <dcp/srp/srp_transmissible_types.h>

/**
 * @brief This module provides a data type / class holding all of the
 *        SRP demons runtime data and a support class concerned with
 *        locks for the SRP neighbour store shared memory area
 */

using dcp::bp::BPStaticClientInfo;

namespace dcp::srp {


  /**
   * @brief This class holds all the data that the SRP demon needs
   *        at runtime
   */
  
  class SRPRuntimeData : public BPClientRuntime {
  public:
    
    SRPRuntimeData () = delete;


    /**
     * @brief Constructor
     *
     * @param static_client_info: static BP client protocol data to
     *        use (e.g. protocol name, queueing mode)
     * @param cfg: SRP configuration
     *
     * Initializes SRP as BP client (i.e. performs protocol
     * registration) and also initializes the SRP store (global shared
     * memory segment)
     */
    SRPRuntimeData (const BPStaticClientInfo static_client_info,
		    const SRPConfiguration cfg)
      : BPClientRuntime (cfg, static_client_info, false),   // generateTransmitPayloadConfirms
	srp_store (cfg.shm_conf.shmAreaName.c_str(),
		   true,
		   cfg.srp_conf.srpGapSizeEWMAAlpha,
		   get_own_node_identifier()),
	srp_config (cfg),
	srp_exitFlag (false)
    {
    };


    /**
     * @brief This holds the SRP store, containing the neighbour table
     *        and the own safety data for transmission, as well as
     *        some other global data (e.g. SRP active flag)
     */
    DefaultSRPStoreType srp_store;
    
        
    /**
     * @brief Holds the configuration data
     */
    SRPConfiguration   srp_config;


    /**
     * @brief Flag set by signal handlers to exit SRP demon
     */
    bool srp_exitFlag = false;
  };


  // -----------------------------------------------------------------


  /**
   * @brief Acquires the mutex for the neighbour table of the SRP
   *        store. The mutex is held throughout the lifetime of this
   *        object.
   *
   * Note that an object of this class maintains a pointer to a
   * SRPRuntimeData object. The caller must guarantee that the
   * SRPRuntimedata object lives at least as long as this locking
   * object.
   */
  class ScopedNeighbourTableMutex {
  private:
    SRPRuntimeData* ptr = nullptr;
  public:
    ScopedNeighbourTableMutex() = delete;
    ScopedNeighbourTableMutex (SRPRuntimeData& runtime)
    {
      ptr = &runtime;
      runtime.srp_store.lock_neighbour_table();
    };
    
    ~ScopedNeighbourTableMutex ()
    {
      if (ptr)
	ptr->srp_store.unlock_neighbour_table ();
    };
  };


  /**
   * @brief Acquires the mutex for the own safety data part of the SRP
   *        store. The mutex is held throughout the lifetime of this
   *        object.
   *
   * Note that an object of this class maintains a pointer to a
   * SRPRuntimeData object. The caller must guarantee that the
   * SRPRuntimedata object lives at least as long as this locking
   * object.
   */
  class ScopedOwnSDMutex {
  private:
    SRPRuntimeData* ptr = nullptr;
  public:
    ScopedOwnSDMutex() = delete;
    ScopedOwnSDMutex (SRPRuntimeData& runtime)
    {
      ptr = &runtime;
      runtime.srp_store.lock_own_safety_data ();
    };
    
    ~ScopedOwnSDMutex ()
    {
      if (ptr)
	ptr->srp_store.unlock_own_safety_data ();
    };
    
  };
  
};  // namespace dcp::srp
