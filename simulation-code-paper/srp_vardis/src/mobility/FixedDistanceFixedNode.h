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

#ifndef __SRP_VARDIS_FIXEDDISTANCEFIXEDNODE_H_
#define __SRP_VARDIS_FIXEDDISTANCEFIXEDNODE_H_

#include <omnetpp.h>

using namespace omnetpp;

#include <inet/mobility/base/StationaryMobilityBase.h>

class FixedDistanceFixedNode : public inet::StationaryMobilityBase
{
    private:
        int numNodes;
        double separation;
        double finalNodeX;

    protected:
        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

        /** @brief Initialises mobility model parameters.*/
        virtual void initialize(int stage) override;

        /** @brief Initialises the position according to the mobility model. */
        virtual void setInitialPosition() override;

        /** @brief Save the host position. */
        virtual void finish() override;

    public:
        FixedDistanceFixedNode() {};
};

#endif
