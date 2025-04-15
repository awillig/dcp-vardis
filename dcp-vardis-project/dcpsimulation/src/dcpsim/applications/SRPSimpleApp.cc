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

#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/common/Protocol.h>
#include <dcp/applications/SRPSimpleApp.h>


using namespace dcp;


Define_Module(SRPSimpleApp);

// ========================================================================================
// Standard OMNeT++ and class methods
// ========================================================================================


void SRPSimpleApp::initialize(int stage)
{
    SRPApplication::initialize(stage);

    if (stage == INITSTAGE_LAST)
    {
        dbg_setModuleName("SRPSimpleApp");
        dbg_enter("initialize");
        assert(getOwnNodeId() != nullIdentifier);

        // read parameters
        isActive   = par("isActive");

        if (isActive)
        {
            dbg_string ("SRPSimpleApp is active");

            // create and schedule self-messages
            sampleMsg = new cMessage("SRPSimpleApp:sampleMsg");
            scheduleAt(simTime() + par("srpGenerationPeriodDistr"), sampleMsg);

            // register a separate protocol for this producer and register it
            // as SRP client protocol with dispatcher
            std::stringstream ssLc, ssUc;
            ssLc << "srpsimpleapp[" << getOwnNodeId() << "]";
            ssUc << "SRPSIMPLEAPP[" << getOwnNodeId() << "]";
            createProtocol(ssLc.str().c_str(), ssUc.str().c_str());

            // find pointer to mobility model
            findModulePointers();
        }

        dbg_leave();
    }
}

// ----------------------------------------------------

void SRPSimpleApp::handleMessage(cMessage *msg)
{
    dbg_assertToplevel();
    dbg_enter("handleMessage");

    // dispatch based on type of message

    if (msg == sampleMsg)
    {
        dbg_string ("got sampleMsg");
        handleSampleMsg();
        dbg_leave();
        return;
    }

    if ((msg->arrivedOn(gidFromDcpProtocol)) && dynamic_cast<SRPUpdateSafetyData_Confirm*>(msg))
    {
        dbg_string ("got confirm");
        SRPUpdateSafetyData_Confirm* srpConf = (SRPUpdateSafetyData_Confirm*) msg;
        handleSRPUpdateSafetyDataConfirm (srpConf);
        dbg_leave();
        return;
    }

    error("SRPSimpleApp::handleMessage: unknown message type");

    dbg_leave();
}

// ----------------------------------------------------


SRPSimpleApp::~SRPSimpleApp ()
{
    cancelAndDelete(sampleMsg);
}


void SRPSimpleApp::handleSampleMsg ()
{
    dbg_enter("handleSampleMsg");

    assert (_mobility);
    auto currPosition  = _mobility->getCurrentPosition();
    auto currVelocity  = _mobility->getCurrentVelocity();

    SafetyDataT  currSD;
    currSD.position_x = currPosition.getX();
    currSD.position_y = currPosition.getY();
    currSD.position_z = currPosition.getZ();
    currSD.velocity_x = currVelocity.getX();
    currSD.velocity_y = currVelocity.getY();
    currSD.velocity_z = currVelocity.getZ();

    dbg_prefix();
    EV << "Position = (" << currSD.position_x
       << ", " << currSD.position_y
       << ", " << currSD.position_z
       << ")" << endl;

    SRPUpdateSafetyData_Request* srpReq = new SRPUpdateSafetyData_Request;
    srpReq->setSafetyData (currSD);
    sendToSRP(srpReq);

    scheduleAt(simTime() + par("srpGenerationPeriodDistr"), sampleMsg);

    dbg_leave();

}

// -------------------------------------------

/**
 * Finds a pointer to the mobility model, so we can query it for our own
 * position
 */
void SRPSimpleApp::findModulePointers (void)
{
  dbg_enter("findModulePointers");

  cModule *host = getContainingNode (this);
  assert(host);
  _mobility     = check_and_cast<IMobility *>(host->getSubmodule("mobility"));
  assert(_mobility);

  dbg_leave();
}

// -------------------------------------------


void SRPSimpleApp::handleSRPUpdateSafetyDataConfirm (SRPUpdateSafetyData_Confirm* srpConf)
{
    dbg_enter("handleSRPUpdateSafetyDataConfirm");
    assert(srpConf);
    assert(isActive);

    handleSRPConfirmation(srpConf);
    SRPStatus status = srpConf->getStatus();
    delete srpConf;

    if (status != SRP_STATUS_OK)
    {
        error("SRPSimpleApp::handleSRPUpdateSafetyDataConfirm: Not SRP_STATUS_OK");
    }

    dbg_leave();
}



