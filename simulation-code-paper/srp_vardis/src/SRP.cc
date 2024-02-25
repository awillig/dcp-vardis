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

#include "SRP.h"
#include <inet/common/ModuleAccess.h>
#include <inet/common/packet/Packet.h>
#include "messages/SRPGenerateBeacon_m.h"
#include "messages/SRPBeacon_m.h"
#include "messages/SourceTag_m.h"

Define_Module(SRP);

using namespace inet;

void SRP::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {
        gcMessage = new cMessage("GC_NEIGHBOUR_DATA");
        posCheckMessage = new cMessage("POS_SAMPLE");

        garbageCollectionPeriod = par("garbageCollectionPeriod").doubleValueInUnit("seconds");
        posCheckPeriod = par("mobilitySamplingPeriod").doubleValueInUnit("seconds");
        staleDataTimeout = par("staleDataTimeout").doubleValueInUnit("seconds");

        scheduleAt(SIMTIME_DBL(simTime()) + garbageCollectionPeriod, gcMessage);
        scheduleAt(SIMTIME_DBL(simTime()) + posCheckPeriod, posCheckMessage);
    } else if (stage == INITSTAGE_LAST) {
        cModule* host = getContainingNode(this);
        assert(host);
        mobility = check_and_cast<IMobility *>(host->getSubmodule("mobility"));
        assert(mobility);

        ourPos = mobility->getCurrentPosition();
        ourVelocity = mobility->getCurrentVelocity();
        mobInfoTimestamp = simTime();
//
//        std::cout << ourPos << std::endl;
    }
}

void SRP::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        handleSelfMessage(msg);
    } else if (dynamic_cast<Packet*>(msg)) {
        auto pkt = static_cast<Packet*>(msg);
        auto src_node = pkt->getTag<SourceTag>()->getSenderID();
        auto beacon = pkt->popAtFront<SRPBeacon>();

        double expiry = SIMTIME_DBL(simTime()) + staleDataTimeout;

        //Don't bother checking if we're the source node of this data, the
        //beaconing protocol has done that for us.
        bool found = false;
        for (auto it = neighbourData.begin(); it != neighbourData.end() && !found; ++it) {
            if (it->id == src_node) {
                found = true;
                it->pos = beacon->getPos();
                it->velocity = beacon->getVelocity();
                it->dataTimestamp = beacon->getTimestamp();
                it->expiration = expiry;
            }
        }

        if (!found) {
            auto data = NeighbourData(src_node, beacon->getPos(),
                                      beacon->getVelocity(),
                                      beacon->getTimestamp(),
                                      expiry);
            neighbourData.push_back(data);
        }

        delete msg;
    } else if (dynamic_cast<SRPGenerateBeacon*>(msg)) {
        auto pkt = new Packet("SRPBeacon");
        auto beacon = makeShared<SRPBeacon>();
        beacon->setPos(ourPos);
        beacon->setVelocity(ourVelocity);
        beacon->setTimestamp(mobInfoTimestamp);
        beacon->setChunkLength(B((3 * 2 * sizeof(double)) + sizeof(int64_t) + sizeof(int32_t) + 2));
        beacon->setLength(B(beacon->getChunkLength()).get());
        pkt->insertAtBack(beacon);
        send(pkt, "net_out");
        delete msg;
    } else {
        throw cRuntimeError("SRP: Received a message of an unknown type: %s",
                            msg->getName());
    }
}


void SRP::handleSelfMessage(cMessage* msg) {
    if (msg == gcMessage) {
        neighbourData.remove_if([this](NeighbourData d){
            if (SIMTIME_DBL(simTime()) > (d.expiration)) {
                return true;
            } else {
                return false;
            }});

        scheduleAt(SIMTIME_DBL(simTime()) + garbageCollectionPeriod, gcMessage);
    } else if (msg == posCheckMessage) {
        ourPos = mobility->getCurrentPosition();
        ourVelocity = mobility->getCurrentVelocity();
        mobInfoTimestamp = simTime();

        scheduleAt(SIMTIME_DBL(simTime()) + posCheckPeriod, posCheckMessage);
    } else {
        throw cRuntimeError("SRP: Received an unknown self message: %s",
                            msg->getName());
    }
}

SRP::~SRP(void) {
    cancelAndDelete(gcMessage);
    cancelAndDelete(posCheckMessage);

    neighbourData.clear();
}


NeighbourData::NeighbourData(inet::MacAddress id_, inet::Coord pos_,
                             inet::Coord vel_, simtime_t data_ts_,
                             double expiration_) : id(id_), pos(pos_),
                             velocity(vel_), dataTimestamp(data_ts_),
                             expiration(expiration_) {};
