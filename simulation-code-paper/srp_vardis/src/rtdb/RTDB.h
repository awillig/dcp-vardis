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

#ifndef __SRD_VARDIS_RTDB_H_
#define __SRD_VARDIS_RTDB_H_

#include <omnetpp.h>

#include "srp_vardis_config.h"

#include <unordered_map>
#include <queue>
#include "RTDBVariable.h"
#include "RTDBInformationElements.h"

#include "messages/rtdb_api/RTDBCreate_m.h"
#include "messages/rtdb_api/RTDBUpdate_m.h"

#include <inet/linklayer/common/MacAddress.h>
#include <inet/common/packet/Packet.h>

#include <fstream>
#include <iostream>
#include <ctime>

using namespace omnetpp;


class RTDB : public cSimpleModule
{
  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual int  numInitStages () const override { return inet::NUM_INIT_STAGES; };
    ~RTDB();

  private:
    int MAX_VARIABLE_LEN;
    int MAX_DESCRIPTION_LEN;
    int MAX_REPETITIONS;
    int MAX_PACKET_SIZE;
    int MAX_NUM_SUMMARIES;

    inet::MacAddress ourID = inet::MacAddress::UNSPECIFIED_ADDRESS;

    std::unordered_map<varID_t, RTDBVariable*> varDB;
    std::queue<RTDBVarCreate*> createQ;
    std::queue<varID_t> summaryQ;
    std::queue<RTDBVarUpdate*> updateQ;
    std::queue<RTDBVarReqUpdate*> reqUpdateQ;
    std::queue<RTDBVarReqCreate*> reqCreateQ;
    std::queue<RTDBVarDelete*> deleteQ;


    void sendAPIResponse(int, int);
    void handleSelfMessage(cMessage*);

    void handleAPICreateReq(RTDBCreate* req);
    void handleAPIUpdateReq(RTDBUpdate* req);

    void constructBeacon(int initBeaconLen);

    void getOurID(void);

    void processVarDisBeacon(inet::Packet* pkt);

    void updateQRemoveID(varID_t id);
    void createQRemoveID(varID_t id);
    void reqCreateQRemoveID(varID_t id);
    void reqUpdateQRemoveID(varID_t id);

    void informApplicationOfUpdate(varID_t id, varLen_t len, uint8_t* val);

    static std::time_t start_time;
};

#endif
