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

#ifndef __SWARMSTACK_SEQNOSINK_H_
#define __SWARMSTACK_SEQNOSINK_H_

#include <omnetpp.h>
#include <unordered_map>
#include <inet/common/InitStages.h>

using namespace omnetpp;


class SeqnoSink : public cSimpleModule
{
  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual int  numInitStages () const override { return inet::NUM_INIT_STAGES; };

  private:
    simsignal_t delaySig;
    simsignal_t seqnoDif;
    simsignal_t delayHist;
    simsignal_t seqnoDifHist;
    int num_nodes;
    bool node_of_interest;

    std::unordered_map<uint32_t, uint32_t> db;
};

#endif
