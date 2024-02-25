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

#include "FixedDistanceFixedNode.h"

Define_Module(FixedDistanceFixedNode);

void FixedDistanceFixedNode::initialize(int stage)
{
    StationaryMobilityBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        separation = par("separation").doubleValue();
        numNodes = par("numNodesX");
        finalNodeX = par("finalNodeX").doubleValue();
    }
}

void FixedDistanceFixedNode::setInitialPosition()
{
    int index = subjectModule->getIndex();
    double x_offset;
    if (index == (numNodes - 1)) {
        x_offset = finalNodeX;
    } else {
        x_offset = separation * index;
    }

    lastPosition = inet::Coord(x_offset, 0, 0);
}

void FixedDistanceFixedNode::finish()
{
    StationaryMobilityBase::finish();
    recordScalar("x", lastPosition.x);
    recordScalar("y", lastPosition.y);
    recordScalar("z", lastPosition.z);
}
