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

#include <omnetpp.h>
#include <inet/mobility/contract/IMobility.h>
#include <dcp/srp/SRPClientProtocol.h>
#include <dcp/srp/SRPUpdateSafetyData_m.h>

using namespace omnetpp;
using namespace inet;

// ---------------------------------------------------------

namespace dcp {

class SRPApplication : public SRPClientProtocol {

public:
    virtual ~SRPApplication ();
    virtual void initialize (int stage) override;
    virtual void handleMessage (cMessage* msg) override;

protected:
    bool        isActive   = false;
    cMessage*   sampleMsg  = nullptr;

    int  gidToSRP;
    int  gidFromSRP;

    IMobility* _mobility   = nullptr;      // pointer to mobility model, needed to query position

    /**
     * Finds the pointer to the mobility model
     */
    void findModulePointers (void);


    void handleSampleMsg ();
    void handleSRPUpdateSafetyDataConfirm (SRPUpdateSafetyData_Confirm* srpConf);
};


};  // namespace dcp
