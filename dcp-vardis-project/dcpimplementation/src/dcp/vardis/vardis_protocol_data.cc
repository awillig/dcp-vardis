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


#include <dcp/vardis/vardis_logging.h>
#include <dcp/vardis/vardis_protocol_data.h>


namespace dcp::vardis {


  // -----------------------------------------------------------------
  
  /* The following 'addXX' functions perform the serialization of the
   * known instruction container entries, assuming a 'packed' representation.
   */
  

  
  void VardisProtocolData::addVarCreate (VarIdT, const DBEntry& theEntry, AssemblyArea& area) const
  {
    VarCreateT create;
    create.spec.varId     =  theEntry.varId;
    create.spec.prodId    =  theEntry.prodId;
    create.spec.repCnt    =  theEntry.repCnt;
    create.spec.descr     =  vardis_store.read_description (theEntry.varId);
    create.update.varId   =  theEntry.varId;
    create.update.seqno   =  theEntry.seqno;
    create.update.value   =  vardis_store.read_value (theEntry.varId);
    create.serialize (area);
  }
  
  
  
  // -----------------------------------------------------------------
  
  
  void VardisProtocolData::addVarSummary (VarIdT varId, const DBEntry& theEntry, AssemblyArea& area) const
  {
    VarSummT summ;
    summ.varId  = varId;
    summ.seqno  = theEntry.seqno;
    summ.serialize (area);
  }
  
  // -----------------------------------------------------------------
  
  void VardisProtocolData::addVarUpdate (VarIdT, const DBEntry& theEntry, AssemblyArea& area) const
  {
    VarUpdateT update;
    update.varId   =  theEntry.varId;
    update.seqno   =  theEntry.seqno;
    update.value   =  vardis_store.read_value (theEntry.varId);
    update.serialize (area);
  }
  
  
  
  // -----------------------------------------------------------------
  
  void VardisProtocolData::addVarDelete (VarIdT varId, AssemblyArea& area) const
  {
    VarDeleteT del;
    del.varId = varId;    
    del.serialize (area);
  }
  
  
  
  // -----------------------------------------------------------------
  
  void VardisProtocolData::addVarReqCreate (VarIdT varId, AssemblyArea& area) const
  {
    VarReqCreateT cr;
    cr.varId = varId;
    cr.serialize (area);
  }
  
  // -----------------------------------------------------------------
  
  void VardisProtocolData::addVarReqUpdate (VarIdT varId, const DBEntry& theEntry, AssemblyArea& area) const
  {
    VarReqUpdateT upd;
    upd.updSpec.varId = varId;
    upd.updSpec.seqno = theEntry.seqno;
    upd.serialize (area);
  }
  
  // -----------------------------------------------------------------


  
  /**
   * This function calculates how many information instruction records referenced
   * in the given queue and of the given type (cf 'instructionSizeFunction' parameter)
   * fit into the number of bytes still available in the VarDis payload
   */
  unsigned int VardisProtocolData::numberFittingRecords(
							const std::deque<VarIdT>& queue,
							AssemblyArea& area,
							std::function<unsigned int (VarIdT)> instructionSizeFunction
							)
  {
    // first work out how many records we can add
    unsigned int   numberRecordsToAdd = 0;
    unsigned int   bytesToBeAdded = ICHeaderT::fixed_size();
    auto           it = queue.begin();
    while(    (it != queue.end())
	      && (bytesToBeAdded + instructionSizeFunction(*it) <= area.available())
	      && (numberRecordsToAdd < ICHeaderT::max_records()))
      {
        numberRecordsToAdd++;
        bytesToBeAdded += instructionSizeFunction(*it);
        it++;
      }
    
    return (std::min(numberRecordsToAdd, (unsigned int) ICHeaderT::max_records()));
  }


  // -----------------------------------------------------------------
  

  /**
   * This serializes an instruction container for VarCreateT's, it generates
   * an ICHeader and a as many VarCreateT records as possible / available.
   */
  void VardisProtocolData::makeICTypeCreateVariables (AssemblyArea& area, unsigned int& containers_added)
  {
    // check for empty createQ or insufficient size to add at least the first instruction record
    if (    createQ.empty()
         || (instructionSizeVarCreate(createQ.queue.front()) + ICHeaderT::fixed_size()) > area.available())
      {
        return;
      }
    
    // first work out how many records we will add
    std::function<unsigned int(VarIdT)> instSizeFn = [&] (VarIdT varId) { return instructionSizeVarCreate(varId); };
    auto numberRecordsToAdd = numberFittingRecords(createQ.queue, area, instSizeFn);
    
    if (numberRecordsToAdd <= 0)
      {
	throw VardisTransmitException ("makeICTypeCreateVariables: numberRecordsToAdd is zero");
      }
    
    // initialize and serialize ICHeader
    ICHeaderT   icHeader;
    icHeader.icType       = ICTYPE_CREATE_VARIABLES;
    icHeader.icNumRecords = numberRecordsToAdd;
    icHeader.serialize(area);
    
    // serialize the records
    for (unsigned int i=0; i<numberRecordsToAdd; i++)
      {
        VarIdT nextVarId = createQ.front();
        createQ.pop_front();
        DBEntry& nextVar = vardis_store.get_db_entry_ref(nextVarId);
	
	if (nextVar.countCreate.val <= 0)
	  {
	    throw VardisTransmitException ("makeICTypeCreateVariables: nextVar.countCreate is zero");
	  }
	
        nextVar.countCreate--;
	
        addVarCreate(nextVarId, nextVar, area);
	
        if (nextVar.countCreate.val > 0)
	  {
            createQ.insert(nextVarId);
	  }
      }
    containers_added += 1;
  }
  

    // -----------------------------------------------------------------
  
  /**
   * This serializes an instruction container for VarSummT's, it generates
   * an ICHeader and a as many VarSummT records as possible / available.
   */
  void VardisProtocolData::makeICTypeSummaries (AssemblyArea& area, unsigned int& containers_added)
  {    
    // check for empty summaryQ, insufficient size to add at least the first instruction record,
    // or whether summaries function is enabled
    if (    summaryQ.empty()
	|| (instructionSizeVarSummary(summaryQ.front()) + ICHeaderT::fixed_size() > area.available())
	|| (maxSummaries == 0))
      {
        return;
      }
    
    // first work out how many records we will add, cap at vardisMaxSummaries
    std::function<unsigned int(VarIdT)> instSizeFn = [&] (VarIdT varId) { return instructionSizeVarSummary(varId); };
    auto numberRecordsToAdd = numberFittingRecords(summaryQ.queue, area, instSizeFn);
    numberRecordsToAdd = std::min(numberRecordsToAdd, (unsigned int) maxSummaries);
    
    if (numberRecordsToAdd <= 0)
      {
	throw VardisTransmitException ("makeICTypeSummaries: numberRecordsToAdd is zero");
      }
    
    // initialize and serialize ICHeader
    ICHeaderT   icHeader;
    icHeader.icType       = ICTYPE_SUMMARIES;
    icHeader.icNumRecords = numberRecordsToAdd;
    icHeader.serialize(area);
    
    // serialize the records
    for (unsigned int i=0; i<numberRecordsToAdd; i++)
      {
        VarIdT nextVarId  = summaryQ.front();
	
        summaryQ.pop_front();
        summaryQ.insert(nextVarId);
        DBEntry&   theNextEntry  = vardis_store.get_db_entry_ref(nextVarId);
        addVarSummary(nextVarId, theNextEntry, area);
      }
    
    containers_added += 1;
  }
  


  // -----------------------------------------------------------------

  /**
   * This serializes an instruction container for VarUpdateT's, it generates
   * an ICHeader and a as many VarUpdateT records as possible / available.
   */
  void VardisProtocolData::makeICTypeUpdates (AssemblyArea& area, unsigned int& containers_added)
  {    
    // check for empty updateQ or insufficient size to add at least the first instruction record
    if (    updateQ.empty()
	 || (instructionSizeVarUpdate(updateQ.front()) + ICHeaderT::fixed_size() > area.available()))
      {
        return;
      }
    
    // first work out how many records we will add
    std::function<unsigned int(VarIdT)> instSizeFn = [&] (VarIdT varId) { return instructionSizeVarUpdate(varId); };
    auto numberRecordsToAdd = numberFittingRecords(updateQ.queue, area, instSizeFn);
    
    if (numberRecordsToAdd <= 0)
      {
	throw VardisTransmitException ("makeICTypeUpdates: numberRecordsToAdd is zero");
      }
    
    // initialize and serialize ICHeader
    ICHeaderT   icHeader;
    icHeader.icType       = ICTYPE_UPDATES;
    icHeader.icNumRecords = numberRecordsToAdd;
    icHeader.serialize(area);

    // serialize required records
    for (unsigned int i=0; i<numberRecordsToAdd; i++)
    {
        VarIdT nextVarId = updateQ.front();
        updateQ.pop_front();
        DBEntry& nextVar = vardis_store.get_db_entry_ref(nextVarId);

	if (nextVar.countUpdate.val <= 0)
	  {
	    throw VardisTransmitException ("makeICTypeUpdates: nextVar.countUpdate is zero");
	  }
	
        nextVar.countUpdate--;

        addVarUpdate(nextVarId, nextVar, area);

        if (nextVar.countUpdate.val > 0)
        {
            updateQ.insert(nextVarId);
        }
    }

    containers_added += 1;    
  }


  // -----------------------------------------------------------------
  
  /**
   * This serializes an instruction container for VarDeleteT's, it generates
   * an ICHeader and a as many VarDeleteT records as possible / available.
   */
  void VardisProtocolData::makeICTypeDeleteVariables (AssemblyArea& area, unsigned int& containers_added)
  {
    // check for empty deleteQ or insufficient size to add at least the first instruction record
    if (    deleteQ.empty()
	 || (instructionSizeVarDelete(deleteQ.front()) + ICHeaderT::fixed_size() > area.available()))
    {
        return;
    }

    // first work out how many records we will add
    std::function<unsigned int(VarIdT)> instSizeFn = [&] (VarIdT varId) { return instructionSizeVarDelete(varId); };
    auto numberRecordsToAdd = numberFittingRecords(deleteQ.queue, area, instSizeFn);

    if (numberRecordsToAdd <= 0)
      {
	throw VardisTransmitException ("makeICTypeDeleteVariables: numberRecordsToAdd is zero");
      }

    // initialize and serialize ICHeader
    ICHeaderT   icHeader;
    icHeader.icType       = ICTYPE_DELETE_VARIABLES;
    icHeader.icNumRecords = numberRecordsToAdd;
    icHeader.serialize(area);

    // serialize required records
    for (unsigned int i=0; i<numberRecordsToAdd; i++)
    {
        VarIdT nextVarId = deleteQ.front();
        deleteQ.pop_front();

	if (not variableExists(nextVarId))
	  {
	    throw VardisTransmitException ("makeICTypeDeleteVariables: variable does not exist");
	  }
	
        DBEntry& nextVar = vardis_store.get_db_entry_ref(nextVarId);

	if (nextVar.countDelete.val <= 0)
	  {
	    throw VardisTransmitException ("makeICTypeDeleteVariables: nextVar.countDelete is zero");
	  }

        nextVar.countDelete--;

        addVarDelete(nextVarId, area);

        if (nextVar.countDelete.val > 0)
        {
            deleteQ.insert(nextVarId);
        }
        else
        {
	  BOOST_LOG_SEV(log_tx, trivial::info) << "Deleting variable " << nextVarId;
	  vardis_store.deallocate_identifier (nextVarId);
	  active_variables.erase (nextVarId);
        }
    }

    containers_added += 1;    
  }


    // -----------------------------------------------------------------
  
  /**
   * This serializes an instruction container for VarReqUpdateT's, it
   * generates an ICHeader and a as many VarReqUpdateT records as
   * possible / available.
   */
  void VardisProtocolData::makeICTypeRequestVarUpdates (AssemblyArea& area, unsigned int& containers_added)
  {
    // check for empty reqUpdQ or insufficient size to add at least the first instruction record
    if (    reqUpdQ.empty()
	 || (instructionSizeReqUpdate(reqUpdQ.front()) + ICHeaderT::fixed_size() > area.available()))
    {
        return;
    }

    // first work out how many records we will add
    std::function<unsigned int(VarIdT)> instSizeFn = [&] (VarIdT varId) { return instructionSizeReqUpdate(varId); };
    auto numberRecordsToAdd = numberFittingRecords(reqUpdQ.queue, area, instSizeFn);

    if (numberRecordsToAdd <= 0)
      {
	throw VardisTransmitException ("makeICTypeRequestVarUpdates: numberRecordsToAdd is zero");
      }

    // initialize and serialize ICHeader
    ICHeaderT   icHeader;
    icHeader.icType       = ICTYPE_REQUEST_VARUPDATES;
    icHeader.icNumRecords = numberRecordsToAdd;
    icHeader.serialize(area);

    // serialize required records
    for (unsigned int i=0; i<numberRecordsToAdd; i++)
    {
        VarIdT nextVarId = reqUpdQ.front();
        reqUpdQ.pop_front();
        DBEntry& nextVar = vardis_store.get_db_entry_ref(nextVarId);

        addVarReqUpdate(nextVarId, nextVar, area);
    }

    containers_added += 1;
  }


    // -----------------------------------------------------------------
  
  /**
   * This serializes an instruction container for VarReqCreateT's, it
   * generates an ICHeader and a as many VarReqCreateT records as
   * possible / available.
   */
  void VardisProtocolData::makeICTypeRequestVarCreates (AssemblyArea& area, unsigned int& containers_added)
  {
    // check for empty reqCreateQ or insufficient size to add at least the first instruction record
    if (    reqCreateQ.empty()
	 || (instructionSizeReqCreate(reqCreateQ.front()) + ICHeaderT::fixed_size() > area.available()))
    {
        return;
    }

    // first work out how many records we will add
    std::function<unsigned int(VarIdT)> instSizeFn = [&] (VarIdT varId) { return instructionSizeReqCreate(varId); };
    auto numberRecordsToAdd = numberFittingRecords(reqCreateQ.queue, area, instSizeFn);

    if (numberRecordsToAdd <= 0)
      {
	throw VardisTransmitException ("makeICTypeRequestVarCreates: numberRecordsToAdd is zero");
      }

    // initialize and serialize ICHeader
    ICHeaderT   icHeader;
    icHeader.icType       = ICTYPE_REQUEST_VARCREATES;
    icHeader.icNumRecords = numberRecordsToAdd;
    icHeader.serialize(area);

    // serialize required records
    for (unsigned int i=0; i<numberRecordsToAdd; i++)
    {
        VarIdT nextVarId = reqCreateQ.front();
        reqCreateQ.pop_front();

        addVarReqCreate(nextVarId, area);
    }

    containers_added += 1;
  }
  

  /**
   * Processes a received VarCreate entry. If variable does not already exist,
   * it will be added to the local RTDB, including description and value, and
   * its metadata will be initialized.
   */
  void VardisProtocolData::process_var_create(const VarCreateT& create)
  {
    VarIdT               varId  = create.spec.varId;
    NodeIdentifierT      prodId = create.spec.prodId;

      
    if (    (not variableExists(varId))
         && (prodId != ownNodeIdentifier)
         && (create.spec.descr.length <= maxDescriptionLength)
	 && (create.spec.descr.length > 0)
         && (create.update.value.length <= maxValueLength)
         && (create.update.value.length > 0)
	 && (create.spec.repCnt <= maxRepetitions)
	 && (create.spec.repCnt > 0)
       )
      {
	BOOST_LOG_SEV(log_rx, trivial::info) << "process_var_create: adding new variable to database, varId = " << varId
					     << ", description = " << create.spec.descr;

        // create and initialize new DBEntry
        DBEntry newEntry;
	newEntry.varId        =  create.spec.varId;
	newEntry.prodId       =  create.spec.prodId;
	newEntry.repCnt       =  create.spec.repCnt;
        newEntry.seqno        =  create.update.seqno;
        newEntry.tStamp       =  TimeStampT::get_current_system_time();
        newEntry.countUpdate  =  0;
        newEntry.countCreate  =  create.spec.repCnt;
        newEntry.countDelete  =  0;
        newEntry.toBeDeleted  =  false;
	vardis_store.allocate_identifier (varId);
	vardis_store.set_db_entry (varId, newEntry);
	vardis_store.update_description (varId, create.spec.descr);
	vardis_store.update_value (varId, create.update.value);
	active_variables.insert (varId);

        // just to be safe, delete varId from all queues before inserting it
        // into the right ones
        createQ.remove (varId);
        deleteQ.remove (varId);
        updateQ.remove (varId);
        summaryQ.remove (varId);
        reqUpdQ.remove (varId);
        reqCreateQ.remove (varId);

        // add varId to relevant queues
        createQ.insert (varId);
        summaryQ.insert (varId);

	// maintain statistics
	auto vardis_stats = vardis_store.get_vardis_protocol_statistics_ref ();
	vardis_stats.count_process_var_create++;
    }
  }


  // ----------------------------------------------------
  
  /**
   * Processes a received VarDelete entry. If variable exists, its state will
   * be set to move into the to-be-deleted state, and it will be removed from
   * the relevant queues
   */
  void VardisProtocolData::process_var_delete (const VarDeleteT& del)
  {
    VarIdT   varId           = del.varId;
    
    if (variableExists(varId))
      {
        DBEntry&        theEntry = vardis_store.get_db_entry_ref(varId);

        if (    (not theEntry.toBeDeleted)
   	     && (not producerIsMe(varId)))
        {
	  BOOST_LOG_SEV(log_rx, trivial::info) << "process_var_delete: deleting variable from database, varId = " << varId;
	  
	  // update variable state
	  theEntry.toBeDeleted  = true;
	  theEntry.countUpdate  = 0;
	  theEntry.countCreate  = 0;
	  theEntry.countDelete  = theEntry.repCnt;
	  
	  // remove varId from relevant queues
	  updateQ.remove (varId);
	  createQ.remove (varId);
	  reqUpdQ.remove (varId);
	  reqCreateQ.remove (varId);
	  summaryQ.remove (varId);
	  deleteQ.remove (varId);

	  // add it to deleteQ
	  deleteQ.insert(varId);

	  // maintain statistics
	  vardis_store.get_vardis_protocol_statistics_ref().count_process_var_delete++;
        }
      }
  }


  // ----------------------------------------------------
  
  /**
   * Processes a received VarUpdate entry. If variable exists and certain
   * conditions are met, its value will be updated and the variable will
   * be added to the relevant queues.
   */
  void VardisProtocolData::process_var_update (const VarUpdateT& update)
  {
    VarIdT  varId               = update.varId;

    BOOST_LOG_SEV(log_rx, trivial::trace) << "process_var_update: got variable update, varId = " << varId;
    
    // check if variable exists -- if not, add it to queue to generate ReqVarCreate
    if (not variableExists(varId))
    {
      BOOST_LOG_SEV(log_rx, trivial::trace) << "process_var_update: got update for unknown variable, varId = " << varId << ". Stopping processing.";

      if (not reqCreateQ.contains (varId))
	reqCreateQ.insert(varId);
      
      return;
    }

    DBEntry& theEntry = vardis_store.get_db_entry_ref(varId);

    // perform some checks

    if (theEntry.toBeDeleted)
    {
        return;
    }

    if (producerIsMe(varId))
    {
        return;
    }

    if (    (update.value.length > maxValueLength)
	 || (update.value.length <= 0))
    {
        return;
    }

    if (theEntry.seqno == update.seqno)
    {
        return;
    }

    // If received update is older than what I have, schedule transmissions of
    // VarUpdate's for this variable to educate the sender
    if (more_recent_seqno(theEntry.seqno, update.seqno))
      {
        // I have a more recent sequence number
        if (not updateQ.contains (varId))
	  {
            updateQ.insert (varId);
            theEntry.countUpdate = theEntry.repCnt;
	  }
        return;
      }

    BOOST_LOG_SEV(log_rx, trivial::trace) << "process_var_update: updating variable value, varId = " << varId;

    // update variable with new value, update relevant queues
    theEntry.seqno        =  update.seqno;
    theEntry.tStamp       =  TimeStampT::get_current_system_time();
    theEntry.countUpdate  =  theEntry.repCnt;
    vardis_store.update_value (varId, update.value);

    if (not updateQ.contains (varId))
    {
        updateQ.insert (varId);
    }
    reqUpdQ.remove (varId);
    
    // maintain statistics
    vardis_store.get_vardis_protocol_statistics_ref().count_process_var_update++;
  }
  

  // ----------------------------------------------------
  
  /**
   * Processes a received VarSummary entry. If variable exists but we have
   * it only in outdated version, we send a ReqVarUpdate for this variable
   */
  void VardisProtocolData::process_var_summary (const VarSummT& summ)
  {
    VarIdT     varId            = summ.varId;
    VarSeqnoT  seqno            = summ.seqno;
    
    BOOST_LOG_SEV(log_rx, trivial::trace) << "process_var_summary: got variable summary, varId = " << varId
					  << ", seqno = " << seqno;
    
    // if variable does not exist in local RTDB, request a VarCreate
    if (not variableExists(varId))
      {
        if (not reqCreateQ.contains (varId))
	  {
            reqCreateQ.insert (varId);
	  }
        return;
      }
    
    DBEntry& theEntry = vardis_store.get_db_entry_ref(varId);
    
    // perform some checks
    
    if (theEntry.toBeDeleted)
      {
        return;
      }
    
    if (producerIsMe(varId))
      {
        return;
      }
    
    if (theEntry.seqno == seqno)
      {
        return;
      }
    
    // schedule transmission of VarUpdate's if the received seqno is too
    // old
    if (more_recent_seqno(theEntry.seqno, seqno))
      {
        if (not updateQ.contains (varId))
	  {
            updateQ.insert (varId);
            theEntry.countUpdate = theEntry.repCnt;
	  }
        return;
      }
    
    // If my own value is too old, schedule transmission of VarReqUpdate
    if (not reqUpdQ.contains (varId))
      {
        reqUpdQ.insert (varId);
      }

    // maintain statistics
    vardis_store.get_vardis_protocol_statistics_ref().count_process_var_summary++;
  }
  
  
  // ----------------------------------------------------
  
  /**
   * Processes a received VarReqUpdate entry. If variable exists and we
   * have it in a more recent version, schedule transmissions of VarUpdate's
   * for this variable
   */
  void VardisProtocolData::process_var_requpdate (const VarReqUpdateT& requpd)
  {
    VarIdT     varId         = requpd.updSpec.varId;
    VarSeqnoT  seqno         = requpd.updSpec.seqno;
    
    BOOST_LOG_SEV(log_rx, trivial::trace) << "process_var_requpdate: got request for varId = " << varId
					  << " with seqno = " << seqno;
    // check some conditions
    
    if (not variableExists(varId))
      {
        if (not reqCreateQ.contains (varId))
	  {
            reqCreateQ.insert (varId);
	  }
        return;
      }
    
    DBEntry& theEntry = vardis_store.get_db_entry_ref(varId);
    
    if (theEntry.toBeDeleted)
      {
        return;
      }
    
    if (more_recent_seqno(seqno, theEntry.seqno))
      {
        return;
      }
    
    theEntry.countUpdate = theEntry.repCnt;
    
    if (not updateQ.contains (varId))
      {
	updateQ.insert (varId);
      }

    // maintain statistics
    vardis_store.get_vardis_protocol_statistics_ref().count_process_var_requpdate++;
  }

  // ----------------------------------------------------
  
  /**
   * Processes a received VarReqCreate entry. If variable exists, schedule
   * transmissions of VarCreate's for this variable
   */
  void VardisProtocolData::process_var_reqcreate (const VarReqCreateT& reqcreate)
  {
    VarIdT varId             = reqcreate.varId;

    BOOST_LOG_SEV(log_rx, trivial::trace) << "process_var_reqcreate: got request for varId = " << varId;
    
    if (not variableExists(varId))
      {
        if (not reqCreateQ.contains (varId))
	  {
            reqCreateQ.insert (varId);
	  }
        return;
      }
    
    DBEntry& theEntry = vardis_store.get_db_entry_ref(varId);

    if (theEntry.toBeDeleted)
      {
        return;
      }

    theEntry.countCreate = theEntry.repCnt;
    
    if (not createQ.contains (varId))
      {
	createQ.insert (varId);
      }

    // maintain statistics
    vardis_store.get_vardis_protocol_statistics_ref().count_process_var_reqcreate++;
  }


  // ----------------------------------------------------

  RTDB_Create_Confirm VardisProtocolData::handle_rtdb_create_request (const RTDB_Create_Request& createReq)
  {
    const VarSpecT&    spec   = createReq.spec;
    const VarValueT&   value  = createReq.value;
    const VarIdT       varId  = spec.varId;    

    BOOST_LOG_SEV(log_tx, trivial::trace) << "Received RTDB-Create request for variable " << spec.varId;
    
    if (not vardis_store.get_vardis_isactive())
      {
	return RTDB_Create_Confirm (VARDIS_STATUS_INACTIVE, varId);
      }
    
    if (variableExists(spec.varId))
      {
	return RTDB_Create_Confirm (VARDIS_STATUS_VARIABLE_EXISTS, varId);
      }
    
    if (spec.descr.length > maxDescriptionLength)
      {
	return RTDB_Create_Confirm (VARDIS_STATUS_VARIABLE_DESCRIPTION_TOO_LONG, varId);
      }
    
    if (value.length > maxValueLength)
      {
	return RTDB_Create_Confirm (VARDIS_STATUS_VALUE_TOO_LONG, varId);
      }
    
    if (value.length == 0)
      {
	return RTDB_Create_Confirm (VARDIS_STATUS_EMPTY_VALUE, varId);
      }
    
    if ((spec.repCnt == 0) || (spec.repCnt > maxRepetitions))
      {
	return RTDB_Create_Confirm (VARDIS_STATUS_ILLEGAL_REPCOUNT, varId);
      }

    BOOST_LOG_SEV(log_tx, trivial::trace) << "Processing RTDB-Create request for variable " << spec.varId;
    
    // initialize new database entry and add it
    DBEntry newent;
    newent.varId         =  spec.varId;
    newent.prodId        =  ownNodeIdentifier;
    newent.repCnt        =  spec.repCnt;
    newent.seqno         =  0;
    newent.tStamp        =  TimeStampT::get_current_system_time();
    newent.countUpdate   =  0;
    newent.countCreate   =  spec.repCnt;
    newent.countDelete   =  0;
    newent.toBeDeleted   =  false;
    vardis_store.allocate_identifier (spec.varId);
    vardis_store.set_db_entry (spec.varId, newent);
    vardis_store.update_description (spec.varId, spec.descr);
    vardis_store.update_value (spec.varId, value);
    active_variables.insert (spec.varId);

    // clean out varId from all queues, just to be safe
    createQ.remove (spec.varId);
    updateQ.remove (spec.varId);
    summaryQ.remove (spec.varId);
    deleteQ.remove (spec.varId);
    reqUpdQ.remove (spec.varId);
    reqCreateQ.remove ( spec.varId);

    // add new variable to relevant queues
    createQ.insert (spec.varId);
    summaryQ.insert (spec.varId);

    // Maintain statistics
    vardis_store.get_vardis_protocol_statistics_ref().count_handle_rtdb_create++;
    
    // send confirmation
    return RTDB_Create_Confirm (VARDIS_STATUS_OK, varId);
  }

  // ----------------------------------------------------
  
  RTDB_Update_Confirm VardisProtocolData::handle_rtdb_update_request (const RTDB_Update_Request& updateReq)
  {
    const VarIdT  varId  = updateReq.varId;
    const VarLenT varLen = updateReq.value.length;

    BOOST_LOG_SEV(log_tx, trivial::trace) << "Received RTDB-Update request for variable " << varId;
    
    // perform various checks

    if (not vardis_store.get_vardis_isactive())
    {
      return RTDB_Update_Confirm (VARDIS_STATUS_INACTIVE, varId);
    }
    
    if (not variableExists(varId))
    {
      return RTDB_Update_Confirm (VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST, varId);
    }
    
    DBEntry& theEntry = vardis_store.get_db_entry_ref(varId);

    if (not producerIsMe(varId))
    {
      return RTDB_Update_Confirm (VARDIS_STATUS_NOT_PRODUCER, varId);
    }

    if (theEntry.toBeDeleted)
    {
      return RTDB_Update_Confirm (VARDIS_STATUS_VARIABLE_BEING_DELETED, varId);
    }

    if (varLen > maxValueLength)
    {
      return RTDB_Update_Confirm (VARDIS_STATUS_VALUE_TOO_LONG, varId);
    }

    if (varLen == 0)
    {
      return RTDB_Update_Confirm (VARDIS_STATUS_EMPTY_VALUE, varId);
    }

    BOOST_LOG_SEV(log_tx, trivial::trace) << "Handling RTDB-Update request for variable " << varId;
    
    // update the DB entry
    theEntry.seqno        = (theEntry.seqno.val + 1) % (VarSeqnoT::modulus());
    theEntry.countUpdate  = theEntry.repCnt;
    theEntry.tStamp       = TimeStampT::get_current_system_time();
    vardis_store.update_value (varId, updateReq.value);

    // add varId to updateQ if necessary
    if (not updateQ.contains (varId))
    {
        updateQ.insert (varId);
    }

    // Maintain statistics
    vardis_store.get_vardis_protocol_statistics_ref().count_handle_rtdb_update++;
    
    return RTDB_Update_Confirm (VARDIS_STATUS_OK, varId);
  }

  // ----------------------------------------------------

  RTDB_Delete_Confirm VardisProtocolData::handle_rtdb_delete_request (const RTDB_Delete_Request& deleteReq)
  {
    VarIdT varId = deleteReq.varId;

    BOOST_LOG_SEV(log_tx, trivial::trace) << "Received RTDB-Delete request for variable " << varId;
    
    // perform some checks

    if (not vardis_store.get_vardis_isactive())
      {
	return RTDB_Delete_Confirm (VARDIS_STATUS_INACTIVE, varId);
      }
    
    if (not variableExists(varId))
      {
	return RTDB_Delete_Confirm (VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST, varId);
      }

    if (not producerIsMe(varId))
      {
	return RTDB_Delete_Confirm (VARDIS_STATUS_NOT_PRODUCER, varId);
      }

    DBEntry& theEntry = vardis_store.get_db_entry_ref(varId);

    if (theEntry.toBeDeleted)
    {
      return RTDB_Delete_Confirm (VARDIS_STATUS_VARIABLE_BEING_DELETED, varId);
    }

    BOOST_LOG_SEV(log_tx, trivial::trace) << "Handling RTDB-Delete request for variable " << varId;
    
    // add varId to deleteQ, remove it from any other queue
    // assert(not isVarIdInQueue(deleteQ, varId));

    createQ.remove (varId);
    deleteQ.remove (varId);
    summaryQ.remove (varId);
    updateQ.remove (varId);
    reqUpdQ.remove (varId);
    reqCreateQ.remove (varId);

    deleteQ.insert(varId);
    
    // update variable status
    theEntry.toBeDeleted = true;
    theEntry.countDelete = theEntry.repCnt;
    theEntry.countCreate = 0;
    theEntry.countUpdate = 0;

    // Maintain statistics
    vardis_store.get_vardis_protocol_statistics_ref().count_handle_rtdb_delete++;
    
    return RTDB_Delete_Confirm (VARDIS_STATUS_OK, varId);
  }


    // ----------------------------------------------------

  RTDB_Read_Confirm VardisProtocolData::handle_rtdb_read_request (const RTDB_Read_Request& readReq)
  {
    VarIdT varId = readReq.varId;

    BOOST_LOG_SEV(log_tx, trivial::trace) << "Received RTDB-Read request for variable " << varId;
    
    // perform some checks

    if (not vardis_store.get_vardis_isactive())
      {
	return RTDB_Read_Confirm (VARDIS_STATUS_INACTIVE, varId);
      }
    
    if (not variableExists(varId))
      {
	return RTDB_Read_Confirm (VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST, varId);
      }

    DBEntry& theEntry = vardis_store.get_db_entry_ref(varId);

    if (theEntry.toBeDeleted)
    {
      return RTDB_Read_Confirm (VARDIS_STATUS_VARIABLE_BEING_DELETED, varId);
    }

    BOOST_LOG_SEV(log_tx, trivial::trace) << "Handling RTDB-Delete request for variable " << varId;

    VarValueT the_value = vardis_store.read_value (varId);
    RTDB_Read_Confirm conf (VARDIS_STATUS_OK, varId, the_value.length, the_value.data);
    conf.tStamp = theEntry.tStamp;

    // Maintain statistics
    vardis_store.get_vardis_protocol_statistics_ref().count_handle_rtdb_read++;
    
    return conf;
  }

  
  
};  // namespace dcp::vardis
