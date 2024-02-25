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

#include "BasicApplication.h"
#include <string.h>

#include <inet/linklayer/common/MacAddress.h>
#include <lbp/LocalBroadcastProtocol.h>

#include "messages/rtdb_api/RTDBResponseCode_m.h"
#include "messages/rtdb_api/RTDBCreate_m.h"
#include "messages/rtdb_api/RTDBUpdate_m.h"

#include "srp_vardis_config.h"

#define UPDATE_VARIABLE_MSG "UpdateVar"
#define CREATE_VARIABLE_MSG "CreateVar"

Define_Module(BasicApplication);


void BasicApplication::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {
        this->variableID = par("variableID").intValue();
        this->variableSize = par("variableSize").intValue();
        this->variableRep = par("variableRepetitions").intValue();
    } else if (stage == INITSTAGE_LAST) {
        cModule *host = getContainingNode (this);
        assert(host);
        LocalBroadcastProtocol *lbp = check_and_cast<LocalBroadcastProtocol*>(host->getSubmodule("lbp"));
        assert(lbp);

        ourId = lbp->getOwnMacAddress();

        if (this->variableID == -1) {
            this->variableID = (int)((varID_t)(ourId.getInt()));
        }

        if (this->variableID != -2) {
            scheduleAt(simTime() + (par("variableUpdatePeriodDist").doubleValue() / 1000.0f),
                                   new cMessage(CREATE_VARIABLE_MSG));
        }
    }
}


void BasicApplication::handleMessage(cMessage *msg) {
    if (dynamic_cast<RTDBResponseCode*>(msg)) {
        auto rtdb_msg = static_cast<RTDBResponseCode*>(msg);
        int r_kind = rtdb_msg->getResponseKind();
        int r_code = rtdb_msg->getResponseCode();


        bool response_type_mismatch = (r_kind == RTDB_CREATE && this->varState != RTDB_VARIABLE_CREATION_REQUESTED) ||
                                      (r_kind == RTDB_UPDATE && this->varState != RTDB_VARIABLE_UPDATE_REQUESTED);

        if (response_type_mismatch) {
            //If the response is the wrong kind for the current variable state
            throw cRuntimeError("Basic VarDis Application: RTDB module response "
                                "kind does not match our current state!");
        }

        if (r_code != RC_OK) {
            throw cRuntimeError("Basic VarDis Application: RTDB module request "
                                "failed (request type: %d, error code: %d)",
                                r_kind, r_code);
        }

        if (this->varState == RTDB_VARIABLE_CREATION_REQUESTED) {
            this->varState = RTDB_VARIABLE_CREATED;
        } else if (this->varState == RTDB_VARIABLE_UPDATE_REQUESTED) {
            this->varState = RTDB_VARIABLE_UPDATED;
        }

        //Schedule the next update based on the random distribution we are
        //configured with.
        scheduleAt(simTime() + (par("variableUpdatePeriodDist").doubleValue() / 1000.0f),
                   new cMessage(UPDATE_VARIABLE_MSG));
    } else if (!strcmp(msg->getName(), UPDATE_VARIABLE_MSG)) {
        if (this->varState == RTDB_VARIABLE_CREATED ||
            this->varState == RTDB_VARIABLE_UPDATED) {

            //Send the update request to the RTDB
            auto rtdb_req = new RTDBUpdate();
            rtdb_req->setVarID(this->variableID);

            auto buf = generateVariablePayload();

            //Fill the variable with random data!
            for (int i = 0; i < this->variableSize; ++i) {
                rtdb_req->appendVarBuf(buf[i]);
            }
            delete[] buf;

            rtdb_req->setVarLen(this->variableSize);

            send(rtdb_req, "rtdb$o");

            this->varState = RTDB_VARIABLE_UPDATE_REQUESTED;
        } else {
            throw cRuntimeError("Basic VarDis Application: Received a variable "
                                 "update generation request before the "
                                 "previous request had completed!");
        }
    } else if (!strcmp(msg->getName(), CREATE_VARIABLE_MSG)) {
        if (this->varState == RTDB_VARIABLE_UNDEFINED) {
            //Generate random value and send the update request to the RTDB
            auto rtdb_req = new RTDBCreate();
            rtdb_req->setVarID(this->variableID);

            auto buf = generateVariablePayload();

            //Fill the variable with random data!
            for (int i = 0; i < this->variableSize; ++i) {
                rtdb_req->appendVarBuf(buf[i]);
            }
            delete[] buf;

            rtdb_req->setVarLen(this->variableSize);
            rtdb_req->setVarRepCnt(this->variableRep);
            rtdb_req->setVarDescr("DUMMY VARIABLE");

            send(rtdb_req, "rtdb$o");

            this->varState = RTDB_VARIABLE_CREATION_REQUESTED;
        } else {
            throw cRuntimeError("Basic VarDis Application: Received a variable "
                                 "update generation request before the "
                                 "previous request had completed!");
        }
    }

    delete msg; //Always dispose of the received message
}

char* BasicApplication::generateVariablePayload(void) {
    char* buf = new char[this->variableSize];

    for (int i = 0; i < this->variableSize; i++) {
        buf[i] = static_cast<char>(uniform(0, 256));
    }

    return buf;
}

