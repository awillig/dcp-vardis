/*
 * BeaconingBase.cc
 *
 *  Created on: Sep 6, 2020
 *      Author: awillig
 */


#include <assert.h>
#include <iostream>
#include <cstdio>
#include <beaconing/base/BeaconingBase.h>


// -------------------------------------------

bool beaconWellFormed (const Beacon *beacon)
{
  assert(beacon);

  if (beacon->getMagicNo() != SWARMSTACK_BEACON_MAGICNO)
    {
      EV << "beaconWellFormed: magicno is wrong, beacon = " << beacon << std::endl;
      return false;
    }

  if (beacon->getVersion() != SWARMSTACK_VERSION)
    {
      EV << "beaconWellFormed: wrong version number, beacon = " << beacon << std::endl;
      return false;
    }

  return true;

}
