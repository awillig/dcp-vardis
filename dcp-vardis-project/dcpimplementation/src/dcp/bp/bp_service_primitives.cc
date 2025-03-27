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


#include <dcp/bp/bp_service_primitives.h>
#include <dcp/bp/bp_queueing_mode.h>


namespace dcp::bp {

  std::ostream& operator<<(std::ostream& os, const BPRegisterProtocol_Request& req)
  {
    os << "BPRegisterProtocol_Request{s_type = " << bp_service_type_to_string(req.s_type)
       << ", static_client_info=" << req.static_info
       << ", generatePayloadConfirms=" << req.generateTransmitPayloadConfirms
       << ", shm_area_name=" << req.shm_area_name
       << "}";
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os, const BPRegisterProtocol_Confirm& conf)
  {
    os << "BPRegisterProtocol_Confirm{s_type = " << bp_service_type_to_string (conf.s_type)
       << ", status_code = " << bp_status_to_string (conf.status_code)
       << ", ownNodeIdentifier = " << conf.ownNodeIdentifier
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPDeregisterProtocol_Request& req)
  {
    os << "BPDeregisterProtocol_Request{s_type = " << bp_service_type_to_string(req.s_type)
       << ", protocolId=" << req.protocolId
       << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPDeregisterProtocol_Confirm& conf)
  {
    os << "BPDeregisterProtocol_Confirm{s_type = " << bp_service_type_to_string (conf.s_type)
       << ", status_code = " << bp_status_to_string (conf.status_code)
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPListRegisteredProtocols_Request& req)
  {
    os << "BPListRegisteredProtocols_Request{s_type = " << bp_service_type_to_string(req.s_type)
       << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPRegisteredProtocolDataDescription& descr)
  {
    os << "BPRegisteredProtocolDataDescription{protocolId = " << descr.protocolId
       << ", protocolName = " << descr.protocolName
       << ", maxPayloadSize = " << descr.maxPayloadSize
       << ", queueingMode = " << bp_queueing_mode_to_string (descr.queueingMode)
       << ", timeStampRegistration = " << descr.timeStampRegistration
       << ", maxEntries = " << descr.maxEntries
       << ", allowMultiplePayloads = " << descr.allowMultiplePayloads
       << ", cntOutgoingPayloads = " << descr.cntOutgoingPayloads
       << ", cntReceivedPayloads = " << descr.cntReceivedPayloads
       << ", cntDroppedOutgoingPayloads = " << descr.cntDroppedOutgoingPayloads
       << ", cntDroppedIncomingPayloads = " << descr.cntDroppedIncomingPayloads
       << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPListRegisteredProtocols_Confirm& conf)
  {
    os << "BPListRegisteredProtocols_Confirm{s_type = " << bp_service_type_to_string (conf.s_type)
       << ", status_code = " << bp_status_to_string (conf.status_code)
       << ", numberProtocols = " << conf.numberProtocols
       << ", bpIsActive = " << conf.bpIsActive
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPReceivePayload_Indication& ind)
  {
    os << "BPReceivePayload_Indication{s_type = " << bp_service_type_to_string (ind.s_type)
       << ", length = " << (int) ind.length.val
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPTransmitPayload_Request& req)
  {
    os << "BPTransmitPayload_Request{s_type = " << bp_service_type_to_string(req.s_type)
       << ", protocolId = " << (int) req.protocolId.val
       << ", length = " << (int) req.length.val
       << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPTransmitPayload_Confirm& conf)
  {
    os << "BPTransmitPayload_Confirm{s_type = " << bp_service_type_to_string (conf.s_type)
       << ", status_code = " << bp_status_to_string (conf.status_code)
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPClearBuffer_Request& req)
  {
    os << "BPClearBuffer_Request{s_type = " << bp_service_type_to_string(req.s_type)
       << ", protocolId = " << (int) req.protocolId.val
       << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPClearBuffer_Confirm& conf)
  {
    os << "BPClearBuffer_Confirm{s_type = " << bp_service_type_to_string (conf.s_type)
       << ", status_code = " << bp_status_to_string (conf.status_code)
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPQueryNumberBufferedPayloads_Request& req)
  {
    os << "BPQueryNumberBufferedPayloads_Request{s_type = " << bp_service_type_to_string(req.s_type)
       << ", protocolId = " << (int) req.protocolId.val
       << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPQueryNumberBufferedPayloads_Confirm& conf)
  {
    os << "BPQueryNumberBufferedPayloads_Confirm{s_type = " << bp_service_type_to_string (conf.s_type)
       << ", status_code = " << bp_status_to_string (conf.status_code)
       << ", num_payloads_buffered = " << conf.num_payloads_buffered
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPShutDown_Request& req)
  {
    os << "BPShutDown_Request{s_type = " << bp_service_type_to_string(req.s_type)
       << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPActivate_Request& req)
  {
    os << "BPActivate_Request{s_type = " << bp_service_type_to_string(req.s_type)
       << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPActivate_Confirm& conf)
  {
    os << "BPActivate_Confirm{s_type = " << bp_service_type_to_string (conf.s_type)
       << ", status_code = " << bp_status_to_string (conf.status_code)
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPDeactivate_Request& req)
  {
    os << "BPDeactivate_Request{s_type = " << bp_service_type_to_string(req.s_type)
       << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPDeactivate_Confirm& conf)
  {
    os << "BPDeactivate_Confirm{s_type = " << bp_service_type_to_string (conf.s_type)
       << ", status_code = " << bp_status_to_string (conf.status_code)
       << " }";
    return os;
  }





  std::ostream& operator<<(std::ostream& os, const BPGetStatistics_Request& req)
  {
    os << "BPGetStatistics_Request{s_type = " << bp_service_type_to_string(req.s_type)
       << "}";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const BPGetStatistics_Confirm& conf)
  {
    os << "BPGetStatistics_Confirm{s_type = " << bp_service_type_to_string (conf.s_type)
       << ", status_code = " << bp_status_to_string (conf.status_code)
       << ", avg_inter_beacon_time = " << conf.avg_inter_beacon_time
       << ", avg_beacon_size = " << conf.avg_beacon_size
       << " }";
    return os;
  }


  
  
};  // namespace dcp::bp
