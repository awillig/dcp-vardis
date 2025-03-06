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

#include <atomic>
#include <cstdint>
#include <functional>
#include <list>
#include <type_traits>
#include <dcp/common/array_avl_tree.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/srp/srp_store_interface.h>

/**
 * @brief This module implements the SRP store abstraction in a
 *        fixed-size memory region (allocated outside this module).
 *
 * The fixed-size memory region will include an array-based AVL tree
 * for the neighbour table, the own safety data for transmission and
 * relevant flags for managing the own safety data.
 *
 * None of the operations implemented here perform any locking /
 * unlocking of their own, that is left to calling code.
 */



namespace dcp::srp {


  /**
   * @brief This class represents all the 'global' data (i.e. not
   *        related to any specific neighour) stored in the SRP store.
   *
   * Includes the own safety data for transmission (and related
   * flags), next sequence number to use, own node identifier, and the
   * srp_isActive flag.
   */
  class GlobalStateBase {
  public:
    SafetyDataT            own_sd;                             /*!< Own safety data for transmission */
    TimeStampT             last_own_sd_write;                  /*!< Timestamp of last write to own safety data (transmission is suppressed if more time than keepaliveTimeoutMS time has passed */
    bool                   own_sd_written;                     /*!< Indicates whether valid own safety data has been written into own_sd field */
    uint32_t               next_seqno;                         /*!< Sequence number to use for next outgoing ExtendedSafetyDataT record */
    NodeIdentifierT        ownNodeIdentifier;                  /*!< Own node identifier */
    double                 gapSizeEstimatorEWMAAlphaValue;     /*!< Alpha value to be used for EWMA estimator of average sequence number gap size for a neighbour */
    std::atomic<bool>      srp_isActive;                       /*!< Flag indicating whether SRP demon is active (generating and processing SRP payloads) or not */
  };

  
  /**
   * @brief Concept verifying that given class is derived from GlobalStateBase
   */
  template <typename T>
  concept GlobalStateT = std::is_base_of<GlobalStateBase, T>::value;



  /**
   * @brief This is the main fixed-memory SRP store template class
   *
   * @tparam GlobalState: type to be used for the global state (has to
   *         be derived from type GlobalStateBase)
   * @tparam maxNeighbours: maximum size of neighbour table
   *
   * The data held by this structure includes:
   *   - A 'global data' structure of type GlobalStateBase or derived
   *   - An array-based AVL tree holding meta-data for each node in the
   *     neighbour table
   *   - An array of ExtendedSafetyDataT entries for neighbours
   *   - A free list indicating which array entry (of the ExtendedSafetyDataT
   *     array) are still available
   */
  template <GlobalStateT GlobalState, uint64_t maxNeighbours>
  class FixedMemSRPStoreBase : public SRPStoreI {
  public:

    /**
     * @brief Returns the number of allowed neighbours
     */
    static constexpr uint64_t get_max_neighbours () { return maxNeighbours; };


  protected:


    /**
     * @brief This represents the data we store for one neighbour in
     *        the AVL tree
     */
    class NeighbourState {
    public:
      NodeIdentifierT  nodeId     = nullNodeIdentifier;  /*!< node identifier of neighbour */
      uint64_t         esd_offs   = 0;                   /*!< Byte offset (relative to start of neighbour_ESD memory block) for storing ExtendedSafetyDataT value */
      uint32_t         last_seqno         = 0;           /*!< Last sequence number received from this neighbour */
      double           avg_seqno_gap_size = 0;           /*!< Estimated average sequence number gap size for this neighbour (EWMA estimator) */
      TimeStampT       last_esd_received;                /*!< Timestamp at which last ExtendedSafetyDataT record from this neighbour was received */
    };


    /**
     * @brief Represents one entry in the list of free ExtendedSafetyDataT buffers
     */
    typedef struct FreeListEntry {
      uint64_t  esd_offs = 0;   /*!< Offset for ExtendedSafetyDataT value */
    } FreeListEntry;


    /**
     * @brief This class specifies the actual structure that is stored
     *        in the given memory block.
     */
    class FixedMemContents {
    public:
      GlobalState   global_state;                                                      /*!< Global state (own safety data etc) */
      ExtendedSafetyDataT    neighbour_ESD [maxNeighbours];                            /*!< Buffers for ExtendedSafetyDataT records of neighbours */
      RingBufferBase<FreeListEntry, get_max_neighbours()+1>  freeList;                 /*!< Ring buffer of free ExtendedSafetyDataT buffers */
      ArrayAVLTree<NodeIdentifierT, NeighbourState, maxNeighbours> neighbour_table;    /*!< AVL tree containing neighbour table (with per-neighbour metadata) */
      
      /**
       * @brief Constructor, initializes free list
       */
      FixedMemContents () : freeList ("FixedMemContents::freeList", get_max_neighbours()) {};
    };


    /**
     * @brief Holds the start address in memory of the FixedMemContents structure
     */
    byte*               memory_start_address = nullptr;


    /**
     * @brief A typed pointer to the memory_start_address
     */
    FixedMemContents*   pContents            = nullptr;


  public:

    // ---------------------------------------
    
    FixedMemSRPStoreBase () {};


    // ---------------------------------------


    /**
     * @brief Initializing of fixed-memory SRP store
     *
     * @param mem_start_addr: Points to address in memory at which
     *        FixedMemContents will start. This memory region must
     *        have been previously allocated by caller and must be of
     *        sufficient size to hold the FixedMemContents
     *        structure. Callers can check size beforehand by calling
     *        get_fixedmem_contents_size() method.
     * @param own_node_id: value of own node identifier to be stored
     *        in the global data section
     * @param gap_ewma_estimator_alpha: alpha value to be used for
     *        the EWMA estimator for the average sequence number gap
     *        size of a neighbour. This is not checked for validity.
     *
     * This mainly sets the memory_start_address and pContents
     * pointers, adds all ExtendedSafetyDataT buffers to the free list
     * and initializes the global state.
     */
    void initialize_srp_store (byte* mem_start_addr,
			       NodeIdentifierT own_node_id,
			       double gap_ewma_estimator_alpha)
      
       
    {
      memory_start_address             = mem_start_addr;
      
      if (memory_start_address == nullptr)
	throw SRPStoreException ("initialize_srp_store: Memory start address is null");

      pContents = new (memory_start_address) FixedMemContents;

      for (uint64_t i = 0; i < get_max_neighbours(); i++)
	{
	  FreeListEntry entry;
	  entry.esd_offs = i * sizeof(ExtendedSafetyDataT);
	  pContents->freeList.push (entry);
	}

      pContents->global_state.srp_isActive        = true;
      pContents->global_state.ownNodeIdentifier   = own_node_id;
      pContents->global_state.gapSizeEstimatorEWMAAlphaValue   = gap_ewma_estimator_alpha;
      pContents->global_state.last_own_sd_write   = TimeStampT::get_current_system_time();
      pContents->global_state.next_seqno          = 0;
      pContents->global_state.own_sd_written      = false;

    };

    // ---------------------------------------

    /**
     * @brief Returns the actual size needed for the FixedMemContents
     *        structure in the given memory block.
     */
    static constexpr size_t get_fixedmem_contents_size () { return sizeof(FixedMemContents); };


    // ---------------------------------------


    /**
     * @brief Returns current value of srp_isActive flag
     */
    virtual bool     get_srp_isactive () const { return pContents->global_state.srp_isActive; };

    
    /**
     * @brief Sets current value of srp_isActive flag
     *
     * @param active: new value of flag
     */
    virtual void     set_srp_isactive (bool active) { pContents->global_state.srp_isActive = active; };


    /**
     * @brief Returns value of ownNodeIdentifier parameter
     */
    virtual NodeIdentifierT get_own_node_identifier () const { return pContents->global_state.ownNodeIdentifier; };

    
    // ---------------------------------------


    /**
     * @brief Inserts a given ExtendedSafetyDataT record into the neighbour table
     *
     * @param new_esd: new ExtendedSafetyDataT record
     *
     * If a record for the received nodeId already exists, the
     * ExtendedSafetyDataT record for this entry is merely updated
     * (storing its SafetyDataT, updating EWMA estimator etc). If no
     * such record exists yet, a new one is added to the neighbour
     * table and initialized.
     *
     * This is particularly useful when processing received SRP
     * payloads. Note that the own safety data is not stored in the
     * table.
     */
    virtual void insert_esd_entry (const ExtendedSafetyDataT& new_esd)
    {
      FixedMemContents&  FMC = *pContents;
      NodeIdentifierT nodeId = new_esd.nodeId;

      if (nodeId == get_own_node_identifier())
	return;
      
      if (FMC.neighbour_table.is_member(nodeId))
	{
	  // handling existing node id
	  NeighbourState& nstate = FMC.neighbour_table.lookup_data_ref (nodeId);
	  double alpha           = FMC.global_state.gapSizeEstimatorEWMAAlphaValue;
	  
	  byte* effective_address = (byte*) FMC.neighbour_ESD + nstate.esd_offs;
	  std::memcpy (effective_address, (byte*) &new_esd, sizeof(ExtendedSafetyDataT));
	  nstate.last_esd_received  = TimeStampT::get_current_system_time ();
	  
	  double new_gap_size = (double) (new_esd.seqno - nstate.last_seqno);
	  nstate.last_seqno = new_esd.seqno;
	  nstate.avg_seqno_gap_size = (1 - alpha) * new_gap_size + alpha * nstate.avg_seqno_gap_size;
	  
	  return;
	}


      // handling new nodeId
      FreeListEntry fl_entry = FMC.freeList.pop ();
      NeighbourState new_nstate;
      new_nstate.nodeId              = nodeId;
      new_nstate.esd_offs            = fl_entry.esd_offs;
      new_nstate.last_seqno          = new_esd.seqno;
      new_nstate.avg_seqno_gap_size  = 0;
      new_nstate.last_esd_received   = TimeStampT::get_current_system_time();

      byte* effective_address = (byte*) FMC.neighbour_ESD + new_nstate.esd_offs;
      std::memcpy (effective_address, (byte*) &new_esd, sizeof(ExtendedSafetyDataT));
      
      FMC.neighbour_table.insert (nodeId, new_nstate);      
    };

    // ---------------------------------------


    /**
     * @brief Remove neighbour table entry for given node identifier
     *        (useful for scrubbing)
     *
     * @param nodeId: node identifier of node to be removed
     *
     * The ExtendedSafetyDataT buffer of the node is returned to the
     * free list, and the node is removed from the neighbour table.
     */
    virtual void remove_esd_entry (const NodeIdentifierT nodeId)
    {
      FixedMemContents&  FMC = *pContents;

      if (not FMC.neighbour_table.is_member (nodeId))
	return;

      NeighbourState nstat = FMC.neighbour_table.lookup_data_ref (nodeId);
      FreeListEntry fl_entry;
      fl_entry.esd_offs = nstat.esd_offs;
      FMC.freeList.push (fl_entry);

      FMC.neighbour_table.remove (nodeId);
    };

    // ---------------------------------------


    /**
     * @brief Returns reference to the ExtendedSafetyDataT record of
     *        the given node identifier.
     *
     * @param nodeId: node identifier
     *
     * This does not check whether the node identifier refers to a
     * current entry in the neighbour table or not.
     */
    virtual ExtendedSafetyDataT& get_esd_entry_ref (const NodeIdentifierT nodeId) const
    {
      FixedMemContents&  FMC = *pContents;
      NeighbourState& nstate = FMC.neighbour_table.lookup_data_ref (nodeId);
      byte* effective_address = (byte*) FMC.neighbour_ESD + nstate.esd_offs;
      return * ((ExtendedSafetyDataT*) effective_address);
    };
    
    // ---------------------------------------


    /**
     * @brief Checks whether neighbour table holds an entry for given
     *        node identifier
     *
     * @param nodeId: node identifier to look up
     */
    virtual bool does_esd_entry_exist (const NodeIdentifierT nodeId) const
    {
      FixedMemContents&  FMC = *pContents;
      return FMC.neighbour_table.is_member (nodeId);
    };

    // ---------------------------------------


    /**
     * @brief Returns the timestamp of the last write to the own
     *        safety data field
     */
    virtual TimeStampT   get_own_safety_data_timestamp ()  const
    {
      FixedMemContents&  FMC = *pContents;
      return FMC.global_state.last_own_sd_write;
    };

    // ---------------------------------------


    /**
     * @brief Returns a reference to the own safety data field
     */
    virtual SafetyDataT& get_own_safety_data ()
    {
      FixedMemContents&  FMC = *pContents;
      return FMC.global_state.own_sd;
    };

    // ---------------------------------------


    /**
     * @brief Returns the current sequence number value
     */
    virtual uint32_t get_own_sequence_number () const
    {
      FixedMemContents&  FMC = *pContents;
      return FMC.global_state.next_seqno;
    };

    // ---------------------------------------


    /**
     * @brief Sets the own sequence number (sequence number of next
     *        outgoing payload) to the given value
     *
     * @param newseqno: new sequence number value
     */
    virtual void set_own_sequence_number (uint32_t newseqno)
    {
      FixedMemContents&  FMC = *pContents;
      FMC.global_state.next_seqno = newseqno;
    };

    // ---------------------------------------


    /**
     * @brief Sets own safety data field to given safety data record
     *
     * @param own_sd: new SafetyDataT record
     *
     * The new record is copied into the global data.
     */
    virtual void set_own_safety_data (const SafetyDataT& own_sd)
    {
      FixedMemContents&  FMC = *pContents;
      FMC.global_state.own_sd            = own_sd;
      FMC.global_state.last_own_sd_write = TimeStampT::get_current_system_time();
      FMC.global_state.own_sd_written    = true;
    };
    
    // ---------------------------------------


    /**
     * @brief Returns the own safety data written flag
     *
     * This flag indicates that valid own safety data is available for
     * transmission. No transmission should take place if no valid
     * data is available.
     */
    virtual bool get_own_safety_data_written_flag () const
    {
      FixedMemContents&  FMC = *pContents;
      return FMC.global_state.own_sd_written;
    };

    // ---------------------------------------


    /**
     * @brief Sets the own safety data written flag
     *
     * @param new_flag: new value of the flag
     *
     * This flag indicates that valid own safety data is available for
     * transmission. No transmission should take place if no valid
     * data is available.
     */
    virtual void set_own_safety_data_written_flag (bool new_flag)
    {
      FixedMemContents&  FMC = *pContents;
      FMC.global_state.own_sd_written = new_flag;
    };

    // ---------------------------------------


    /**
     * @brief Returns a list of all of the nodes to scrub (i.e. nodes
     *        for which their last reception timestamp is older than
     *        the given timeout value)
     *
     * @param current_time: current time value
     * @param timeoutMS: nodes for which the last reception of an
     *        ExtendedSafetyDataT record is older than this timeout
     *        will be included in the result list
     *
     * @return List of all node identifiers whose last reception time
     *         is older than the given timeout
     */
    virtual std::list<NodeIdentifierT> find_nodes_to_scrub (TimeStampT current_time, uint16_t timeoutMS) const
    {
      FixedMemContents&  FMC = *pContents;
      
      std::function<bool (const NeighbourState&)> predicate =
	[current_time, timeoutMS] (const NeighbourState& nstate)
	{
	  return current_time.milliseconds_passed_since (nstate.last_esd_received) >= timeoutMS;
	};
      
      std::list<NodeIdentifierT> result_list;
      FMC.neighbour_table.find_matching_keys (predicate, result_list);
      return result_list;
    };
    
    // ---------------------------------------
    
    
  };


  
};  // namespace dcp::srp
