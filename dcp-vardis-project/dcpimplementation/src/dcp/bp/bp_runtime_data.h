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
#include <mutex>
#include <tins/tins.h>
#include <dcp/common/command_socket.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/bp/bp_configuration.h>
#include <dcp/bp/bp_client_protocol_data.h>

using dcp::CommandSocket;

namespace dcp::bp {

  /**
   * @brief This struct holds all the data that the BP instance needs at runtime
   */
  
  typedef struct BPRuntimeData {

    /*********************************************************************
     * Data members
     ********************************************************************/
    
    /**
     * @brief Holds the configuration data
     */
    BPConfiguration  bp_config;


    /**
     * @brief Indicates whether BP is currently active or not.
     *
     * The BP needs to be in active mode to process or generate beacons
     */
    bool bp_isActive = true;


    /**
     * @brief Flag set by signal handlers to exit BP demon
     */
    bool bp_exitFlag = false;


    /**
     * @brief Holds network interface information data (obtained from libtins)
     */
    Tins::NetworkInterface::Info nw_if_info;


    /**
     * @brief Holds a libtins PacketSender object
     */
    Tins::PacketSender pktSender;
    
    /**
     * @brief Holds the own node identifier
     */
    NodeIdentifierT ownNodeIdentifier;
    

    /**
     * @brief Holds own sequence number for outgoing beacons
     */
    uint32_t bpSequenceNumber = 0;
    
    /**
     * @brief Holds the list of all currently known client protocols
     */
    std::map<BPProtocolIdT, BPClientProtocolData>  clientProtocols;


    /**
     * @brief Mutex for access to clientProtocols member
     */
    std::mutex clientProtocols_mutex;
    

    /**
     * @brief Socket used for command / service interface
     */
    // int commandSocket = -1;
    CommandSocket  commandSocket;
    

    /*********************************************************************
     * Some statistics
     ********************************************************************/

    /**
     * @brief Number of received payloads from unknown / unregistered client protocol id's
     */
    unsigned int  cntDroppedUnknownPayloads  =  0;


    /**
     * @brief Number of received BP payloads
     */
    unsigned int cntBPPayloads  = 0;


    /**
     * @brief Estimation of average inter-beacon reception time (in ms)
     */
    double avg_inter_beacon_reception_time = 0;
    

    /**
     * @brief Estimation of average received beacon size (in bits)
     */
    double avg_received_beacon_size = 0;

    
    /*********************************************************************
     * Methods
     ********************************************************************/

    /**
     * @brief Forbid generation / use of default constructor
     */
    BPRuntimeData () = delete;


    /**
     * @brief Constructor requiring configuration data.
     *
     * It is assumed that the configuration data is valid
     */
    BPRuntimeData (BPConfiguration& cfg);

    
  } BPRuntimeData;
  
};  // namespace dcp::bp
