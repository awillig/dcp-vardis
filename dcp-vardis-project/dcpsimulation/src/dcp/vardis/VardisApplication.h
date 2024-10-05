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
#include <dcp/common/DcpApplication.h>
#include <dcp/common/DcpTypesGlobals.h>
#include <dcp/vardis/VardisStatus_m.h>
#include <dcp/vardis/VardisRTDBConfirmation_m.h>

// --------------------------------------------------------------------------

namespace dcp {

/**
 * This module implements basic functionalities that any VarDis application
 * should have, and any VarDis application (protocol) should inherit from
 * this class. It mainly makes sure that the application module and VarDis
 * can talk to each other via an INET message dispatcher.
 */

class VardisApplication : public DcpApplication {

public:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

protected:

    // --------------------------------------------
    // helpers
    // --------------------------------------------

    /**
     * Translate VarDis status value into string
     */
    std::string getVardisStatusString(VardisStatus status);

    /**
     * Prints received VarDis status as string to logging output
     */
    void printStatus (VardisStatus status);

    /**
     * Default reaction to any received status value from a VarDis confirm
     * primitive. This just calls printStatus.
     */
    virtual void handleVardisConfirmation(VardisConfirmation *conf);

    /**
     * Sending messages or packets down to VarDis
     */
    void sendToVardis (Message* message);
    void sendToVardis (Packet* packet);

};


} //namespace

