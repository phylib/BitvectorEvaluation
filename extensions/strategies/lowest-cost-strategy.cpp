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
#include "fw/algorithm.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("LowestCostStrategy");

const Name LowestCostStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/lowest-cost/%FD%01/");
NFD_REGISTER_STRATEGY(LowestCostStrategy);

LowestCostStrategy::LowestCostStrategy(Forwarder& forwarder, const Name& name)
 :  Strategy(forwarder, name), 
    ownStrategyChoice(forwarder.getStrategyChoice()),
    priorityType(RequirementType::DELAY),
    currentBestOutFaceId(0)
{
}

void LowestCostStrategy::afterReceiveInterest(const Face& inFace, 
                                              const Interest& interest,
                                              const shared_ptr<pit::Entry>& pitEntry)
{

  // NFD_LOG_INFO("Incoming interest: " << interest.getName());

  Name currentPrefix;
  shared_ptr < MeasurementInfo > measurementInfo;
  nfd::MeasurementsAccessor& ma = this->getMeasurements();
  std::tie(currentPrefix, measurementInfo) = StrategyHelper::findPrefixMeasurements(interest, ma);

  // Check if prefix is unknown
  if (measurementInfo == nullptr) 
  {
    // Create new prefix
    // nfd::MeasurementsAccessor& ma = this->getMeasurements();
    // measurementInfo = StrategyHelper::addPrefixMeasurements(interest, ma);
    // measurementInfo->req.setParameter(RequirementType::DELAY, REQUIREMENT_MAXDELAY);
    // measurementInfo->req.setParameter(RequirementType::LOSS, REQUIREMENT_MAXLOSS);
    // measurementInfo->req.setParameter(RequirementType::BANDWIDTH, REQUIREMENT_MINBANDWIDTH);
  }

  /**
  * Fetch and prepare the fibEntry (needed since switch to ndnSIM 2.3, 
  * where fibentry is no longer provided out of the box by "afterReceiveInterest")
  */ 
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  // Check if currentBestOutFaceId is still uninitialised
  if (currentBestOutFaceId == 0) 
  {
    // NFD_LOG_DEBUG("currentBestOutFace == nullptr");
    currentBestOutFaceId = getFaceViaBestRoute(nexthops, pitEntry).getId();
  }

  // Create a pointer to the outface that this Interest will be forwarded to
  FaceId selectedOutFaceId = currentBestOutFaceId;

  // Check if packet is a probe (push Interests should not be redirected)
  if (interest.getName().toUri().find(PROBE_SUFFIX) != std::string::npos)
  {
    // Determine best outFace (could be another one than currentBestOutFace)
    // currentBestOutFaceId = lookForBetterOutFace(nexthops, pitEntry, measurementInfo->req, currentBestOutFaceId);
    selectedOutFaceId = currentBestOutFaceId;

    // Check if packet is untainted (tainted packets must not be redirected or measured)
    if (!interest.isTainted())
    {
      // Check if there is more than one outFace (no need to probe if no alternatives available)
      if (nexthops.size() >= MIN_NUM_OF_FACES_FOR_PROBING)
      {
        // Check if this router is allowed to use some probes for monitoring alternative routes 
        if (helper.probingDue() && PROBING_ENABLED) //TODO: write own solution for probingDue()?
        {
          // Mark Interest as tainted, so other routers don't use it or its data packtes for measurements
          // NOTE: const_cast is a hack and should generally be avoided!
          Interest& nonConstInterest = const_cast<Interest&>(interest);
          nonConstInterest.setTainted(true);
          // TODO: check if the conversion back to const is working/necessiary
          // Interest& interest = const_cast<const Interest&>(nonConstInterest); 

          // NFD_LOG_DEBUG("Tainted this interest: " << interest.getName());

          // Remember that this probe was tainted by this router, so the corresponding data can be recognized
          myTaintedProbes.insert(interest.getName().toUri());

          // Prepare an alternative path for the probe 
          selectedOutFaceId = getAlternativeOutFaceId(currentBestOutFaceId, nexthops);   

          // Send a NACK back to the previous routers so they don't keep measurement data of the tainted Interest 
          lp::NackHeader nackHeader;
          nackHeader.setReason(lp::NackReason::TAINTED);
          this->sendNack(pitEntry, inFace, nackHeader);

          // NFD_LOG_DEBUG("Send NACK for interest: " << interest.getName() << " on face " << inFace.getId() << " with reason " << nackHeader.getReason());

          // Manually re-insert an in-record for the pit entry, so the Interest can still be sent.
          // NOTE: const_cast is a hack and should generally be avoided!
          Face& nonConstInFace = const_cast<Face&> (inFace);
          pitEntry->insertOrUpdateInRecord(nonConstInFace, interest);

        }
      }
      // Save the probe's sending time in a map for later calculations of rtt. This is a workaround  
      // since "outRecord->getLastRenewed()" somehow doesn't provide the right value. 
      rttTimeTable[interest.getName().toUri()] = time::steady_clock::now(); 

      // Inform the original estimators (by Klaus Schneider) about the probe
      faceInfoTable[selectedOutFaceId].addSentInterest(interest.getName().toUri()); 
    }
  } 

  // Check if chosen face is the face the interest came from
  if (selectedOutFaceId == inFace.getId())
  {
    NFD_LOG_INFO("selectedOutFaceId " << selectedOutFaceId << " == inFace " << inFace.getId() << " " << interest.getName());
    selectedOutFaceId = getAlternativeOutFaceId(selectedOutFaceId, nexthops);
  }

  // After everthing else is handled, forward the Interest.
  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
    Face& outFace = it->getFace();
    if (outFace.getId() == selectedOutFaceId) 
    {
      this->sendInterest(pitEntry, outFace, interest);
      return;
    }
  }
  NFD_LOG_WARN("No apropriate face found.");
  return; 
}


Face& LowestCostStrategy::lookForBetterOutFace( const fib::NextHopList& nexthops,
                                                    const shared_ptr<pit::Entry> pitEntry, 
                                                    StrategyRequirements &requirements, 
                                                    Face& currentWorkingFace)
{
  // NFD_LOG_DEBUG("nexthops size: " << nexthops.size());

  // Check if there is only one available face anyway.
  if (nexthops.size() <= 2)
  {
    NFD_LOG_INFO("nexthops.size() <= 2, return getFaceViaBestRoute() " << pitEntry->getInterest().getName());
    return getFaceViaBestRoute(nexthops, pitEntry);
  }

  // Check if no currently working face was found
/*  if (currentWorkingFace == NULL) 
  {
    NFD_LOG_INFO("currentWorkingFace == NULL, return getFaceViaBestRoute() " << pitEntry->getInterest().getName());
    return getFaceViaBestRoute(nexthops, pitEntry);
  }*/

  double delayLimit = requirements.getLimit(RequirementType::DELAY); 
  double lossLimit = requirements.getLimit(RequirementType::LOSS);
  double bandwidthLimit = requirements.getLimit(RequirementType::BANDWIDTH);
  double currentDelay = faceInfoTable[currentWorkingFace.getId()].getCurrentValue(RequirementType::DELAY); 
  double currentLoss = faceInfoTable[currentWorkingFace.getId()].getCurrentValue(RequirementType::LOSS); 
  double currentBandwidth = faceInfoTable[currentWorkingFace.getId()].getCurrentValue(RequirementType::BANDWIDTH);

  // Check if current working path measurements are still uninitialised
  if (currentDelay == 10 && currentLoss == 0 && currentBandwidth == 0)
  { 
    NFD_LOG_INFO("currentDelay == 10 && currentLoss == 0 && currentBandwidth == 0, return currentWorkingFace " << pitEntry->getInterest().getName());
    return currentWorkingFace;
  }

  // Check if current working path underperforms
  if (currentDelay > delayLimit || currentLoss > lossLimit || currentBandwidth < bandwidthLimit)
  {
    NFD_LOG_INFO("Current face underperforms: Face " << currentWorkingFace.getId() << ", " << currentDelay << ", " << currentLoss * 100 << "%, " << currentBandwidth);
    // Find potential alternative and get its performance
    Face& alternativeOutFace = getFaceViaId(getAlternativeOutFaceId(currentWorkingFace.getId(), nexthops), nexthops);
    double alternativeDelay = faceInfoTable[alternativeOutFace.getId()].getCurrentValue(RequirementType::DELAY); 
    double alternativeLoss = faceInfoTable[alternativeOutFace.getId()].getCurrentValue(RequirementType::LOSS); 
    double alternativeBandwidth = faceInfoTable[alternativeOutFace.getId()].getCurrentValue(RequirementType::BANDWIDTH);
    
    // Check if alternative performs well enough
    if (alternativeDelay <= delayLimit && alternativeLoss <= lossLimit && alternativeBandwidth >= bandwidthLimit)
    {
      NFD_LOG_INFO("Well performing alternative face found: " << alternativeOutFace.getId());
      if (canForwardToLegacy(*pitEntry, alternativeOutFace)) { return alternativeOutFace; }
    }
    else 
    {
      NFD_LOG_INFO("Taking next best alternative out of desperation: " << getAlternativeOutFaceId(alternativeOutFace.getId(), nexthops) << " " << pitEntry->getInterest().getName());
      /* 
      * If alternative also underperforms, take the next best alternative and hope for the best 
      * (since there is no performance data available yet)
      */
       if (canForwardToLegacy(*pitEntry, alternativeOutFace)) { return getFaceViaId(getAlternativeOutFaceId(alternativeOutFace.getId(), nexthops), nexthops); }      
    }
  } 
  return currentWorkingFace;
}

static inline bool
predicate_PitEntry_canForwardTo_NextHop(shared_ptr<pit::Entry> pitEntry, const fib::NextHop& nexthop)
{
  return canForwardToLegacy(*pitEntry, nexthop.getFace());
}

Face& LowestCostStrategy::getFaceViaBestRoute( const fib::NextHopList& nexthops, 
                                                          const shared_ptr<pit::Entry> pitEntry)
{
/*  fib::NextHopList::const_iterator it = std::find_if(nexthops.begin(), nexthops.end(),
    bind(&predicate_PitEntry_canForwardTo_NextHop, pitEntry, _1));

  if (it == nexthops.end()) {
    // this->rejectPendingInterest(pitEntry);
    return nullptr;
  }

  return shared_ptr<Face>(&it->getFace());*/
  return nexthops[0].getFace();
}


FaceId LowestCostStrategy::getAlternativeOutFaceId(FaceId outFaceId, 
                                                    const fib::NextHopList& nexthops)
{
  if (nexthops.size() > 1)
  {
    bool faceFound = false;
    int iterations = 0;
    while (iterations < 2) {
      for (auto n : nexthops) 
      { 
        if (faceFound) {return n.getFace().getId();} 
        if (n.getFace().getId() == outFaceId) {faceFound = true;}
      }
      iterations++;
    }
  }
  return outFaceId;
}

Face& LowestCostStrategy::getFaceViaId( FaceId faceId, 
                                        const fib::NextHopList& nexthops)
{
  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
    if (it->getFace().getId() == faceId)
    {
      return it->getFace();
    }
  }
  NFD_LOG_WARN("Face was not found in nexthops. Returned face " << (int) nexthops[0].getFace().getId() << " instead.");
  return nexthops[0].getFace();
}

void LowestCostStrategy::beforeSatisfyInterest( shared_ptr<pit::Entry> pitEntry, 
                                                const Face& inFace,
                                                const Data& data)
{
  NFD_LOG_DEBUG("Received data: " << data.getName());

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

void 
LowestCostStrategy::afterReceiveNack( const Face& inFace, 
                                      const lp::Nack& nack, 
                                      const shared_ptr<pit::Entry>& pitEntry) 
{
  if (nack.getReason() == lp::NackReason::TAINTED)
  {
    // NFD_LOG_DEBUG("Received NACK for " << pitEntry->getInterest().getName() << " with NackReason::TAINTED");

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
