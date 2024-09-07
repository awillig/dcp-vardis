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


enum VardisStatus
{
    VARDIS_STATUS_OK                              =  0;
    VARDIS_STATUS_VARIABLE_EXISTS                 =  1;
    VARDIS_STATUS_VARIABLE_DESCRIPTION_TOO_LONG   =  2;
    VARDIS_STATUS_VALUE_TOO_LONG                  =  3;
    VARDIS_STATUS_INVALID_VALUE                   =  4;
    VARDIS_STATUS_ILLEGAL_REPCOUNT                =  5;
	VARDIS_STATUS_VARIABLE_DOES_NOT_EXIST         =  6;
	VARDIS_STATUS_NOT_PRODUCER                    =  7;
	VARDIS_STATUS_VARIABLE_BEING_DELETED          =  8;
	VARDIS_STATUS_INACTIVE                        =  9;
};