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


import inet.common.packet.Message;

cplusplus {{
#include <dcpsim/srp/SafetyDataT.h>
typedef dcp::SafetyDataT SafetyDataT;
}}

class SafetyDataT {
    @existingClass;
}


class SRPUpdateSafetyData_Request extends inet::Message
{
    SafetyDataT         safetyData;
}

enum SRPStatus
{
    SRP_STATUS_OK   =  0;
};


class SRPUpdateSafetyData_Confirm extends inet::Message
{
    SRPStatus   status;
}
