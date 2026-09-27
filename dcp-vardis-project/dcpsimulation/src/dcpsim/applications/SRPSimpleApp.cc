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
#include <dcpsim/applications/SRPSimpleApp.h>


using inet::getContainingNode;
using inet::IMobility;
using inet::INITSTAGE_LAST;
using inet::InitStageRegistry;
using omnetpp::cMessage;
using omnetpp::simTime;

namespace dcp {

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
        EvTracing dbg(*this, "initialize");
        assert(getOwnNodeId() != nullNodeIdentifier);

        // read parameters
        isActive   = par("isActive");

        if (isActive)
        {
            dbg.text ("SRPSimpleApp is active");

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
    }
}

// ----------------------------------------------------

void SRPSimpleApp::handleMessage(cMessage *msg)
{
    EvTracing dbg(*this, "handleMessage");

    if (msg == sampleMsg)
    {
        dbg.text ("got sampleMsg");
        handleSampleMsg();
        return;
    }

    if ((msg->arrivedOn(gidFromDcpProtocol)) && dynamic_cast<SRPUpdateSafetyData_Confirm*>(msg))
    {
        dbg.text ("got confirm");
        SRPUpdateSafetyData_Confirm* srpConf = (SRPUpdateSafetyData_Confirm*) msg;
        handleSRPUpdateSafetyDataConfirm (srpConf);
        return;
    }

    error("SRPSimpleApp::handleMessage: unknown message type");
}

// ----------------------------------------------------


SRPSimpleApp::~SRPSimpleApp ()
{
    cancelAndDelete(sampleMsg);
}


void SRPSimpleApp::handleSampleMsg ()
{
    EvTracing dbg(*this, "handleSampleMsg");

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

    dbg.prefix();
    dbg.the_stream() << "Position = (" << currSD.position_x
       << ", " << currSD.position_y
       << ", " << currSD.position_z
       << ")" << std::endl;

    SRPUpdateSafetyData_Request* srpReq = new SRPUpdateSafetyData_Request;
    srpReq->setSafetyData (currSD);
    sendToSRP(srpReq);

    scheduleAt(simTime() + par("srpGenerationPeriodDistr"), sampleMsg);
}

// -------------------------------------------

/**
 * Finds a pointer to the mobility model, so we can query it for our own
 * position
 */
void SRPSimpleApp::findModulePointers (void)
{
  EvTracing dbg(*this, "findModulePointers");

  cModule *host = getContainingNode (this);
  assert(host);
  _mobility     = check_and_cast<IMobility *>(host->getSubmodule("mobility"));
  assert(_mobility);
}

// -------------------------------------------


void SRPSimpleApp::handleSRPUpdateSafetyDataConfirm (SRPUpdateSafetyData_Confirm* srpConf)
{
    EvTracing dbg(*this, "handleSRPUpdateSafetyDataConfirm");
    assert(srpConf);
    assert(isActive);

    handleSRPConfirmation(srpConf);
    SRPStatus status = srpConf->getStatus();
    delete srpConf;

    if (status != SRP_STATUS_OK)
    {
        error("SRPSimpleApp::handleSRPUpdateSafetyDataConfirm: Not SRP_STATUS_OK");
    }
}

}
