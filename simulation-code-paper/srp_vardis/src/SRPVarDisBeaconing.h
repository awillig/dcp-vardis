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

#ifndef __SRP_VARDIS_SRPVARDISBEACONING_H_
#define __SRP_VARDIS_SRPVARDISBEACONING_H_

#include <omnetpp.h>
#include <lbp/LBPClientBase.h>

// #include "messages/RTDBBeacon_m.h"

using namespace omnetpp;

typedef enum {
    WAITING_FOR_BEACON_GENERATION_PERIOD,
    REQUESTED_SRP_BEACON,
    RECEIVED_SRP_BEACON_REQUESTED_VARDIS_BEACON
} beacon_state_t;

class SRPVarDisBeaconing : public LBPClientBase {
  public:
    SRPVarDisBeaconing(void);
    virtual void initialize(int stage);
//    virtual void finish();

  protected:
    virtual void handleSelfMessage(cMessage *msg);
    virtual void handleOtherMessage(cMessage *msg);
    virtual void handleReceivedBroadcast (Packet* packet);

    virtual const inet::Protocol& getProtocol(void) const;

    virtual ~SRPVarDisBeaconing();

  private:
    double beaconPeriod; //seconds
    double jitter;
    beacon_state_t state;
    int maxPacketSize;
    bool srpEnabled;
    bool randomPeriod;

    cMessage* generateMessage;
    Packet* currentPacket;

    void scheduleNextBeacon(void);
    bool prepapreRTDBBeaconForTx(Packet* pkt);
};

#endif
