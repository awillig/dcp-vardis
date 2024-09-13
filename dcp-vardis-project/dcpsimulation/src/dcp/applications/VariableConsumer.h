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

#ifndef __DCPV1_VARIABLECONSUMER_H_
#define __DCPV1_VARIABLECONSUMER_H_

#include <omnetpp.h>
#include <inet/common/InitStages.h>
#include <dcp/vardis/VardisClientProtocol.h>
#include <dcp/vardis/VardisDatatypes.h>
#include <dcp/vardis/VardisRTDBConfirmation_m.h>
#include <dcp/vardis/VardisRTDBDescribeDatabase_m.h>
#include <dcp/vardis/VardisRTDBRead_m.h>
#include <dcp/applications/VariableExample.h>

using namespace omnetpp;

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------


namespace dcp {




/**
 * This module, if activated (cf consumerActive parameter), interacts periodically
 * with the local VarDis instance to obtain a database description and then query
 * and output the current values of all registered variables listed in that
 * description. The variables have fixed type ExampleVariable.
 */
class VariableConsumer : public VardisClientProtocol
{
  public:
    virtual ~VariableConsumer();
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

  protected:

    // represents current state of consumer module
    typedef enum ConsumerState
    {
        cState_WaitForSampling        =  0,     // waiting for next round of sampling
        cState_WaitForDBDescription   =  1,     // just requested DBDescription, waiting for response
        cState_WaitForReadResponses   =  2      // waiting for requested read responses
    } ConsumerState;

    // contains last read values for all variables
    std::map<VarIdT, ExampleVariable>   lastReceived;

  protected:

    // configuration
    simtime_t  samplingPeriod;
    bool       consumerActive = false;
    int        varIdToObserve = -1;

    // internal states
    ConsumerState state          = cState_WaitForSampling;
    unsigned int  readsRequested = 0;
    cMessage*  sampleMsg = nullptr;

    // -----------------------------

    /**
     * Output signals for statistics. Values for these are only emitted for one
     * variable configured by the user (cf parameter varIdToObserve)
     */
    simsignal_t delaySig;     // delay between generation of current value and reception on this node
    simsignal_t seqnoSig;     // sequence number
    simsignal_t rxTimeSig;    // collects times at which read responses for chosen variable have been received


    // -----------------------------

    /**
     * sends a RTDBDescribeDatabase.request to local VarDis instance
     */
    void handleSampleMsg ();

    /**
     * Processes received RTDBDescribeDatabase.confirm primitive, listing all
     * the currently known variables, and requests current value for each
     * variable
     */
    void handleRTDBDescribeDatabaseConfirm(RTDBDescribeDatabase_Confirm* dbConf);

    /**
     * Processes received value for one variable
     */
    void handleRTDBReadConfirm(RTDBRead_Confirm* readConf);
};


} // namespace

#endif
