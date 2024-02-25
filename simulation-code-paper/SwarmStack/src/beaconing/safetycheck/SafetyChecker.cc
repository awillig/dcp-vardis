/*
 * SafetyChecker.cc
 *
 *  Created on: Oct 26, 2020
 *      Author: awillig
 */


#include <beaconing/safetycheck/SafetyChecker.h>
#include <inet/common/InitStages.h>
#include <inet/common/ModuleAccess.h>
#include <lbp/LocalBroadcastProtocol.h>

Define_Module(SafetyChecker);

using namespace inet;

simsignal_t iatSignalId     = cComponent::registerSignal("SafetyCheckerBeaconIAT");
simsignal_t missSignalId    = cComponent::registerSignal("SafetyCheckerDeadlineMiss");

// -------------------------------------------

struct NodeInformation {
    NodeIdentifier     id;
    Coord              pos;
};


std::map<NodeIdentifier,NodeInformation>  nodeMap;


// -------------------------------------------



void SafetyChecker::initialize(int stage)
{

    if (stage == INITSTAGE_LOCAL)
    {
        gidBeaconsIn = findGate("beaconsIn");
        assert(gidBeaconsIn != -1);

        safetyRadius             = par("safetyRadius");             assert(safetyRadius>0);
        safetyDeadline           = par("safetyDeadline");           assert(safetyDeadline>0);
        positionSamplingPeriod   = par("positionSamplingPeriod");   assert(positionSamplingPeriod>0);

        pMsgSamplePosition = new cMessage ("PeriodicBeaconing::SamplePosition");
        pMsgCheckNeighbors = new cMessage ("PeriodicBeaconing::CheckNeighbors");
        assert (pMsgSamplePosition);
        assert (pMsgCheckNeighbors);
        scheduleAt (simTime(), pMsgSamplePosition);

        mobility           = nullptr;
    }

    if (stage == INITSTAGE_LAST)
    {
        cModule *host = getContainingNode (this);
        assert(host);
        mobility      = check_and_cast<IMobility *>(host->getSubmodule("mobility"));
        assert(mobility);

        LocalBroadcastProtocol *lbp    = check_and_cast<LocalBroadcastProtocol*>(host->getSubmodule("lbp"));
        assert(lbp);
        ownIdentifier  =  lbp->getOwnMacAddress();
    }
}

// -------------------------------------------

SafetyChecker::~SafetyChecker()
{
    cancelAndDelete(pMsgSamplePosition);
    cancelAndDelete(pMsgCheckNeighbors);
}

// -------------------------------------------

void SafetyChecker::samplePosition (void)
{
  EV << "Entering SafetyChecker::samplePosition" << endl;

  assert (mobility);
  currPosition = mobility->getCurrentPosition();

  NodeInformation selfinfo;
  selfinfo.id   = ownIdentifier;
  selfinfo.pos  = currPosition;

  nodeMap[ownIdentifier] = selfinfo;

  scheduleAt(simTime() + positionSamplingPeriod, pMsgSamplePosition);
  scheduleAt(simTime() + 0.00001, pMsgCheckNeighbors);
}

// -------------------------------------------

void SafetyChecker::checkNeighbors()
{
    EV << "Entering SafetyChecker::checkNeighbors" << endl;

    for (auto const& node : nodeMap)
    {
        NodeIdentifier nodeId  = node.first;
        Coord          nodePos = node.second.pos;

        if (nodeId != ownIdentifier)
        {
            double dist = nodePos.distance(currPosition);

            EV << "SafetyChecker[" << ownIdentifier
               << "]: got position update from " << nodeId
               << " at distance " << dist
               << std::endl;

            if (dist <= safetyRadius)
            {
                if (safetyNeighbors.find(nodeId) == safetyNeighbors.end())
                {
                    safetyNeighbors[nodeId] = simTime();
                    EV << "SafetyChecker[" << ownIdentifier
                       << "]: adding it to map"
                       << std::endl;
                }
            }
            else
            {
                // have we just left our safety neighborhood?
                if (safetyNeighbors.find(nodeId) != safetyNeighbors.end())
                {
                    safetyNeighbors.erase(nodeId);
                    EV << "SafetyChecker[" << ownIdentifier
                       << "]: removing it from map"
                       << std::endl;
                }
            }

        }
    }
}

// -------------------------------------------


void SafetyChecker::handleMessage(cMessage* msg)
{
    if (dynamic_cast<BeaconReport*>(msg) && msg->arrivedOn(gidBeaconsIn))
    {
       processBeaconReport((BeaconReport*) msg);
       return;
    }

    if (msg == pMsgSamplePosition)
    {
        samplePosition();
        return;
    }

    if (msg == pMsgCheckNeighbors)
    {
        checkNeighbors();
        return;
    }

    error("SafetyChecker::handleMessage: illegal message type or arrival gate");
}

// -------------------------------------------


void SafetyChecker::processBeaconReport(BeaconReport* report)
{
    assert(report);

    NodeIdentifier   senderId = report->getSenderId();

    if (safetyNeighbors.find(senderId) != safetyNeighbors.end())
    {
        simtime_t  lastTime = safetyNeighbors[senderId];
        simtime_t  currTime = simTime();
        simtime_t  iat      = currTime - lastTime;
        emit(iatSignalId, iat);
        safetyNeighbors[senderId] = currTime;
        if (iat >= safetyDeadline)
        {
            emit(missSignalId, true);
        }
        else
        {
            emit(missSignalId, false);
        }
    }

    delete report;
}

