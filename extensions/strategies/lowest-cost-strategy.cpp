/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Klaus Schneider, University of Bamberg, Germany
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Softwarestd::string params = "maxdelay=100";
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Klaus Schneider <klaus.schneider@uni-bamberg.de>
 */
#include "lowest-cost-strategy.hpp"
#include "core/logger.hpp"
#include "fw/measurement-info.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("LowestCostStrategy");

const Name LowestCostStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/lowest-cost/%FD%01/");
NFD_REGISTER_STRATEGY(LowestCostStrategy);

LowestCostStrategy::LowestCostStrategy(Forwarder& forwarder, const Name& name)
 :  Strategy(forwarder, name), 
    ownStrategyChoice(forwarder.getStrategyChoice()),
    priorityType(RequirementType::DELAY)
{
}

void LowestCostStrategy::afterReceiveInterest(const Face& inFace, 
                                              const Interest& interest,
                                              shared_ptr<fib::Entry> fibEntry, 
                                              shared_ptr<pit::Entry> pitEntry)
{
  if (interest.isTainted())
  {
    NFD_LOG_INFO("Incoming tainted interest: " << interest.getName());
  }

  Name currentPrefix;
  shared_ptr < MeasurementInfo > measurementInfo;
  nfd::MeasurementsAccessor& ma = this->getMeasurements();
  std::tie(currentPrefix, measurementInfo) = StrategyHelper::findPrefixMeasurements(interest, ma);

  // Check if prefix is unknown
  if (measurementInfo == nullptr) 
  {
    // Create new prefix
    nfd::MeasurementsAccessor& ma = this->getMeasurements();
    measurementInfo = StrategyHelper::addPrefixMeasurements(interest, ma);
    measurementInfo->req.setParameter(RequirementType::DELAY, REQUIREMENT_MAXDELAY);
    measurementInfo->req.setParameter(RequirementType::LOSS, REQUIREMENT_MAXLOSS);
    measurementInfo->req.setParameter(RequirementType::BANDWIDTH, REQUIREMENT_MINBANDWIDTH);
  }
  // Check if incoming packet is a retransmission
  if (pitEntry->hasUnexpiredOutRecords()) 
  {
    if (interest.isPush()) 
    {
      // Refresh Interest from consumer. Forward normally.
      forwardInterest(inFace, interest, fibEntry, pitEntry, measurementInfo);
      return;
    }
    if (interest.getName().toUri().find(PROBE_SUFFIX) != std::string::npos) 
    {
      // Looping probe Interest from other router. Don't forward.
      return;
    }
    else 
    {
      // Retransmitted Interest from non-push consumer application. Don't forward.
      return;
    }
  } 
  // New Interest. Forward normally.
  forwardInterest(inFace, interest, fibEntry, pitEntry, measurementInfo);
}


void LowestCostStrategy::forwardInterest( const Face& inFace, 
                                          const Interest& interest,
                                          shared_ptr<fib::Entry> fibEntry, 
                                          shared_ptr<pit::Entry> pitEntry,
                                          shared_ptr<MeasurementInfo> measurementInfo)
{
  // Check if currentBestOutFace is still uninitialised
  if (currentBestOutFace == NULL) 
  {
    currentBestOutFace = getFaceViaBestRoute(fibEntry->getNextHops(), pitEntry);
  }

  // Create a pointer to the outface that this Interest will be forwarded to
  shared_ptr<Face> outFace = currentBestOutFace;

  // Check if packet is a probe (push Interests should not be redirected)
  if (interest.getName().toUri().find(PROBE_SUFFIX) != std::string::npos)
  {
    // Determine best outFace (could be another one than currentBestOutFace)
    currentBestOutFace = lookForBetterOutFace(fibEntry->getNextHops(), pitEntry, measurementInfo->req, 
      currentBestOutFace);
    outFace = currentBestOutFace;

    // Check if packet is untainted (tainted packets must not be redirected or measured)
    if (!interest.isTainted())
    {
      // Check if there is more than one outFace (no need to probe if no alternatives available)
      if (fibEntry->getNextHops().size() >= MIN_NUM_OF_FACES_FOR_PROBING)
      {
        // Check if this router is allowed to use some probes for monitoring alternative routes 
        if (helper.probingDue() && PROBING_ENABLED) //TODO: write own solution for probingDue()?
        {
          // Mark Interest as tainted, so other routers don't use it or its data packtes for measurements
          // NOTE: const_cast is a hack and should generally be avoided!
          Interest& nonConstInterest = const_cast<Interest&> (interest);
          nonConstInterest.setTainted(true);

          NFD_LOG_DEBUG("Tainted this interest: " << interest.getName());

          // Remember that this probe was tainted by this router, so the corresponding data can be recognized
          myTaintedProbes.insert(interest.getName().toUri());

          // Prepare an alternative path for the probe 
          outFace = getAlternativeOutFace(currentBestOutFace, fibEntry->getNextHops());   

          NFD_LOG_DEBUG("PitEntry in-records for interest before sending NACK: " << interest.getName());
          for (auto it = pitEntry->getInRecords().begin(), end = pitEntry->getInRecords().end(); it != end; ++it)
          {
            NFD_LOG_DEBUG(it->getInterest().getName());
          }

          // Send a NACK back to the previous routers so they don't keep measurement data of the tainted Interest 
          lp::NackHeader nackHeader;
          nackHeader.setReason(lp::NackReason::NO_ROUTE);
          // this->sendNack(pitEntry, inFace, nackHeader);

          // Manually re-insert an in-record for the pit entry, so the Interest can still be sent.
          shared_ptr<Face> face = const_pointer_cast<Face>(inFace.shared_from_this());
          pitEntry->insertOrUpdateInRecord(face, interest);

          NFD_LOG_DEBUG("PitEntry in-records for interest after sending NACK: " << interest.getName());
          for (auto it = pitEntry->getInRecords().begin(), end = pitEntry->getInRecords().end(); it != end; ++it)
          {
            NFD_LOG_DEBUG(it->getInterest().getName());
          }

          // NOTE: Don't enter outgoing Nack pipeline because it needs an in-record.
/*          lp::Nack nack(interest);
          nack.setReason(lp::NackReason::TAINTED);
          const_cast<Face&>(inFace).sendNack(nack);*/

          NFD_LOG_DEBUG("Send NACK for interest: " << interest.getName() << " on face " << inFace.getId());
          std::cout << "Send NACK for interest: " << interest.getName() << " on face " << inFace.getId() << std::endl;
        }
      }
      // Save the probe's sending time in a map for later calculations of rtt. This is a workaround  
      // since "outRecord->getLastRenewed()" somehow doesn't provide the right value. 
      rttTimeTable[interest.getName().toUri()] = time::steady_clock::now(); 

      // Inform the original estimators (by Klaus Schneider) about the probe
      faceInfoTable[outFace->getId()].addSentInterest(interest.getName().toUri()); 
    }
  } 

  // Check if chosen face is the face the interest came from
  if (outFace->getId() == inFace.getId())
  {
    NFD_LOG_INFO("outFace " << outFace->getId() << " == inFace " << inFace.getId() << " " << interest.getName());
    outFace = getAlternativeOutFace(outFace, fibEntry->getNextHops());
  }

  // After everthing else is handled, forward the Interest.
  this->sendInterest(pitEntry, outFace);

  if (interest.isTainted())
  {
    NFD_LOG_INFO("Sent tainted interest " << interest.getName() << " to " << outFace->getId());
  }


/*  if (outFace->getId() != currentBestOutFace->getId())
  {
    // Printing current measurement status to console. 
    InterfaceEstimation& faceInfo1 = faceInfoTable[currentBestOutFace->getId()]; 
    NFD_LOG_INFO("Face (W): "    << currentBestOutFace->getId() 
                  << " - delay: "  << faceInfo1.getCurrentValue(RequirementType::DELAY)  
                  << "ms, loss: " << faceInfo1.getCurrentValue(RequirementType::LOSS) * 100  
                  << "%, bw: "    << faceInfo1.getCurrentValue(RequirementType::BANDWIDTH)); 

    InterfaceEstimation& faceInfo2 = faceInfoTable[outFace->getId()]; 
    NFD_LOG_INFO("Face (P): "    << outFace->getId() 
                  << " - delay: "  << faceInfo2.getCurrentValue(RequirementType::DELAY)  
                  << "ms, loss: " << faceInfo2.getCurrentValue(RequirementType::LOSS) * 100  
                  << "%, bw: "    << faceInfo2.getCurrentValue(RequirementType::BANDWIDTH)); 
    // std::cout << std::endl;
  }*/
  return; 
}


shared_ptr<Face> LowestCostStrategy::lookForBetterOutFace( const fib::NextHopList& nexthops,
                                                    shared_ptr<pit::Entry> pitEntry, 
                                                    StrategyRequirements &requirements, 
                                                    shared_ptr<Face> currentWorkingFace)
{
  // NFD_LOG_DEBUG("nexthops size: " << nexthops.size());

  // Check if there is only one available face anyway.
  if (nexthops.size() <= 2)
  {
    NFD_LOG_INFO("nexthops.size() <= 2, return getFaceViaBestRoute() " << pitEntry->getInterest().getName());
    return getFaceViaBestRoute(nexthops, pitEntry);
  }

  // Check if no currently working face was found
  if (currentWorkingFace == NULL) 
  {
    NFD_LOG_INFO("currentWorkingFace == NULL, return getFaceViaBestRoute() " << pitEntry->getInterest().getName());
    return getFaceViaBestRoute(nexthops, pitEntry);
  }

  double delayLimit = requirements.getLimit(RequirementType::DELAY); 
  double lossLimit = requirements.getLimit(RequirementType::LOSS);
  double bandwidthLimit = requirements.getLimit(RequirementType::BANDWIDTH);
  double currentDelay = faceInfoTable[currentWorkingFace->getId()].getCurrentValue(RequirementType::DELAY); 
  double currentLoss = faceInfoTable[currentWorkingFace->getId()].getCurrentValue(RequirementType::LOSS); 
  double currentBandwidth = faceInfoTable[currentWorkingFace->getId()].getCurrentValue(RequirementType::BANDWIDTH);

  // Check if current working path measurements are still uninitialised
  if (currentDelay == 10 && currentLoss == 0 && currentBandwidth == 0)
  { 
    NFD_LOG_INFO("currentDelay == 10 && currentLoss == 0 && currentBandwidth == 0, return currentWorkingFace " << pitEntry->getInterest().getName());
    return currentWorkingFace;
  }

  // Check if current working path underperforms
  if (currentDelay > delayLimit || currentLoss > lossLimit || currentBandwidth < bandwidthLimit)
  {
    NFD_LOG_INFO("Current face underperforms: Face " << currentWorkingFace->getId() << ", " << currentDelay << ", " << currentLoss * 100 << "%, " << currentBandwidth);
    // Find potential alternative and get its performance
    shared_ptr<Face> alternativeOutFace = getAlternativeOutFace(currentWorkingFace, nexthops);
    double alternativeDelay = faceInfoTable[alternativeOutFace->getId()].getCurrentValue(RequirementType::DELAY); 
    double alternativeLoss = faceInfoTable[alternativeOutFace->getId()].getCurrentValue(RequirementType::LOSS); 
    double alternativeBandwidth = faceInfoTable[alternativeOutFace->getId()].getCurrentValue(RequirementType::BANDWIDTH);
    
    // Check if alternative performs well enough
    if (alternativeDelay <= delayLimit && alternativeLoss <= lossLimit && alternativeBandwidth >= bandwidthLimit)
    {
      NFD_LOG_INFO("Well performing alternative face found: " << alternativeOutFace->getId());
      if (pitEntry->canForwardTo(*alternativeOutFace)) { return alternativeOutFace; }
    }
    else 
    {
      NFD_LOG_INFO("Taking next best alternative out of desperation: " << getAlternativeOutFace(alternativeOutFace, nexthops)->getId() << " " << pitEntry->getInterest().getName());
      /* 
      * If alternative also underperforms, take the next best alternative and hope for the best 
      * (since there is no performance data available yet)
      */
       if (pitEntry->canForwardTo(*alternativeOutFace)) { return getAlternativeOutFace(alternativeOutFace, nexthops); }      
    }
  } 
  return currentWorkingFace;
}

static inline bool
predicate_PitEntry_canForwardTo_NextHop(shared_ptr<pit::Entry> pitEntry, const fib::NextHop& nexthop)
{
  return pitEntry->canForwardTo(*nexthop.getFace());
}

shared_ptr<Face> LowestCostStrategy::getFaceViaBestRoute( const fib::NextHopList& nexthops, 
                                                          shared_ptr<pit::Entry> pitEntry)
{
  fib::NextHopList::const_iterator it = std::find_if(nexthops.begin(), nexthops.end(),
    bind(&predicate_PitEntry_canForwardTo_NextHop, pitEntry, _1));

  if (it == nexthops.end()) {
    // this->rejectPendingInterest(pitEntry);
    return nullptr;
  }

  return it->getFace();
}


shared_ptr<Face> LowestCostStrategy::getLowestTypeFace( const fib::NextHopList& nexthops,
                                                        shared_ptr<pit::Entry> pitEntry, 
                                                        RequirementType type, 
                                                        StrategyRequirements &requirements,
                                                        FaceId currentWorkingFaceId, 
                                                        bool isUpwardAttribute)
{
  shared_ptr < Face > outFace = NULL;

  // Returning lowest cost face.
  if (type == RequirementType::COST) {
    return nexthops.front().getFace();
  }

  for (auto n : nexthops) {
    bool isWorkingFace = (n.getFace()->getId() == currentWorkingFaceId);
    double currentLimit;
    currentLimit = requirements.getLimit(type);
    if (!isWorkingFace) {
      if (StrategyRequirements::isUpwardAttribute(type)) {
        currentLimit *= (1.0 + HYSTERESIS_PERCENTAGE);
      }
      else {
        currentLimit /= (1.0 + HYSTERESIS_PERCENTAGE);
      }
    }
    double currentValue = faceInfoTable[n.getFace()->getId()].getCurrentValue(type);
    if (pitEntry->canForwardTo(*n.getFace())) {
      if (!isUpwardAttribute && currentValue < currentLimit) {
        outFace = n.getFace();
        break;
      }
      if (isUpwardAttribute && currentValue > currentLimit) {
        outFace = n.getFace();
        break;
      }
    }
  }

  // If no face meets the requirement: Send out on best face.
  if (outFace == NULL) {
    double lowestValue = std::numeric_limits<double>::infinity();
    double highestValue = -1;
    for (auto n : nexthops) {
      double currentValue = faceInfoTable[n.getFace()->getId()].getCurrentValue(type);
      if (!isUpwardAttribute && pitEntry->canForwardTo(*n.getFace())
          && currentValue < lowestValue) {
        lowestValue = currentValue;
        outFace = n.getFace();
      }
      if (isUpwardAttribute && pitEntry->canForwardTo(*n.getFace())
          && currentValue > highestValue) {
        NFD_LOG_TRACE(
            "Highest value: " << currentValue << ", " << highestValue << ", face: "
                << n.getFace()->getId());
        highestValue = currentValue;
        outFace = n.getFace();
      }

    }
  }
  return outFace;
}


shared_ptr<Face> LowestCostStrategy::getAlternativeOutFace( const shared_ptr<Face> outFace, 
                                                            const fib::NextHopList& nexthops)
{
  if (nexthops.size() > 1)
  {
    bool faceFound = false;
    int iterations = 0;
    while (iterations < 2) {
      for (auto n : nexthops) 
      { 
        if (faceFound) {return n.getFace();} 
        if (n.getFace() == outFace) {faceFound = true;}
      }
      iterations++;
    }
  }
  return outFace;
}

void LowestCostStrategy::beforeSatisfyInterest( shared_ptr<pit::Entry> pitEntry, 
                                                const Face& inFace,
                                                const Data& data)
{
  // Check if incoming data is probe data
  if (data.getName().toUri().find(PROBE_SUFFIX) != std::string::npos)
  {
    // Check if it's an answer to one of the probes tainted by this router
    auto myTaintedProbesIterator = myTaintedProbes.find(data.getName().toUri());
    bool taintedByThisRouter = (myTaintedProbesIterator != myTaintedProbes.end()) ? true : false;

    // Check if usable for measurement (tainted by this router or not tainted at all)
    if (taintedByThisRouter || !data.isTainted())
    {
      if (taintedByThisRouter)
      {
        // Forget about the corresponding tainted probe (since it is satisfied now)
        myTaintedProbes.erase(myTaintedProbesIterator);

        // TODO: Find a way to stop the data packet from being forwarded any further.
      }
      // Inform loss estimator
      InterfaceEstimation& faceInfo = faceInfoTable[inFace.getId()];
      faceInfo.addSatisfiedInterest(data.getContent().value_size(), data.getName().toUri());
      pit::OutRecordCollection::const_iterator outRecord = pitEntry->getOutRecord(inFace);

      // Check if not already satisfied by another upstream
      if (!pitEntry->getInRecords().empty() && outRecord != pitEntry->getOutRecords().end()) 
      {
        // There is an in and outrecord --> inform RTT estimator
        time::steady_clock::Duration rtt = time::steady_clock::now() - rttTimeTable[data.getName().toUri()];
        faceInfo.addRttMeasurement(time::duration_cast < time::microseconds > (rtt));
        rttTimeTable.erase(data.getName().toUri()); 

        // Delete every entry in rttTimeTable that is older than a certain threshold.
        for ( auto it = rttTimeTable.begin(); it != rttTimeTable.end();) 
        {
          (time::steady_clock::now()-it->second > RTT_TIME_TABLE_MAX_DURATION) ? it=rttTimeTable.erase(it) : it++ ;
        } 
      }   
    }    
    else 
    {
      // Cancel measurements for tainted data packet (so that measurements are not skewed by 'missing packtets') 
      /*
      * Delay: Just dont calculate rtt, entries will drop out of the custom list by themeselves
      * Loss: Omit "addSatisfiedInterest" and remove the corresponding entry from the estimator
      * Bandwith: Omit "addSatisfiedInterest"
      */ 
      faceInfoTable[inFace.getId()].removeSentInterest(data.getName().toUri());
    }


  } 
}

/*void LowestCostStrategy::afterReceiveNack(const Face& inFace, 
                                          const lp::Nack& nack, 
                                          const shared_ptr<pit::Entry>& pitEntry)*/ 
void
LowestCostStrategy::afterReceiveNack(const Face& inFace, 
                                     const lp::Nack& nack,
                                     shared_ptr<fib::Entry> fibEntry,
                                     shared_ptr<pit::Entry> pitEntry)
{
  std::cout << "received NACK for " << pitEntry->getInterest().getName() << std::endl;
  NFD_LOG_DEBUG("received NACK for " << pitEntry->getInterest().getName());

  if (nack.getReason() == lp::NackReason::TAINTED)
  {
    NFD_LOG_DEBUG("received NACK for " << pitEntry->getInterest().getName() << " with NackReason::TAINTED");

      // Cancel measurements for tainted data packet (so that measurements are not skewed by 'missing packtets') 
      /*
      * Delay: Just dont calculate rtt, entries will drop out of the custom list by themeselves
      * Loss: Omit "addSatisfiedInterest" and remove the corresponding entry from the estimator
      * Bandwith: Omit "addSatisfiedInterest"
      */ 
      faceInfoTable[inFace.getId()].removeSentInterest(pitEntry->getInterest().getName().toUri());
  }

}

}  // namespace fw
}  // namespace nfd
