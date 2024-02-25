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

#ifndef __SRP_VARDIS_SRP_H_
#define __SRP_VARDIS_SRP_H_

#include <omnetpp.h>
#include <inet/mobility/contract/IMobility.h>
#include <inet/common/geometry/common/Coord.h>
#include <inet/linklayer/common/MacAddress.h>
#include <list>

using namespace omnetpp;

class NeighbourData {
public:
    NeighbourData(inet::MacAddress id_, inet::Coord pos_, inet::Coord vel_, simtime_t data_ts_, double expiration_);

    inet::MacAddress id;
    inet::Coord pos;
    inet::Coord velocity;
    simtime_t dataTimestamp;
    double expiration;
};


class SRP : public cSimpleModule
{
  public:
    ~SRP(void);

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual int  numInitStages () const override { return inet::NUM_INIT_STAGES; };

  private:
    inet::IMobility* mobility = nullptr;
    cMessage* gcMessage;
    cMessage* posCheckMessage;

    inet::Coord ourPos;
    inet::Coord ourVelocity;
    simtime_t mobInfoTimestamp;

    double garbageCollectionPeriod;
    double posCheckPeriod;
    double staleDataTimeout;

    std::list<NeighbourData> neighbourData;

    void handleSelfMessage(cMessage* msg);
};

#endif
