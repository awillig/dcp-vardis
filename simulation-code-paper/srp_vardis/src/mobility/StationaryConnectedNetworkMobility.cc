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

#include "StationaryConnectedNetworkMobility.h"

Define_Module(StationaryConnectedNetworkMobility);

void StationaryConnectedNetworkMobility::initialize(int stage)
{
    StationaryMobilityBase::initialize(stage);
    EV_WARN << "initialising StationaryConnectedNetworkMobility stage " << stage << endl;
    if (stage == inet::INITSTAGE_LOCAL) {
        maxSeparation = par("maxSeparation").doubleValue();
//        std::cout << maxSeparation << std::endl;
    }
}

void StationaryConnectedNetworkMobility::setInitialPosition()
{
    int index = subjectModule->getIndex();

    if (index == 0) {
        //If we are the first module to be placed. Just generate a new random
        //position within our constraint bounds.
        lastPosition = getRandomPosition();
    } else {
        //If we are after the first module, generate a random position. Then
        //check that we are within maxSeparation metres of at least one other
        //module, until we have a position which works.
        bool done = false;
        int j = 0;
        while (!done) {
            lastPosition = getRandomPosition();

            cModule* parent = subjectModule->getParentModule();
            int i = 0;
            while (i < index && !done) {
                cModule* module = parent->getSubmodule(subjectModule->getName(), i)->getSubmodule("mobility");
                inet::Coord pos = dynamic_cast<StationaryMobilityBase*>(module)->getCurrentPosition();
                double dist = lastPosition.distance(pos);
                if (dist < maxSeparation) {
                    done = true;
//                    std::cout << index << "," << dist << std::endl;
                }
                i++;
            }
        }
    }
}

void StationaryConnectedNetworkMobility::finish()
{
    StationaryMobilityBase::finish();
    recordScalar("x", lastPosition.x);
    recordScalar("y", lastPosition.y);
    recordScalar("z", lastPosition.z);
}
