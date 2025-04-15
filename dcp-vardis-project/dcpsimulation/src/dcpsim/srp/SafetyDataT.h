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

#include <inet/linklayer/common/MacAddress.h>
#include <dcpsim/common/TransmissibleType.h>
#include <dcpsim/common/DcpTypesGlobals.h>

using namespace omnetpp;

namespace dcp {

class SafetyDataT : public TransmissibleType <6*sizeof(double)> {
public:
    double  position_x;
    double  position_y;
    double  position_z;
    double  velocity_x;
    double  velocity_y;
    double  velocity_z;

    virtual void serialize (AssemblyArea& area)
    {
        area.serialize_byte_block (sizeof(double), (byte*) &position_x);
        area.serialize_byte_block (sizeof(double), (byte*) &position_y);
        area.serialize_byte_block (sizeof(double), (byte*) &position_z);
        area.serialize_byte_block (sizeof(double), (byte*) &velocity_x);
        area.serialize_byte_block (sizeof(double), (byte*) &velocity_y);
        area.serialize_byte_block (sizeof(double), (byte*) &velocity_z);
    };
    virtual void deserialize (DisassemblyArea& area)
    {
        area.deserialize_byte_block (sizeof(double), (byte*) &position_x);
        area.deserialize_byte_block (sizeof(double), (byte*) &position_y);
        area.deserialize_byte_block (sizeof(double), (byte*) &position_z);
        area.deserialize_byte_block (sizeof(double), (byte*) &velocity_x);
        area.deserialize_byte_block (sizeof(double), (byte*) &velocity_y);
        area.deserialize_byte_block (sizeof(double), (byte*) &velocity_z);
    };

};


class ExtendedSafetyDataT : public TransmissibleType<6*sizeof(double)+MAC_ADDRESS_SIZE+sizeof(simtime_t)+sizeof(uint32_t)> {
public:
    SafetyDataT       safetyData;
    NodeIdentifierT   nodeId;
    simtime_t         timeStamp;
    uint32_t          seqno;

    virtual void serialize (AssemblyArea& area)
    {
        safetyData.serialize(area);
        nodeId.serialize(area);
        area.serialize_byte_block(sizeof(simtime_t), (byte*) &timeStamp);
        area.serialize_byte_block(sizeof(uint32_t), (byte*) &seqno);
    };
    virtual void deserialize (DisassemblyArea& area)
    {
        safetyData.deserialize(area);
        nodeId.deserialize(area);
        area.deserialize_byte_block(sizeof(simtime_t), (byte*) &timeStamp);
        area.deserialize_byte_block(sizeof(uint32_t), (byte*) &seqno);
    };

};


};  // namespace dcp
