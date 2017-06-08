// ndn-grid-topo-plugin.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"
#include "../extensions/utils/parameterconfiguration.h"
#include "../extensions/strategies/lowest-cost-strategy.hpp"

using ns3::ndn::StrategyChoiceHelper;

namespace ns3 {

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);

  ParameterConfiguration::getInstance()->APP_SUFFIX = "/app";
  ParameterConfiguration::getInstance()->PROBE_SUFFIX = "/probe";
  ParameterConfiguration::getInstance()->setParameter("PREFIX_OFFSET", 1);
  ParameterConfiguration::getInstance()->setParameter("TAINTING_ENABLED", 1);
  ParameterConfiguration::getInstance()->setParameter("MIN_NUM_OF_FACES_FOR_TAINTING", 3);
  ParameterConfiguration::getInstance()->setParameter("MAX_TAINTED_PROBES_PERCENTAGE", 10);
  ParameterConfiguration::getInstance()->setParameter("REQUIREMENT_MAXDELAY", 200.0);
  ParameterConfiguration::getInstance()->setParameter("REQUIREMENT_MAXLOSS", 0.1);
  ParameterConfiguration::getInstance()->setParameter("REQUIREMENT_MINBANDWIDTH", 0.0);
  ParameterConfiguration::getInstance()->setParameter("RTT_TIME_TABLE_MAX_DURATION", 1000);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("scenarios/topologies/lowest-cost-topology.txt");
  topologyReader.Read();

  // Defining prefix
  std::string prefix1 = "/dst1";
  std::string prefix1App = prefix1 + ParameterConfiguration::getInstance()->APP_SUFFIX;
  std::string prefix1Probe = prefix1 + ParameterConfiguration::getInstance()->PROBE_SUFFIX;
  std::string prefix2 = "/dst2";
  std::string prefix2App = prefix2 + ParameterConfiguration::getInstance()->APP_SUFFIX;
  std::string prefix2Probe = prefix2 + ParameterConfiguration::getInstance()->PROBE_SUFFIX;

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.InstallAll();

  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll(prefix2, "/localhost/nfd/strategy/best-route"); //best-route2
  // ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route/%FD%01"); //best-route1 (deprecated)
  // ndn::StrategyChoiceHelper::InstallAll(prefix1App, "/../extensions/strategies/lowest-cost-strategy"); //best-route1 (deprecated)
 
  NodeContainer nodesWithNewStrat;
  nodesWithNewStrat.Add(Names::Find<Node>("Cons1"));
  nodesWithNewStrat.Add(Names::Find<Node>("Cons2"));
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
  nodesWithNewStrat.Add(Names::Find<Node>("Prod1")); // TODO find solution so this can be enabled too.
  nodesWithNewStrat.Add(Names::Find<Node>("Prod2")); // TODO find solution so this can be enabled too.


  // Choosing a forwarding strategy
  std::string strategy = "lowest-cost";

/*  ndn::StrategyChoiceHelper::Install(Names::Find<Node>("End1"), prefix1,
    "/localhost/nfd/strategy/" + strategy + "/%FD%01/" + params);*/
/*  ndn::StrategyChoiceHelper::Install(NodeContainer::GetGlobal(), prefix1,
    "/localhost/nfd/strategy/" + strategy + "/%FD%01/" + params);*/
  ndn::StrategyChoiceHelper::Install(nodesWithNewStrat, prefix1,
    "/localhost/nfd/strategy/" + strategy + "/%FD%01/");
  ndn::StrategyChoiceHelper::Install(nodesWithNewStrat, prefix2,
    "/localhost/nfd/strategy/" + strategy + "/%FD%01/");

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
  Ptr<Node> producer1 = Names::Find<Node>("Prod1");
  Ptr<Node> consumer2 = Names::Find<Node>("Cons2");
  Ptr<Node> producer2 = Names::Find<Node>("Prod2");

  // Consumer1
  ndn::AppHelper consumerHelper("ns3::ndn::PushConsumer");
  consumerHelper.SetAttribute("PIRefreshInterval", StringValue("4")); // 1 interests every 4 seconds
  consumerHelper.SetAttribute("ProbeFrequency", StringValue("30")); // 30 probes per second
  consumerHelper.SetAttribute("LifeTime", StringValue("5s"));
  consumerHelper.SetPrefix(prefix1App);
  consumerHelper.Install(consumer1); 

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

  // Consumer2
  ndn::AppHelper consumerTestHelper("ns3::ndn::PushConsumer");
  consumerTestHelper.SetAttribute("PIRefreshInterval", StringValue("4")); // 1 interests every 4 seconds
  consumerTestHelper.SetAttribute("ProbeFrequency", StringValue("30")); // 30 probes per second
  consumerTestHelper.SetAttribute("LifeTime", StringValue("5s"));
  consumerTestHelper.SetPrefix(prefix2App);
  consumerTestHelper.Install(consumer2); 

  // Producer2 
  ndn::AppHelper TrafficProducerHelper("ns3::ndn::PushProducer");
  TrafficProducerHelper.SetAttribute("Frequency", StringValue("50")); // One packet every 0.02 Seconds
  TrafficProducerHelper.SetAttribute("PayloadSize", StringValue("1000")); // 64kbps * 0.02sec + 58byte Packet-Overhead
  TrafficProducerHelper.SetPrefix(prefix2App);
  TrafficProducerHelper.Install(producer2);

  // Producer2 (Probes)
  ndn::AppHelper ProbeProducerHelper2("ns3::ndn::ProbeDataProducer");
  ProbeProducerHelper2.SetAttribute("PayloadSize", StringValue("1")); //bytes per probe data packet
  ProbeProducerHelper2.SetPrefix(prefix2Probe);
  ProbeProducerHelper2.Install(producer2);

  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins(prefix1App, producer1);
  ndnGlobalRoutingHelper.AddOrigins(prefix1Probe, producer1);
  ndnGlobalRoutingHelper.AddOrigins(prefix2App, producer2);
  ndnGlobalRoutingHelper.AddOrigins(prefix2Probe, producer2);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes();

  // Simulator::Schedule(Seconds(0.0), ndn::LinkControlHelper::FailLink, 
  //   Names::Find<Node>("Cons2"), Names::Find<Node>("NodeA"));
  // Simulator::Schedule(Seconds(10.0), ndn::LinkControlHelper::UpLink, 
    // Names::Find<Node>("Cons2"), Names::Find<Node>("NodeA"));

  Simulator::Schedule(Seconds(10.0), ndn::LinkControlHelper::FailLink, 
    Names::Find<Node>("NodeG"), Names::Find<Node>("End2"));
  Simulator::Schedule(Seconds(20.0), ndn::LinkControlHelper::FailLink, 
    Names::Find<Node>("NodeE"), Names::Find<Node>("NodeH"));
  Simulator::Schedule(Seconds(25.0), ndn::LinkControlHelper::UpLink, 
    Names::Find<Node>("NodeG"), Names::Find<Node>("End2"));
  Simulator::Schedule(Seconds(30.0), ndn::LinkControlHelper::FailLink, 
    Names::Find<Node>("NodeF"), Names::Find<Node>("NodeI"));
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
