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

#include "RTDB.h"

#include <cstdlib>
#include <cstring>
#include <new>
#include <cassert>

#include "../messages/rtdb_api/RTDBResponseCode_m.h"
#include "../messages/RTDBGenerateBeacon_m.h"
#include "../messages/ietypes/IETypeHeader_m.h"
#include "../messages/SourceTag_m.h"

#include "../messages/RTDBVarUpdateIndication_m.h"

#include <lbp/LocalBroadcastProtocol.h>

#define RES_BUF_MAX_LEN 2048


Define_Module(RTDB);

void RTDB::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {
        MAX_VARIABLE_LEN = par("maxVariableLen").intValue();
        MAX_DESCRIPTION_LEN = par("maxDescriptionLen").intValue();
        MAX_REPETITIONS = par("maxRepetitions").intValue();
        MAX_NUM_SUMMARIES = par("maxNumSummaries").intValue();
        MAX_PACKET_SIZE = par("maxBeaconSize").intValue();
    } else if (stage == INITSTAGE_LAST){
        getOurID();
    }
}


void RTDB::getOurID(void) {
    if (ourID == inet::MacAddress::UNSPECIFIED_ADDRESS) {
        cModule *host = getContainingNode (this);
        assert(host);

        LocalBroadcastProtocol *lbp = check_and_cast<LocalBroadcastProtocol*>(host->getSubmodule("lbp"));
        assert(lbp);

        ourID = lbp->getOwnMacAddress();
    }
}


void RTDB::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        this->handleSelfMessage(msg);
    } else if (dynamic_cast<RTDBCreate*>(msg)) {
        auto req = static_cast<RTDBCreate*>(msg);
        this->handleAPICreateReq(req);
    } else if (dynamic_cast<RTDBUpdate*>(msg)) {
        auto req = static_cast<RTDBUpdate*>(msg);
        this->handleAPIUpdateReq(req);
    } else if (dynamic_cast<RTDBGenerateBeacon*>(msg)) {
        auto req = static_cast<RTDBGenerateBeacon*>(msg);
        this->constructBeacon(req->getInitBeaconLen());
    } else if (dynamic_cast<Packet*>(msg)){
        auto pkt = static_cast<Packet*>(msg);
        this->processVarDisBeacon(pkt);
    } else {
        throw cRuntimeError("RTDB received unknown message: %s", msg->getName());
    }

    delete msg;
}


void RTDB::sendAPIResponse(int r_kind, int r_code) {
    auto resp = new RTDBResponseCode();
    resp->setResponseKind(r_kind);
    resp->setResponseCode(r_code);
    send(resp, "application$o");
//    printf("RTDB: Sent RC_OK response at %.4f\n", SIMTIME_DBL(simTime()));
}


void RTDB::handleSelfMessage(cMessage *msg) {
    throw cRuntimeError("RTDB received an unknown self message: %s", msg->getName());
}


void RTDB::handleAPICreateReq(RTDBCreate* req) {
    varID_t varID = req->getVarID();
    if (this->varDB.find(varID) != this->varDB.end()) {
        this->sendAPIResponse(RTDB_CREATE, RC_VARIABLE_EXISTS);
        return;
    }

    const char* varDescr = req->getVarDescr();
    varDescrLen_t descrLen = strlen(varDescr);
    if (descrLen > MAX_DESCRIPTION_LEN) {
        this->sendAPIResponse(RTDB_CREATE, RC_VARIABLE_DESCRIPTION_TOO_LONG);
        return;
    }

    uint8_t* descr = new uint8_t[descrLen];
    memcpy(descr, varDescr, descrLen);

    int varLen = req->getVarLen();
    if (varLen > MAX_VARIABLE_LEN) {
        this->sendAPIResponse(RTDB_CREATE, RC_VALUE_TOO_LONG);
        return;
    }

    int repCnt = req->getVarRepCnt();
    if (repCnt > MAX_REPETITIONS) {
        this->sendAPIResponse(RTDB_CREATE, RC_WRONG_REPETITION_NUMBER);
        return;
    }

    uint8_t* val = new uint8_t[varLen];
    for (int i = 0; i < varLen; ++i) {
        val[i] = req->getVarBuf(i);
    }

    RTDBVariable* var = new RTDBVariable(ourID, varID, repCnt, descrLen,
                                         descr, varLen, val, 1);
    delete[] val;
    delete[] descr;

    this->varDB[varID] = var;

    //Push a creation information element onto the creation queue to be
    //included in the new beacons.
    RTDBVarSpec* spec = new RTDBVarSpec(varID, ourID, 1, descrLen,
                                        var->getDescription(), repCnt);
    RTDBVarUpdate* update = new RTDBVarUpdate(varID, 1, varLen, var->getVar(),
                                              repCnt);
    createQ.push(new RTDBVarCreate(spec, update));

    //Push the item into the summary queue so that we announce summaries for it
    summaryQ.push(varID);

    this->sendAPIResponse(RTDB_CREATE, RC_OK);
}

void RTDB::handleAPIUpdateReq(RTDBUpdate* req) {
    varID_t varID = req->getVarID();
    if (this->varDB.find(varID) == this->varDB.end()) {
        this->sendAPIResponse(RTDB_UPDATE, RC_VARIABLE_DOES_NOT_EXIST);
        return;
    }

    RTDBVariable* var = this->varDB[varID];

    if (var->getProducer() != this->ourID) {
        this->sendAPIResponse(RTDB_UPDATE, RC_VARIABLE_IS_NOT_PRODUCER);
        return;
    }

    if (var->isForDeletion()) {
        this->sendAPIResponse(RTDB_UPDATE, RC_VARIABLE_BEING_DELETED);
        return;
    }

    int varLen = req->getVarLen();
    if (varLen > MAX_VARIABLE_LEN) {
        this->sendAPIResponse(RTDB_UPDATE, RC_VALUE_TOO_LONG);
        return;
    }

    uint8_t* val = new uint8_t[varLen];
    for (int i = 0; i < varLen; ++i) {
        val[i] = req->getVarBuf(i);
    }

    var->update(val, varLen);
    delete[] val;

    //Remove prior updates we may still need to broadcast.
    updateQRemoveID(varID);

    //Push a new update event onto the queue,
    updateQ.push(new RTDBVarUpdate(varID, var->getSeqNo(), varLen,
                                   var->getVar(),
                                   var->getRepetitionCount()));

    this->sendAPIResponse(RTDB_UPDATE, RC_OK);
}


//Returns true if we should drop this element from processing whilst creating
//beacons. Returns false if we should add them to the beacons.
bool dropNonExistent(std::unordered_map<varID_t, RTDBVariable*>& db, varID_t id) {
    if (db.find(id) != db.end()) {
        if (!db.at(id)->isForDeletion()) {
            return false;
        }
    }

    return true;
}

void RTDB::constructBeacon(int initBeaconLen) {
    int beacon_size_bytes = initBeaconLen;
    int num_summaries_added = 0;
    bool stop = false;
    auto pkt = new Packet("RTDBBeacon");

    //Check through (in this order) variable creates, variable deletes,
    //variable updates, variable summaries, variable update requests, variable
    //creation requests, adding them to the packet if there is enough space.

    //TODO: Abstract this while loop out as we should be able to use it for all
    //      non summary operations
    uint16_t create_type_len = 3;
    std::list<inet::Ptr<VarDisCreate>> create_list;
    if (!createQ.empty()) {
        RTDBVarCreate* first_valid = nullptr;

        while (!createQ.empty() && beacon_size_bytes < MAX_PACKET_SIZE && !stop) {
            auto create_req = createQ.front();

            //Drop the create item from the queue if it does not exist in the
            //database, or if it is marked for deletion. Note that even when a
            //variable is announced in a beacon,
            if (!dropNonExistent(this->varDB, create_req->varSpec->varID)) {

                //Check we've not handled this element before in this beacon
                //construction round by comparing it to our record of the first
                //valid record we placed back in the queue (i.e. that we have
                //got back to the front of the queue)
                bool already_processed = first_valid != nullptr &&
                                         first_valid->varSpec->varID == create_req->varSpec->varID &&
                                         first_valid->varSpec->varSeqNo == create_req->varSpec->varSeqNo;
                if (!already_processed) {
                    int add_len = create_req->getIETypeLen();
                    if (beacon_size_bytes + create_type_len + add_len <= MAX_PACKET_SIZE) {
                        //If we can fit it into the packet, put it in
                        createQ.pop();

                        create_list.push_back(create_req->getPacketElement());
                        create_type_len += add_len;

                        create_req->remainingRepetitions--;
                        if (create_req->remainingRepetitions > 0) {
                            createQ.push(create_req);

                            if (first_valid == nullptr) {
                                first_valid = create_req;
                            }
                        } else {
                            delete create_req;
                        }
                    } else {
                        //If adding this create request will push us over the
                        //packet size limit, stop adding these resources,
                        //whilst leaving the queue as it currently is.
                        stop = true;
                    }
                } else {
                    stop = true;
                }
            } else {
                createQ.pop();
                delete create_req;
            }
        }
    }

    beacon_size_bytes += create_type_len;

    //TODO Handle variable deletion

    stop = false;
    uint16_t update_type_len = 3;
    std::list<inet::Ptr<VarDisUpdate>> update_list;
    if (!updateQ.empty()) {
        RTDBVarUpdate* first_valid = nullptr;

        while (!updateQ.empty() && beacon_size_bytes < MAX_PACKET_SIZE && !stop) {
            auto update_req = updateQ.front();

            //Drop the create item from the queue if it does not exist in the
            //database, or if it is marked for deletion. Note that even when a
            //variable is announced in a beacon,
            if (!dropNonExistent(this->varDB, update_req->varID)) {

                //Check we've not handled this element before in this beacon
                //construction round by comparing it to our record of the first
                //valid record we placed back in the queue (i.e. that we have
                //got back to the front of the queue)
                bool already_processed = first_valid != nullptr &&
                                         first_valid->varID == update_req->varID &&
                                         first_valid->varSeqNo == update_req->varSeqNo;
                if (!already_processed) {
                    int add_len = update_req->getIETypeLen();
                    if (beacon_size_bytes + update_type_len + add_len <= MAX_PACKET_SIZE) {
                        //If we can fit it into the packet, put it in
                        updateQ.pop();

                        update_list.push_back(update_req->getPacketElement());
                        update_type_len += add_len;

                        update_req->remainingRepetitions--;
                        if (update_req->remainingRepetitions > 0) {
                            updateQ.push(update_req);

                            if (first_valid == nullptr) {
                                first_valid = update_req;
                            }
                        } else {
                            delete update_req;
                        }
                    } else {
                        //If adding this create request will push us over the
                        //packet size limit, stop adding these resources,
                        //whilst leaving the queue as it currently is.
                        stop = true;
                    }
                } else {
                    stop = true;
                }
            } else {
                updateQ.pop();
                delete update_req;
            }
        }
    }

    beacon_size_bytes += update_type_len;

    uint16_t summary_len = 3;
    std::list<inet::Ptr<VarDisSummary>> summary_list;
    //Note: Summaries are a special type of announcement, where we add up to
    //      MAX_NUM_SUMMARIES per packet announcing every variable we know
    //      about in a round robin fashion.
    if (!summaryQ.empty()) {
        varID_t first_id;
        varID_t id = summaryQ.front();

        while (num_summaries_added < MAX_NUM_SUMMARIES && beacon_size_bytes + summary_len < MAX_PACKET_SIZE && !stop) {
            if (!dropNonExistent(this->varDB, id)) {
                auto sum = RTDBVarSummary(id, varDB[id]->getSeqNo(), 0);
                auto add_len = sum.getIETypeLen();

                //Don't add item to the packet if we have looped around to the
                //front of the list.
                if (summary_list.size() == 0) {
                    first_id = id;
                } else if (first_id == id) {
                    stop = true;
                }

                if (!stop) {
                    //If we there is enough space in the packet to add this
                    //summary message, add it. Otherwise, stop adding it.
                    if (beacon_size_bytes + summary_len + add_len < MAX_PACKET_SIZE) {
                        summary_list.push_back(sum.getPacketElement());

                        num_summaries_added++;
                        summary_len += add_len;

                        summaryQ.pop();
                        summaryQ.push(id);
                    } else {
                        stop = true;
                    }
                }
            } else {
                summaryQ.pop();
            }

            id = summaryQ.front();
        }
    }

    beacon_size_bytes += summary_len;

    stop = false;

    uint16_t create_req_type_len = 3;
    std::list<inet::Ptr<VarDisReqCreate>> create_req_list;
    if (!reqCreateQ.empty()) {
        RTDBVarReqCreate* first_valid = nullptr;

        while (!reqCreateQ.empty() && !stop) {
            RTDBVarReqCreate* req = reqCreateQ.front();

            //Drop the create item from the queue if it exists in the database
            if (varDB.find(req->varID) == varDB.end()) {
//                varDB[req->varID]->isForDeletion()) {

                //Check we've not handled this element before in this beacon
                //construction round by comparing it to our record of the first
                //valid record we placed back in the queue (i.e. that we have
                //got back to the front of the queue)
                bool already_processed = first_valid != nullptr &&
                                         first_valid->varID == req->varID;
                if (!already_processed) {
                    int add_len = req->getIETypeLen();
                    if (beacon_size_bytes + create_req_type_len + add_len <= MAX_PACKET_SIZE) {
                        //If we can fit it into the packet, put it in
                        reqCreateQ.pop();

                        create_req_list.push_back(req->getPacketElement());
                        create_req_type_len += add_len;

                        req->remainingRepetitions--;
                        if (req->remainingRepetitions > 0) {
                            reqCreateQ.push(req);

                            if (first_valid == nullptr) {
                                first_valid = req;
                            }
                        } else {
                            delete req;
                        }
                    } else {
                        //If adding this create request will push us over the
                        //packet size limit, stop adding these resources,
                        //whilst leaving the queue as it currently is.
                        stop = true;
                    }
                } else {
                    stop = true;
                }
            } else {
                reqCreateQ.pop();
                delete req;
            }
        }
    }

    beacon_size_bytes += create_req_type_len;

    uint16_t update_req_type_len = 3;
    std::list<inet::Ptr<VarDisReqUpdate>> update_req_list;
    if (!reqUpdateQ.empty()) {
        RTDBVarReqUpdate* first_valid = nullptr;

        while (!reqUpdateQ.empty() && !stop) {
            auto req = reqUpdateQ.front();

            //Drop the create item from the queue if it does not exist in the
            //database, or if it is marked for deletion. Note that even when a
            //variable is announced in a beacon,
            if (!dropNonExistent(this->varDB, req->varID)) {

                //Check we've not handled this element before in this beacon
                //construction round by comparing it to our record of the first
                //valid record we placed back in the queue (i.e. that we have
                //got back to the front of the queue)
                bool already_processed = first_valid != nullptr &&
                                         first_valid->varID == req->varID;
                if (!already_processed) {
                    int add_len = req->getIETypeLen();
                    if (beacon_size_bytes + update_req_type_len + add_len <= MAX_PACKET_SIZE) {
                        //If we can fit it into the packet, put it in
                        reqUpdateQ.pop();

                        update_req_list.push_back(req->getPacketElement());
                        update_req_type_len += add_len;

                        req->remainingRepetitions--;
                        if (req->remainingRepetitions > 0) {
                            reqUpdateQ.push(req);

                            if (first_valid == nullptr) {
                                first_valid = req;
                            }
                        } else {
                            delete req;
                        }
                    } else {
                        //If adding this create request will push us over the
                        //packet size limit, stop adding these resources,
                        //whilst leaving the queue as it currently is.
                        stop = true;
                    }
                } else {
                    stop = true;
                }
            } else {
                reqUpdateQ.pop();
                delete req;
            }
        }
    }

    beacon_size_bytes += update_req_type_len;

    if (!create_list.empty()) {
        auto create_header = makeShared<IETypeHeader>();
        create_header->setType(CREATE_VARIABLES);
        create_header->setLen(create_type_len - 3);
        create_header->setChunkLength(inet::B(3));

        pkt->insertAtBack(create_header);

        while (!create_list.empty()) {
            pkt->insertAtBack(create_list.front());
            create_list.pop_front();
        }
    }

    if (!update_list.empty()) {
        auto update_header = makeShared<IETypeHeader>();
        update_header->setType(UPDATES);
        update_header->setLen(update_type_len - 3);
        update_header->setChunkLength(inet::B(3));

        pkt->insertAtBack(update_header);

        while (!update_list.empty()) {
            pkt->insertAtBack(update_list.front());
            update_list.pop_front();
        }
    }

    //Add all of the fields to the packet
    if (num_summaries_added > 0) {
        auto summaries_header = makeShared<IETypeHeader>();
        summaries_header->setType(SUMMARIES);
        summaries_header->setLen(summary_len - 3);
        summaries_header->setChunkLength(inet::B(3));
        pkt->insertAtBack(summaries_header);

        while (!summary_list.empty()) {
            pkt->insertAtBack(summary_list.front());
            summary_list.pop_front();
        }
    }

    if (!create_req_list.empty()) {
        auto create_req_header = makeShared<IETypeHeader>();
        create_req_header->setType(REQUEST_VAR_CREATES);
        create_req_header->setLen(create_req_type_len - 3);
        create_req_header->setChunkLength(inet::B(3));

        pkt->insertAtBack(create_req_header);

        while (!create_req_list.empty()) {
            pkt->insertAtBack(create_req_list.front());
            create_req_list.pop_front();
        }
    }

    if (!update_req_list.empty()) {
        auto update_req_header = makeShared<IETypeHeader>();
        update_req_header->setType(REQUEST_VAR_UPDATES);
        update_req_header->setLen(update_req_type_len - 3);
        update_req_header->setChunkLength(inet::B(3));

        pkt->insertAtBack(update_req_header);

        while (!update_req_list.empty()) {
            pkt->insertAtBack(update_req_list.front());
            update_req_list.pop_front();
        }
    }

    //Sanity check our beacon size and number of summary elements
    if (pkt->getByteLength() + initBeaconLen > MAX_PACKET_SIZE) {
        throw cRuntimeError("Created beacon larger than allowed size (%ld B vs %d B)",
                                 pkt->getByteLength() + initBeaconLen, MAX_PACKET_SIZE);
    }

    if (num_summaries_added > MAX_NUM_SUMMARIES) {
        throw cRuntimeError("Add too many summary elements to our generated beacon!");
    }

    //Provide the beacon to the network interface.
    send(pkt, "net_out");
}


RTDB::~RTDB(void) {
    while (!updateQ.empty()) {
        auto i = updateQ.front();
        updateQ.pop();
        delete i;
    }

    while (!createQ.empty()) {
        auto i = createQ.front();
        createQ.pop();
        delete i;
    }

    while (!reqUpdateQ.empty()) {
        auto i = reqUpdateQ.front();
        reqUpdateQ.pop();
        delete i;
    }

    while (!reqCreateQ.empty()) {
        auto i = reqCreateQ.front();
        reqCreateQ.pop();
        delete i;
    }

    while (!deleteQ.empty()) {
        auto i = deleteQ.front();
        deleteQ.pop();
        delete i;
    }

//    std::cout << ourID << " knows about " << this->varDB.size() << " variables (ID, seqNo): ";
    for (auto& it: this->varDB) {
//        printf("(%d, %d) ", it.first, it.second->getSeqNo());
        delete it.second;
    }
//    std::cout << "\n";
    this->varDB.clear();
}


void RTDB::updateQRemoveID(varID_t id) {
    std::queue<RTDBVarUpdate*> tempQ;
    while (!updateQ.empty()) {
        auto u = updateQ.front();
        updateQ.pop();

        if (u->varID != id) {
            tempQ.push(u);
        } else {
            delete u;
        }
    }

    while (!tempQ.empty()) {
        updateQ.push(tempQ.front());
        tempQ.pop();
    }
}


void RTDB::createQRemoveID(varID_t id) {
    std::queue<RTDBVarCreate*> tempQ;
    while (!createQ.empty()) {
        auto u = createQ.front();
        createQ.pop();

        if (u->varSpec->varID != id) {
            tempQ.push(u);
        } else {
            delete u;
        }
    }

    while (!tempQ.empty()) {
        createQ.push(tempQ.front());
        tempQ.pop();
    }
}

void RTDB::reqUpdateQRemoveID(varID_t id) {
    std::queue<RTDBVarReqUpdate*> tempQ;
    while (!reqUpdateQ.empty()) {
        auto u = reqUpdateQ.front();
        reqUpdateQ.pop();

        if (u->varID != id) {
            tempQ.push(u);
        } else {
            delete u;
        }
    }

    while (!tempQ.empty()) {
        reqUpdateQ.push(tempQ.front());
        tempQ.pop();
    }
}


void RTDB::reqCreateQRemoveID(varID_t id) {
    std::queue<RTDBVarReqCreate*> tempQ;
    while (!reqCreateQ.empty()) {
        auto u = reqCreateQ.front();
        reqCreateQ.pop();

        if (u->varID != id) {
            tempQ.push(u);
        } else {
            delete u;
        }
    }

    while (!tempQ.empty()) {
        reqCreateQ.push(tempQ.front());
        tempQ.pop();
    }
}

void RTDB::informApplicationOfUpdate(varID_t id, varLen_t varLen, uint8_t* val) {
    auto indication = new RTDBVarUpdateIndication();

    indication->setVarLen(varLen);
    indication->setVarID(id);

    for (int i = 0; i < varLen; i++) {
        indication->appendVarBuf(static_cast<char>(val[i]));
    }

    send(indication, "application$o");
}

//This returns true if the new_seqno is more recent than the our_seqno
bool seqNoMoreRecent(seqNo_t our_seqno, seqNo_t new_seqno) {
    //Use the method outlined in RFC1982 Section 3.2.
    //(https://www.rfc-editor.org/rfc/rfc1982) to determine which sequence
    //number comes first.
    uint64_t max_value;
    switch (sizeof(seqNo_t)) {
        case 1: max_value = UINT8_MAX + 1;  break;
        case 2: max_value = UINT16_MAX + 1; break;
        case 4: max_value = UINT32_MAX + 1; break;
        default: throw cRuntimeError("Unknown seqNo_t width"); break;
    }

    if (our_seqno != new_seqno) {
        int64_t gap1 = (new_seqno - our_seqno) % max_value;
        int64_t gap2 = (our_seqno - new_seqno) % max_value;

        if (gap1 < gap2) {
            return true;
        } else {
            return false;
        }
    } else {
        //If the seq no match, return false.
        return false;
    }
}


void RTDB::processVarDisBeacon(inet::Packet* pkt) {
    int total_length = inet::B(pkt->getDataLength()).get();
    auto hdr = pkt->popAtFront<IETypeHeader>();
    enum IEType type = hdr->getType();
    auto sourceID = pkt->getTag<SourceTag>()->getSenderID();

    //variable creation, summaries, and updates must be processed in an order
    //which does not match the order in which they are inserted into the packet
    //thus, we put them in a list so that we can process them in the correct
    //order after we have dissected the entire packet.
    std::list<inet::Ptr<const VarDisCreate>> creates;
    std::list<inet::Ptr<const VarDisUpdate>> updates;
    std::list<inet::Ptr<const VarDisSummary>> summaries;
    std::list<inet::Ptr<const VarDisReqCreate>> createReqs;
    std::list<inet::Ptr<const VarDisReqUpdate>> updateReqs;

    if (type == CREATE_VARIABLES) {
        uint16_t read_len = 0;
        uint16_t expected_len = hdr->getLen();

        if (expected_len > total_length) {
            throw cRuntimeError("RTDB: Create Var IEType is larger than the "
                                "whole packet (%d vs %d)", expected_len,
                                total_length);
        }

        do {
            auto cre = pkt->popAtFront<VarDisCreate>();
            if (cre == nullptr) {
                throw cRuntimeError("RTDB: Popped a null pointer from a "
                                    "received packet...");
            } else {
                read_len += inet::B(cre->getChunkLength()).get();
                creates.push_back(cre);
            }
        } while (read_len < expected_len);

        //Since we've dealt with all the summaries, if there is still packet
        //left to process we should fetch the next header.
        total_length = inet::B(pkt->getDataLength()).get();
        if (total_length > 0) {
            hdr = pkt->popAtFront<IETypeHeader>();
            type = hdr->getType();
        }
    }

    //TODO: Handle delete operations

    if (type == UPDATES) {
        uint16_t read_len = 0;
        uint16_t expected_len = hdr->getLen();

        if (expected_len > total_length) {
            throw cRuntimeError("RTDB: Create Var IEType is larger than the "
                                "whole packet (%d vs %d)", expected_len,
                                total_length);
        }

        do {
            auto upd = pkt->popAtFront<VarDisUpdate>();
            if (upd == nullptr) {
                throw cRuntimeError("RTDB: Popped a null pointer from a "
                                    "received packet...");
            } else {
                read_len += inet::B(upd->getChunkLength()).get();
                updates.push_back(upd);
            }
        } while (read_len < expected_len);

        //Since we've dealt with all the summaries, if there is still packet
        //left to process we should fetch the next header.
        total_length = inet::B(pkt->getDataLength()).get();
        if (total_length > 0) {
            hdr = pkt->popAtFront<IETypeHeader>();
            type = hdr->getType();
        }
    }

    if (type == SUMMARIES) {
           uint16_t read_len = 0;
           uint16_t expected_len = hdr->getLen();

           if (expected_len > total_length) {
               throw cRuntimeError("RTDB: Summaries IEType is larger than the "
                                   "whole packet (%d vs %d)", expected_len,
                                   total_length);
           }

           do {
               auto sum = pkt->popAtFront<VarDisSummary>();
               if (sum == nullptr) {
                   throw cRuntimeError("RTDB: Popped a null pointer from a "
                                       "received packet...");
               } else {
                   read_len += inet::B(sum->getChunkLength()).get();
                   summaries.push_back(sum);
               }
           } while (read_len < expected_len);

           //Since we've dealt with all the summaries, if there is still packet
           //left to process we should fetch the next header.
           total_length = inet::B(pkt->getDataLength()).get();
           if (total_length > 0) {
               hdr = pkt->popAtFront<IETypeHeader>();
               type = hdr->getType();
           }
       }

    if (type == REQUEST_VAR_CREATES) {
        uint16_t read_len = 0;
        uint16_t expected_len = hdr->getLen();

        if (expected_len > total_length) {
            throw cRuntimeError("RTDB: Request Var Create IEType is larger "
                                "than the whole packet (%d vs %d)",
                                expected_len, total_length);
        }

        do {
            auto upd = pkt->popAtFront<VarDisReqCreate>();
            if (upd == nullptr) {
                throw cRuntimeError("RTDB: Popped a null pointer from a "
                                    "received packet...");
            } else {
                read_len += inet::B(upd->getChunkLength()).get();
                createReqs.push_back(upd);
            }
        } while (read_len < expected_len);

        //Since we've dealt with all the summaries, if there is still packet
        //left to process we should fetch the next header.
        total_length = inet::B(pkt->getDataLength()).get();
        if (total_length > 0) {
            hdr = pkt->popAtFront<IETypeHeader>();
            type = hdr->getType();
        }
    }

    if (type == REQUEST_VAR_UPDATES) {
        uint16_t read_len = 0;
        uint16_t expected_len = hdr->getLen();

        if (expected_len > total_length) {
            throw cRuntimeError("RTDB: Request Var Update IEType is larger "
                                "than the whole packet (%d vs %d)",
                                expected_len, total_length);
        }

        do {
            auto upd = pkt->popAtFront<VarDisReqUpdate>();
            if (upd == nullptr) {
                throw cRuntimeError("RTDB: Popped a null pointer from a "
                                    "received packet...");
            } else {
                read_len += inet::B(upd->getChunkLength()).get();
                updateReqs.push_back(upd);
            }
        } while (read_len < expected_len);

        //Since we've dealt with all the summaries, if there is still packet
        //left to process we should fetch the next header.
        total_length = inet::B(pkt->getDataLength()).get();
        if (total_length > 0) {
            hdr = pkt->popAtFront<IETypeHeader>();
            type = hdr->getType();
        }
    }


    //If there is packet left over, something fishy is going on..
    if (total_length > 0) {
        throw cRuntimeError("We failed to deal with all items in the packet, "
                            "despite processing all the currently supported "
                            "IETypes...");
    }

    //Processing order: create, deletes, updates, summaries, var create
    //                  requests, var update requests
    for (auto& it: creates) {
        auto spec = it->getSpec();
        auto upd = it->getUpdate();

        varID_t id = spec.getVarID();
        if (varDB.find(id) == this->varDB.end()) {
            //If the variable is not in the data base, create it!
            inet::MacAddress prodID = spec.getProducer();
            repCnt_t repCnt = spec.getVarRepCnt();
            varDescrLen_t descrLen = spec.getVarDescrLen();
            seqNo_t seqNo = upd.getVarSeqNo();
            uint8_t* descr = new uint8_t[descrLen];
            spec.getVarDescr().copyToBuffer(descr, descrLen);

            varLen_t varLen = upd.getDataLen();
            uint8_t* val = new uint8_t[varLen];
            upd.getVarData().copyToBuffer(val, varLen);

            RTDBVariable* var = new RTDBVariable(prodID, id, repCnt, descrLen,
                                                 descr, varLen, val, seqNo);

            this->varDB[id] = var;

            informApplicationOfUpdate(id, varLen, val);

            delete[] val;
            delete[] descr;

            auto s = new RTDBVarSpec(id, prodID, seqNo, descrLen,
                                     var->getDescription(), repCnt);
            auto u = new RTDBVarUpdate(id, seqNo, varLen, var->getVar(),
                                       repCnt);
            createQ.push(new RTDBVarCreate(s, u));
            reqCreateQRemoveID(id);

            //Push the item onto the summary queue so that we create summaries
            //for the item.
            summaryQ.push(id);
        }
    }

    //TODO Handle deletes

    for (auto& it: updates) {
        varID_t id = it->getVarID();
        if (varDB.find(id) != this->varDB.end()) {
            //If the variable exists...

            auto var = this->varDB[id];
            if (!var->isForDeletion() && var->getProducer() != ourID) {
                //and is not marked for deletion and we aren't the producer...
                seqNo_t seqNo = it->getVarSeqNo();
                EV << ourID << " considering updating variable " << id
                   << " to data w/ seqNo " << seqNo << " compared to "
                   << "the seqNo in our database " << var->getSeqNo()
                   << std::endl;

                if (seqNoMoreRecent(var->getSeqNo(),  seqNo)) {
                    //and the information we received is newer than what we
                    //have in our database, update the variable...
                    EV << ourID << " updating variable " << id
                       << " to data w/ seqNo " << seqNo << std::endl;

                    varLen_t varLen = it->getDataLen();
                    uint8_t* val = new uint8_t[varLen];
                    it->getVarData().copyToBuffer(val, varLen);

                    var->update(val, varLen, seqNo);

                    informApplicationOfUpdate(id, varLen, val);

                    delete[] val;

                    auto update = new RTDBVarUpdate(id, seqNo, varLen,
                                                    var->getVar(),
                                                    var->getRepetitionCount());

                    updateQRemoveID(id);
                    reqUpdateQRemoveID(id);
                    updateQ.push(update);
                }
            }
        } else {
            //If we don't know about the variable, we need to ask our
            //neighbours about it. However, we only perform a single broadcast
            auto req = new RTDBVarReqCreate(id, 1);
            reqCreateQ.push(req);
        }
    }

    for (auto& it: summaries) {
        varID_t id = it->getVarID();
        if (varDB.find(id) != this->varDB.end()) {
            //If we know about the variable
            auto var = this->varDB[id];
            if (!var->isForDeletion() && var->getProducer() != ourID) {
                //it is not marked for deletion and we aren't the producer...
                seqNo_t seqNo = it->getVarSeqNo();
                if (seqNo != var->getSeqNo()) {
                    if (seqNoMoreRecent(var->getSeqNo(),  seqNo)) {
                        //and the information we received is newer than what we
                        //have in our database, we need to ask our neighbours for
                        //that information
                        EV << ourID << " requesting update about variable " << id
                         << " as we've heard that " << sourceID << " has newer "
                         << "information than us for it (" << seqNo << ")"
                         << std::endl;

                        auto req = new RTDBVarReqUpdate(id, var->getSeqNo(),
                                                      var->getRepetitionCount());
                        reqUpdateQ.push(req);
                    } else {
                        //If our neighbour has out of date information about the
                        //variable we need to tell them about our more up to date
                        //information by reissuing an update for the variable.
                        updateQRemoveID(id);
                        auto update = new RTDBVarUpdate(id, var->getSeqNo(),
                                                      var->getVarLen(),
                                                      var->getVar(),
                                                      var->getRepetitionCount());
                        updateQ.push(update);
                    }
                }
            }
        } else {
            //If we don't know about the variable, we need to ask our
            //neighbours about it. However, we only perform a single broadcast
            auto req = new RTDBVarReqCreate(id, 1);
            reqCreateQ.push(req);
        }
    }

    for (auto&it: createReqs) {
        varID_t id = it->getVarID();
        if (varDB.find(id) != this->varDB.end()) {
            //If we know about the variable...
            auto var = this->varDB[id];
            if (!var->isForDeletion()) {
                //and it is not marked for deletion, push what we know about it
                //into the creation queue.
                createQRemoveID(id);

                auto s = new RTDBVarSpec(id, var->getProducer(), var->getSeqNo(),
                                       var->getDescrLen(),
                                       var->getDescription(),
                                       var->getRepetitionCount());
                auto u = new RTDBVarUpdate(id, var->getSeqNo(), var->getVarLen(),
                                         var->getVar(),
                                         var->getRepetitionCount());
                createQ.push(new RTDBVarCreate(s, u));
            }
        }
    }

    for (auto&it: updateReqs) {
        varID_t id = it->getVarID();
        if (varDB.find(id) != this->varDB.end()) {
            //If we know about the variable...
            auto var = this->varDB[id];
            if (!var->isForDeletion()) {
                //and it is not marked for deletion, push what we know about it
                //into the update queue.
                updateQRemoveID(id);
                auto u = new RTDBVarUpdate(id, var->getSeqNo(), var->getVarLen(),
                                         var->getVar(),
                                         var->getRepetitionCount());
                updateQ.push(u);
            }
        }
    }
}
