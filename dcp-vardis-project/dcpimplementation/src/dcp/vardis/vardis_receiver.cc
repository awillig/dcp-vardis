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


#include <queue>
#include <thread>
#include <chrono>
#include <dcp/common/area.h>
#include <dcp/common/services_status.h>
#include <dcp/bp/bpclient_lib.h>
#include <dcp/vardis/vardis_configuration.h>
#include <dcp/vardis/vardis_logging.h>
#include <dcp/vardis/vardis_receiver.h>



namespace dcp::vardis {

  static const uint16_t rx_buffer_length = 4000;

  // #############################################################################

  // -----------------------------------------------------------------

  template <typename T>
  void extractInstructionContainerElements (DisassemblyArea& area, const ICHeaderT& icHeader, std::deque<T>& result_list)
  {
    if (icHeader.icNumRecords == 0)
      {
	BOOST_LOG_SEV(log_rx, trivial::info) << "extractInstructionContainerElements: number of records is zero";
	throw VardisReceiveException ("extractInstructionContainerElements: number of records is zero");
      }
    
    for (int i=0; i<icHeader.icNumRecords; i++)
      {
        T element;
        element.deserialize (area);
        result_list.push_back(element);
      }
  }
  
  
  // -----------------------------------------------------------------

  void process_received_payload (VardisRuntimeData& runtime, DisassemblyArea& area)
  {
    std::deque<VarSummT>       icSummaries;
    std::deque<VarUpdateT>     icUpdates;
    std::deque<VarReqUpdateT>  icRequestVarUpdates;
    std::deque<VarReqCreateT>  icRequestVarCreates;
    std::deque<VarCreateT>     icCreateVariables;
    std::deque<VarDeleteT>     icDeleteVariables;

    // Dispatch on ICType
    try {
      while (area.used() < area.available())
	{
	  ICHeaderT icHeader;
	  icHeader.deserialize(area);
	  switch(icHeader.icType.val)
	    {
	    case ICTYPE_SUMMARIES:
	      {
		extractInstructionContainerElements<VarSummT> (area, icHeader, icSummaries);
		break;
	      }
	    case ICTYPE_UPDATES:
	      {
		extractInstructionContainerElements<VarUpdateT> (area, icHeader, icUpdates);
		break;
	      }
	    case ICTYPE_REQUEST_VARUPDATES:
	      {
		extractInstructionContainerElements<VarReqUpdateT> (area, icHeader, icRequestVarUpdates);
		break;
	      }
	    case ICTYPE_REQUEST_VARCREATES:
	      {
		extractInstructionContainerElements<VarReqCreateT> (area, icHeader, icRequestVarCreates);
		break;
	      }
	    case ICTYPE_CREATE_VARIABLES:
	      {
		extractInstructionContainerElements<VarCreateT> (area, icHeader, icCreateVariables);
		break;
	      }
	    case ICTYPE_DELETE_VARIABLES:
	      {
		extractInstructionContainerElements<VarDeleteT> (area, icHeader, icDeleteVariables);
		break;
	      }
	    default:
	      {
		BOOST_LOG_SEV(log_rx, trivial::info) << "Unknown type of instruction container: "
						     << vardis_instruction_container_to_string (InstructionContainerT(area.peek_byte()))
						     << ", stopping processing";
		throw VardisReceiveException ("process_received_payload: wrong instruction container type ");
	      }
	    }
	}
    }
    catch (std::exception& e)
      {
	BOOST_LOG_SEV(log_rx, trivial::info)
	  << "process_received_payload -- extracting elements: caught exception " << e.what()
	  << ", stopping processing";

        return;
      }


    // Now process the received containers in the specified order
    // (database updates).
    //
    // ISSUE: It is unclear on whether it is better / more efficient to
    // acquire a lock just once and process all containers in one go,
    // or do them separately. The current do-both solution is not
    // really elegant
    try {
      if (runtime.vardis_config.vardis_conf.lockingForIndividualContainers)
	{
	  { ScopedVariableStoreMutex mtx (runtime);
	    for (auto it = icCreateVariables.begin(); it != icCreateVariables.end(); ++it)
	      runtime.protocol_data.process_var_create (*it);
	  }

	  { ScopedVariableStoreMutex mtx (runtime);
	    for (auto it = icDeleteVariables.begin(); it != icDeleteVariables.end(); ++it)
	      runtime.protocol_data.process_var_delete (*it);
	  }
	  
	  { ScopedVariableStoreMutex mtx (runtime);
	    for (auto it = icUpdates.begin(); it != icUpdates.end(); ++it)
	      runtime.protocol_data.process_var_update (*it);
	  }
	  
	  { ScopedVariableStoreMutex mtx (runtime);
	    for (auto it = icSummaries.begin(); it != icSummaries.end(); ++it)
	      runtime.protocol_data.process_var_summary (*it);
	  }
	  
	  { ScopedVariableStoreMutex mtx (runtime);
	    for (auto it = icRequestVarUpdates.begin(); it != icRequestVarUpdates.end(); ++it)
	      runtime.protocol_data.process_var_requpdate (*it);
	  }

	  { ScopedVariableStoreMutex mtx (runtime);
	    for (auto it = icRequestVarCreates.begin(); it != icRequestVarCreates.end(); ++it)
	      runtime.protocol_data.process_var_reqcreate (*it);
	  }
	}
      else
	{
	  ScopedVariableStoreMutex mtx (runtime);
	  for (auto it = icCreateVariables.begin(); it != icCreateVariables.end(); ++it)
	    runtime.protocol_data.process_var_create (*it);
	  for (auto it = icDeleteVariables.begin(); it != icDeleteVariables.end(); ++it)
	    runtime.protocol_data.process_var_delete (*it);
	  for (auto it = icUpdates.begin(); it != icUpdates.end(); ++it)
	    runtime.protocol_data.process_var_update (*it);
	  for (auto it = icSummaries.begin(); it != icSummaries.end(); ++it)
	    runtime.protocol_data.process_var_summary (*it);
	  for (auto it = icRequestVarUpdates.begin(); it != icRequestVarUpdates.end(); ++it)
	    runtime.protocol_data.process_var_requpdate (*it);
	  for (auto it = icRequestVarCreates.begin(); it != icRequestVarCreates.end(); ++it)
	    runtime.protocol_data.process_var_reqcreate (*it);
	}
    }
    catch (std::exception& e)
      {
	BOOST_LOG_SEV(log_rx, trivial::info) << "process_received_payload -- applying changes: caught exception " << e.what()
					     << ", stopping processing";
	return;
      }
  }

  // -----------------------------------------------------------------
  
  void receiver_thread (VardisRuntimeData& runtime)
  {
    BOOST_LOG_SEV(log_rx, trivial::info) << "Starting receive thread.";
    while (not runtime.vardis_exitFlag)
      {
	std::this_thread::sleep_for (std::chrono::milliseconds (runtime.vardis_config.vardis_conf.pollReceivePayloadMS));
				     
	if (not runtime.protocol_data.vardis_store.get_vardis_isactive())
	  continue;

	// check if we have received a payload
	BPLengthT result_length = 0;
	DcpStatus rx_stat;
	byte rx_buffer [rx_buffer_length];
	bool more_payloads = false;

	do {
	  rx_stat = runtime.receive_payload (result_length, rx_buffer, more_payloads);

	  if ((result_length > 0) && (rx_stat == BP_STATUS_OK))
	    {
	      BOOST_LOG_SEV(log_rx, trivial::info) << "Processing payload of length " << result_length;
	      MemoryChunkDisassemblyArea area ("vd-rx", (size_t) result_length.val, rx_buffer);
	      process_received_payload (runtime, area);
	    }
	  else
	    {
	      if (rx_stat != BP_STATUS_OK)
		BOOST_LOG_SEV(log_rx, trivial::info) << "Retrieving received payload issued error " << bp_status_to_string (rx_stat);
	    }
	} while (more_payloads);
      }
    BOOST_LOG_SEV(log_rx, trivial::info) << "Exiting receive thread.";
  }
  
};  // namespace dcp::vardis
