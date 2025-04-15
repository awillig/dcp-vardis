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

#include <inet/common/packet/Message.h>
#include <inet/common/packet/Packet.h>
#include <dcpsim/common/DcpApplication.h>
#include <dcpsim/common/DcpTypesGlobals.h>
#include <dcpsim/srp/SRPUpdateSafetyData_m.h>

// --------------------------------------------------------------------------

namespace dcp {

/**
 * This module implements basic functionalities that any SRP application
 * should have, and any SRP application (protocol) should inherit from
 * this class. It mainly makes sure that the application module and SRP
 * can talk to each other via an INET message dispatcher.
 */

class SRPApplication : public DcpApplication {

public:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

protected:

    // --------------------------------------------
    // helpers
    // --------------------------------------------

    /**
     * Translate SRP status value into string
     */
    std::string getSRPStatusString(SRPStatus status);

    /**
     * Prints received SRP status as string to logging output
     */
    void printStatus (SRPStatus status);

    /**
     * Default reaction to any received status value from a SRP confirm
     * primitive. This just calls printStatus.
     */
    virtual void handleSRPConfirmation(SRPUpdateSafetyData_Confirm *conf);

    /**
     * Sending messages or packets down to SRP
     */
    void sendToSRP (Message* message);
    void sendToSRP (Packet* packet);

};


} //namespace

