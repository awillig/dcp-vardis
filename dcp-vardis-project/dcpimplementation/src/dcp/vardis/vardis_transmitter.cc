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
#include <dcp/common/debug_helpers.h>
#include <dcp/common/services_status.h>
#include <dcp/bp/bpclient_lib.h>
#include <dcp/bp/bp_service_primitives.h>
#include <dcp/vardis/vardis_transmitter.h>
#include <dcp/vardis/vardis_logging.h>


using dcp::bp::BPTransmitPayload_Request;


namespace dcp::vardis {

  // -----------------------------------------------------------------

  void construct_payload (VardisRuntimeData& runtime, AssemblyArea& area, unsigned int& containers_added)
  {
    VardisProtocolData& pd         = runtime.protocol_data;
    try {
      if (runtime.vardis_config.vardis_conf.lockingForIndividualContainers)
	{
	  { ScopedVariableStoreMutex mtx (runtime);  pd.makeICTypeCreateVariables (area, containers_added); }
	  { ScopedVariableStoreMutex mtx (runtime);  pd.makeICTypeDeleteVariables (area, containers_added); }
	  { ScopedVariableStoreMutex mtx (runtime);  pd.makeICTypeRequestVarCreates (area, containers_added); }
	  { ScopedVariableStoreMutex mtx (runtime);  pd.makeICTypeSummaries (area, containers_added); }
	  { ScopedVariableStoreMutex mtx (runtime);  pd.makeICTypeUpdates (area, containers_added); }
	  { ScopedVariableStoreMutex mtx (runtime);  pd.makeICTypeRequestVarUpdates (area, containers_added); }
	}
      else
	{
	  ScopedVariableStoreMutex mtx (runtime);
	  pd.makeICTypeCreateVariables (area, containers_added);
	  pd.makeICTypeDeleteVariables (area, containers_added);
	  pd.makeICTypeRequestVarCreates (area, containers_added);
	  pd.makeICTypeSummaries (area, containers_added);
	  pd.makeICTypeUpdates (area, containers_added);
	  pd.makeICTypeRequestVarUpdates (area, containers_added);
	}
    }
    catch (DcpException& e) {
      BOOST_LOG_SEV(log_mgmt_command, trivial::fatal)
	<< "Caught exception during payload construction. "
	<< "Exception type: " << e.ename()
	<< ", module: " << e.modname()
	<< ", message: " << e.what()
	<< "Exiting.";
      runtime.vardis_exitFlag = true;
      containers_added = 0;
    }

    catch (std::exception& e)
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal) << "Caught exception during payload construction: " << e.what() << ". Exiting.";
	runtime.vardis_exitFlag = true;
	containers_added = 0;
      }
  }
  
  // -----------------------------------------------------------------
  
  void transmitter_thread (VardisRuntimeData& runtime)
  {
    BOOST_LOG_SEV(log_tx, trivial::info) << "Starting transmit thread.";

    BPShmControlSegment& CS = *(runtime.pSCS);

    try {
      while (not runtime.vardis_exitFlag)
	{
	  std::this_thread::sleep_for (std::chrono::milliseconds (runtime.vardis_config.vardis_conf.payloadGenerationIntervalMS));
	  
	  if (not runtime.protocol_data.vardis_store.get_vardis_isactive())
	    continue;
	  
	  PushHandler handler = [&] (byte* memaddr, size_t bufferSize)
	  {
	    BPTransmitPayload_Request*  pldReq_ptr = new (memaddr) BPTransmitPayload_Request;
	    byte* area_ptr = memaddr + sizeof(BPTransmitPayload_Request);
	    MemoryChunkAssemblyArea area ("vd-tx", std::min((size_t) runtime.vardis_config.vardis_conf.maxPayloadSize, bufferSize), area_ptr);
	    unsigned int containers_added = 0;
	    
	    construct_payload (runtime, area, containers_added);
	    
	    if (containers_added == 0)
	      return (size_t) 0;
	    
	    pldReq_ptr->protocolId = BP_PROTID_VARDIS;
	    pldReq_ptr->length     = area.used();
	    
	    return (area.used() + sizeof(BPTransmitPayload_Request));
	  };
	  
	  bool timed_out;
	  
	  CS.queue.push_wait (handler, timed_out);
	  if (timed_out)
	    {
	      BOOST_LOG_SEV(log_tx, trivial::fatal) << "Shared memory timeout. Exiting.";
	      runtime.vardis_exitFlag = true;
	    }
	}
    }
    catch (DcpException& e)
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal)
	  << "Caught DCP exception in Vardis transmitter main loop. "
	  << "Exception type: " << e.ename()
	  << ", module: " << e.modname()
	  << ", message: " << e.what()
	  << "Exiting.";
	runtime.vardis_exitFlag = true;
      }
    catch (std::exception& e)
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal)
	  << "Caught other exception in Vardis transmitter main loop. "
	  << "Message: " << e.what()
	  << "Exiting.";
	runtime.vardis_exitFlag = true;
      }
    
    BOOST_LOG_SEV(log_tx, trivial::info) << "Exiting transmit thread.";
  }
  
};  // namespace dcp::vardis
