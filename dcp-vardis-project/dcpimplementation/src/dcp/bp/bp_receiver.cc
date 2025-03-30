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


#include <exception>
#include <tins/tins.h>
#include <dcp/common/area.h>
#include <dcp/common/debug_helpers.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/bp/bp_receiver.h>
#include <dcp/bp/bp_transmissible_types.h>
#include <dcp/bp/bp_logging.h>
#include <dcp/bp/bp_client_protocol_data.h>
#include <dcp/bp/bp_service_primitives.h>

using namespace Tins;

namespace dcp::bp {

  // ------------------------------------------------------------------
  
  void deliver_payload (BPRuntimeData& runtime, DisassemblyArea& area, const BPPayloadHeaderT& pldHdr)
  {
    if (not runtime.clientProtocols.contains(pldHdr.protocolId))
      {
	BOOST_LOG_SEV(log_rx, trivial::info)
	  << "deliver_payload: payload for unregistered protocol, dropping payload.";
	
	area.incr (pldHdr.length.val);  // skip actual payload
	return;
      }
    
    BPClientProtocolData& clientProt = runtime.clientProtocols[pldHdr.protocolId];
    clientProt.cntReceivedPayloads += 1;
    
    if (clientProt.static_info.protocolId != pldHdr.protocolId)
      {
	BOOST_LOG_SEV(log_rx, trivial::error)
	  << "deliver_payload: found internal consistency around protocol identifiers, dropping payload.";
	
	area.incr (pldHdr.length.val);
	clientProt.cntDroppedIncomingPayloads += 1;
	return;
      }

    // now we start working in the shared memory area proper
    BPShmControlSegment& CS = *(clientProt.pSCS);
    
    if (pldHdr.length.val + sizeof(BPReceivePayload_Indication) > CS.pqReceivePayloadIndication.get_buffer_size())
      {
	BOOST_LOG_SEV(log_rx, trivial::error)
	  << "deliver_payload: shared memory buffer is too small, dropping payload."
	  << " pldHdr.length = " << pldHdr.length
	  << ", sizeof(BPReceivePayloadIndication) = " << sizeof (BPReceivePayload_Indication)
	  << ", buffer_size = " << CS.pqReceivePayloadIndication.get_buffer_size();
	
	area.incr (pldHdr.length.val);
	clientProt.cntDroppedIncomingPayloads += 1;
	return;
      }



    PushHandler handler = [&] (byte* memaddr, size_t)
    {
      BPReceivePayload_Indication pldIndication;
      pldIndication.length = pldHdr.length;
      std::memcpy (memaddr, (void*) &pldIndication, sizeof(pldIndication));
      area.deserialize_byte_block (pldHdr.length.val, memaddr + sizeof(pldIndication));

      BOOST_LOG_SEV(log_rx, trivial::trace)
	<< "deliver_payload::handler: pldHdr.length = " << pldHdr.length
	<< ", return size = " << sizeof(pldIndication) + pldHdr.length.val
	<< ", memaddr contents [PldInd+pld] = " << byte_array_to_string (memaddr, 40)
	;
      
      return (sizeof(pldIndication) + pldHdr.length.val);
    };
    bool timed_out;

    // now try to push received payload into indication queue, but
    // with only a short timeout, and dropping payload after timeout
    // expiry
    const uint16_t shortTimeoutMS = 5;
    CS.pqReceivePayloadIndication.push_wait (handler, timed_out, shortTimeoutMS);
    if (timed_out)
      {
	BOOST_LOG_SEV(log_rx, trivial::info)
	  << "deliver_payload: no free buffer available in shared memory, dropping payload.";
	area.incr (pldHdr.length.val);
	clientProt.cntDroppedIncomingPayloads += 1;
      }
    else
      {
	BOOST_LOG_SEV(log_rx, trivial::trace)
	  << "pushed payload of length " << pldHdr.length << " into pqReceivePayloadIndication"
	  << ", queue occupancy is " << CS.report_stored_buffers();
      }    
  }
  
  // ------------------------------------------------------------------

  void process_received_payload (BPRuntimeData& runtime, DisassemblyArea& area)
  {
    BPHeaderT bpHdr;

    if (area.available() <= dcp::bp::BPHeaderT::fixed_size())
      {
	BOOST_LOG_SEV(log_rx, trivial::error) << "process_received_payload: insufficient length to accommodate BPHeaderT, no further processing";
	return;
      }

    bpHdr.deserialize (area);
    if (not bpHdr.isWellFormed (runtime.ownNodeIdentifier))
      {
	BOOST_LOG_SEV(log_rx, trivial::trace) << "process_received_payload: malformed BPHeaderT, no further processing. Header is " << bpHdr;
	return;
      }

    BOOST_LOG_SEV(log_rx, trivial::trace) << "process_received_payload: got packet with valid BPHeaderT, senderId = " << bpHdr.senderId
					  << ", length = " << bpHdr.length
					  << ", numPayloads = " << (int) bpHdr.numPayloads
					  << ", seqno = " << bpHdr.seqno;
    
    uint8_t     numberPayloads = bpHdr.numPayloads;
    BPLengthT   pldLength      = bpHdr.length;

    if (pldLength.val > area.available())
      {
	BOOST_LOG_SEV(log_rx, trivial::fatal) << "process_received_payload: BPHeaderT.length is larger than payload length, BPHeaderT.length = " << bpHdr.length
					      << ", payload length = " << area.available();
	runtime.bp_exitFlag = true;
	return;
      }

    if (pldLength.val < area.available())
      {
	BOOST_LOG_SEV(log_rx, trivial::trace) << "process_received_payload: area is larger than payload length, re-sizing. pldLength = " << pldLength
					      << ", area length = " << area.available();
	area.resize (BPHeaderT::fixed_size() + pldLength.val);
      }

    for (uint8_t cntPayload = 0; cntPayload < numberPayloads; cntPayload++)
      {
	BPPayloadHeaderT pldHdr;

	if (area.available() < dcp::bp::BPPayloadHeaderT::fixed_size())
	  {
	    BOOST_LOG_SEV(log_rx, trivial::info) << "process_received_payload: insufficient length to accommodate BPPayloadHeaderT, no further processing";
	    return;
	  }
	
	pldHdr.deserialize(area);

	BOOST_LOG_SEV(log_rx, trivial::trace) << "process_received_payload: payload header is " << pldHdr;
	
	if (area.available() < pldHdr.length.val)
	  {
	    BOOST_LOG_SEV(log_rx, trivial::info) << "process_received_payload: insufficient length to retrieve payload, no further processing";
	    return;
	  }

	runtime.clientProtocols_mutex.lock();
	deliver_payload (runtime, area, pldHdr);
	runtime.clientProtocols_mutex.unlock();
	
      }
    
  }

  // ------------------------------------------------------------------

  void receiver_thread (BPRuntimeData& runtime)
  {
    BOOST_LOG_SEV(log_rx, trivial::info) << "Starting receiver thread.";

    SnifferConfiguration sniff_config;
    Sniffer*   pSniffer = nullptr;
    double     bcnSizeAlpha = runtime.bp_config.bp_conf.beaconSizeEWMAAlpha;
    double     ibTimeAlpha  = runtime.bp_config.bp_conf.interBeaconTimeEWMAAlpha;
    TimeStampT last_beacon_reception_time;
    
    try {
      sniff_config.set_promisc_mode (true);
      sniff_config.set_snap_len (runtime.bp_config.bp_conf.mtuSize + 256);
      std::stringstream filt_ss;
      filt_ss << "ether dst ff:ff:ff:ff:ff:ff and ether proto " << runtime.bp_config.bp_conf.etherType;
      sniff_config.set_filter (filt_ss.str());
      sniff_config.set_immediate_mode (true);
      sniff_config.set_timeout (defaultPacketSnifferTimeoutMS);

      pSniffer = new Sniffer (runtime.bp_config.bp_conf.interfaceName, sniff_config);
    }
    catch (std::exception& e) {
      BOOST_LOG_SEV(log_rx, trivial::fatal) << "Could not listen on network interface. Wrong interface or permissions missing? Caught exception " << e.what() << ". exiting.";
      runtime.bp_exitFlag = true;
      return;
    }

    while ((not runtime.bp_exitFlag) && pSniffer)
      {
	PDU* rx_pdu = pSniffer->next_packet();

	if (rx_pdu)
	  {
	    const EthernetII& eth_frame = rx_pdu->rfind_pdu<EthernetII>();

	    BOOST_LOG_SEV(log_rx, trivial::trace)
	      << "Got frame with srcaddr = " << eth_frame.src_addr()
	      << ", dstaddr = " << eth_frame.dst_addr()
	      << ", payload-type = " << eth_frame.payload_type()
	      << ", size = " << eth_frame.size();
	    
	    
	    if ((eth_frame.dst_addr() == EthernetII::BROADCAST) && (eth_frame.payload_type() == runtime.bp_config.bp_conf.etherType))
	      {
		const RawPDU& raw_pdu = rx_pdu->rfind_pdu<RawPDU>();
		bytevect payload      = raw_pdu.payload();
		ByteVectorDisassemblyArea area ("bp-rx", payload);

		TimeStampT current_time = TimeStampT::get_current_system_time();
		auto ib_time = current_time.milliseconds_passed_since(last_beacon_reception_time);

		// update beacon size statistics
		if (runtime.cntBPPayloads == 0)
		  {
		    runtime.avg_received_beacon_size = (double) payload.size ();
		  }
		else
		  {
		    runtime.avg_received_beacon_size =
		      bcnSizeAlpha * runtime.avg_received_beacon_size
		      + (1 - bcnSizeAlpha) * ((double) payload.size());		    
		  }

		// update inter beacon time statistics
		if (runtime.cntBPPayloads == 1)
		  {
		    runtime.avg_inter_beacon_reception_time = (double) ib_time;
		  }
		else if (runtime.cntBPPayloads > 1)
		  {
		    runtime.avg_inter_beacon_reception_time =
		      ibTimeAlpha * runtime.avg_inter_beacon_reception_time
		      + (1 - ibTimeAlpha) * ((double) ib_time);
		  }
		
		last_beacon_reception_time = TimeStampT::get_current_system_time();
		runtime.cntBPPayloads++;

		BOOST_LOG_SEV(log_rx, trivial::trace) << "process_received_payload: avg inter beacon time (ms) = " << runtime.avg_inter_beacon_reception_time
						      << ", avg beacon size (B) = " << runtime.avg_received_beacon_size;

		if (runtime.bp_isActive)
		  {
		    process_received_payload (runtime, area);
		  }
		
	      }
	    
	    delete rx_pdu;
	  }
      }
    if (pSniffer) delete pSniffer;

    BOOST_LOG_SEV(log_rx, trivial::info) << "Stopping receiver thread.";
  }
  
  // ------------------------------------------------------------------
  
};  // namespace dcp::bp
