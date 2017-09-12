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

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

// #include "ns3/ndnSIM/utils/tracers/push-tracer.hpp"
//#include "ns3/ndnSIM/utils/tracers/ndn-l3-packet-tracer.hpp"

namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      |    0     | <------------> |    2   | <------------> |    3     |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *                                     ^
 **      +----------+     1Mbps        |
 *      |    1     | <-----------------|
 *      +----------+         10ms   
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

  Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (0.5));
  Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(4);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  //p2p.SetQueue("ns3::WeightedFairQueue", "Mode", StringValue("QUEUE_MODE_BYTES"));
  p2p.Install(nodes.Get(0), nodes.Get(2));
  p2p.Install(nodes.Get(1), nodes.Get(2));
  p2p.Install(nodes.Get(2), nodes.Get(3));



  //p2p.Install(nodes.Get(1), nodes.Get(2));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Installing applications

  // Consumer
  ndn::AppHelper pushCallerHelper("ns3::ndn::PushConsumer"); // Caller helper for push
  pushCallerHelper.SetAttribute("LifeTime", StringValue("5s"));
  pushCallerHelper.SetAttribute("PIRefreshInterval", StringValue("2s"));
  pushCallerHelper.SetAttribute("ProbeFrequency", StringValue("0"));
  pushCallerHelper.SetPrefix("/voip/00/app/");
  pushCallerHelper.Install(nodes.Get(0));                        // first node

  pushCallerHelper.SetPrefix("/voip/01/app/");
  ApplicationContainer consumer = pushCallerHelper.Install(nodes.Get(1));                        // first node
  consumer.Start(MilliSeconds(1000));

  // Producer
  ndn::AppHelper calleeHelper("ns3::ndn::PushProducer") ; // Callee Helper for Push simulations
  calleeHelper.SetAttribute("PayloadSize", StringValue("82"));
  calleeHelper.SetAttribute("Frequency", StringValue("100"));
  calleeHelper.SetPrefix("/voip/00/app");
  calleeHelper.Install(nodes.Get(3)); // last node
  calleeHelper.SetPrefix("/voip/01/app");
  calleeHelper.Install(nodes.Get(3)); // last node

  //ns3::ndn::AppDelayTracer::Install(nodes.Get(0), std::string("./AppDelayTracer_result.txt"));
  NodeContainer pushParticipants;
  pushParticipants.Add(nodes.Get(0));
  pushParticipants.Add(nodes.Get(1));
  pushParticipants.Add(nodes.Get(3));
  //ns3::ndn::PushTracer::Install(pushParticipants, std::string("./PushData_trace.txt"));
  //ns3::ndn::PushDataTracer::Install(nodes.Get(2), std::string("./PushData_trace.txt"));

  /*ndn::AppHelper consumerHelper("ns3::ndn::VoipClient");
  std::string prefix = "/voip/2";
  consumerHelper.SetPrefix(prefix);
  consumerHelper.SetAttribute("Frequency", StringValue("0.5")); // 1 interests every 2 seconds
  consumerHelper.SetAttribute("LifeTime", StringValue("5s"));
  consumerHelper.SetAttribute("Name", StringValue(prefix));
  consumerHelper.SetAttribute("AddCommunicationPartner", StringValue("/voip/1"));
  
  ndnGlobalRoutingHelper.AddOrigins(prefix, nodes.Get(1));
  consumerHelper.Install(nodes.Get(1));

  ndn::AppHelper producerHelper("ns3::ndn::VoipClient");
  prefix = "/voip/1";
  producerHelper.SetPrefix(prefix);
  producerHelper.SetAttribute("Frequency", StringValue("0.5")); // 1 interests every 2 seconds
  producerHelper.SetAttribute("LifeTime", StringValue("5s"));
  producerHelper.SetAttribute("Name", StringValue(prefix));*/

  //ndn::L3RateTracer::InstallAll("rate-trace.txt", Seconds(0.5));
  //ns3::ndn::L3PacketTracer::InstallAll("Data_trace.txt");
  
  ndnGlobalRoutingHelper.AddOrigins("/voip/00", nodes.Get(3));
  ndnGlobalRoutingHelper.AddOrigins("/voip/01", nodes.Get(3));
  ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes (); 

  Simulator::Stop(Seconds(20.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
