/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/
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
#include <math.h>

//#include <boost/random/mersenne_twister.hpp>
//#include <boost/random/uniform_real_distribution.hpp>

#include "ns3/random-variable-stream.h"

namespace nfd {
namespace fw {

/** 
 * @brief Lowest Cost Strategy
 *
 * This strategy uses probing to assess the quality of the current working path and one alternative path.
 * If the quality of the working path crosses a threshold (maxldelay, maxloss and minbandwith) all future 
 * interests are sent on the alternative path. The alternative then becomes the working path and a new 
 * alternative is searched for. 
 * Alternative paths are found by using the algorithm from the bestroute_1 strategy (taking the first valid 
 * entry in nexthops). In the case that neither working path nor alternative meet the requirements, the 
 * next best face (other than those two) is selected.
 *
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

protected:

  virtual void
  afterReceiveData(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace, const Data& data);

  virtual void
  afterReceiveProbeData(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace, const Data& data);

private:

  /**
   * Finds an alternative path for probing by selecting the next entry in the FIB
   * in regards to the current working face.
   *
   * @param outFaceId The FaceId of current outFace.
   * @param nexthops The list of nexthops in which to search for the face.
   * @returns FaceId of alternative outFace
   */
  FaceId getAlternativeOutFaceId(FaceId outFaceId, const fib::NextHopList& nexthops);

  /**
   * If the current path performs well according to the requirements (maxdelay, maxloss, minbandwith)
   * this method will just return the current path. If it underperforms,this method will try to provide 
   * an alternative path that does meet the requirements. If no such path is found, it will return an
   * untested alternative since there is at least the chance it will perform well enough.
   *
   * @param nexthops The list of nexthops in which to search for the face.
   * @param pitEntry The pitEntry of the interest the face is intended for.
   * @param currentPrefix The Prefix of the interest which needs an outface for forwarding.
   * @returns FaceId of face that should be used for forwarding.
   */
  FaceId lookForBetterOutFaceId(const fib::NextHopList& nexthops, const shared_ptr<pit::Entry> pitEntry, std::string currentPrefix);

  /**
   * Tries to return a face by using the original bestRout algorithm. If no face is found this way
   * the first face in the list of nexthops is chosen.
   *
   * @param nexthops The list of nexthops in which to search for the face.
   * @param pitEntry The pitEntry of the interest the face is intended for.
   * @returns FaceId of face found by using the original BestRoute algorithm.
   */
  FaceId getFaceIdViaBestRoute(const fib::NextHopList& nexthops, const shared_ptr<pit::Entry> pitEntry);

  /**
   * Searches a list of nexthops for a face with the given id.
   *
   * @param faceId The FaceId of the face for which an alternative should be found.
   * @param nexthops The list of nexthops in which to search for the face.
   * @returns the face which corresponds to the given id.
   */
  Face& getFaceViaId(FaceId faceId , const fib::NextHopList& nexthops);

  /**
   * A simple helper function which helps to regulate tainting decisions.
   *
   * @returns true every n-th call, where n is the percentage specified in MAX_TAINTED_PROBES_PERCENTAGE.
   */
  bool taintingAllowed();

  /**
   * Makes sure all parameters are set according to the values specified in ParameterConfiguration.
   *
   * @param currentPrefix The Prefix for which the parameter values should be refreshed. 
   */
  void refreshParameters(std::string currentPrefix) ;

protected:
  /**
   * To ensure that the entropy of an unkown face is high, the vector has to be set filled with
   * balanced values in the beginning.
   *
   * @param currentPrefix 
   * @param faceId
   * @param value Current loss value. It is converted to the alphabet in this method
   */
  void
  initializeOrUpdateLastValues(std::string currentPrefix, FaceId faceId, double value) {

    // Initialize
    if (measurementMap[currentPrefix].lastValues.find(faceId) == measurementMap[currentPrefix].lastValues.end()) {
      initilizeLastValues(currentPrefix, faceId);
    }

    // Convert value to the alphabet (10 values from 0 to 1)
    value = ((int)(value * 10))/10.0;

    // Update
    measurementMap[currentPrefix].lastValues[faceId].pop_back();
    measurementMap[currentPrefix].lastValues[faceId].push_front(value);

  }

  void initilizeLastValues(std::string currentPrefix, FaceId faceId) {
    std::list<double> emptyLastValues;
    for (int i = 0; i < LAST_VALUES_VECTOR_LENGTH; i++) {
      emptyLastValues.push_back((i % 10) * 0.1);
    }
    measurementMap[currentPrefix].lastValues[faceId] = emptyLastValues;
  }

  /**
   * Calculates the entropy of the last N messages, where N represents the
   * LAST_VALUES_VECTOR_LENGTH. The Entropy is normalized to [0..1].
   *
   * @param currentPrefix 
   * @param faceId
   */
  double
  getNormalizedEntropy(std::string currentPrefix, FaceId faceId)
  {
    double maxEntropy = 3.33;

    float bins[10] = {};

    if (measurementMap[currentPrefix].lastValues.find(faceId) == measurementMap[currentPrefix].lastValues.end()) {
      initilizeLastValues(currentPrefix, faceId);
    }

    auto history = measurementMap[currentPrefix].lastValues[faceId];
    for (auto elem : measurementMap[currentPrefix].lastValues[faceId]) {
      if (elem >= 1) {
        bins[9]  += 1;  
      } else {
        bins[(int)(elem * 10)]  += 1;
      }
    }
    
    double entropy = 0;
    for (auto bin : bins) {
      if (bin > 0) {
        float percentage = bin / (float)history.size();
        entropy += percentage * log2(percentage);
      }
    }
    if (entropy < 0){
      entropy *= -1;
    }

    // normalize entropy to [0..1]
    entropy = entropy / maxEntropy;

    return entropy;
  }

  uint64_t
  getProbingOutFaceId(const std::string currentPrefix, const FaceId inFace, const FaceId currentOutFace, const std::vector<uint64_t> nexthops);



private:
  StrategyChoice& ownStrategyChoice;

  // Class variables for all the relevant parameters in ParameterConfiguration (for more readable code)
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
  int LAST_VALUES_VECTOR_LENGTH;
  
  // Simple counter used in taintingAllowed().
  int taintingCounter; 

  // A map containing measurements for each prefix this strategy is currently dealing with.
  std::unordered_map<std::string, MeasurementInfo> measurementMap;

  ::ns3::Ptr<::ns3::UniformRandomVariable> randomVariable;
};

}  // namespace fw
}  // namespace nfd

#endif