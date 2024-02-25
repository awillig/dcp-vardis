/*
 * SwarmStackBase.h
 *
 *  Created on: Sep 6, 2020
 *      Author: awillig
 */

#ifndef BASE_SWARMSTACKBASE_H_
#define BASE_SWARMSTACKBASE_H_

#include <omnetpp.h>
#include <cstdio>
#include <inet/linklayer/common/MacAddress.h>

using namespace omnetpp;

typedef inet::MacAddress    MacAddress;
typedef inet::MacAddress    NodeIdentifier;

static const NodeIdentifier   nullIdentifier = MacAddress::UNSPECIFIED_ADDRESS;

std::string xsprintf(const std::string fmt_str, ...);

#endif /* BASE_SWARMSTACKBASE_H_ */
