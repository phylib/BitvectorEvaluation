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

/** \brief Lowest Cost Strategy
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

  static const Name STRATEGY_NAME;
  std::string PROBE_SUFFIX;
  int PREFIX_OFFSET;
  bool TAINTING_ENABLED;
  uint MIN_NUM_OF_FACES_FOR_TAINTING;
  int MAX_TAINTED_PROBES_PERCENTAGE;
  double REQUIREMENT_MAXDELAY;
  double REQUIREMENT_MAXLOSS;
  double REQUIREMENT_MINBANDWIDTH;
  double HYSTERESIS_PERCENTAGE;
  time::nanoseconds RTT_TIME_TABLE_MAX_DURATION; 

private:

  /**
   * Returns an alternative path for probing by selecting the next entry in the FIB
   * in regards to the current working face.
   */
  FaceId getAlternativeOutFaceId(FaceId outFaceId, const fib::NextHopList& nexthops);

  /**
   * Returns the face with the lowest cost that satisfies all requirements.
   */
  FaceId lookForBetterOutFaceId(const fib::NextHopList& nexthops, const shared_ptr<pit::Entry> pitEntry, std::string currentPrefix);

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

  /**
   * A simple helper function which helps to regulate tainting decisions.
   * Returns true every n-th call, where n is the percentage specified in MAX_TAINTED_PROBES_PERCENTAGE.
   */
  bool taintingAllowed();

private:
  std::unordered_map<FaceId, InterfaceEstimation> faceInfoTable;
  StrategyChoice& ownStrategyChoice;
  int probingCounter; // Simple counter used in taintingAllowed().

  // A list containing one MeasurementInfo object for each prefix this strategy is currently dealing with.
  std::unordered_map<std::string, MeasurementInfo> measurementMap;
};

}  // namespace fw
}  // namespace nfd

#endif