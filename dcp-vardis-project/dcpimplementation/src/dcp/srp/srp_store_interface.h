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
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <dcp/common/global_types_constants.h>
#include <dcp/srp/srp_transmissible_types.h>


namespace dcp::srp {

  /**
   * @brief This class defines the abstraction of an SRP neighbour
   *        store, providing key operations on the neighbour table
   *        and the current node safety data
   *
   * Logically, the neighbour store contains the neighbour table
   * (holding information about all currently known neighbours) and an
   * area in which an application can supply the latest information
   * about the own position / speed / heading etc.
   */

  
  class SRPStoreI {
  public:
    
    /***************************************************************
     * Getters and setters for key runtime data (srp_isActive) used in
     * SRP protocol processing
     **************************************************************/

    /**
     * @brief Returns the own node identifier
     */
    virtual NodeIdentifierT get_own_node_identifier () const = 0;


    /**
     * @brief Return the current srp_isActive flag
     */
    virtual bool     get_srp_isactive () const = 0;


    /**
     * @brief Sets the srp_isActive flag to given value
     *
     * @param active: new value for srp_isActive flag
     */
    virtual void     set_srp_isactive (bool active) = 0;

    
    /***************************************************************
     * Locking/Unlocking access to neighbour store
     *
     * Note: these need only be implemented in multi-process
     * implementations
     **************************************************************/


    /**
     * @brief Lock access to neighbour store
     */
    virtual void lock_neighbour_table () {};


    /**
     * @brief Unlock access to neighbour store
     */
    virtual void unlock_neighbour_table () {};


    /**
     * @brief Lock access to own safety data
     */
    virtual void lock_own_safety_data () {};
    

    /**
     * @brief Unlock access to own safety data
     */
    virtual void unlock_own_safety_data () {};

    
    /***************************************************************
     * Management of ExtendedSafetyData for a neighbour
     **************************************************************/

    /**
     * @brief Set the ExtendedSafetyData for the given neighbour
     *
     * Note: this operation does not perform locking / unlocking.
     */
    virtual void insert_esd_entry (const ExtendedSafetyDataT& new_esd) = 0;


    /**
     * @brief Get a reference to the ExtendedSafetyDataT for the given
     *        neighbour.
     *
     * @param nodeId: node identifier
     *
     * Throws if no such entry exists. Note: this operation does not
     * perform locking / unlocking.
     *
     * Note: this operation does not perform locking / unlocking.
     */
    virtual ExtendedSafetyDataT& get_esd_entry_ref (const NodeIdentifierT nodeId) const = 0;


    /**
     * @brief Checks whether an entry for the given nodeId exists
     *
     * Note: this operation does not perform locking / unlocking.
     */
    virtual bool does_esd_entry_exist (const NodeIdentifierT nodeId) const = 0;
    

    virtual void remove_esd_entry (const NodeIdentifierT nodeId) = 0;

    
    /***************************************************************
     * Management of my own (Extended)SafetyData and related data
     **************************************************************/

    virtual void set_own_safety_data (const SafetyDataT& own_sd) = 0;
    virtual void set_own_sequence_number (uint32_t newseqno) = 0;
    virtual SafetyDataT& get_own_safety_data () = 0;
    virtual TimeStampT   get_own_safety_data_timestamp ()  const = 0;
    virtual uint32_t     get_own_sequence_number () const = 0;

    virtual std::list<NodeIdentifierT> find_nodes_to_scrub (TimeStampT current_time, uint16_t timeoutMS) const = 0;
    
  };

  
};  // namespace dcp::srp
