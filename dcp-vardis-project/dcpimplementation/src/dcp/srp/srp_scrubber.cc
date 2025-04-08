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


#include <functional>
#include <list>
#include <queue>
#include <thread>
#include <chrono>
#include <dcp/common/area.h>
#include <dcp/common/services_status.h>
#include <dcp/bp/bpclient_lib.h>
#include <dcp/srp/srp_logging.h>
#include <dcp/srp/srp_scrubber.h>



namespace dcp::srp {

  // -----------------------------------------------------------------
  
  void scrubber_thread (SRPRuntimeData& runtime)
  {
    BOOST_LOG_SEV(log_scrub, trivial::info) << "Starting scrubbing thread.";

    uint16_t timeoutMS = runtime.srp_config.srp_conf.srpScrubbingTimeoutMS;

    try {
      while (not runtime.srp_exitFlag)
	{
	  std::this_thread::sleep_for (std::chrono::milliseconds (runtime.srp_config.srp_conf.srpScrubbingPeriodMS));
	  
	  if (not runtime.srp_store.get_srp_isactive())
	    continue;
	  
	  TimeStampT current_time = TimeStampT::get_current_system_time();
	  
	  ScopedNeighbourTableMutex lock (runtime);
	  std::list<NodeIdentifierT> nodes_to_remove = runtime.srp_store.find_nodes_to_scrub (current_time, timeoutMS);
	  
	  for (const auto& nodeId : nodes_to_remove)
	    runtime.srp_store.remove_esd_entry (nodeId);
	}
    }
    catch (DcpException& e)
      {
	BOOST_LOG_SEV(log_scrub, trivial::fatal)
	  << "Caught DCP exception in SRP scrubber main loop. "
	  << "Exception type: " << e.ename()
	  << ", module: " << e.modname()
	  << ", message: " << e.what()
	  << ". Exiting.";
	runtime.srp_exitFlag = true;
      }
    catch (std::exception& e)
      {
	BOOST_LOG_SEV(log_scrub, trivial::fatal)
	  << "Caught other exception in SRP scrubber main loop. "
	  << "Message: " << e.what()
	  << ". Exiting.";
	runtime.srp_exitFlag = true;
      }

      
    BOOST_LOG_SEV(log_scrub, trivial::info) << "Exiting scrubbing thread.";
  }
  
};  // namespace dcp::srp
