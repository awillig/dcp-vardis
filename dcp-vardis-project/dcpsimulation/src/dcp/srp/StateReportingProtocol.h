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

#ifndef DCP_SRP_STATEREPORTINGPROTOCOL_H_
#define DCP_SRP_STATEREPORTINGPROTOCOL_H_


#include <map>
#include <omnetpp.h>
#include <inet/common/InitStages.h>
#include <inet/mobility/contract/IMobility.h>
#include <dcp/common/DcpTypesGlobals.h>
#include <dcp/bp/BPRegisterProtocol_m.h>
#include <dcp/bp/BPDeregisterProtocol_m.h>
#include <dcp/bp/BPReceivePayload_m.h>
#include <dcp/srp/SafetyData_m.h>
#include <dcp/srp/NeighbourTableEntry.h>
#include <dcp/bp/BPClientProtocol.h>


using namespace omnetpp;
using namespace inet;

// ------------------------------------------------------------------------------------

namespace dcp {


typedef uint32_t   SRPSequenceNumberT;


/**
 * This module implements the state reporting protocol (SRP) as a BP client
 * protocol. It frequently samples the current node position from the mobility
 * model (at a configurable sampling period) and generates suitable payloads
 * for outgoing beacons.
 *
 * Conversely, we receive SRP messages from neighbored nodes and store them
 * in a neighbor table. The neighbor table is soft state, entries that are
 * older than a configurable threshold are removed from the table (this is
 * referred to as scrubbing).
 */


class StateReportingProtocol : public BPClientProtocol {

public:
    virtual ~StateReportingProtocol();

    // standard OMNeT++ methods
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage (cMessage* msg) override;

    // register as BP client protocol
    virtual void registerAsBPClient (void) override;

    const BPLengthT maximumSRPPayloadLength = 100;

private:

    // -------------------------------------------
    // data members
    // -------------------------------------------

    // module parameters
    double        _srpPositionSamplingPeriod;       // positions are sampled and sent to BP at this period
    double        _srpNeighbourTableTimeout;        // timeout for aging out entries in neighbor table
    double        _srpNeighbourTableScrubPeriod;    // Period between two invocations of scrubbing process
    double        _srpNeighbourTablePrintPeriod;    // Period for logging the neighbor table contents

    // other data members
    SafetyDataT          _currentSafetyData;       // current safety data (position etc) of this node
    IMobility           *_mobility = nullptr;      // pointer to mobility model, needed to query position
    SRPSequenceNumberT   _seqno    = 0;            // seqno to be used in SRP messages

    // Timer self message
    cMessage*     _generatePayloadMsg      = nullptr;
    cMessage*     _sampleSafetyDataMsg     = nullptr;
    cMessage*     _scrubNeighbourTableMsg  = nullptr;
    cMessage*     _printNeighbourTableMsg  = nullptr;

protected:

    // -------------------------------------------
    // data members
    // -------------------------------------------

    // neighbor table
    std::map<NodeIdentifierT, NeighbourTableEntry>  neighbourTable;


    // -------------------------------------------
    // message handlers
    // -------------------------------------------

    /**
     * retrieves current safety data (position etc), generates an SRP payload
     * and hands it over to BP for transmission
     */
    virtual void handleGeneratePayloadMsg ();

    /**
     * Processes received SPR message from a neighbor, adds it to neighbor table
     */
    virtual void handleReceivedPayload(BPReceivePayload_Indication* payload);

    /**
     * Traverses neighbor table and checks entries whether they are too old.
     * If so, they are removed from the table. Schedules next scrubbing
     * operation.
     */
    virtual void handleScrubNeighbourTableMsg ();

    /**
     * Prints current contents of neighbor table for logging purposes
     */
    virtual void handlePrintNeighbourTableMsg ();

    // -------------------------------------------
    // Helpers
    // -------------------------------------------

    /**
     * Queries current position etc from INET mobility model, and schedules next
     * sampling operation
     */
    void sampleSafetyData (void);

    /**
     * Finds the pointer to the mobility model
     */
    void findModulePointers (void);
};

} // namespace

#endif /* DCP_SRP_STATEREPORTINGPROTOCOL_H_ */
