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


#pragma once

#include <exception>
#include <dcp/bp/bp_runtime_data.h>

namespace dcp::bp {

  /**
   * @brief Start receiver thread (receiving payloads and storing them
   *        in the appropriate shared memory area), run it until
   *        exitFlag is set
   *
   * ISSUE: This module builds on libpcap for receiving packets
   *        (through libtins), and libpcap under Linux can hang when
   *        no packet is available (despite setting a timeout). This
   *        means that this thread may hang indefinitely when no
   *        packets come in.
   */
  void receiver_thread (BPRuntimeData& runtime);
  
};  // namespace dcp::bp
