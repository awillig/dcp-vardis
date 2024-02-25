/*
 * PacketLossEstimator.h
 *
 *  Created on: Sep 6, 2020
 *      Author: awillig
 */

#ifndef BEACONING_NEIGHBORTABLE_PACKETLOSSESTIMATOR_H_
#define BEACONING_NEIGHBORTABLE_PACKETLOSSESTIMATOR_H_

#include <omnetpp.h>

using namespace omnetpp;

// ================================================================
// ================================================================


// Packet loss estimators work on a few assumptions:
//  - they work solely on incoming sequence numbers of type uint32_t

class PacketLossEstimator {

 protected:

  double         currentPLR;
  uint32_t       lastSeqno;
  simtime_t      lastTime;
  bool           hasObservation;


 public:

  PacketLossEstimator () {currentPLR = 0.0; lastSeqno = 0; lastTime = 0; hasObservation = false;};

  double     getCurrentPacketLossRate() const {return currentPLR;};
  uint32_t   getLastSequenceNumber() const {return lastSeqno;};
  simtime_t  getLastTime() const {return lastTime;};

  virtual void recordObservation (uint32_t newseqno, simtime_t newtime)
  {
      if (!hasObservation)
      {
          hasObservation  = true;
          currentPLR      = 0;
          lastSeqno       = newseqno;
          lastTime        = newtime;
          return;
      }

      assert(newseqno > lastSeqno);
      currentPLR = calculateUpdatedPacketLossRate(newseqno, newtime, currentPLR);
      lastSeqno  = newseqno;
      lastTime   = newtime;
  };

  virtual double calculateUpdatedPacketLossRate(uint32_t seqno, simtime_t time, double oldplr) = 0;

};


// ================================================================
// ================================================================

class EwmaPacketLossEstimator : public PacketLossEstimator {
 protected:
  double     ewmaAlpha;    // weight of the history

 public:
  EwmaPacketLossEstimator(double alpha);
  virtual double calculateUpdatedPacketLossRate(uint32_t newSeqno, simtime_t newTime, double oldplr);

  void setAlpha (double alpha);
};




#endif /* BEACONING_NEIGHBORTABLE_PACKETLOSSESTIMATOR_H_ */
