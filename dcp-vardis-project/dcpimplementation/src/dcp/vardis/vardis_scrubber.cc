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


#include <chrono>
#include <thread>
#include <dcp/common/debug_helpers.h>
#include <dcp/vardis/vardis_configuration.h>
#include <dcp/vardis/vardis_logging.h>
#include <dcp/vardis/vardis_scrubber.h>
#include <dcp/vardis/vardis_store_interface.h>



namespace dcp::vardis {

  // -----------------------------------------------------------------
  
  void scrubbing_thread (VardisRuntimeData& runtime)
  {
    DCPLOG_INFO(log_scrubbing) << "Starting scrubbing thread.";

    VardisProtocolData&  PD               = runtime.protocol_data;
    TimeStampT           last_scrub       = TimeStampT::get_current_system_time();
    uint16_t             scrubbing_period = runtime.vardis_config.vardis_conf.scrubbingPeriodMS;
    
    try {
      while (not runtime.vardis_exitFlag)
	{
	  std::this_thread::sleep_for (std::chrono::milliseconds (100));

	  TimeStampT curr_time = TimeStampT::get_current_system_time();
	  if (    (not runtime.protocol_data.vardis_store.get_vardis_isactive())
	       || (curr_time.milliseconds_passed_since (last_scrub) <= scrubbing_period))
	    {
	      continue;
	    }

	  last_scrub = TimeStampT::get_current_system_time();

	  // now we iterate over the variable store

	  auto var_it                =  PD.active_variables.begin ();
	  size_t numvars             =  PD.active_variables.size ();
	  const size_t batch_size    =  50;

	  while (numvars > 0)
	    {
	      ScopedVariableStoreMutex mtx (runtime);
	      for (size_t i = 0; i<std::min(batch_size, numvars); i++)
		{
		  VarIdT     varId     = *var_it;
		  DBEntry&   ent       = PD.vardis_store.get_db_entry_ref (varId);

		  --numvars;
		  ++var_it;
		  
		  if (    (ent.isDeleted)
		       || (ent.timeout == 0)
		       || (curr_time.milliseconds_passed_since (ent.tStamp) <= ent.timeout))
		    {
		      continue;
		    }

		  DCPLOG_INFO(log_scrubbing) << "Marking variable " << varId
					     << " as deleted after timeout of " << ent.timeout << " milliseconds"
					     << ", timestamp was " << ent.tStamp
					     << ", currtime was " << curr_time
					     << ".";
		  
		  // mark varId as deleted
		  ent.isDeleted    = true;
		  ent.countUpdate  = 0;
		  ent.countDelete  = ent.repCnt;
		  ent.countCreate  = 0;

		  PD.createQ.remove (varId);
		  PD.deleteQ.remove (varId);
		  PD.updateQ.remove (varId);
		  PD.summaryQ.remove (varId);
		  PD.reqUpdQ.remove (varId);
		  PD.reqCreateQ.remove (varId);

		  PD.deleteQ.insert (varId);
		}
	    }
	  
	}
    }
    catch (DcpException& e)
      {
	DCPLOG_FATAL(log_scrubbing)
	  << "Caught DCP exception in Vardis scrubbing loop. "
	  << "Exception type: " << e.ename()
	  << ", module: " << e.modname()
	  << ", message: " << e.what()
	  << ". Exiting.";
	runtime.vardis_exitFlag = true;
      }
    catch (std::exception& e)
      {
	DCPLOG_FATAL(log_scrubbing)
	  << "Caught other exception in Vardis scrubbing loop. "
	  << "Message: " << e.what()
	  << ". Exiting.";
	runtime.vardis_exitFlag = true;
      }

    
    DCPLOG_INFO(log_scrubbing) << "Exiting scrubbing thread.";
  }
    
};  // namespace dcp::vardis
