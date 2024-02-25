//
// This module keeps track of the own position and (through signals) of the positions
// of other nodes. For any node within a configurable safety radius, we keep statistics
// about the times between beacon receptions, to assess update time violations
// 

package swarmstack.beaconing.safetycheck;


simple SafetyChecker 
{
    parameters:
        @signal[SafetyCheckerBeaconIAT](type=simtime_t);
        @signal[SafetyCheckerDeadlineMiss](type=bool);
        @statistic[SafetyCheckerBeaconIAT](record=stats);
        @statistic[SafetyCheckerDeadlineMiss](record=stats);
        double    safetyRadius @unit(m)           =  default(100m);
        double    positionSamplingPeriod @unit(s) =  default(0.01s);
        double    safetyDeadline @unit(s)         =  default(0.05s);
	gates:
	    input  beaconsIn;
}
