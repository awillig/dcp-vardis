/*
 * NeighborTable.h
 *
 *  Created on: Sep 6, 2020
 *      Author: awillig
 */

#ifndef BEACONING_NEIGHBORTABLE_NEIGHBORTABLE_H_
#define BEACONING_NEIGHBORTABLE_NEIGHBORTABLE_H_

#include <map>
#include <list>
#include <iostream>
#include <omnetpp.h>
#include <beaconing/base/BeaconingBase.h>
#include <beaconing/base/Beacon_m.h>
#include <beaconing/neighbortable/PacketLossEstimator.h>
#include <inet/common/geometry/common/Coord.h>
#include <inet/common/geometry/common/EulerAngles.h>
#include <inet/linklayer/common/MacAddress.h>


using namespace omnetpp;


// ================================================================
// ================================================================

class NeighborTableEntry {

 protected:
  NodeIdentifier            nbId;
  simtime_t                 lastBeacon;
  Coord                     lastPosition;
  Coord                     lastVelocity;
  uint32_t                  numObservedBeacons;
  EwmaPacketLossEstimator   plrEst;

 public:
  NeighborTableEntry();
  NeighborTableEntry(NodeIdentifier id);
  NeighborTableEntry (NodeIdentifier id, simtime_t lastB, Coord lastPos, Coord lastVel, double alpha);

  NodeIdentifier getNeighborId() const {return nbId;};
  simtime_t  getLastBeaconReceptionTime() const {return lastBeacon;};
  uint32_t   getLastSequenceNumber() const {return plrEst.getLastSequenceNumber();};
  Coord      getLastPosition() const {return lastPosition;};
  Coord      getLastVelocity() const {return lastVelocity;};
  double     getPacketLossRate() const {return plrEst.getCurrentPacketLossRate();};
  uint32_t   getNumObservedBeacons() const {return numObservedBeacons;};

  EwmaPacketLossEstimator& getEstimator() {return plrEst;};

  void setLastBeaconReceptionTime(simtime_t lastt) {lastBeacon=lastt;};
  void setLastPosition(Coord lastp) {lastPosition=lastp;};
  void setLastVelocity(Coord lastv) {lastVelocity=lastv;};
  void incrNumObservedBeacons() {numObservedBeacons += 1;};

  friend std::ostream& operator<<(std::ostream& os, const NeighborTableEntry& nte);
};


// ================================================================
// ================================================================

class NeighborTable : public cSimpleModule {
 protected:

  std::map<NodeIdentifier, NeighborTableEntry>  nbTable;

  double     ewmaAlpha;
  simtime_t  timeoutTime;
  simtime_t  cleanupPeriod;

  cMessage  *pMsgTableCleanup;

 public:

  virtual void initialize(int stage);
  virtual void finish();
  virtual void handleMessage(cMessage *msg);
  virtual int numInitStages () const override { return inet::NUM_INIT_STAGES;};

 public:

  NeighborTable () { ewmaAlpha = 0.95; timeoutTime = 10.0; };
  NeighborTable (double alpha, double timeout);

  double getEwmaAlpha () const {return ewmaAlpha;};
  void   setEwmaAlpha (double alpha);

  double getTimeoutTime () const {return timeoutTime.dbl();};
  void   setTimeoutTime (simtime_t timeout);

  double getCleanupPeriod () const {return cleanupPeriod.dbl();};
  void   setCleanupPeriod (simtime_t period);

  void recordBeacon(const Beacon* beaon);
  void cleanupTable (void);

  bool entryAvailable (NodeIdentifier id) {return nbTable.find(id) != nbTable.end();};
  NeighborTableEntry& operator[] (const NodeIdentifier& id) {return nbTable[id];};
  size_t size (void) const {return nbTable.size();};

  std::list<NodeIdentifier> getNeighborIdList (void);

  friend std::ostream& operator<<(std::ostream& os, const NeighborTable& nt);
};





#endif /* BEACONING_NEIGHBORTABLE_NEIGHBORTABLE_H_ */
