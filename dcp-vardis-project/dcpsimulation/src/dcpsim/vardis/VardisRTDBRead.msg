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
import VardisRTDBConfirmation;

cplusplus {{
#include <dcp/vardis/vardis_service_primitives.h>
typedef dcp::vardis::RTDB_Read_Request RTDB_Read_Request;
typedef dcp::vardis::RTDB_Read_Confirm RTDB_Read_Confirm;
}}

class RTDB_Read_Request {
    @existingClass;
}

class RTDB_Read_Confirm {
    @existingClass;
}

class RTDBRead_Request extends inet::Message
{
    RTDB_Read_Request readReq;
};


class RTDBRead_Confirm extends VardisConfirmation
{
    RTDB_Read_Confirm readConf;
}