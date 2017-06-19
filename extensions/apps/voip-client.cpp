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

#include "voip-client.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "helper/ndn-fib-helper.hpp"

#include <cstdlib>
#include <ndn-cxx/name.hpp>

#include <typeinfo>

NS_LOG_COMPONENT_DEFINE("ndn.VoipClient");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(VoipClient);

TypeId
VoipClient::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::VoipClient")
      .SetGroupName("Ndn")
      .SetParent<Consumer>()
      .AddConstructor<VoipClient>()

      .AddAttribute("PIRefreshInterval", "Refresh Interval for Persitent Interests", StringValue("2s"),
                    MakeTimeAccessor(&VoipClient::m_refreshInterval), MakeTimeChecker())
      .AddAttribute("Name", "Name of VoIP Client", StringValue("/"),
                    MakeStringAccessor(&VoipClient::m_name), MakeStringChecker())
      .AddAttribute("AddCommunicationPartner", "Name of communication partner", StringValue(""),
                    MakeStringAccessor(&VoipClient::AddCommunicationPartner), MakeStringChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(82),
                    MakeUintegerAccessor(&VoipClient::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("DataFrequency", "Frequency of data packets", StringValue("100"),
                    MakeDoubleAccessor(&VoipClient::m_dataFrequency), MakeDoubleChecker<double>())
      .AddAttribute("QCI",
                    "QoS class Identifier (QCI)",
                    UintegerValue(0), MakeUintegerAccessor(&VoipClient::m_qci), MakeUintegerChecker<uint32_t>())

      .AddAttribute("Randomize",
                    "Type of send time randomization: none (default), uniform, exponential",
                    StringValue("none"),
                    MakeStringAccessor(&VoipClient::SetRandomize),
                    MakeStringChecker());

    ;

  return tid;
}

VoipClient::VoipClient()
  : m_firstTime(true)
{
  NS_LOG_FUNCTION_NOARGS();
  m_seqMax = std::numeric_limits<uint32_t>::max();
}

void
VoipClient::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_name, m_face, 0);

  VoipClient::SendPacket();
}

void
VoipClient::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  m_active = false;
  // cleanup base stuff
  App::StopApplication();
}

VoipClient::~VoipClient()
{
}

void
VoipClient::SetName(std::string name)
{
  m_name = name;
}

std::string
VoipClient::GetName(void)
{
  return m_name;
}

void
VoipClient::AddCommunicationPartner(std::string communicationPartner)
{
  if (communicationPartner.size() == 0) {
    return;
  }
  if (m_lifeTimes.find(communicationPartner) != m_lifeTimes.end()) {
    return;
  }
  m_communicationPartner.push_back(communicationPartner);
  m_lifeTimes.insert({{communicationPartner, time::milliseconds(0)}});
  NS_LOG_DEBUG("communicationPartner added, total " << m_communicationPartner.size());
}

void
VoipClient::OnData(shared_ptr<const Data> data)
{
  //NS_LOG_DEBUG("Got Data " << data->getName());
  if (!m_active)
    return;

  Consumer::OnData(data); // tracing inside

/*
  NS_LOG_FUNCTION(this << data);

  // NS_LOG_INFO ("Received content object: " << boost::cref(*data));

  // This could be a problem......
  uint32_t seq = data->getName().at(-1).toSequenceNumber();
  NS_LOG_INFO("< DATA for " << seq);

  int hopCount = 0;
  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) { // e.g., packet came from local node's cache
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
      NS_LOG_DEBUG("Hop count: " << hopCount);
    }
  }*/

  /*
  m_seqRetxCounts.erase(seq);
  m_seqFullDelay.erase(seq);
  m_seqLastDelay.erase(seq);

  m_seqTimeouts.erase(seq);
  m_retxSeqs.erase(seq);

  m_rtt->AckSeq(SequenceNumber32(seq)); */
}

void
VoipClient::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_DEBUG("Received Interest " << interest->getName() << " with priority " << interest->getQCI());

  Name name = interest->getName().getPrefix(interest->getName().size() - 1);
  std::string nameStr = name.toUri();

  time::milliseconds timeout = interest->getInterestLifetime();
  timeout = timeout + time::milliseconds(Simulator::Now().GetMilliSeconds());

  if (interest->getRequesterName().size() > 0) {
    nameStr = interest->getRequesterName();
  }

  m_communicationPartner.push_back(nameStr);
  auto lifetimePair = m_lifeTimes.find(nameStr);
  if (lifetimePair != m_lifeTimes.end()) {
    if (lifetimePair->second > time::milliseconds(0)) {
      NS_LOG_DEBUG("Updated timeout for: " << nameStr);
      lifetimePair->second = timeout;
    }
  } else {
    NS_LOG_DEBUG("New callee registered: " << nameStr);
    m_lifeTimes.insert({{nameStr, timeout}});  
    m_communicationPartner.push_back(nameStr);
    m_firstTime = true;
    VoipClient::SendInterest(nameStr);
  }

  if (!m_producing) {
    VoipClient::SendData();
    m_producing = true;
  }
}

void
VoipClient::ScheduleNextPacket(std::string callee)
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";
  //if (m_firstTime) {
  //  m_sendEvent = Simulator::Schedule(Seconds(0.0), &VoipClient::SendInterest, this, callee);
  //  m_firstTime = false;
  //}
  //else 
  // if (!m_sendEvent.IsRunning()) {
    NS_LOG_DEBUG("Schedule interest for " << callee);
    m_sendEvent = Simulator::Schedule((m_random == 0) ? m_refreshInterval
                                                      : Seconds(m_random->GetValue()),
                                                      &VoipClient::SendInterest, this, callee);
  // }
}

void
VoipClient::ScheduleNextPacket()
{
  VoipClient::ScheduleNextPacket(nullptr);
}

void
VoipClient::SendInterest(std::string callee)
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION(callee);

  auto it = std::find (m_communicationPartner.begin(), m_communicationPartner.end(), callee);
  if (it == m_communicationPartner.end()) {
    NS_LOG_DEBUG("Cancel send event because no communication partner was found: " << callee);
    return;
  }

  auto lifeTimePair = m_lifeTimes.find(callee);
  if (lifeTimePair != m_lifeTimes.end()) {
    if (lifeTimePair->second > time::milliseconds(0) && lifeTimePair->second < time::milliseconds(Simulator::Now().GetMilliSeconds())) {
      NS_LOG_DEBUG("Lifetime of callee " << callee << " exceeded");
      m_communicationPartner.erase(std::remove(m_communicationPartner.begin(), m_communicationPartner.end(), callee), m_communicationPartner.end());
      //std::remove(m_communicationPartner.begin(), m_communicationPartner.end(), callee);
      m_lifeTimes.erase(callee);
      return;
    }
  }

  //
  shared_ptr<Name> name = make_shared<Name>(callee);

  // shared_ptr<Interest> interest = make_shared<Interest> ();
  shared_ptr<Interest> interest = make_shared<Interest>();
  time::milliseconds creationTime(Now().GetMilliSeconds());
  if (m_qci != 0) {
    interest->setQCI(m_qci);
  }
  interest->setNonce(creationTime.count() + (rand() % 1000));
  interest->setName(*name);
  interest->setPush(true);
  if (m_firstTime) {
    m_firstTime = false;
  }
  interest->setRequesterName(m_name);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO("> Interest for " << interest->toUri());

  //WillSendOutInterest(seq);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  ScheduleNextPacket(callee);
}

void
VoipClient::SendPacket()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  for (auto iterator = m_communicationPartner.begin(); iterator != m_communicationPartner.end(); iterator++) {

    std::string callee = *iterator;
    //NS_LOG_DEBUG("Call SendInterest for " << callee);
    VoipClient::SendInterest(callee);

  }
}

void
VoipClient::SendData() {

  Name dataName(m_name);
  dataName.appendSequenceNumber(m_seq);

  // Only increase sequence number if it is a regular packet
  m_seq++; 
  
  // dataName.append(m_postfix);
  // dataName.appendVersion();

  auto data = make_shared<Data>();
  data->setName(dataName);
  data->setPush(true); 
  
  // todo: Freshness Period is now unlimited
  data->setFreshnessPeriod(::ndn::time::milliseconds(0));

  data->setContent(make_shared< ::ndn::Buffer>(m_virtualPayloadSize));
  if (m_qci != 0) {
    data->setQCI(m_qci);
  }

  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  //signature.setInfo(signatureInfo);
  //signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

  //data->setSignature(signature);

  //NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());

  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);
  //m_face->sendData(*data);

  VoipClient::ScheduleNextData();
}

void
VoipClient::ScheduleNextData()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";
  // if (m_firstTime) {
  //   m_sendEvent = Simulator::Schedule(Seconds(0.0), &PushConsumer::SendPacket, this);
  //   m_firstTime = false;
  // }
  // else if (!m_sendEvent.IsRunning())
  //  m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
  //                                                    : Seconds(m_random->GetValue()),
  //                                                    &PushConsumer::SendPacket, this);
  Simulator::Schedule(Seconds(1.0 / m_dataFrequency), &VoipClient::SendData, this);                                    
}

void
VoipClient::SetRandomize(const std::string& value)
{
  /*if (value == "uniform") {
    m_random = CreateObject<UniformRandomVariable>();
    m_random->SetAttribute("Min", DoubleValue(0.0));
    m_random->SetAttribute("Max", DoubleValue(2 * 1.0 / m_frequency));
  }
  else if (value == "exponential") {
    m_random = CreateObject<ExponentialRandomVariable>();
    m_random->SetAttribute("Mean", DoubleValue(1.0 / m_frequency));
    m_random->SetAttribute("Bound", DoubleValue(50 * 1.0 / m_frequency));
  }
  else
    m_random = 0;*/
  m_random = 0;

}

} // namespace ndn
} // namespace ns3
