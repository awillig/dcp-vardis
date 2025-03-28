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
#include <exception>
#include <functional>
#include <thread>
extern "C" {
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
}
#include <dcp/common/command_socket.h>
#include <dcp/common/other_helpers.h>
#include <dcp/common/services_status.h>
#include <dcp/bp/bp_management_command.h>
#include <dcp/bp/bp_transmissible_types.h>
#include <dcp/bp/bp_service_primitives.h>
#include <dcp/bp/bp_logging.h>
#include <dcp/bp/bp_client_protocol_data.h>
#include <dcp/bp/bp_shm_control_segment.h>



using std::size_t;
using dcp::ShmException;
using dcp::command_sock_buffer_size;


namespace dcp::bp {

  // ------------------------------------------------------------------

  static const int commandSocketListenBufferBacklog = 20;
  
  
  // ------------------------------------------------------------------

  void sendRegisterConfirmation (BPRuntimeData& runtime, DcpStatus statcode)
  {
    BPRegisterProtocol_Confirm conf;
    conf.status_code        = statcode;
    conf.ownNodeIdentifier  = runtime.ownNodeIdentifier;
    runtime.commandSocket.send_raw_confirmation (log_mgmt_command, conf, sizeof(BPRegisterProtocol_Confirm), runtime.bp_exitFlag);
  }

  // ------------------------------------------------------------------

  template <typename CT>
  void send_simple_confirmation (BPRuntimeData& runtime, DcpStatus statcode)
  {
    runtime.commandSocket.send_simple_confirmation<CT>(log_mgmt_command, statcode, runtime.bp_exitFlag);
  }
  
  // ------------------------------------------------------------------


  /**
   * @brief Implement shutdown service
   *
   * Note: this does not send a confirmation primitive, as there is no
   * guarantee that the BP command socket still exists when the client
   * attempts to read from it
   */
  void handleBPShutDown_Request (BPRuntimeData& runtime, byte*, size_t)
  {
    BOOST_LOG_SEV(log_mgmt_command, trivial::info) << "Processing request: ShutDown";
    runtime.bp_exitFlag = true;
    return;
  }

    // ------------------------------------------------------------------

  void handleBPActivate_Request (BPRuntimeData& runtime, byte*, size_t nbytes)
  {
    if (nbytes != sizeof(BPActivate_Request))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::fatal)
	    << "Processing BPActivate request: wrong data size = "
	    << nbytes
	    << ", exiting"
          ;
	runtime.bp_exitFlag = true;
	send_simple_confirmation<BPActivate_Confirm>(runtime, BP_STATUS_INTERNAL_ERROR);
	return;
      }

    BOOST_LOG_SEV(log_mgmt_command, trivial::info) << "Processing request: Activate";
    runtime.bp_isActive = true;
    send_simple_confirmation<BPActivate_Confirm>(runtime, BP_STATUS_OK);
    return;
  }

    // ------------------------------------------------------------------

  void handleBPDeactivate_Request (BPRuntimeData& runtime, byte*, size_t nbytes)
  {
    if (nbytes != sizeof(BPDeactivate_Request))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::fatal)
	    << "Processing BPDeactivate request: wrong data size = "
	    << nbytes
	    << ", exiting."
           ;
	runtime.bp_exitFlag = true;
	send_simple_confirmation<BPDeactivate_Confirm>(runtime, BP_STATUS_INTERNAL_ERROR);
	return;
      }

    BOOST_LOG_SEV(log_mgmt_command, trivial::info) << "Processing request: Deactivate";
    runtime.bp_isActive = false;
    send_simple_confirmation<BPDeactivate_Confirm>(runtime, BP_STATUS_OK);
    return;
  }
  
  // ------------------------------------------------------------------

  void handleBPGetStatistics_Request (BPRuntimeData& runtime, byte*, size_t nbytes)
  {
    if (nbytes != sizeof(BPGetStatistics_Request))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::fatal)
	    << "Processing BPGetStatistics request: wrong data size = "
	    << nbytes
	    << ", exiting."
           ;
	runtime.bp_exitFlag = true;
	send_simple_confirmation<BPGetStatistics_Confirm>(runtime, BP_STATUS_INTERNAL_ERROR);
	return;
      }

    BPGetStatistics_Confirm gs_conf;
    gs_conf.status_code              = BP_STATUS_OK;
    gs_conf.avg_inter_beacon_time    = runtime.avg_inter_beacon_reception_time;
    gs_conf.avg_beacon_size          = runtime.avg_received_beacon_size;
    gs_conf.number_received_beacons  = runtime.cntBPPayloads;

    runtime.commandSocket.send_raw_confirmation (log_mgmt_command, gs_conf, sizeof(gs_conf), runtime.bp_exitFlag);
  }
  
  // ------------------------------------------------------------------

  void handleBPRegisterProtocol_Request (BPRuntimeData& runtime, byte* buffer, size_t nbytes)
  {
    if (nbytes != sizeof(BPRegisterProtocol_Request))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::fatal)
	    << "Processing BPRegisterProtocol request: wrong data size = "
	    << nbytes
	    << ", exiting"
           ;
	runtime.bp_exitFlag = true;
	sendRegisterConfirmation(runtime, BP_STATUS_INTERNAL_ERROR);
	return;
      }

    BPRegisterProtocol_Request*  pReq = (BPRegisterProtocol_Request*) buffer;
    BPStaticClientInfo& sci = pReq->static_info;
    BOOST_LOG_SEV(log_mgmt_command, trivial::info)
      << "Processing request: RegisterProtocol, protocolId = " << sci.protocolId
      << " , name = " << sci.protocolName
      << " , maxPayloadSize = " << sci.maxPayloadSize
      << " , queueingMode = " << bp_queueing_mode_to_string (sci.queueingMode)
      << " , maxEntries = " << sci.maxEntries
      << " , allowMultiplePayloads = " << sci.allowMultiplePayloads;

    // check whether client protocol already exists
    if (runtime.clientProtocols.contains (sci.protocolId))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::error)
	  << "Processing BPRegisterProtocol request: protocol already exists";

	sendRegisterConfirmation(runtime, BP_STATUS_PROTOCOL_ALREADY_REGISTERED);
	return;
      }

    // check if maxPayloadSize is not strictly positive
    if (sci.maxPayloadSize <= 0)
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::error)
	  << "Processing BPRegisterProtocol request: max payload size <= 0";

	sendRegisterConfirmation(runtime, BP_STATUS_ILLEGAL_MAX_PAYLOAD_SIZE);
	return;
      }

    // check if maxPayloadSize is too large
    if (sci.maxPayloadSize.val > runtime.bp_config.bp_conf.maxBeaconSize - (dcp::bp::BPHeaderT::fixed_size() + dcp::bp::BPPayloadHeaderT::fixed_size()))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::error)
	  << "Processing BPRegisterProtocol request: max payload size exceeds allowed maximum";

	sendRegisterConfirmation(runtime, BP_STATUS_ILLEGAL_MAX_PAYLOAD_SIZE);
	return;
      }

    // check maxEntries value
    if (    (    (sci.queueingMode == BP_QMODE_QUEUE_DROPTAIL)
	      || (sci.queueingMode == BP_QMODE_QUEUE_DROPHEAD))
	 && (sci.maxEntries <= 0))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::error)
	  << "Processing BPRegisterProtocol request: illegal dropping queue size";

	sendRegisterConfirmation(runtime, BP_STATUS_ILLEGAL_DROPPING_QUEUE_SIZE);
	return;
      }

    // Now create and initialize new client protocol data entry and add it to the list of registered protocols
    BPClientProtocolData clientProt (pReq->shm_area_name, pReq->static_info, pReq->generateTransmitPayloadConfirms);
    clientProt.static_info                   =  pReq->static_info;
    clientProt.timeStampRegistration         =  TimeStampT::get_current_system_time();
    //clientProt.bufferOccupied                =  false;

    clientProt.cntOutgoingPayloads           =  0;
    clientProt.cntReceivedPayloads           =  0;
    clientProt.cntDroppedOutgoingPayloads    =  0;
    clientProt.cntDroppedIncomingPayloads    =  0;

    // and add client protocol entry to the client protocols list    
    runtime.clientProtocols[sci.protocolId] = std::move(clientProt);

    BOOST_LOG_SEV(log_mgmt_command, trivial::info)
      << "Processing BPRegisterProtocol request: completed successful registration of protocolId "
      << sci.protocolId
      << ", runtime.clientProt.pSCS = " << (void*) runtime.clientProtocols[sci.protocolId].pSCS
      ; 
    sendRegisterConfirmation(runtime, BP_STATUS_OK);

    BOOST_LOG_SEV(log_mgmt_command, trivial::info)
      << "Processing BPRegisterProtocol request: FINISHING";
  }
  
  // ------------------------------------------------------------------

  void handleBPDeregisterProtocol_Request (BPRuntimeData& runtime, byte* buffer, size_t nbytes)
  {
    if (nbytes != sizeof(BPDeregisterProtocol_Request))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::fatal)
	    << "Processing BPDeregisterProtocol request: wrong data size = "
	    << nbytes
	    << ", exiting."
           ;
	runtime.bp_exitFlag = true;
	send_simple_confirmation<BPDeregisterProtocol_Confirm>(runtime, BP_STATUS_INTERNAL_ERROR);
	return;
      }

    BPDeregisterProtocol_Request*  pReq = (BPDeregisterProtocol_Request*) buffer;
    BOOST_LOG_SEV(log_mgmt_command, trivial::info)
      << "Processing request: DeregisterProtocol, protocolId = " << pReq->protocolId;

    // check whether client protocol exists
    if (not runtime.clientProtocols.contains (pReq->protocolId))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::info)
	  << "Processing BPDerregisterProtocol request: protocol is not registered";
	send_simple_confirmation<BPDeregisterProtocol_Confirm>(runtime, BP_STATUS_UNKNOWN_PROTOCOL);
	return;
      }

    BPClientProtocolData& the_client_prot = runtime.clientProtocols[pReq->protocolId];
    auto pSSB = the_client_prot.pSSB;
    BOOST_LOG_SEV(log_mgmt_command, trivial::trace)
      << "Processing BPDerregisterProtocol request: BEFORE erasing: "
      << "this = " << (void*) (&the_client_prot)
      << ", pSCS = " << (void*) the_client_prot.pSCS
      << ", pSSB.use_count = " << pSSB.use_count()
      << ", shm_memory_address() = " << (void*) pSSB->get_memory_address()
      << ", shm_name() = " << pSSB->get_name()
      << ", shm_structure_size = " << pSSB->get_structure_size()
      << ", shm_is_creator = " << pSSB->get_is_creator()
      << ", shm_has_valid_memory = " << pSSB->has_valid_memory()
      ;
    runtime.clientProtocols.erase(pReq->protocolId);
    
    BOOST_LOG_SEV(log_mgmt_command, trivial::info) << "Processing BPDeregisterProtocol request: erased registered protocol";
    send_simple_confirmation<BPDeregisterProtocol_Confirm>(runtime, BP_STATUS_OK);
  }


  // ------------------------------------------------------------------

  void handleBPListRegisteredProtocols_Request (BPRuntimeData& runtime, byte*, size_t nbytes)
  {
    BPListRegisteredProtocols_Confirm conf;
    
    if (nbytes != sizeof(BPListRegisteredProtocols_Request))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::fatal) << "Processing BPListRegisteredProtocols request: wrong data size = " << nbytes;
	runtime.bp_exitFlag = true;
	
	conf.numberProtocols = 0;
	conf.bpIsActive      = false;
	conf.status_code     = BP_STATUS_INTERNAL_ERROR;
	runtime.commandSocket.send_raw_confirmation (log_mgmt_command, conf, sizeof(conf), runtime.bp_exitFlag);

	return;
      }

    BOOST_LOG_SEV(log_mgmt_command, trivial::info) << "Processing request: ListRegisteredProtocols";

    // send confirmation primitive as header
    conf.status_code      = BP_STATUS_OK;
    conf.numberProtocols  = runtime.clientProtocols.size();
    conf.bpIsActive       = runtime.bp_isActive;
    runtime.commandSocket.send_raw_confirmation (log_mgmt_command, conf, sizeof(conf), runtime.bp_exitFlag);

    // and follow this by the protocol entries
    for (auto it = runtime.clientProtocols.begin(); it != runtime.clientProtocols.end(); ++it)
      {
	BPRegisteredProtocolDataDescription  descr;
	BPStaticClientInfo& sci = (it->second).static_info;
	std::strcpy (descr.protocolName, sci.protocolName);
	descr.protocolId             =  sci.protocolId;
	descr.maxPayloadSize         =  sci.maxPayloadSize;
	descr.queueingMode           =  sci.queueingMode;
	descr.maxEntries             =  sci.maxEntries;
	descr.allowMultiplePayloads  =  sci.allowMultiplePayloads;
	descr.timeStampRegistration  =  (it->second).timeStampRegistration;

	descr.cntOutgoingPayloads         =  (it->second).cntOutgoingPayloads;
	descr.cntReceivedPayloads         =  (it->second).cntReceivedPayloads;
	descr.cntDroppedOutgoingPayloads  =  (it->second).cntDroppedOutgoingPayloads;
	descr.cntDroppedIncomingPayloads  =  (it->second).cntDroppedIncomingPayloads;

	if (runtime.commandSocket.send_raw_data (log_mgmt_command, (byte*) &descr, sizeof(descr), runtime.bp_exitFlag) < 0)
	  return;	
      }
  }

  // ------------------------------------------------------------------

  template <typename RT, typename CT>
  void handleRegularRequestUsingSharedMemory (BPRuntimeData& runtime,
					      byte* buffer,
					      size_t nbytes,
					      const std::string& servname,
					      std::function<void (BPRuntimeData&, BPClientProtocolData&, BPShmControlSegment&)> action)
  {
    CT conf;

    // check size of request
    if (nbytes != sizeof(RT))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::fatal)
	    << "Processing "
	    << servname
	    << " request: wrong data size = "
	    << nbytes
           ;
	runtime.bp_exitFlag = true;
	send_simple_confirmation<CT> (runtime, BP_STATUS_INTERNAL_ERROR);
	return;
      }

    RT*  pReq = (RT*) buffer;
    
    BOOST_LOG_SEV(log_mgmt_command, trivial::info)
        << "Processing request: "
        << servname
        << ", protocol id = "
        << pReq->protocolId
       ;

    // check for valid protocol id
    if (not runtime.clientProtocols.contains (pReq->protocolId))
      {
	BOOST_LOG_SEV(log_mgmt_command, trivial::warning)
	    << "Processing "
	    << servname
	    << " request: unknown protocol id = "
	    << pReq->protocolId
           ;
	send_simple_confirmation<CT> (runtime, BP_STATUS_UNKNOWN_PROTOCOL);
	return;
      }

    BPClientProtocolData& clientProt = runtime.clientProtocols [pReq->protocolId];

    // Perform action under lock
    BPShmControlSegment& CS = *(clientProt.pSCS);
    {
      //ScopedShmControlSegmentLock shmlock (CS);
      action (runtime, clientProt, CS);
    }
  }
  
  // ------------------------------------------------------------------

  void handleBPClearBuffer_Request (BPRuntimeData& runtime, byte* buffer, size_t nbytes)
  {
    BPClearBuffer_Confirm conf;

    std::function<void (BPRuntimeData&, BPClientProtocolData&, BPShmControlSegment&)> action_fn =
      [&] (BPRuntimeData& runtime, BPClientProtocolData& clientProt, BPShmControlSegment& CS)
	{    
	  switch(clientProt.static_info.queueingMode)
	    {
	    case BP_QMODE_ONCE:
	    case BP_QMODE_REPEAT:
	      {
		CS.buffer.reset ();
		//clientProt.bufferOccupied = false;
		send_simple_confirmation<BPClearBuffer_Confirm> (runtime, BP_STATUS_OK);
		return;
	      }
	    case BP_QMODE_QUEUE_DROPHEAD:
	    case BP_QMODE_QUEUE_DROPTAIL:
	      {
		CS.queue.reset ();
		send_simple_confirmation<BPClearBuffer_Confirm> (runtime, BP_STATUS_OK);
		return;
	      }
	    default:
	      {
		BOOST_LOG_SEV(log_mgmt_command, trivial::fatal)
	            << "Processing BPClearBuffer request: unknown queueing mode = "
   	            << clientProt.static_info.queueingMode;
		   ;
   	        runtime.bp_exitFlag = true;
		send_simple_confirmation<BPClearBuffer_Confirm> (runtime, BP_STATUS_INTERNAL_ERROR);
		return;
	      }
	    }
	};
    
    handleRegularRequestUsingSharedMemory<BPClearBuffer_Request, BPClearBuffer_Confirm> (runtime, buffer, nbytes, "BPClearBufferPayloads", action_fn);
    
  }
  
  // ------------------------------------------------------------------

  void handleBPQueryNumberBufferedPayloads_Request (BPRuntimeData& runtime, byte* buffer, size_t nbytes)
  {
    BPQueryNumberBufferedPayloads_Confirm conf;
    
    std::function<void (BPRuntimeData&, BPClientProtocolData&, BPShmControlSegment&)> action_fn =
      [&] (BPRuntimeData& runtime, BPClientProtocolData& clientProt, BPShmControlSegment& CS)
	{
	  switch(clientProt.static_info.queueingMode)
	    {
	    case BP_QMODE_ONCE:
	    case BP_QMODE_REPEAT:
	      {
		//conf.num_payloads_buffered = clientProt.bufferOccupied ? 1 : 0;
		conf.num_payloads_buffered = clientProt.pSCS->buffer.stored_elements ();
		runtime.commandSocket.send_raw_confirmation (log_mgmt_command, conf, sizeof(conf), runtime.bp_exitFlag);
		return;
	      }
	    case BP_QMODE_QUEUE_DROPHEAD:
	    case BP_QMODE_QUEUE_DROPTAIL:
	      {
		conf.num_payloads_buffered = CS.queue.stored_elements();
		runtime.commandSocket.send_raw_confirmation (log_mgmt_command, conf, sizeof(conf), runtime.bp_exitFlag);
		return;
	      }
	    default:
	      {
		BOOST_LOG_SEV(log_mgmt_command, trivial::fatal)
	            << "Processing BPQueryNumberBufferedPayloads request: unknown queueing mode = "
   	            << clientProt.static_info.queueingMode;
		   ;
		runtime.bp_exitFlag = true;
		send_simple_confirmation<BPQueryNumberBufferedPayloads_Confirm> (runtime, BP_STATUS_INTERNAL_ERROR);
		return;
	      }
	    }
	  return;
	};

    handleRegularRequestUsingSharedMemory<BPQueryNumberBufferedPayloads_Request, BPQueryNumberBufferedPayloads_Confirm> (runtime, buffer, nbytes, "BPQueryNumberBufferedPayloads", action_fn);
  }

  // ------------------------------------------------------------------

  void handle_command_socket (BPRuntimeData& runtime)
  {
    DcpServiceType serv_type;
    byte buffer [command_sock_buffer_size];
    int nbytes      = runtime.commandSocket.start_read_command (log_mgmt_command, buffer, command_sock_buffer_size, serv_type, runtime.bp_exitFlag);

    if (nbytes <= 0) return;
    
    BOOST_LOG_SEV(log_mgmt_command, trivial::info) << "Command loop: service type is " << bp_service_type_to_string(serv_type);
    switch (serv_type)
      {
	
      case stBP_RegisterProtocol:
	runtime.clientProtocols_mutex.lock();
	handleBPRegisterProtocol_Request (runtime, buffer, nbytes);
	runtime.clientProtocols_mutex.unlock();
	break;
	
      case stBP_DeregisterProtocol:
	runtime.clientProtocols_mutex.lock();
	handleBPDeregisterProtocol_Request (runtime, buffer, nbytes);
	runtime.clientProtocols_mutex.unlock();
	break;
	
      case stBP_ListRegisteredProtocols:
	runtime.clientProtocols_mutex.lock();
	handleBPListRegisteredProtocols_Request (runtime, buffer, nbytes);
	runtime.clientProtocols_mutex.unlock();
	break;
	
      case stBP_ShutDown:
	handleBPShutDown_Request (runtime, buffer, nbytes);
	break;
	
      case stBP_Activate:
	handleBPActivate_Request (runtime, buffer, nbytes);
	break;
	
      case stBP_Deactivate:
	handleBPDeactivate_Request (runtime, buffer, nbytes);
	break;

      case stBP_GetStatistics:
	handleBPGetStatistics_Request (runtime, buffer, nbytes);
	break;
	
      case stBP_ClearBuffer:
	runtime.clientProtocols_mutex.lock();
	handleBPClearBuffer_Request (runtime, buffer, nbytes);
	runtime.clientProtocols_mutex.unlock();
	break;

      case stBP_QueryNumberBufferedPayloads:
	runtime.clientProtocols_mutex.lock();
	handleBPQueryNumberBufferedPayloads_Request (runtime, buffer, nbytes);
	runtime.clientProtocols_mutex.unlock();
	break;
	
      default:
	BOOST_LOG_SEV(log_mgmt_command, trivial::fatal) << "Command loop: unknown service type, val = " << serv_type;
	runtime.bp_exitFlag = true;
	
      }
    runtime.commandSocket.stop_read_command (log_mgmt_command, runtime.bp_exitFlag);
  }



  // ------------------------------------------------------------------

  void management_thread_command (BPRuntimeData& runtime)
  {
    BOOST_LOG_SEV(log_mgmt_command, trivial::info) << "Starting command socket thread.";
    try {      
      runtime.commandSocket.open_owner (log_mgmt_command);
    }
    catch (DcpException& e) {
      BOOST_LOG_SEV(log_mgmt_command, trivial::fatal)
	<< "Could not establish BP command socket. "
	<< "Exception type: " << e.ename()
	<< ", module: " << e.modname()
	<< ", message: " << e.what()
	<< "Exiting.";
      runtime.bp_exitFlag = true;
      return;
    }
    catch (std::exception& e) {
      BOOST_LOG_SEV(log_mgmt_command, trivial::fatal) << "Could not establish BP command socket, exiting.";
      runtime.bp_exitFlag = true;
      return;
    }
    
    BOOST_LOG_SEV(log_mgmt_command, trivial::info) << "Established BP command socket, starting to wait on commands";
    
    while (not runtime.bp_exitFlag)
      {
	handle_command_socket (runtime);
      }
    
    runtime.commandSocket.close_owner ();

    BOOST_LOG_SEV(log_mgmt_command, trivial::info) << "Stopping command socket thread.";
  }
  
};  // namespace dcp::bp
