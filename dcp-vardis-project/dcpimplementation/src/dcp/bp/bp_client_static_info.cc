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


#include <dcp/bp/bp_client_static_info.h>


namespace dcp::bp {

  std::ostream& operator<<(std::ostream& os, const BPStaticClientInfo& sci)
  {
    os << "BPStaticClientInfo{protocolId=" << sci.protocolId
       << ", protocolName=" << sci.protocolName
       << ", maxPayloadSize=" << sci.maxPayloadSize
       << ", queueingMode=" << bp_queueing_mode_to_string (sci.queueingMode)
       << ", maxEntries=" << sci.maxEntries
       << ", allowMultiplePayloads=" << (sci.allowMultiplePayloads ? "true" : "false")
       << "}";
    return os;
  } 
   
};  // namespace dcp::bp
