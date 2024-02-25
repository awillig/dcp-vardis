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

#include "SRPVarDisBeaconing.h"

#include "messages/RTDBGenerateBeacon_m.h"
#include "messages/SRPGenerateBeacon_m.h"
#include "messages/SRPBeacon_m.h"
#include "messages/StateReport_m.h"
#include "messages/SRPVarDisBeaconHeader_m.h"
#include "messages/SourceTag_m.h"

#include <inet/common/IProtocolRegistrationListener.h>
#include <inet/common/ModuleAccess.h>

#include <beaconing/base/BeaconingBase.h>

#include <stdint.h>

#define BEACON_GENERATION_MESSAGE "GENERATE_BEACON"

#define SRPVARDIS_PROTOCOL_ID 0xBEEF //TODO Select a sensible value


Define_Module(SRPVarDisBeaconing);


const inet::Protocol SRPVarDisBeaconingProtocol("SRPVarDisBeaconing", "State Reporting and Real-Time Data Dissemination protocol based on LBP");

SRPVarDisBeaconing::SRPVarDisBeaconing(void) {
    //do nothing
}


void SRPVarDisBeaconing::initialize(int stage) {
    LBPClientBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        beaconPeriod = par("beaconPeriod").doubleValueInUnit("seconds");
        jitter = par("jitter").doubleValueInUnit("ratio");
        state = WAITING_FOR_BEACON_GENERATION_PERIOD;
        currentPacket = nullptr;
        maxPacketSize = par("maxBeaconSize").intValue();
        srpEnabled = !par("disableSRP").boolValue();
        randomPeriod = par("randomPeriod").boolValue();

        generateMessage = new cMessage(BEACON_GENERATION_MESSAGE);
        scheduleNextBeacon();
    } else if (stage == INITSTAGE_LAST) {
        registerProtocol(SRPVarDisBeaconingProtocol, gate("toLBP"), nullptr);
        registerService(SRPVarDisBeaconingProtocol, nullptr, gate("fromLBP"));
    }
}


void SRPVarDisBeaconing::handleSelfMessage(cMessage *msg) {
    if (msg == generateMessage) {
        if (state == WAITING_FOR_BEACON_GENERATION_PERIOD) {
            if (currentPacket == nullptr) {
                currentPacket = new Packet("SRPRDDBeacon");
                auto beacon = makeShared<SRPVarDisBeaconHeader>();
                beacon->setProtocolID(SRPVARDIS_PROTOCOL_ID);
                beacon->setSenderId(ownIdentifier);
                beacon->setChunkLength(inet::B(2 + MAC_ADDRESS_SIZE));

                currentPacket->setKind(SWARMSTACK_BEACON_KIND);
                currentPacket->insertAtBack(beacon);
            } else {
                throw cRuntimeError("SRPVarDisBeaconing: Asked to generate a "
                                    "new beacon, however the last beacon "
                                    "still has not been sent...");
            }

            if (srpEnabled) {
                auto req = new SRPGenerateBeacon();
                send(req, "srp_out");
                state = REQUESTED_SRP_BEACON;
            } else {
                int packet_len = currentPacket->getByteLength();
                auto req = new RTDBGenerateBeacon();
                req->setInitBeaconLen(packet_len);
                send(req, "rtdb_out");
                state = RECEIVED_SRP_BEACON_REQUESTED_VARDIS_BEACON;
            }
        } else {
            throw cRuntimeError("SRPVarDisBeaconing: Received beacon "
                                "generation message in an illegal state "
                                "(%d)", state);
        }
    } else {
        throw cRuntimeError("SRPVarDisBeaconing: Unknown self message...");
    }
}


void SRPVarDisBeaconing::handleOtherMessage(cMessage *msg) {
    if (dynamic_cast<Packet*>(msg)) {
        if (state == REQUESTED_SRP_BEACON) {
            if (currentPacket != nullptr) {
                auto pkt = static_cast<Packet*>(msg);
                auto chunk = pkt->popAtFront<SRPBeacon>();
                currentPacket->insertAtBack(chunk);
            } else {
                throw cRuntimeError("SRPVarDisBeaconing: Asked to add a SRP "
                                    "beacon to the current packet but it does "
                                    "not exist...");
            }

            int packet_len = currentPacket->getByteLength();
            auto req = new RTDBGenerateBeacon();
            req->setInitBeaconLen(packet_len);
            send(req, "rtdb_out");
            state = RECEIVED_SRP_BEACON_REQUESTED_VARDIS_BEACON;
        } else if (state == RECEIVED_SRP_BEACON_REQUESTED_VARDIS_BEACON) {
            if (currentPacket != nullptr) {
                auto pkt = static_cast<Packet*>(msg);
                if (pkt->getBitLength() > 0) {
                    prepapreRTDBBeaconForTx(pkt);
                    sendViaLBP(currentPacket);
                } else {
                    delete currentPacket;
                }
                currentPacket = nullptr;
            } else {
                throw cRuntimeError("SRPVarDisBeaconing: Asked to add a VarDis "
                                    "beacon to the current packet but it does "
                                    "not exist...");
            }

            scheduleNextBeacon();
            state = WAITING_FOR_BEACON_GENERATION_PERIOD;
        } else {
            throw cRuntimeError("SRPVarDisBeaconing: Received a packet in an "
                                "illegal state (%d)", state);
        }
    } else {
        throw cRuntimeError("SRPVarDisBeaconing: Received an unknown message "
                            "type: %s", msg->getName());
    }

    delete msg;
}


void SRPVarDisBeaconing::scheduleNextBeacon(void) {
    double next_generation;
    if (!randomPeriod) {
        next_generation = beaconPeriod + (beaconPeriod * uniform(-jitter, jitter));
    } else {
        next_generation = exponential(beaconPeriod);
    }
    scheduleAt(SIMTIME_DBL(simTime()) + next_generation, generateMessage);
}


const inet::Protocol& SRPVarDisBeaconing::getProtocol(void) const {
    return SRPVarDisBeaconingProtocol;
}


SRPVarDisBeaconing::~SRPVarDisBeaconing(void) {
    cancelAndDelete(generateMessage);

    if (currentPacket != nullptr) {
        delete currentPacket;
    }
}


bool SRPVarDisBeaconing::prepapreRTDBBeaconForTx(Packet* pkt) {
    //We don't check the format of the packet here and assume that the RTDB
    //module has correctly assembled the packet.
    inet::Ptr<inet::Chunk> chunk;
    bool done = false;
    bool at_least_one_added = false;

    while (pkt->getBitLength() > 0) {
        chunk = pkt->removeAtFront();
        if (chunk != nullptr) {
            at_least_one_added = true;
            currentPacket->insertAtBack(chunk);
        } else {
            done = true;
        }
    }
    return at_least_one_added;
}

void SRPVarDisBeaconing::handleReceivedBroadcast(Packet* pkt) {
    if (pkt->getByteLength() > maxPacketSize) {
        EV << ownIdentifier << " received a broadcast packet larger than the "
           << "maximum allowed size. Ignoring it." << std::endl;
        delete pkt;
        return;
    }

    auto header = pkt->popAtFront<SRPVarDisBeaconHeader>();
    if (header->getProtocolID() == SRPVARDIS_PROTOCOL_ID) {
        auto senderID = header->getSenderId();
        if (senderID == ownIdentifier) {
            throw cRuntimeError("SRPVarDisBeaconHeader: %s received its own "
                                "broadcast message!",
                                ownIdentifier.str().c_str());
        }

        if (srpEnabled) {
            //Extract the SRP packet
            auto srp_beacon = pkt->popAtFront<SRPBeacon>();
            if (inet::B(srp_beacon->getChunkLength()).get() != srp_beacon->getLength()) {
                throw cRuntimeError("SRPVarDisBeaconHeader: %s received a malformed "
                                    "SRP beacon from %s.", ownIdentifier.str().c_str(),
                                    senderID.str().c_str());
            }

            auto srp_pkt = new Packet("SRPBeacon");
            srp_pkt->insertAtBack(srp_beacon);
            auto src_tag = srp_pkt->addTag<SourceTag>();
            src_tag->setSenderID(senderID);
            send(srp_pkt, "srp_out");
        }

        inet::Ptr<inet::Chunk> chunk;
        auto rtdb_pkt = new Packet("VarDisBeacon");
        while (pkt->getBitLength() > 0) {
            chunk = pkt->removeAtFront();
            if (chunk != nullptr) {
                rtdb_pkt->insertAtBack(chunk);
            }
        }

        auto src_tag2 = rtdb_pkt->addTag<SourceTag>();
        src_tag2->setSenderID(senderID);
        send(rtdb_pkt, "rtdb_out");

        delete pkt;
    } else {
        EV << ownIdentifier << " received a malformed broadcast packet. "
           << "Ignoring it." << std::endl;
        delete pkt;
    }
}

