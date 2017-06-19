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

#ifndef PUSH_CONSUMER_H
#define PUSH_CONSUMER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ndnSIM/apps/ndn-consumer.hpp"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief Ndn application for sending out Interest packets at a "constant" rate (Poisson process)
 */
class VoipClient : public Consumer {
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  VoipClient();
  virtual ~VoipClient();

  // From Consumer
  virtual void
  OnData(shared_ptr<const Data> contentObject);

  // inherited from NdnApp
  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  void
  SetName(std::string name);

  void
  AddCommunicationPartner(std::string communicationPartner);

  std::string
  GetName(void);

  /**
   * @brief Set type of frequency randomization
   * @param value Either 'none', 'uniform', or 'exponential'
   */
  void
  SetRandomize(const std::string& value);

protected:
  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN
   * protocol
   */
  virtual void
  ScheduleNextPacket();
  
  virtual void
  ScheduleNextPacket(std::string callee);
  
  void
  SendInterest(std::string callee);

  void 
  SendPacket();

  virtual void
  StartApplication();

  virtual void
  StopApplication();

  virtual void
  ScheduleNextData();

  void
  SendData();

protected:
  Time m_refreshInterval; 
  bool m_firstTime;
  bool m_requestingInterests;
  Ptr<RandomVariableStream> m_random;
  uint32_t m_qci = 0;
  std::string m_name = "";
  std::vector<std::string> m_communicationPartner;
  std::unordered_map<std::string, time::milliseconds> m_lifeTimes;

  bool m_producing;
  double m_dataFrequency;
  uint32_t m_seq = 0;
  uint32_t m_virtualPayloadSize;
  uint32_t m_signature;
  Name m_keyLocator;
};

} // namespace ndn
} // namespace ns3

#endif
