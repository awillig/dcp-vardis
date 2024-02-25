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

#ifndef __SRD_VARDIS_BASICAPPLICATION_H_
#define __SRD_VARDIS_BASICAPPLICATION_H_

#include <omnetpp.h>
#include <inet/common/InitStages.h>
#include <inet/linklayer/common/MacAddress.h>

using namespace omnetpp;

typedef enum {
    RTDB_VARIABLE_UNDEFINED,
    RTDB_VARIABLE_CREATION_REQUESTED,
    RTDB_VARIABLE_CREATED,
    RTDB_VARIABLE_UPDATE_REQUESTED,
    RTDB_VARIABLE_UPDATED
} variable_state_t;


class BasicApplication : public cSimpleModule
{
  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual int  numInitStages () const override { return inet::NUM_INIT_STAGES; };
    int variableSize;
    int variableID;
    inet::MacAddress ourId;

  private:
    int variableRep;
    variable_state_t varState = RTDB_VARIABLE_UNDEFINED;

    virtual char* generateVariablePayload(void);
};

#endif
