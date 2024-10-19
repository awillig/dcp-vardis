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
#include <inet/common/InitStages.h>

#include <dcp/vardis/VardisRTDBConfirmation_m.h>
#include <dcp/vardis/VardisApplication.h>
#include <dcp/vardis/VardisDatatypes.h>

using namespace omnetpp;

// --------------------------------------------------------------------

namespace dcp {


/**
 * This module models the life cycle of a variable. It creates a variable after
 * an initial delay (possibly random), it deletes the variable at a given time
 * (possibly random) and in between it generates variable updates with (possibly
 * random) inter-update times and (possibly random) values.
 */
class VardisVariableProducer : public VardisApplication
{
  public:
    virtual ~VardisVariableProducer();
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

  protected:

    // Parameters
    VarIdT         varId;
    VarRepCntT     varRepCnt;
    simtime_t      deletionTime;
    simtime_t      creationTime;

    // internal state
    bool           isActivelyGenerating;
    uint32_t       seqno = 0;

    // self-messages for variable creation, updates and deletion
    cMessage* createMsg = nullptr;
    cMessage* updateMsg = nullptr;
    cMessage* deleteMsg = nullptr;

    // event handlers for self-messages signaling the time for variable creation,
    // updates and deletion
    void handleCreateMsg ();
    void handleUpdateMsg ();
    void handleDeleteMsg ();

    // event handlers for VarDis responses to our service requests
    void handleRTDBCreateConfirm(RTDBCreate_Confirm* createConf);
    void handleRTDBDeleteConfirm(RTDBDelete_Confirm* deleteConf);
    void handleRTDBUpdateConfirm(RTDBUpdate_Confirm* updateConf);


    void sendVarCreateRequest ();
    void sendVarUpdRequest ();
    void sendVarDeleteRequest ();

    void scheduleNextUpdate();
};

} // namespace

