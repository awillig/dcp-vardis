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

#include "SeqnoSource.h"
#include "SeqnoData_m.h"

#include <inet/linklayer/common/MacAddress.h>
#include <lbp/LocalBroadcastProtocol.h>
#include <inet/common/packet/Packet.h>

Define_Module(SeqnoSource);

void SeqnoSource::initialize(int stage) {
    if (stage == INITSTAGE_LOCAL) {
        this->variableID = par("variableID").intValue();
	updateGeneratedSignal = registerSignal("updateGeneratedSignal");
    } else if (stage == INITSTAGE_LAST) {
        cModule *host = getContainingNode (this);
        assert(host);
        LocalBroadcastProtocol *lbp = check_and_cast<LocalBroadcastProtocol*>(host->getSubmodule("lbp"));
        assert(lbp);

        auto ourId = lbp->getOwnMacAddress();

        if (this->variableID == -1) {
            this->variableID = (uint32_t)(ourId.getInt());
        }

        if (this->variableID != -2) {
            wakeup = new cMessage ("SeqnoSource::wakeup");
            scheduleAt(simTime() + (par("variableUpdatePeriodDist").doubleValue() / 1000.0f), wakeup);
        }
    }
}

void SeqnoSource::handleMessage(cMessage *msg) {
    if (msg == wakeup) {
        auto update = inet::makeShared<SeqnoData>();
        update->setVarId(variableID);
        update->setSeqNo(current_seqno++);
        update->setCreationTime(SIMTIME_DBL(simTime()));
        update->setChunkLength(inet::B(2 + 4 + 8));

        auto pkt = new inet::Packet("Update", update);
        send(pkt, "toLower");

	emit(updateGeneratedSignal, true);
	
        scheduleAt(simTime() + (par("variableUpdatePeriodDist").doubleValue() / 1000.0f), wakeup);
    }
}

SeqnoSource::~SeqnoSource(void) {
    cancelAndDelete(wakeup);
}
