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
#include <dcp/vardis/vardis_logging.h>
#include <dcp/vardis/vardis_payload_handling.h>


namespace dcp::vardis {

  // -----------------------------------------------------------------
  
  /**
   * @brief Extracts / parses an entire instruction container (its
   *        instruction records, which are stored in a list)
   *
   * @tparam T: data type of instruction records
   * @param area: disassembly area to extract records from, will be
   *        modified
   * @param icHeader: header of instruction container
   * @param result_list: accumulates the deserialized instruction
   *        records
   */

  template <typename T>
  void extractInstructionContainerElements (DisassemblyArea& area, const ICHeaderT& icHeader, std::deque<T>& result_list)
  {
    if (icHeader.icNumRecords == 0)
      {
	throw VardisReceiveException ("extractInstructionContainerElements", "number of records is zero");
      }
    
    for (int i=0; i<icHeader.icNumRecords; i++)
      {
        T element;
        element.deserialize (area);
        result_list.push_back(element);
      }
  };
  
  // -----------------------------------------------------------------

  void process_received_payload (VardisProtocolData& prot_data, DisassemblyArea& area)
  {
    std::deque<VarSummT>       icSummaries;
    std::deque<VarUpdateT>     icUpdates;
    std::deque<VarReqUpdateT>  icRequestVarUpdates;
    std::deque<VarReqCreateT>  icRequestVarCreates;
    std::deque<VarCreateT>     icCreateVariables;
    std::deque<VarDeleteT>     icDeleteVariables;

    // Dispatch on ICType
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
	      throw VardisReceiveException ("process_received_payload",
					    std::format("wrong instruction container type {}", (int) icHeader.icType.val));
	    }
	  }
      }
    
    
    // Now process the received containers in the specified order
    // (database updates).
    for (auto it = icCreateVariables.begin(); it != icCreateVariables.end(); ++it)
      prot_data.process_var_create (*it);
    for (auto it = icDeleteVariables.begin(); it != icDeleteVariables.end(); ++it)
      prot_data.process_var_delete (*it);
    for (auto it = icUpdates.begin(); it != icUpdates.end(); ++it)
      prot_data.process_var_update (*it);
    for (auto it = icSummaries.begin(); it != icSummaries.end(); ++it)
      prot_data.process_var_summary (*it);
    for (auto it = icRequestVarUpdates.begin(); it != icRequestVarUpdates.end(); ++it)
      prot_data.process_var_requpdate (*it);
    for (auto it = icRequestVarCreates.begin(); it != icRequestVarCreates.end(); ++it)
      prot_data.process_var_reqcreate (*it);
  }


  // -----------------------------------------------------------------


  void construct_payload (VardisProtocolData& prot_data,
			  AssemblyArea& area,
			  unsigned int& containers_added)
  {
    prot_data.makeICTypeCreateVariables (area, containers_added);
    prot_data.makeICTypeDeleteVariables (area, containers_added);
    prot_data.makeICTypeRequestVarCreates (area, containers_added);
    prot_data.makeICTypeSummaries (area, containers_added);
    prot_data.makeICTypeUpdates (area, containers_added);
    prot_data.makeICTypeRequestVarUpdates (area, containers_added);
  }
  
};  // namespace dcp::vardis
