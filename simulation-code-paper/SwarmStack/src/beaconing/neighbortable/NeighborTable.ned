package swarmstack.beaconing.neighbortable;

//
// TODO auto-generated module
//
simple NeighborTable
{
    parameters:
               double  tableCleanupPeriod @unit(s)      =  default(0.1s);
               double  tableTimeoutValue @unit(s)       =  default(10.0s);
               double  plrEstimatorAlpha                =  default(0.95);
   gates:
      input  beaconInput @labels(BStaticBeaconingPacket/up);
}
