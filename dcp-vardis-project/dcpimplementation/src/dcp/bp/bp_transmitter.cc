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
#include <list>
#include <thread>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <tins/tins.h>
#include <dcp/common/area.h>
#include <dcp/common/memblock.h>
#include <dcp/bp/bp_client_protocol_data.h>
#include <dcp/bp/bp_logging.h>
#include <dcp/bp/bp_service_primitives.h>
#include <dcp/bp/bp_transmissible_types.h>
#include <dcp/bp/bp_transmitter.h>





using namespace Tins;


namespace dcp::bp {

  // ------------------------------------------------------------------

  void serialize_payload (BPClientProtocolData&  protEntry,
			  AssemblyArea&          area,
			  const byte*            memaddr,
			  BPLengthT              pld_len,
			  unsigned int&          numPayloadsAdded)
  {
    BOOST_LOG_SEV(log_tx, trivial::trace) << "serialize_payload: serializing payload for protocolId "
					  << protEntry.static_info.protocolId
					  << " of length " << pld_len;
    
    BPPayloadHeaderT pldHdr;
    pldHdr.protocolId = protEntry.static_info.protocolId;
    pldHdr.length     = pld_len;
    pldHdr.serialize (area);
    
    area.serialize_byte_block (pld_len.val, memaddr);

    numPayloadsAdded++;
    protEntry.cntOutgoingPayloads++;
  }
  
  // ------------------------------------------------------------------

  
  void attempt_add_payload (BPRuntimeData&          runtime,
			    BPClientProtocolData&   protEntry,
			    AssemblyArea&           area,
			    unsigned int&           numPayloadsAdded)
  {
    BPShmControlSegment& CS = *protEntry.pSCS;
    BPQueueingMode queueingMode = protEntry.static_info.queueingMode;
    bool timed_out;
    bool more_payloads;
    std::function<void (const byte*, size_t)> handler = [&] (const byte* memaddr, size_t len)
    {
      BPTransmitPayload_Request* pReq = (BPTransmitPayload_Request*) memaddr;

      if (len != sizeof(BPTransmitPayload_Request) + pReq->length)
	{
	  BOOST_LOG_SEV(log_tx, trivial::fatal)
	    << "attempt_add_payload::handler: incorrect length field"
	    << ", len = " << len
	    << ", skippable size = " << sizeof(BPTransmitPayload_Request)
	    << ", payload length = " << pReq->length
	    ;
	  runtime.bp_exitFlag = true;
	}
      else
	serialize_payload (protEntry, area, memaddr + sizeof(BPTransmitPayload_Request), pReq->length, numPayloadsAdded);
    };

    switch (queueingMode)
      {
      case BP_QMODE_QUEUE_DROPTAIL:
      case BP_QMODE_QUEUE_DROPHEAD:
	{
	  CS.queue.pop_nowait (handler, timed_out, more_payloads);
	  break;
	}
      case BP_QMODE_ONCE:
	{
	  CS.buffer.pop_nowait (handler, timed_out, more_payloads);
	  break;
	}
      case BP_QMODE_REPEAT:
	{
	  CS.buffer.peek_nowait (handler, timed_out);
	  break;
	}
      default:
	{
	  BOOST_LOG_SEV(log_tx, trivial::fatal) << "attempt_add_payload: unknown queueingMode " << (int) queueingMode;
	  runtime.bp_exitFlag = true;
	  return;
	}
      }

    if (timed_out)
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal) << "attempt_add_payload: timout when accessing payload in shared memory";
	runtime.bp_exitFlag = true;
      }
    return;
  }
			    
  
  // ------------------------------------------------------------------

  void generate_beacon (BPRuntimeData& runtime)
  {
    if (not runtime.bp_isActive)
      return;
    
    // first determine number and total size of available payloads without
    // yet moving them into a beacon. We use a very simple method allowing
    // only one payload from each client protocol.
    bytevect               bv_payload (runtime.bp_config.bp_conf.maxBeaconSize);
    ByteVectorAssemblyArea area ("bp-tx", runtime.bp_config.bp_conf.maxBeaconSize, bv_payload);
    unsigned int           numPayloadsAdded  = 0;

    // first serialize a dummy version of the BPHeaderT. We will re-do
    // this once we know the total amount of data
    BPHeaderT tmpBPHdr;
    tmpBPHdr.reserve (area);
    
    for (auto it = runtime.clientProtocols.begin(); it != runtime.clientProtocols.end(); ++it)
      {
	BPClientProtocolData& protEntry = it->second;
	attempt_add_payload (runtime, protEntry, area, numPayloadsAdded);

	if (runtime.bp_exitFlag)
	  return;
      }

    if (numPayloadsAdded > 0)
      {
	// resize byte vector to reflect actual size, and prepend
	// BPHeaderT (can to this only at the end since we know the
	// total length only now)
	bv_payload.resize(area.used());
	BPHeaderT bpHdr;
	bpHdr.version      =   bpHeaderVersion;
	bpHdr.magicNo      =   bpMagicNo;
	bpHdr.senderId     =   runtime.ownNodeIdentifier;
	bpHdr.length       =   area.used() - dcp::bp::BPHeaderT::fixed_size();
	bpHdr.numPayloads  =   numPayloadsAdded;
	bpHdr.seqno        =   runtime.bpSequenceNumber++;

	ByteVectorAssemblyArea tmpArea ("bp-tx-tmp", dcp::bp::BPHeaderT::fixed_size(), bv_payload);
	bpHdr.serialize (tmpArea);

	RawPDU payload_pdu (bv_payload);
	EthernetII ethpacket = EthernetII(EthernetII::BROADCAST, runtime.nw_if_info.hw_addr);
	ethpacket.payload_type (runtime.bp_config.bp_conf.etherType);
	ethpacket = ethpacket / payload_pdu;

	runtime.pktSender.send (ethpacket, runtime.bp_config.bp_conf.interfaceName);	  
      }
  }
  
  // ------------------------------------------------------------------

  void transmitter_thread (BPRuntimeData& runtime)
  {
    BOOST_LOG_SEV(log_tx, trivial::info) << "Starting transmit thread.";

    boost::random::mt19937 randgen;
    int lower_bound_int = (int) floor (runtime.bp_config.bp_conf.avgBeaconPeriodMS * (1 - runtime.bp_config.bp_conf.jitterFactor));
    int upper_bound_int = (int) floor (runtime.bp_config.bp_conf.avgBeaconPeriodMS * (1 + runtime.bp_config.bp_conf.jitterFactor));

    if (lower_bound_int <= 0)
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal) << "Average beacon period (ms) has been chosen too small ("
					      << runtime.bp_config.bp_conf.avgBeaconPeriodMS
					      << "), leaving.";

	runtime.bp_exitFlag = true;
	return;
      }

    unsigned int lower_bound = (unsigned int) lower_bound_int;
    unsigned int upper_bound = (unsigned int) upper_bound_int;

    boost::random::uniform_int_distribution<> dist (lower_bound, upper_bound); 

    try {
      while (not runtime.bp_exitFlag)
	{
	  unsigned int wait_time_ms = dist (randgen);
	  std::this_thread::sleep_for (std::chrono::milliseconds (wait_time_ms));
	  
	  runtime.clientProtocols_mutex.lock();
	  generate_beacon (runtime);
	  runtime.clientProtocols_mutex.unlock();
	}
    }
    catch (DcpException& e)
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal)
	  << "Caught DCP exception in BP transmitter main loop. "
	  << "Exception type: " << e.ename()
	  << ", module: " << e.modname()
	  << ", message: " << e.what()
	  << ". Exiting.";
	runtime.bp_exitFlag = true;
      }
    catch (std::exception& e)
      {
	BOOST_LOG_SEV(log_tx, trivial::fatal)
	  << "Caught other exception in BP transmitter main loop. "
	  << "Message: " << e.what()
	  << ". Exiting.";
	runtime.bp_exitFlag = true;
      }

    BOOST_LOG_SEV(log_tx, trivial::info) << "Stopping transmit thread.";
  }

  // ------------------------------------------------------------------
  
};  // namespace dcp::bp
