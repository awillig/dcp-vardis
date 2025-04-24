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

import dcpsim.common.GlobalTypes;

cplusplus {{
#include <dcp/vardis/vardis_transmissible_types.h>
#include <dcp/vardis/vardis_service_primitives.h>
typedef dcp::vardis::VarIdT      VarIdT;
typedef dcp::vardis::VarRepCntT  VarRepCntT;
typedef dcp::vardis::VarSeqnoT   VarSeqnoT;
typedef dcp::vardis::VarValueT   VarValueT;
typedef dcp::vardis::VarSpecT    VarSpecT;
typedef dcp::vardis::DescribeDatabaseVariableDescription DescribeDatabaseVariableDescription;
}}

class VarIdT {
    @existingClass;
}

class VarRepCntT {
    @existingClass;
}

class VarSeqnoT {
    @existingClass;
}

class VarValueT {
    @existingClass;
}

class VarSpecT {
    @existingClass;
}

class DescribeDatabaseVariableDescription {
    @existingClass;
}