/*
 * BeaconingBase.h
 *
 *  Created on: Sep 6, 2020
 *      Author: awillig
 */

#ifndef BEACONING_BASE_BEACONINGBASE_H_
#define BEACONING_BASE_BEACONINGBASE_H_

#include <omnetpp.h>
#include <cstdio>
#include <inet/common/geometry/common/Coord.h>
#include <inet/linklayer/common/MacAddress.h>
#include <base/SwarmStackBase.h>
#include <beaconing/base/Beacon_m.h>

using namespace omnetpp;

typedef inet::Coord         Coord;

static const uint16_t   SWARMSTACK_BEACON_MAGICNO    = 0x497E;
static const uint16_t   SWARMSTACK_VERSION           = 1;
static const uint32_t   SWARMSTACK_BEACON_KIND       = 4805;

bool beaconWellFormed (const Beacon *beacon);

#endif /* BEACONING_BASE_BEACONINGBASE_H_ */
