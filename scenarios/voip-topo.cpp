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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/ndn-cxx/encoding/qci.hpp"

//#include "ns3/ndnSIM/utils/tracers/push-tracer.hpp"
//#include "ns3/ndnSIM/utils/tracers/ndn-l3-packet-tracer.hpp"

#include "../extensions/randnetworks/networkgenerator.h"

#include "../extensions/utils/parameterconfiguration.h"
#include "../extensions/strategies/lowest-cost-strategy.hpp"

#include "../extensions/tracers/push-tracer.hpp"
#include "../extensions/tracers/ndn-l3-packet-tracer.hpp"

#include <sstream>

namespace ns3 {

void
saveCallInfo(std::string fname, std::vector<std::string> callVector)
{
  ofstream file;
  file.open (fname.c_str(), ios::out);

  file << "Caller" << "\t" << "Callee" << "\t" << "startTime" << "\t" << "endTime" << "\n";

  for (auto iterator = callVector.begin(); iterator != callVector.end(); ++iterator) {
    file << *iterator << "\n";
  }

  file.close();
}

void setParametersForPrefix(std::string prefix) 
{
  ParameterConfiguration::getInstance()->setParameter("PREFIX_OFFSET", 2, prefix);
  ParameterConfiguration::getInstance()->setParameter("TAINTING_ENABLED", 1, prefix);
  ParameterConfiguration::getInstance()->setParameter("MIN_NUM_OF_FACES_FOR_TAINTING", 3, prefix);
  ParameterConfiguration::getInstance()->setParameter("MAX_TAINTED_PROBES_PERCENTAGE", 10, prefix);
  ParameterConfiguration::getInstance()->setParameter("REQUIREMENT_MAXDELAY", 200.0, prefix);
  ParameterConfiguration::getInstance()->setParameter("REQUIREMENT_MAXLOSS", 0.1, prefix);
  ParameterConfiguration::getInstance()->setParameter("REQUIREMENT_MINBANDWIDTH", 0.0, prefix);
  ParameterConfiguration::getInstance()->setParameter("RTT_TIME_TABLE_MAX_DURATION", 1000, prefix);
}

int
main(int argc, char* argv[])
{

  std::string confFile = "brite_configs/brite_medium_bw.conf";
  std::string queue = "DropTail_Bytes";
  //std::string forwardingStrategy = "best-route";
  std::string forwardingStrategy = "lowest-cost";
  std::string logDir = "/ndnSim/scenario-push-2_3/results/";
  std::string approach = "push";
  std::string piRefreshFrequency = "2s";
  std::string linkErrorParam = "0";

  // Read Commandline Parameters
  CommandLine cmd;
  cmd.AddValue("queueName", "Name of the queue to use", queue);
  cmd.AddValue("forwardingStrategy", "Used forwarding strategy on all nodes [default=best-route]", forwardingStrategy);
  cmd.AddValue("logDir", "Folder where logfiles are stored", logDir);
  cmd.AddValue("briteConfig", "Brite-config file", confFile);
  cmd.AddValue("approach", "Approach to simulate (push|prerequest|standard). Default: push", approach);
  cmd.AddValue("piRefreshFrequency", "Number of Refresh Persistent Interests per Second", piRefreshFrequency);
  cmd.AddValue("linkErrors", "Number of link errors during simulation", linkErrorParam);
  cmd.Parse(argc, argv);

  std::string appSuffix = "/app";
  std::string probeSuffix = "/probe";

  // Set forwarding strategy parameters
  ParameterConfiguration::getInstance()->APP_SUFFIX = appSuffix;
  ParameterConfiguration::getInstance()->PROBE_SUFFIX = probeSuffix;
  ParameterConfiguration::getInstance()->PREFIX_OFFSET = 2;
  setParametersForPrefix("/");

  if (!(approach.compare("push") == 0 || 
        approach.compare("prerequest") == 0 || 
        approach.compare("standard") == 0)) {
    std::cout << "Invalid Approach parameter: " << approach << std::endl;
    exit(-1);
  }
  int linkErrors = std::stoi(linkErrorParam);

  std::cout << "Parameters" << std::endl;
  std::cout << "logDir: " << logDir << std::endl;
  std::cout << "forwardingStrategy: " << forwardingStrategy << std::endl;
  std::cout << "queue: " << queue << std::endl;
  std::cout << "approach: " << approach << std::endl;
  std::cout << "PI Refresh Frequency: " << piRefreshFrequency << std::endl;
  std::cout << "Link errors: " << linkErrors << std::endl;
  std::cout << std::endl;

  // 1) Parse Brite-Config and generate network with BRITE
  ns3::ndn::NetworkGenerator gen(confFile, queue, 50);

  uint32_t simTime = 10 * 60* 1000; // Simtime in milliseconds (10 minutes)
  uint32_t avgCallDuration = 150000; // http://www.bundesnetzagentur.de/SharedDocs/Pressemitteilungen/DE/2011/110728DauerHandygespraeche.html

  int min_bw_as = -1;
  int max_bw_as = -1;
  int min_bw_leaf = -1;
  int max_bw_leaf = -1;
  int additional_random_connections_as = -1;
  int additional_random_connections_leaf = - 1;

  // Config for Medium connectivity from example.cc
  min_bw_as = 3000;
  max_bw_as = 5000;

  min_bw_leaf = 500;
  max_bw_leaf = 1500;

  //
  additional_random_connections_as = gen.getNumberOfAS ();
  additional_random_connections_leaf = gen.getAllASNodesFromAS (0).size () / 2;

  // Add random connections between nodes
  gen.randomlyAddConnectionsBetweenTwoAS (additional_random_connections_as,min_bw_as,max_bw_as,5,20);
  gen.randomlyAddConnectionsBetweenTwoNodesPerAS(additional_random_connections_leaf,min_bw_leaf,max_bw_leaf,5,20);

  //NodeContainer routers = gen.getAllASNodes ();
  //ns3::ndn::CsTracer::Install(routers, std::string(outputFolder + "/cs-trace.txt"), Seconds(1.0));


  // 2) Create Callees, Callers, and cross-traffic clients/server
  PointToPointHelper *p2p = new PointToPointHelper;
  p2p->SetChannelAttribute ("Delay", StringValue ("2ms"));

  p2p->SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  gen.randomlyPlaceNodes (20, "Callee",ns3::ndn::NetworkGenerator::LeafNode, p2p);

  p2p->SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  gen.randomlyPlaceNodes (20, "Client",ns3::ndn::NetworkGenerator::LeafNode, p2p);

  p2p->SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  gen.randomlyPlaceNodes (4, "DataServer",ns3::ndn::NetworkGenerator::LeafNode, p2p);
  p2p->SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  gen.randomlyPlaceNodes (4, "DataClient",ns3::ndn::NetworkGenerator::LeafNode, p2p);


  // 3) Install NDN Stack on all nodes
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Stats::Lru","MaxSize", "1"); // Disable caches
  ndnHelper.Install(gen.getAllASNodes ());// install all on network nodes...

  NodeContainer server = gen.getCustomNodes ("Callee");
  ndnHelper.Install(server);
  NodeContainer client = gen.getCustomNodes ("Client");
  ndnHelper.Install(client);

  NodeContainer dataServer = gen.getCustomNodes ("DataServer");
  ndnHelper.Install(dataServer);
  NodeContainer dataClient = gen.getCustomNodes ("DataClient");
  ndnHelper.Install(dataClient);

  // 4) Install routing helper on all nodes
  ns3::ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  Ptr<UniformRandomVariable> arrivalDistribution = CreateObject<UniformRandomVariable>();
  Ptr<UniformRandomVariable> calleeDistribution = CreateObject<UniformRandomVariable>();
  Ptr<NormalRandomVariable> durationDistribution = CreateObject<NormalRandomVariable>();


  // 5) Calculate call times
  std::vector<std::string> callLog;
  int callees [client.size()];
  int startingTimes [client.size()];
  int callLengths [client.size()];
  for (uint i = 0; i < client.size(); i++) {
    callees[i] = calleeDistribution->GetInteger(0, server.size() - 1);
    startingTimes[i] = arrivalDistribution->GetInteger(0, simTime);
    // variance = standardDeviation**2 --> Standard-Deviation is 25% of average call-time 
    // Bound: No call is longer than 2*avgCallTime
    double standardDeviation = avgCallDuration * .25;
    callLengths[i] = durationDistribution->GetInteger(avgCallDuration, standardDeviation * standardDeviation, avgCallDuration * 2);
    std::stringstream callInfo;
    callInfo 
      << server.Get(i)->GetId() << "\t" 
      << Names::Find<Node>(std::string("Client_" + boost::lexical_cast<std::string>(i)))->GetId() << "\t" 
      << startingTimes[i] << "\t" << (startingTimes[i] + callLengths[i]);
    callLog.push_back(callInfo.str());
    // std::cout << "Call from node " << caller->GetId() << " to " << callee->GetId() << " starts at " << arrival << " (length=" << callLengths[i] << ")" << std::endl;
  }
  saveCallInfo(logDir + "callInfo.csv", callLog);

  for(int i = 0; i < linkErrors; i++)
    gen.creatRandomLinkFailure(0, simTime, avgCallDuration * 0.8, avgCallDuration * 1.2);
  gen.exportLinkFailures(logDir + "link-failures.csv");
  

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/data/", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/voip/", "/localhost/nfd/strategy/" + forwardingStrategy);

  // 6) Create Caller and Callee Applications
  ndn::AppHelper pushProducerHelper("ns3::ndn::PushProducer") ; // Callee Helper for Push simulations
  pushProducerHelper.SetAttribute("PayloadSize", StringValue("82"));
  pushProducerHelper.SetAttribute("Frequency", StringValue("100"));
  pushProducerHelper.SetAttribute("QCI", UintegerValue(ndn::QCI_CLASSES::QCI_1)); // Set QCI to Conversational voice

  // Create Caller applications, uniformly distributed start times
  ndn::AppHelper pushConsumerHelper("ns3::ndn::PushConsumer"); // Caller helper for push
  pushConsumerHelper.SetAttribute("LifeTime", StringValue("5s"));
  pushConsumerHelper.SetAttribute("PIRefreshInterval", StringValue(piRefreshFrequency));
  pushConsumerHelper.SetAttribute("QCI", UintegerValue(ndn::QCI_CLASSES::QCI_1)); // Set QCI to Conversational voice
  if (forwardingStrategy.compare("lowest-cost") == 0) {
    // Activate probing for Lowest-Cost strategy
    pushConsumerHelper.SetAttribute("ProbeFrequency", StringValue("30")); // 30 probes per second
  } else {
    // Disable probing for all other Forwarding Strategies
    pushConsumerHelper.SetAttribute("ProbeFrequency", StringValue("0"));
  }

  ns3::ndn::AppHelper voipProducerHelper ("ns3::ndn::VoIPProducer"); // Callee for prerequest and standard approach
  voipProducerHelper.SetAttribute ("PayloadSize", StringValue("82"));
  voipProducerHelper.SetAttribute("QCI", UintegerValue(ndn::QCI_CLASSES::QCI_1)); // Set QCI to Conversational voice 

  ns3::ndn::AppHelper callerHelper ("ns3::ndn::VoipClientPre");
  //we assume voip packets (G.711) with 10ms speech per packet ==> 100 packets per second
  callerHelper.SetAttribute ("Frequency", StringValue ("100"));
  callerHelper.SetAttribute ("Randomize", StringValue ("none"));
  callerHelper.SetAttribute ("LifeTime", StringValue("1.00s"));
  if (approach.compare("prerequest") == 0) {
    callerHelper.SetAttribute ("LookaheadLiftime", IntegerValue(250)); //250ms
  } else if (approach.compare("standard") == 0) {
    callerHelper.SetAttribute ("LookaheadLiftime", IntegerValue(0)); //250ms
  } 

  ndn::AppHelper probeProducerHelper("ns3::ndn::ProbeDataProducer");
  probeProducerHelper.SetAttribute("PayloadSize", StringValue("1")); //bytes per probe data packet

  for(uint i=0; i<client.size (); i++)
  {
      Ptr<Node> caller = Names::Find<Node>(std::string("Client_" + boost::lexical_cast<std::string>(i)));
      auto callerNumber = i;
      Ptr<Node> callee = server.Get(callerNumber);

      std::string callerPrefix = "/voip/" + boost::lexical_cast<std::string>(boost::lexical_cast<std::string>(caller->GetId()));
      std::string calleePrefix = "/voip/" + boost::lexical_cast<std::string>(callee->GetId());

      ndnGlobalRoutingHelper.AddOrigins(callerPrefix, caller);
      ndnGlobalRoutingHelper.AddOrigins(calleePrefix, callee);

      uint32_t arrival = startingTimes[i];
      uint32_t end = arrival + callLengths[i];
      std::cout << "Call from node " << caller->GetId() << " to " << callee->GetId() << " starts at " << arrival << " (length=" << callLengths[i] << ")" << std::endl;

      if (approach.compare("push") == 0) {
        // Install producer on caller-side
        pushProducerHelper.SetPrefix(callerPrefix + appSuffix);
        ApplicationContainer consumer = pushProducerHelper.Install(caller);
        // Install consumer on caller-side
        pushConsumerHelper.SetPrefix(calleePrefix + appSuffix);
        consumer = pushConsumerHelper.Install(caller);
        consumer.Start(MilliSeconds(arrival));
        consumer.Stop(MilliSeconds(end));

        // Install producer on callee-side
        pushProducerHelper.SetPrefix(calleePrefix + appSuffix);
        consumer = pushProducerHelper.Install(callee);
        // Install consumer on callee-side
        pushConsumerHelper.SetPrefix(callerPrefix + appSuffix);
        consumer = pushConsumerHelper.Install (callee);
        consumer.Start(MilliSeconds(arrival));
        consumer.Stop(MilliSeconds(end));

        if (forwardingStrategy.compare("lowest-cost") == 0) {

          setParametersForPrefix(callerPrefix);
          setParametersForPrefix(calleePrefix);

          probeProducerHelper.SetPrefix(callerPrefix + probeSuffix);
          probeProducerHelper.Install(caller);

          probeProducerHelper.SetPrefix(calleePrefix + probeSuffix);
          probeProducerHelper.Install(callee);
        }

      } else {

        // Install producer on caller-side
        voipProducerHelper.SetPrefix(callerPrefix + appSuffix);
        voipProducerHelper.Install(caller);
        // Install consumer on caller-side
        callerHelper.SetPrefix(calleePrefix + appSuffix);
        ApplicationContainer consumer = callerHelper.Install (caller);
        consumer.Start(MilliSeconds(arrival));
        consumer.Stop(MilliSeconds(end));

        // Install producer on callee-side
        voipProducerHelper.SetPrefix(calleePrefix + appSuffix);
        voipProducerHelper.Install(callee);
        // Install consumer on callee-side
        callerHelper.SetPrefix(callerPrefix + appSuffix);
        consumer = callerHelper.Install (callee);
        consumer.Start(MilliSeconds(arrival));
        consumer.Stop(MilliSeconds(end));
      }
  }


  // 7) Configure Cross-Traffic
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  for (uint i = 0; i < dataServer.size(); i++) {
    Ptr<Node> server = dataServer.Get(i);
    std::string prefix = "/data/" + boost::lexical_cast<std::string>(server->GetId());
    producerHelper.SetPrefix(prefix);
    producerHelper.Install(server);
    ndnGlobalRoutingHelper.AddOrigins(prefix, server);
  }

  // Cross-Traffic Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute("Frequency", StringValue("100")); // 100 interests a second
  for (uint i = 0; i < dataClient.size(); i++) {

    Ptr<Node> server = dataServer.Get(i);
    Ptr<Node> cl = dataClient.Get(i);
    std::string prefix = "/data/" + boost::lexical_cast<std::string>(server->GetId());
    consumerHelper.SetPrefix(prefix);

    consumerHelper.Install(cl);
  }

  // 8) Configure Traces
  NodeContainer pushParticipants;
  pushParticipants.Add(server);
  pushParticipants.Add(client);
  if (approach.compare("standard") == 0) {
    //ndn::AppDelayTracer::Install(pushParticipants, std::string(logDir + "push-trace.txt"));
  } else {
    ns3::ndn::PushTracer::Install(pushParticipants, std::string(logDir + "push-trace.txt"));
  }
  ndn::L3RateTracer::Install(pushParticipants, std::string(logDir + "push-rate-trace.txt"), Seconds(600.0));
  ndn::AppDelayTracer::Install(dataClient, std::string(logDir + "data-trace.txt"));
  ndn::L3PacketTracer::InstallAll(std::string(logDir + "packet-trace.txt"));
  L2RateTracer::InstallAll("drop-trace.txt", Seconds(1));


  // Calculate and install FIBs
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes (); 

  std::cout << "Start" << std::endl;

  Simulator::Stop(MilliSeconds(simTime));

  Simulator::Run();
  Simulator::Destroy();

  std::cout << "Simulation completed" << std::endl;

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}