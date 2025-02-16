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
#include <dcp/vardis/vardis_client_protocol_data.h>
#include <dcp/vardis/vardis_configuration.h>
#include <dcp/vardis/vardis_protocol_data.h>
#include <dcp/vardis/vardis_rtdb_entry.h>
#include <dcp/vardis/vardis_transmissible_types.h>


/**
 * @brief This module provides a data type / class holding all of
 *        Vardis' runtime data and two support classes concerned with
 *        locks for certain parts of the runtime data
 */


namespace dcp::vardis {


  /**
   * @brief This class holds all the data that the Vardis demon needs
   *        at runtime
   */
  
  class VardisRuntimeData : public BPClientRuntime {
  public:
    
    VardisRuntimeData () = delete;


    /**
     * @brief Constructor, registers with BP, initializes Vardis
     *        command socket, and initializes Vardis runtime data
     *
     * Note: Vardis does not allow multiple payloads in one beacon and
     * the demon does not request or process
     * BP-TransmitPayload.confirm primitives
     */
    VardisRuntimeData (const BPProtocolIdT protocol_id,
		       const std::string protname,
		       const VardisConfiguration& cfg)
    : BPClientRuntime (protocol_id,
		       protname,
		       cfg.vardis_conf.maxPayloadSize,
		       dcp::bp::BP_QMODE_QUEUE_DROPHEAD,
		       cfg.vardis_conf.queueMaxEntries,
		       false,   // allowMultiplepayloads
		       false,   // generateTransmitPayloadConfirms
		       cfg),
    vardisCommandSock(cfg.vardis_cmdsock_conf.commandSocketFile, cfg.vardis_cmdsock_conf.commandSocketTimeoutMS),
    vardis_config (cfg),
    vardis_exitFlag (false),
    protocol_data (cfg.vardis_conf, get_own_node_identifier())
    {
    };


     
    /**
     * @brief Command socket for Vardis client applications
     */
    CommandSocket  vardisCommandSock;

    
    /**
     * @brief Holds the configuration data
     */
    VardisConfiguration   vardis_config;


    /**
     * @brief Flag set by signal handlers to exit Vardis demon
     */
    bool vardis_exitFlag = false;

    /**
     * @brief Holds the mutex for access to the real-time variable
     *        database and the queues
     */
    std::mutex protocol_data_mutex;


    /**
     * @brief Contains the runtime data of the protocol proper
     *        (real-time variable database, queues)
     */
    VardisProtocolData  protocol_data;



   /**
     * @brief Mutex for access to clientApplications member
     */
    std::mutex clientApplications_mutex;

    
    /**
     * @brief Map containing VardisClientProtocolData records for each
     *        client application identifier
     */
    std::map<std::string, VardisClientProtocolData>  clientApplications;
  
  };


  // -----------------------------------------------------------------


  /**
   * @brief Acquires the mutex for the protocol_data member of a
   *        VardisRuntimeData object. The mutex is held throughout the
   *        lifetime of this object.
   *
   * Note that an object of this class maintains a pointer to a
   * VardisRuntimeData object. The caller must guarantee that the
   * VardisRuntimedata object lives at least as long as this locking
   * object.
   */
  class ScopedProtocolDataMutex {
  private:
    VardisRuntimeData* ptr = nullptr;
  public:
    ScopedProtocolDataMutex() = delete;
    ScopedProtocolDataMutex (VardisRuntimeData& runtime)
    {
      ptr = &runtime;
      runtime.protocol_data_mutex.lock();
    };
    
    ~ScopedProtocolDataMutex ()
    {
      if (ptr)
	ptr->protocol_data_mutex.unlock();
    };
  };


  // -----------------------------------------------------------------


  /**
   * @brief Acquires the mutex for the clientApplications member of a
   *        VardisRuntimeData object. The mutex is held throughout the
   *        lifetime of this object.
   *
   * Note that an object of this class maintains a pointer to a
   * VardisRuntimeData object. The caller must guarantee that the
   * VardisRuntimedata object lives at least as long as this locking
   * object.
   */
  class ScopedClientApplicationsMutex {
  private:
    VardisRuntimeData* ptr = nullptr;
  public:
    ScopedClientApplicationsMutex() = delete;
    ScopedClientApplicationsMutex (VardisRuntimeData& runtime)
    {
      ptr = &runtime;
      runtime.clientApplications_mutex.lock();
    };
    
    ~ScopedClientApplicationsMutex ()
    {
      if (ptr)
	ptr->clientApplications_mutex.unlock();
    };
  };
  
  
};  // namespace dcp::vardis
