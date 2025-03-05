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


namespace dcp::srp {

  class GlobalStateBase {
  public:
    SafetyDataT            own_sd;
    TimeStampT             last_own_sd_write;
    uint32_t               next_seqno;
    NodeIdentifierT        ownNodeIdentifier;
    double                 gapSizeEstimatorEWMAAlphaValue;
    std::atomic<bool>      srp_isActive;
  };

  
  
  template <typename T>
  concept GlobalStateT = std::is_base_of<GlobalStateBase, T>::value;

  template <GlobalStateT GlobalState, uint64_t maxNeighbours>
  class FixedMemSRPStoreBase : public SRPStoreI {
  public:

    /**
     * @brief Returns the number of allowed neighbours
     */
    static constexpr uint64_t get_max_neighbours () { return maxNeighbours; };


  protected:

    class NeighbourState {
    public:
      NodeIdentifierT  nodeId     = nullNodeIdentifier;
      uint64_t         esd_offs   = 0;   /*!< Offset (relative to start of memory block) for storing ExtendedSafetyDataT value */
      
      uint32_t         last_seqno         = 0;
      double           avg_seqno_gap_size = 0;
      
      TimeStampT       last_esd_received;
    };

    
    typedef struct FreeListEntry {
      uint64_t  esd_offs = 0;   /*!< Offset for ExtendedSafetyDataT value */
    } FreeListEntry;


    /**
     * @brief This class specifies the actual structure that is stored
     *        in the given memory block.
     */
    class FixedMemContents {
    public:
      GlobalState   global_state;
      
      ExtendedSafetyDataT    neighbour_ESD [maxNeighbours];
      RingBufferBase<FreeListEntry, get_max_neighbours()+1>  freeList;

      ArrayAVLTree<NodeIdentifierT, NeighbourState, maxNeighbours> neighbour_table;
      
      /**
       * @brief Constructor, initializes free list
       */
      FixedMemContents () : freeList ("FixedMemContents::freeList", get_max_neighbours()) {};
    };

    byte*               memory_start_address = nullptr;
    FixedMemContents*   pContents            = nullptr;


  public:

    // ---------------------------------------
    
    FixedMemSRPStoreBase () {};


    // ---------------------------------------


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
     */
    virtual void     set_srp_isactive (bool active) { pContents->global_state.srp_isActive = active; };


    /**
     * @brief Returns value of ownNodeIdentifier parameter
     */
    virtual NodeIdentifierT get_own_node_identifier () const { return pContents->global_state.ownNodeIdentifier; };

    
    // ---------------------------------------

    virtual void insert_esd_entry (const ExtendedSafetyDataT& new_esd)
    {
      FixedMemContents&  FMC = *pContents;
      NodeIdentifierT nodeId = new_esd.nodeId;

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

    virtual ExtendedSafetyDataT& get_esd_entry_ref (const NodeIdentifierT nodeId) const
    {
      FixedMemContents&  FMC = *pContents;
      NeighbourState& nstate = FMC.neighbour_table.lookup_data_ref (nodeId);
      byte* effective_address = (byte*) FMC.neighbour_ESD + nstate.esd_offs;
      return * ((ExtendedSafetyDataT*) effective_address);
    };
    
    // ---------------------------------------

    virtual bool does_esd_entry_exist (const NodeIdentifierT nodeId) const
    {
      FixedMemContents&  FMC = *pContents;
      return FMC.neighbour_table.is_member (nodeId);
    };

    // ---------------------------------------

    virtual TimeStampT   get_own_safety_data_timestamp ()  const
    {
      FixedMemContents&  FMC = *pContents;
      return FMC.global_state.last_own_sd_write;
    };

    // ---------------------------------------

    virtual SafetyDataT& get_own_safety_data ()
    {
      FixedMemContents&  FMC = *pContents;
      return FMC.global_state.own_sd;
    };

    // ---------------------------------------

    virtual uint32_t get_own_sequence_number () const
    {
      FixedMemContents&  FMC = *pContents;
      return FMC.global_state.next_seqno;
    };

    // ---------------------------------------

    virtual void set_own_sequence_number (uint32_t newseqno)
    {
      FixedMemContents&  FMC = *pContents;
      FMC.global_state.next_seqno = newseqno;
    };

    // ---------------------------------------

    virtual void set_own_safety_data (const SafetyDataT& own_sd)
    {
      FixedMemContents&  FMC = *pContents;
      FMC.global_state.own_sd            = own_sd;
      FMC.global_state.last_own_sd_write = TimeStampT::get_current_system_time();
    };
    
    // ---------------------------------------

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
