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

#include "Experiment3Application.h"
#include "messages/RTDBVarUpdateIndication_m.h"

Define_Module(Experiment3Application);

typedef struct __attribute__((packed)) {
    double time;
    uint32_t seq_no;
} variable_t;

void Experiment3Application::initialize(int stage)
{
    BasicApplication::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        current_seqno = 0;
        delaySig = registerSignal("updateDelaySignal");
        seqnoDif = registerSignal("seqnoDeltaSignal");
        delayHist = registerSignal("updateDelayHistSignal");
        seqnoDifHist = registerSignal("seqnoDeltaHistSignal");
    }

    if (stage == inet::INITSTAGE_LAST) {
        id_no = (int)((varID_t)(ourId.getInt()));
        num_nodes = par("numNodes").intValue();
        if (par("onlyFinalLogging").boolValue()) {
            if (id_no == num_nodes) {
                log_data = true;
            }
        } else {
            log_data = true;
        }

        int n = sqrt(num_nodes);

        int index = id_no - 1;
        int centre_index = ((n * n) / 2);
        int intermediate_index = ((n + 1 ) * (n / 4));
        if (index == 0) {
            node_of_interest = true;
        } else if (index == centre_index) {
            node_of_interest = true;
        } else if (index == intermediate_index) {
            node_of_interest = true;
        } else {
            node_of_interest = false;
        }
    }
}

void Experiment3Application::handleMessage(cMessage *msg)
{
    if (dynamic_cast<RTDBVarUpdateIndication*>(msg)) {
        if (log_data) {
            auto indication = static_cast<RTDBVarUpdateIndication*>(msg);
            if (indication->getVarLen() != sizeof(variable_t)) {
                throw cRuntimeError("Variable is the wrong size! %d vs %ld", indication->getVarLen(), sizeof(variable_t));
            }

            union {
                char* array;
                variable_t* data;
            } buffer;

            buffer.array = new char[sizeof(variable_t)];
            for (int i = 0; i < sizeof(variable_t); i++) {
                buffer.array[i] = indication->getVarBuf(i);
            }
            variable_t* data = buffer.data;

            varID_t id = indication->getVarID();
            if (db.find(id) != db.end()) {
                if (id == num_nodes) {
                    emit(seqnoDif, data->seq_no - db[id]);
                    emit(delaySig, 1000.0d * (SIMTIME_DBL(simTime()) - data->time));

                    if (node_of_interest) {
                        emit(seqnoDifHist, data->seq_no - db[id]);
                        emit(delayHist, 1000.0d * (SIMTIME_DBL(simTime()) - data->time));
                    }
                }
            }
            db[id] = data->seq_no;
        }

        delete msg;
    } else {
        //Let the basic application handle the main response loop. We are just
        //responsible for recording results and implementing an overridden
        //version of the
        //The basic application deletes the message for us.
        BasicApplication::handleMessage(msg);
    }
}


char* Experiment3Application::generateVariablePayload(void) {
    variable_t data;

    data.time = SIMTIME_DBL(simTime());
    data.seq_no = ++current_seqno;

    char* b = new char[sizeof(variable_t)];
    memcpy(b, &data, sizeof(variable_t));
    this->variableSize = sizeof(variable_t);
    return b;
}
