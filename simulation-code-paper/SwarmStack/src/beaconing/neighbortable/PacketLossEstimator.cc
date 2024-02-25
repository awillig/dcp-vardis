/*
 * PacketLossEstimator.cc
 *
 *  Created on: Sep 6, 2020
 *      Author: awillig
 */


#include <beaconing/neighbortable/PacketLossEstimator.h>
#include <assert.h>


// ----------------------------------------------------

EwmaPacketLossEstimator::EwmaPacketLossEstimator (double alpha)
  :PacketLossEstimator()
{
  setAlpha(alpha);
}

// ----------------------------------------------------

void EwmaPacketLossEstimator::setAlpha(double alpha)
{
  assert((0.0 <= alpha) && (alpha <= 1));
  ewmaAlpha = alpha;
}

// ----------------------------------------------------

double EwmaPacketLossEstimator::calculateUpdatedPacketLossRate(uint32_t newSeqno, simtime_t newTime, double oldplr)
{
  uint32_t diff   =  newSeqno - lastSeqno;
  for (uint32_t i = 0; i<diff-1; i++)
    {
      oldplr = (1-ewmaAlpha) + oldplr * ewmaAlpha;
    }
  return oldplr * ewmaAlpha;
}



