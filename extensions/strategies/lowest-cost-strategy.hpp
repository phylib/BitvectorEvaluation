/* -*- Mode:C++; c-file-style:"gnu";
 * indent-tabs-mode:nil; -*- */
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Klaus Schneider <klaus.schneider@uni-bamberg.de>
 */
#ifndef NFD_DAEMON_FW_LOWEST_COST_STRATEGY_HPP
#define NFD_DAEMON_FW_LOWEST_COST_STRATEGY_HPP

#include "fw/strategy.hpp"
#include "fw/strategy-helper.hpp"
#include "fw/../../core/common.hpp"
#include "fw/../face/face.hpp"
#include "fw/../table/fib-entry.hpp"
#include "fw/../table/pit-entry.hpp"
#include "fw/../table/strategy-choice.hpp"
#include "fw/forwarder.hpp"
#include "fw/strategy-requirements.hpp"
#include "fw/interface-estimation.hpp"

namespace nfd {
namespace fw {

/** \brief Lowest Cost Strategy version 1
 *
 * Sends out interest packets on the face that satisfies all requirements.
 *
 * Current parameters:
 * \param maxloss double of loss percentage (between 0 and 1)
 * \param maxdelay double maximal round trip delay in milliseconds
 * \parm  minbw  minimal bandwidth in Kbps
 */
class LowestCostStrategy : public Strategy
{
public:

  LowestCostStrategy(Forwarder& forwarder, const Name& name = STRATEGY_NAME);

  virtual void
  afterReceiveInterest(const Face& inFace, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry);

  virtual void
  beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace, const Data& data);

  virtual void 
  afterReceiveNack(const Face& inFace, const lp::Nack& nack, const shared_ptr<pit::Entry>& pitEntry);


public:

  static const Name       STRATEGY_NAME;
  const std::string       PROBE_SUFFIX                  = "/probe";
  const bool              PROBING_ENABLED               = true;
  const uint              MIN_NUM_OF_FACES_FOR_PROBING  = 3;
  const int               MAX_TAINTED_PROBES_PERCENTAGE = 10; // Percentage of working path probes that may be redirected
  const double            REQUIREMENT_MAXDELAY          = 220.0; // Milliseconds
  const double            REQUIREMENT_MAXLOSS           = 0.5; // Percentage
  const double            REQUIREMENT_MINBANDWIDTH      = 0.0; // TODO: find out what unit this is in (set to 0 for now).
  const double            HYSTERESIS_PERCENTAGE         = 0.00; // TODO: find out what this does (set to 0 for now). 
  const time::nanoseconds TIME_BETWEEN_PROBES           = time::milliseconds(2000); 
  const time::nanoseconds RTT_TIME_TABLE_MAX_DURATION   = time::milliseconds(1000);
  

private:

  /**
   * Returns an alternative path for probing by selecting the next entry in the FIB
   * in regards to the current working face.
   */
  FaceId getAlternativeOutFaceId(FaceId outFaceId, const fib::NextHopList& nexthops);

  /**
   * Returns the face with the lowest cost that satisfies all requirements.
   */
  FaceId lookForBetterOutFaceId(const fib::NextHopList& nexthops, const shared_ptr<pit::Entry> pitEntry,
      StrategyRequirements &paramPtr, FaceId currentWorkingFaceId);

  /**
   * Returns a face by using the original BestRoute algorithm.
   * Returns NULL if no valid face can be found.
   */
  FaceId getFaceIdViaBestRoute(const fib::NextHopList& nexthops, const shared_ptr<pit::Entry> pitEntry);

  /**
   * Searches the given NextHopList for a face with the given id.
   * Returns the face which corresponds to the given id.
   */
  Face& getFaceViaId(FaceId faceId , const fib::NextHopList& nexthops);

private:
  scheduler::EventId probeTimer;

  StrategyHelper helper;
  std::unordered_map<FaceId, InterfaceEstimation> faceInfoTable;
  StrategyChoice& ownStrategyChoice;

  // The type to use when not all requirements can be met. Defaults to "DELAY"
  RequirementType priorityType;

  // A map where timestamps of sent Interests are saved for RTT measurement.
  std::unordered_map<std::string, time::steady_clock::TimePoint> rttTimeTable;

  // A set containing the names of all the probes that have been redirected (and therefore been tainted) by this router.
  std::set<std::string> myTaintedProbes;

  // A pointer to the currently best outface on which the PIs should be forwarded 
  FaceId currentBestOutFaceId;

  // An object which can hold all the hard limits required for this strategy.
  StrategyRequirements stratReq;
};

}  // namespace fw
}  // namespace nfd

#endif