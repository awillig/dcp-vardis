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


namespace dcp::srp {


  /**
   * @brief This class holds all the data that the SRP demon needs
   *        at runtime
   */
  
  class SRPRuntimeData : public BPClientRuntime {
  public:
    
    SRPRuntimeData () = delete;

    SRPRuntimeData (const BPProtocolIdT protocol_id,
		    const std::string protname,
		    const SRPConfiguration& cfg)
    : BPClientRuntime (protocol_id,
		       protname,
		       ExtendedSafetyDataT::fixed_size(),
		       dcp::bp::BP_QMODE_REPEAT,
		       0,
		       false,   // allowMultiplepayloads
		       false,   // generateTransmitPayloadConfirms
		       cfg),
      srp_store (cfg.shm_conf.shmAreaName.c_str(),
		 true,
		 0.9,    // alpha value for EWMA estimator
		 get_own_node_identifier()),
      srp_config (cfg),
      srp_exitFlag (false)
    {
    };


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
   * @brief Acquires the mutex for the neighbour_store member of a
   *        SRPRuntimeData object. The mutex is held throughout the
   *        lifetime of this object.
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
