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


#include <dcp/vardis/vardis_transmissible_types.h>

namespace dcp::vardis {

  std::ostream& operator<< (std::ostream& os, const VarIdT& vid)
  {
    os << (int) vid.val;
    return os;
  }

  std::ostream& operator<< (std::ostream& os, const VarLenT& vlen)
  {
    os << (int) vlen.val;
    return os;
  }

  std::ostream& operator<< (std::ostream& os, const VarRepCntT& vrc)
  {
    os << (int) vrc.val;
    return os;
  }

  std::ostream& operator<< (std::ostream& os, const VarSeqnoT& vs)
  {
    os << (int) vs.val;
    return os;
  }

  // ------------------------------------------
  
  std::ostream& operator<<(std::ostream& os, const VarValueT& vv)
  {
    os << "VarValueT { length = " << vv.length << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VarSummT& vs)
  {
    os << "VarSummT { varId = " << vs.varId
       << " , seqno = " << vs.seqno
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VarUpdateT& vu)
  {
    os << "VarUpdateT { varId = " << vu.varId
       << " , seqno = " << vu.seqno
       << " , value = " << vu.value
       << " }";
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os, const VarSpecT& vs)
  {
    os << "VarSpecT { varId = " << vs.varId
       << " , prodId = " << vs.prodId
       << " , repCnt = " << vs.repCnt
       << " , descr = " << vs.descr
       << " }";
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os, const VarCreateT& vc)
  {
    os << "VarCreateT { spec = " << vc.spec
       << " , update = " << vc.update
       << " }";
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os, const VarDeleteT& vd)
  {
    os << "VarDeleteT { varId = " << vd.varId
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VarReqUpdateT& vru)
  {
    os << "VarReqUpdateT { updSpec = " << vru.updSpec
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const VarReqCreateT& vrc)
  {
    os << "VarReqCreateT { varId = " << vrc.varId
       << " }";
    return os;
  }

  std::ostream& operator<<(std::ostream& os, const ICHeaderT& ich)
  {
    os << "ICHeaderT { icType = " << vardis_instruction_container_to_string (ich.icType)
       << " , icNumRecords = " << (int) ich.icNumRecords
       << " }";
    return os;
  }

  std::string vardis_instruction_container_to_string (InstructionContainerT ic)
  {
    switch (ic.val)
      {
      case  ICTYPE_SUMMARIES:           return "ICTYPE_SUMMARIES";
      case  ICTYPE_UPDATES:             return "ICTYPE_UPDATES";
      case  ICTYPE_REQUEST_VARUPDATES:  return "ICTYPE_REQUEST_VARUPDATES";
      case  ICTYPE_REQUEST_VARCREATES:  return "ICTYPE_REQUEST_VARCREATES";
      case  ICTYPE_CREATE_VARIABLES:    return "ICTYPE_CREATE_VARIABLES";
      case  ICTYPE_DELETE_VARIABLES:    return "ICTYPE_DELETE_VARIABLES";
      
      default:
	throw std::invalid_argument("vardis_status_to_string: illegal status code");
      }
    return "";
  }

  std::ostream& operator<< (std::ostream& os, const InstructionContainerT ic)
  {
    os << "InstructionContainerT { val = " << vardis_instruction_container_to_string (ic.val) << " }";
    return os;
  }
    
};  // namespace dcp::vardis
