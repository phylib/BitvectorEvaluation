// ndn-grid-topo-plugin.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

// for custom forwarding strategy
#include "../extensions/strategies/lowest-cost-strategy.hpp"
//#include "/home/julian/persistent-interests/pi-scenario/extensions/strategies/random-load-balancer-strategy.hpp"

// for LinkStatusControl::FailLinks and LinkStatusControl::UpLinks
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"

using ns3::ndn::StrategyChoiceHelper;

namespace ns3 {

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("scenarios/topologies/lowest-cost-topology.txt");
  topologyReader.Read();

  // Defining prefix
  std::string prefix1 = "/dst1";
  std::string prefix1App = prefix1 + "/app";
  std::string prefix1Probe = prefix1 + "/probe";
  std::string prefix2 = "/dst2";

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.InstallAll();

  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll(prefix2, "/localhost/nfd/strategy/best-route"); //best-route2
  // ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route/%FD%01"); //best-route1 (deprecated)
  // ndn::StrategyChoiceHelper::InstallAll(prefix1App, "/../extensions/strategies/lowest-cost-strategy"); //best-route1 (deprecated)
 
  NodeContainer nodesWithNewStrat;
  nodesWithNewStrat.Add(Names::Find<Node>("Cons1"));
  nodesWithNewStrat.Add(Names::Find<Node>("End1"));
  nodesWithNewStrat.Add(Names::Find<Node>("NodeA"));
  nodesWithNewStrat.Add(Names::Find<Node>("NodeB"));
  nodesWithNewStrat.Add(Names::Find<Node>("NodeC"));
  nodesWithNewStrat.Add(Names::Find<Node>("NodeD"));
  nodesWithNewStrat.Add(Names::Find<Node>("NodeE"));
  nodesWithNewStrat.Add(Names::Find<Node>("NodeF"));
  nodesWithNewStrat.Add(Names::Find<Node>("NodeG"));
  nodesWithNewStrat.Add(Names::Find<Node>("NodeH"));
  nodesWithNewStrat.Add(Names::Find<Node>("NodeI"));
  nodesWithNewStrat.Add(Names::Find<Node>("End2"));
  nodesWithNewStrat.Add(Names::Find<Node>("Prod1"));


  // Choosing a forwarding strategy
  std::string strategy = "lowest-cost";
  std::string params = ""; // Empty for now, since most parameters are set in the strategy's header file.

/*  ndn::StrategyChoiceHelper::Install(Names::Find<Node>("End1"), prefix1,
    "/localhost/nfd/strategy/" + strategy + "/%FD%01/" + params);*/
/*  ndn::StrategyChoiceHelper::Install(NodeContainer::GetGlobal(), prefix1,
    "/localhost/nfd/strategy/" + strategy + "/%FD%01/" + params);*/
  ndn::StrategyChoiceHelper::Install(nodesWithNewStrat, prefix1,
    "/localhost/nfd/strategy/" + strategy + "/%FD%01/" + params);

  // Set Custom Strategy
  // StrategyChoiceHelper::Install<nfd::fw::PiForwardingStrategy>(Names::Find<Node>("End1"),prefix1App);
  // StrategyChoiceHelper::Install<nfd::fw::LowestCostStrategy>(NodeContainer::GetGlobal(),prefix1App);
  // StrategyChoiceHelper::Install<nfd::fw::LowestCostStrategy>(Names::Find<Node>("End1"),prefix1App);
  // StrategyChoiceHelper::Install<nfd::fw::LowestCostStrategy>(NodeContainer::GetGlobal(),"/");
  // StrategyChoiceHelper::Install<nfd::fw::LowestCostStrategy>(Names::Find<Node>("End1"),"/dst1");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Getting containers for the consumer/producer
  Ptr<Node> consumer1 = Names::Find<Node>("Cons1");
  Ptr<Node> consumer2 = Names::Find<Node>("Cons2");
  Ptr<Node> producer1 = Names::Find<Node>("Prod1");
  Ptr<Node> producer2 = Names::Find<Node>("Prod2");
  Ptr<Node> producer1Probe = Names::Find<Node>("Prod1");

  // Consumer1
  ndn::AppHelper consumerHelper("ns3::ndn::PushConsumer");
  consumerHelper.SetAttribute("PIRefreshInterval", StringValue("4")); // 1 interests every 4 seconds
  consumerHelper.SetAttribute("ProbeFrequency", StringValue("30")); // 30 probes per second
  consumerHelper.SetAttribute("LifeTime", StringValue("5s"));
  consumerHelper.SetPrefix(prefix1App);
  consumerHelper.Install(consumer1); 

  // Consumer2
  ndn::AppHelper consumerTestHelper("ns3::ndn::ConsumerCbr");
  consumerTestHelper.SetAttribute("Frequency", StringValue("1")); 
  consumerTestHelper.SetPrefix(prefix2);
  consumerTestHelper.Install(consumer2); 

  // Producer1
  ndn::AppHelper pushProducerHelper("ns3::ndn::PushProducer");
  pushProducerHelper.SetAttribute("Frequency", StringValue("50")); // One packet every 0.02 Seconds
  pushProducerHelper.SetAttribute("PayloadSize", StringValue("1000")); // 64kbps * 0.02sec + 58byte Packet-Overhead
  pushProducerHelper.SetPrefix(prefix1App);
  pushProducerHelper.Install(producer1);

  // Producer1 (Probes)
  ndn::AppHelper ProbeProducerHelper("ns3::ndn::ProbeDataProducer");
  ProbeProducerHelper.SetAttribute("PayloadSize", StringValue("1")); //bytes per probe data packet
  ProbeProducerHelper.SetPrefix(prefix1Probe);
  ProbeProducerHelper.Install(producer1);

  // Producer2 
  ndn::AppHelper TrafficProducerHelper("ns3::ndn::Producer");
  TrafficProducerHelper.SetAttribute("PayloadSize", StringValue("1024")); //bytes per packet
  TrafficProducerHelper.SetPrefix(prefix2);
  TrafficProducerHelper.Install(producer2);

  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins(prefix1App, producer1);
  ndnGlobalRoutingHelper.AddOrigins(prefix1Probe, producer1);
  ndnGlobalRoutingHelper.AddOrigins(prefix2, producer2);


  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes();

  Simulator::Schedule(Seconds(0.0), ndn::LinkControlHelper::FailLink, 
    Names::Find<Node>("Cons2"), Names::Find<Node>("NodeA"));
  // Simulator::Schedule(Seconds(10.0), ndn::LinkControlHelper::UpLink, 
  //   Names::Find<Node>("Cons2"), Names::Find<Node>("NodeA"));

  // Simulator::Schedule(Seconds(10.0), ndn::LinkControlHelper::FailLink, 
  //   Names::Find<Node>("NodeG"), Names::Find<Node>("End2"));
  // Simulator::Schedule(Seconds(25.0), ndn::LinkControlHelper::UpLink, 
  //   Names::Find<Node>("NodeG"), Names::Find<Node>("End2"));


  // Simulator::Schedule(Seconds(20.0), ndn::LinkControlHelper::FailLink, 
  //   Names::Find<Node>("NodeA"), Names::Find<Node>("NodeD"));
  // Simulator::Schedule(Seconds(30.0), ndn::LinkControlHelper::FailLink, 
  //   Names::Find<Node>("NodeC"), Names::Find<Node>("DelayNode"));
  // Simulator::Schedule(Seconds(40.0), ndn::LinkControlHelper::FailLink, 
  //   Names::Find<Node>("NodeC"), Names::Find<Node>("Prod2"));
  // Simulator::Schedule(Seconds(50.0), ndn::LinkControlHelper::FailLink, 
  //   Names::Find<Node>("NodeE"), Names::Find<Node>("Prod1"));
  // Simulator::Schedule(Seconds(60.0), ndn::LinkControlHelper::FailLink, 
  //   Names::Find<Node>("NodeF"), Names::Find<Node>("Prod1"));

  // Simulator::Schedule(Seconds(70.0), ndn::LinkControlHelper::UpLink, 
  //   Names::Find<Node>("NodeA"), Names::Find<Node>("DelayNode"));
  // Simulator::Schedule(Seconds(80.0), ndn::LinkControlHelper::UpLink, 
  //   Names::Find<Node>("NodeB"), Names::Find<Node>("DelayNode"));
  // Simulator::Schedule(Seconds(90.0), ndn::LinkControlHelper::UpLink, 
  //   Names::Find<Node>("NodeC"), Names::Find<Node>("DelayNode"));
  // Simulator::Schedule(Seconds(100.0), ndn::LinkControlHelper::UpLink, 
  //   Names::Find<Node>("NodeC"), Names::Find<Node>("Prod2"));
  // Simulator::Schedule(Seconds(110.0), ndn::LinkControlHelper::UpLink, 
  //   Names::Find<Node>("NodeE"), Names::Find<Node>("Prod1"));
  // Simulator::Schedule(Seconds(120.0), ndn::LinkControlHelper::UpLink, 
  //   Names::Find<Node>("NodeF"), Names::Find<Node>("Prod1"));


  Simulator::Stop(Seconds(60.0));

  // Tracer
  //ndn::L3RateTracer::InstallAll("rate-trace.txt", Seconds(0.5));
  //L2RateTracer::InstallAll("drop-trace.txt", Seconds(0.5));

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
