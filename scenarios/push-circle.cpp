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
#include "ns3/ndnSIM-module.h"

namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
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

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("scenarios/topologies/push-circle-topo.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast");
  //ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Get Producer Node
  Ptr<Node> producer = Names::Find<Node>("Prod0");
  // Get Consumer Nodes
  NodeContainer consumerNodes;
  consumerNodes.Add(Names::Find<Node>("Cons1"));

  Ptr<Node> consumer2 = Names::Find<Node>("Cons2");
  consumerNodes.Add(Names::Find<Node>("Cons2"));

  // Installing applications


  // Consumer
  /*ndn::AppHelper consumerHelper("ns3::ndn::PushConsumer");
  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("0.25")); // 1 interests every 4 seconds
  consumerHelper.SetAttribute("LifeTime", StringValue("5s"));

  consumerHelper.Install(consumerNodes);

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::PushProducer");
  // Producer will reply to all requests starting with /prefix
  consumerHelper.SetAttribute("Frequency", StringValue("50")); // One packet every 0.02 Seconds
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("218")); // 64kbps * 0.02sec + 58byte Packet-Overhead
  producerHelper.Install(producer); // last node */


  std::string prefix = "/voip/1";
  ndn::AppHelper consumerHelper("ns3::ndn::PushConsumer"); // Caller helper for push
  consumerHelper.SetAttribute("LifeTime", StringValue("5s"));
  consumerHelper.SetAttribute("PIRefreshInterval", StringValue("1.5s"));
  consumerHelper.SetPrefix(prefix);
  
  consumerHelper.Install(consumerNodes);

  consumerHelper.SetPrefix(prefix);
  ApplicationContainer container = consumerHelper.Install(consumer2);
  container.Start(MilliSeconds(1000));

  ndn::AppHelper producerHelper("ns3::ndn::PushProducer");
  producerHelper.SetAttribute("PayloadSize", StringValue("82"));
  producerHelper.SetAttribute("Frequency", StringValue("100"));
  producerHelper.SetPrefix(prefix);
  
  ndnGlobalRoutingHelper.AddOrigins(prefix, producer);
  producerHelper.Install(producer);

  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins("/prefix", producer);

  // Calculate and install FIBs
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes (); 

  Simulator::Stop(Seconds(15.0));

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

