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


enum BPStatus
{
    BP_STATUS_OK                            =  0;
    BP_STATUS_PROTOCOL_ALREADY_REGISTERED   =  1;
    BP_STATUS_ILLEGAL_MAX_PAYLOAD_SIZE      =  2;
    BP_STATUS_UNKNOWN_PROTOCOL              =  3;
    BP_STATUS_PAYLOAD_TOO_LARGE             =  4;
    BP_STATUS_EMPTY_PAYLOAD                 =  5;
    BP_STATUS_ILLEGAL_DROPTAIL_QUEUE_SIZE   =  6;
    BP_STATUS_DROPTAIL_QUEUE_FULL           =  7;
};
