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

#include <dcp/common/transmissible_type.h>
#include <dcp/common/area.h>
#include <dcp/common/global_types_constants.h>


namespace dcp::srp {

  class SafetyDataT : public TransmissibleType <3*sizeof(double)> {
  public:
    double  position_x;
    double  position_y;
    double  position_z;
    
    virtual void serialize (AssemblyArea& area)
    {
      area.serialize_byte_block (sizeof(double), (byte*) &position_x);
      area.serialize_byte_block (sizeof(double), (byte*) &position_y);
      area.serialize_byte_block (sizeof(double), (byte*) &position_z);
    };
    virtual void deserialize (DisassemblyArea& area)
    {
      area.deserialize_byte_block (sizeof(double), (byte*) &position_x);
      area.deserialize_byte_block (sizeof(double), (byte*) &position_y);
      area.deserialize_byte_block (sizeof(double), (byte*) &position_z);
    };

    friend std::ostream& operator<<(std::ostream& os, const SafetyDataT& sd);
  };
  
  
  class ExtendedSafetyDataT : public TransmissibleType<SafetyDataT::fixed_size()
						       + NodeIdentifierT::fixed_size()
						       + TimeStampT::fixed_size()
						       + sizeof(uint32_t)> {
  public:
    SafetyDataT       safetyData;
    NodeIdentifierT   nodeId;
    TimeStampT        timeStamp;
    uint32_t          seqno;
    
    virtual void serialize (AssemblyArea& area)
    {
      safetyData.serialize (area);
      nodeId.serialize (area);
      timeStamp.serialize (area);
      area.serialize_byte_block(sizeof(uint32_t), (byte*) &seqno);
    };
    virtual void deserialize (DisassemblyArea& area)
    {
      safetyData.deserialize(area);
      nodeId.deserialize(area);
      timeStamp.deserialize (area);
      area.deserialize_byte_block(sizeof(uint32_t), (byte*) &seqno);
    };

    friend std::ostream& operator<<(std::ostream& os, const ExtendedSafetyDataT& esd);
    
  };

  
};  // namespace dcp::srp
