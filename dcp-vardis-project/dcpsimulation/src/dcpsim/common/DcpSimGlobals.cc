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

#include <omnetpp.h>
#include <inet/common/IProtocolRegistrationListener.h>
#include <dcpsim/common/DcpTypesGlobals.h>


namespace dcp {


std::string bv_to_str (const bytevect& bv, size_t numbytes)
{
    if (numbytes == 0) return std::string("");

    std::stringstream ss;
    ss << std::hex;

    ss << "{";
    for (unsigned int i=0; i<numbytes-1; i++)
        ss << std::setw(2) << std::setfill('0') << (int) bv[i] << ",";
    ss << std::setw(2) << std::setfill('0') << (int) bv[numbytes-1];
    ss << "}";

    return ss.str();
}


std::ostream& operator<<(std::ostream& os, const NodeIdentifierT& nodeid)
{
    os << nodeid.to_str();
    return os;
}


bool operator< (const NodeIdentifierT& left, const NodeIdentifierT& right)
{
    return (left.to_macaddr() < right.to_macaddr());
}


static DcpSimGlobals theSimulationGlobals;

Protocol* DcpSimGlobals::protocolDcpBP       = nullptr;
Protocol* DcpSimGlobals::protocolDcpSRP      = nullptr;
Protocol* DcpSimGlobals::protocolDcpVardis   = nullptr;


DcpSimGlobals::DcpSimGlobals()
{
    // note: do not treat protocolDcpBP here, this is done in BeaconingProtocol
    // due to issues with ProtocolGroup implementation
    // DcpSimGlobals::protocolDcpBP       = new Protocol ("dcp-bp", "DCP-BP");
    DcpSimGlobals::protocolDcpSRP      = new Protocol ("dcp-srp", "DCP State Reporting Protocol");
    DcpSimGlobals::protocolDcpVardis   = new Protocol ("dcp-vardis", "DCP Variable Dissemination Protocol");
}

DcpSimGlobals::~DcpSimGlobals()
{
    // delete DcpSimGlobals::protocolDcpBP;
    // don't delete the BP here, as it is oddly done in ProtocolGroup.cc (in the
    // destructor)

    delete DcpSimGlobals::protocolDcpSRP;
    delete DcpSimGlobals::protocolDcpVardis;
}



Protocol *convertProtocolIdToProtocol(BPProtocolIdT protId)
{
    switch(protId)
    {
    case BP_PROTID_SRP:     return DcpSimGlobals::protocolDcpSRP;
    case BP_PROTID_VARDIS:  return DcpSimGlobals::protocolDcpVardis;
    default:
        cSimpleModule cs;
        cs.error("convertProtocolIdToProtocol: unknown protocol id");
        return nullptr;
    }
}

};  // namespace dcp

