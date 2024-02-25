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

#include "DefinedGridMobility.h"

Define_Module(DefinedGridMobility);

void DefinedGridMobility::initialize(int stage)
{
    StationaryMobilityBase::initialize(stage);
    EV_WARN << "initialising StationaryConnectedNetworkMobility stage " << stage << endl;
    if (stage == inet::INITSTAGE_LOCAL) {
        separation = par("separation").doubleValue();
        numNodesX = par("numNodesX");
        numNodesY = par("numNodesY");
    }
}

void DefinedGridMobility::setInitialPosition()
{
    int index = subjectModule->getIndex();

    double x_offset = separation * (index % numNodesX);
    double y_offset = separation * floor(index / numNodesX);

    lastPosition = inet::Coord(x_offset, y_offset, 0);
}

void DefinedGridMobility::finish()
{
    StationaryMobilityBase::finish();
    recordScalar("x", lastPosition.x);
    recordScalar("y", lastPosition.y);
    recordScalar("z", lastPosition.z);
}
