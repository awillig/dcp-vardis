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

#ifndef DCP_APPLICATIONS_VARDISVARIABLEEXAMPLE_H_
#define DCP_APPLICATIONS_VARDISVARIABLEEXAMPLE_H_

#include <omnetpp.h>


#pragma pack(push, 1)
typedef struct VardisExampleVariable {
    uint32_t     seqno;
    double       value;
    simtime_t    tstamp;
} VardisExampleVariable;
#pragma pack(pop)

#endif /* DCP_APPLICATIONS_VARDISVARIABLEEXAMPLE_H_ */