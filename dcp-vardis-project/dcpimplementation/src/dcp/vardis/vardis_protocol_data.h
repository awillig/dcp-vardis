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

#include <functional>
#include <map>
#include <queue>
#include <dcp/common/area.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/vardis/vardis_configuration.h>
#include <dcp/vardis/vardis_protocol_statistics.h>
#include <dcp/vardis/vardis_rtdb_entry.h>
#include <dcp/vardis/vardis_service_primitives.h>
#include <dcp/vardis/vardis_transmissible_types.h>



/**
 * @brief This module defines a record with the pure protocol data of
 *        Vardis (real-time database, queues), and implements all
 *        operations manipulating the protocol data.
 *
 * This module deliberately does *not* concern itself with issues like
 * mutual exclusion, etc., which are to be handled outside as
 * necessary.
 *
 * Any using class / code must set the 'ownNodeIdentifier' member
 * itself.
 */


namespace dcp::vardis {


  /**
   * @brief This class contains all the core Vardis protocol data
   */
  class VardisProtocolData {
  public:
    
    VardisProtocolData () = delete;
    VardisProtocolData (const VardisConfigurationBlock& cfg,
			const NodeIdentifierT nodeid)
    : ownNodeIdentifier (nodeid)
    , vardis_conf (cfg)
    {};

    
    /**
     * @brief Holds the own node identifier
     */
    NodeIdentifierT  ownNodeIdentifier;

    
    /**
     * @brief Vardis protocol configuration data
     */
    VardisConfigurationBlock vardis_conf;

    
    /**
     * @brief Holds the actual real-time variable database.
     */
    std::map<VarIdT, DBEntry>  theVariableDatabase;


    /**
     * @brief The Vardis queues
     */
    std::deque<VarIdT>    createQ;
    std::deque<VarIdT>    deleteQ;
    std::deque<VarIdT>    updateQ;
    std::deque<VarIdT>    summaryQ;
    std::deque<VarIdT>    reqUpdQ;
    std::deque<VarIdT>    reqCreateQ;

    
    /**
     * @brief Indicates whether Vardis is currently active or not.
     *
     * Vardis needs to be in active mode to process or generate
     * payloads, and to process RTDB service requests from clients.
     */
    bool vardis_isActive = true;


    /**
     * @brief Statistics for selected Vardis operations
     */
    VardisProtocolStatistics vardis_stats;
    
    // ====================================================================================
    // ====================================================================================

    
  protected:
    
    /**
     * @brief Calculates the size in bytes that a VarCreate
     *        instruction for the given varId would currently need
     *
     * @param varId: the varId for which to calculate instruction size
     */
    inline unsigned int instructionSizeVarCreate(VarIdT varId) const
    {
      const DBEntry& theEntry = theVariableDatabase.at(varId);
      return    theEntry.spec.total_size()
	      + VarUpdateT::fixed_size()
	      + theEntry.value.length;
    };


    /**
     * @brief Calculates the size in bytes that a VarSummary
     *        instruction for the given varId would currently need
     */
    inline unsigned int instructionSizeVarSummary(VarIdT) const
    {
      return VarSummT::fixed_size();
    };
    

    /**
     * @brief Calculates the size in bytes that a VarUpdate
     *        instruction for the given varId would currently need
     *
     * @param varId: the varId for which to calculate instruction size
     */
    inline unsigned int instructionSizeVarUpdate(VarIdT varId) const
    {
      const DBEntry& theEntry = theVariableDatabase.at(varId);
      return VarUpdateT::fixed_size() + theEntry.value.length;
    };
    

    /**
     * @brief Calculates the size in bytes that a VarDelete
     *        instruction for the given varId would currently need
     */
    inline unsigned int instructionSizeVarDelete(VarIdT) const
    {
      return VarDeleteT::fixed_size();
    };
  

    /**
     * @brief Calculates the size in bytes that a VarReqCreate
     *        instruction for the given varId would currently need
     */
    inline unsigned int instructionSizeReqCreate(VarIdT) const
    {
      return VarReqCreateT::fixed_size();
    };
    

    /**
     * @brief Calculates the size in bytes that a VarReqUpdate
     *        instruction for the given varId would currently need
     */
    inline unsigned int instructionSizeReqUpdate(VarIdT) const
    {
      return VarReqUpdateT::fixed_size();
    };
  

    // ====================================================================================
    // ====================================================================================


    /**
     * @brief This internal method calculates how many information
     *        instruction records referenced in the given queue and of
     *        the given type (cf 'instructionSizeFunction' parameter)
     *        fit into the number of bytes still available in the
     *        VarDis payload
     *
     * @param queue: The queue for which to construct an instruction container
     * @param area: assembly area to serialize into
     * @param instructionSizeFunction: the function to apply to each
     *        varId in the queue to query its serialized size
     * @return The number of instruction records can still fit into
     *         the remaining payload
     */
    unsigned int numberFittingRecords(
				      const std::deque<VarIdT>& queue,
				      AssemblyArea& area,
				      std::function<unsigned int (VarIdT)> instructionSizeFunction
				      );

  public:

    
    /**
     * @brief This serializes an instruction container for
     *        VarCreateT's, it generates an ICHeader and a as many
     *        VarCreateT records as possible / available.
     *
     * @param area: the assembly area to serialize into
     * @param containers_added: this variable will be incremented when
     *        an instruction container for VarCreates is added
     */
    void makeICTypeCreateVariables (AssemblyArea& area, unsigned int& containers_added);


    /**
     * @brief This serializes an instruction container for
     *        VarSummT's, it generates an ICHeader and a as many
     *        VarSummT records as possible / available.
     *
     * @param area: the assembly area to serialize into
     * @param containers_added: this variable will be incremented when
     *        an instruction container for VarSumms is added
     */
    void makeICTypeSummaries (AssemblyArea& area, unsigned int& containers_added);


    /**
     * @brief This serializes an instruction container for
     *        VarUopdateT's, it generates an ICHeader and a as many
     *        VarUpdateT records as possible / available.
     *
     * @param area: the assembly area to serialize into
     * @param containers_added: this variable will be incremented when
     *        an instruction container for VarUpdates is added
     */
    void makeICTypeUpdates (AssemblyArea& area, unsigned int& containers_added);


    /**
     * @brief This serializes an instruction container for
     *        VarDeleteT's, it generates an ICHeader and a as many
     *        VarDeleteT records as possible / available.
     *
     * @param area: the assembly area to serialize into
     * @param containers_added: this variable will be incremented when
     *        an instruction container for VarDeletes is added
     */
    void makeICTypeDeleteVariables (AssemblyArea& area, unsigned int& containers_added);


    /**
     * @brief This serializes an instruction container for
     *        VarReqUpdateT's, it generates an ICHeader and a as many
     *        VarReqUpdateT records as possible / available.
     *
     * @param area: the assembly area to serialize into
     * @param containers_added: this variable will be incremented when
     *        an instruction container for VarReqUpdates is added
     */
    void makeICTypeRequestVarUpdates (AssemblyArea& area, unsigned int& containers_added);


    /**
     * @brief This serializes an instruction container for
     *        VarReqCreateT's, it generates an ICHeader and a as many
     *        VarReqCreateT records as possible / available.
     *
     * @param area: the assembly area to serialize into
     * @param containers_added: this variable will be incremented when
     *        an instruction container for VarReqCreate's is added
     */
    void makeICTypeRequestVarCreates (AssemblyArea& area, unsigned int& containers_added);


    // ====================================================================================
    // ====================================================================================


    /**
     * @brief Processing the known types of instruction records,
     *        updating the RTDB where needed
     */
    void process_var_create  (const VarCreateT& create);
    void process_var_delete  (const VarDeleteT& del);
    void process_var_update  (const VarUpdateT& update);
    void process_var_summary (const VarSummT& summ);
    void process_var_requpdate (const VarReqUpdateT& requpd);
    void process_var_reqcreate (const VarReqCreateT& reqcreate);
    
    // ====================================================================================
    // ====================================================================================


    /**
     * @brief Processing the known types of RTDB service requests,
     *        updating the RTDB where needed
     */
    RTDB_Create_Confirm handle_rtdb_create_request (const RTDB_Create_Request& createReq);
    RTDB_Update_Confirm handle_rtdb_update_request (const RTDB_Update_Request& updateReq);
    RTDB_Delete_Confirm handle_rtdb_delete_request (const RTDB_Delete_Request& deleteReq);
    RTDB_Read_Confirm handle_rtdb_read_request (const RTDB_Read_Request& readReq);

        
    // ====================================================================================
    // ====================================================================================

    /**
     * @brief Checks if variable with given varId exists in the database
     */
    inline bool variableExists (VarIdT varId)
    {
      return (theVariableDatabase.find(varId) != theVariableDatabase.end());
    };

    
    /**
     * @brief Checks for an existing variable whether its producer is
     *        myself. Throws if variable does not exist.
     */
    inline bool producerIsMe (VarIdT varId)
    {
      DBEntry& theEntry = theVariableDatabase.at(varId);
      return theEntry.spec.prodId == ownNodeIdentifier;
    };


    /**
     * @brief Checks whether an entry for the given varId is in the given queue
     */
    inline bool isVarIdInQueue(const std::deque<VarIdT>& q, VarIdT varId)
    {
      return (std::find(q.begin(), q.end(), varId) != q.end());
    };

  protected:

    /**
     * @brief Removes all entries for given varId from the given queue
     */
    inline void removeVarIdFromQueue(std::deque<VarIdT>& q, VarIdT varId)
    {
      auto rems = std::remove(q.begin(), q.end(), varId);
      q.erase(rems, q.end());
    };


    /**
     * @brief Removes all varId's from the given queue for which
     *        either no entry exists in the RTDB or the entry is to be
     *        deleted
     */
    inline void dropNonexistingDeleted(std::deque<VarIdT>& q)
    {
      auto rems = std::remove_if(q.begin(),
				 q.end(),
				 [&](VarIdT varId){ return (    (theVariableDatabase.find(varId) == theVariableDatabase.end())
								|| (theVariableDatabase.at(varId).toBeDeleted));}
				 );
      q.erase(rems, q.end());
    };

    /**
     * @brief Removes all varId's from the given queue for which no
     *        entry exists in the RTDB
     */
    inline void dropNonexisting(std::deque<VarIdT>& q)
    {
      auto rems = std::remove_if(q.begin(),
				 q.end(),
				 [&](VarIdT varId){ return (theVariableDatabase.find(varId) == theVariableDatabase.end());}
				 );
      q.erase(rems, q.end());
    }


    /**
     * @brief Removes all varId's from the given queue for which an
     *        entry exists in the RTDB and this entry is to be deleted
     */
    inline void dropDeleted(std::deque<VarIdT>& q)
    {
      auto rems = std::remove_if(q.begin(),
				 q.end(),
				 [&](VarIdT varId){ return (    (theVariableDatabase.find(varId) != theVariableDatabase.end())
								&& (theVariableDatabase.at(varId).toBeDeleted));}
				 );
      q.erase(rems, q.end());
    };
  };

};  // namespace dcp::vardis

