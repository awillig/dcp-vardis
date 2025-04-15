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
#include <dcpsim/common/DcpProtocol.h>
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

class DcpApplication : public DcpProtocol {

public:
    virtual ~DcpApplication();
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

protected:

    // --------------------------------------------
    // data members
    // --------------------------------------------

    Protocol *theProtocol = nullptr;

    // gate id's
    int gidFromDcpProtocol = -1;
    int gidToDcpProtocol   = -1;


    // --------------------------------------------
    // helpers
    // --------------------------------------------

    /**
     * Create and register application with INET, so that dispatcher
     * can be used.
     */
    void createProtocol(const char* descr1, const char* descr2);

    /**
     * Return (pointer to) the created protocol object
     */
    Protocol* getProtocol () {assert(theProtocol); return theProtocol;};


    /**
     * Sending messages or packets down to SRP
     */
    void sendToDcpProtocol (Protocol* targetProtocol, Message* message);
    void sendToDcpProtocol (Protocol* targetProtocol, Packet* packet);

};


} //namespace

