// Copyright (C) 2024 Andreas Willig, University of Canterbury
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#pragma once

namespace dcp {

typedef uint16_t      BPProtocolIdT;     // Identifier for client protocols
typedef uint16_t      BPLengthT;         // length of a payload block



// Pre-defined protocol id's
const BPProtocolIdT BP_PROTID_SRP     =  0x0001;
const BPProtocolIdT BP_PROTID_VARDIS  =  0x0002;


}; // namespace dcp
