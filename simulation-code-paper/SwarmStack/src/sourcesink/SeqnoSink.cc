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

#include "SeqnoSink.h"
#include "SeqnoData_m.h"

#include <inet/common/packet/Packet.h>
#include <inet/linklayer/common/MacAddress.h>
#include <lbp/LocalBroadcastProtocol.h>

Define_Module(SeqnoSink);

void SeqnoSink::initialize(int stage) {
    if (stage == inet::INITSTAGE_LOCAL) {
        num_nodes = par("numNodes").intValue();
        delaySig = registerSignal("updateDelaySignal");
        seqnoDif = registerSignal("seqnoDeltaSignal");

        delayHist = registerSignal("updateDelayHistSignal");
        seqnoDifHist = registerSignal("seqnoDeltaHistSignal");
    } else if (stage == inet::INITSTAGE_LAST) {
        cModule *host = getContainingNode (this);
        assert(host);
        LocalBroadcastProtocol *lbp = check_and_cast<LocalBroadcastProtocol*>(host->getSubmodule("lbp"));
        assert(lbp);

        auto ourId = lbp->getOwnMacAddress();
        int id_no = (uint32_t)(ourId.getInt());

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

void SeqnoSink::handleMessage(cMessage *msg) {
    if (dynamic_cast<inet::Packet*>(msg)) {
        inet::Packet* packet = (inet::Packet*) msg;
        auto upd = packet->popAtFront<SeqnoData>();
        auto id = upd->getVarId();
        auto seqno = upd->getSeqNo();
        auto time = upd->getCreationTime();
	
        if (db.find(id) != db.end()) {
            if (id == num_nodes) {
                emit(seqnoDif, seqno - db[id]);
                emit(delaySig, 1000.0d * (SIMTIME_DBL(simTime()) - time));

                if (node_of_interest) {
                    emit(seqnoDifHist, seqno - db[id]);
                    emit(delayHist, 1000.0d * (SIMTIME_DBL(simTime()) - time));
                }

            }
        }
        db[id] = seqno;
    }
    delete msg;
}
