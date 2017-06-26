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
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"
#include "../extensions/utils/parameterconfiguration.h"
#include "../extensions/strategies/lowest-cost-strategy.hpp"

using ns3::ndn::StrategyChoiceHelper;

namespace ns3 {

int
main(int argc, char* argv[])
{
  // Variables
  // UniformVariable rng = UniformVariable();
  Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable> ();
  int randomNodeNumber1 = rng->GetInteger(0, 11);
  int randomNodeNumber2 = rng->GetInteger(0, 11);
  uint32_t simTime = 10 * 60* 1000; // Simtime in milliseconds (10 minutes)

  // Parameters
  std::string queue = "DropTail_Bytes";
  std::string forwardingStrategy = "lowest-cost";
  std::string logDir = "ndnSIM_2.3/scenario/results/";
  std::string approach = "push";
  std::string piRefreshFrequency = "2s";
  std::string linkErrorParam = "0";
  std::string appSuffix = "/app";
  std::string probeSuffix = "/probe";
  int prefixOffset = 2;
  int taintingEnabled = 1;
  int minNumOfFacesForTainting = 3;
  int maxTaintedProbesPercentage = 10;
  double requirementMaxDelay = 200.0;
  double requirementMaxLoss = 0.1;
  double requirementMinBandwidth = 0.0;
  int rttTimeTableMaxDuration = 1000;

  // Read Parameters
  CommandLine cmd;
  cmd.AddValue("queueName", "Name of the queue to use", queue);
  cmd.AddValue("forwardingStrategy", "Used forwarding strategy on all nodes [default=best-route]", forwardingStrategy);
  cmd.AddValue("logDir", "Folder where logfiles are stored", logDir);
  cmd.AddValue("approach", "Approach to simulate (push|prerequest|standard). Default: push", approach);
  cmd.AddValue("piRefreshFrequency", "Number of Refresh Persistent Interests per Second", piRefreshFrequency);
  cmd.AddValue("linkErrors", "Number of link errors during simulation", linkErrorParam);
  cmd.AddValue("appSuffix", "---", appSuffix);
  cmd.AddValue("probeSuffix", "---", probeSuffix);
  cmd.AddValue("prefixOffset", "---", prefixOffset);
  cmd.AddValue("taintingEnabled", "---", taintingEnabled);
  cmd.AddValue("minNumOfFacesForTainting", "---", minNumOfFacesForTainting);
  cmd.AddValue("maxTaintedProbesPercentage", "---", maxTaintedProbesPercentage);
  cmd.AddValue("requirementMaxDelay", "---", requirementMaxDelay);
  cmd.AddValue("requirementMaxLoss", "---", requirementMaxLoss);
  cmd.AddValue("requirementMinBandwidth", "---", requirementMinBandwidth);
  cmd.AddValue("rttTimeTableMaxDuration", "---", rttTimeTableMaxDuration);
  cmd.Parse(argc, argv);

  // Set forwarding strategy parameters
  ParameterConfiguration::getInstance()->APP_SUFFIX = appSuffix;
  ParameterConfiguration::getInstance()->PROBE_SUFFIX = probeSuffix;
  ParameterConfiguration::getInstance()->setParameter("PREFIX_OFFSET", prefixOffset);
  ParameterConfiguration::getInstance()->setParameter("TAINTING_ENABLED", taintingEnabled);
  ParameterConfiguration::getInstance()->setParameter("MIN_NUM_OF_FACES_FOR_TAINTING", minNumOfFacesForTainting);
  ParameterConfiguration::getInstance()->setParameter("MAX_TAINTED_PROBES_PERCENTAGE", maxTaintedProbesPercentage);
  ParameterConfiguration::getInstance()->setParameter("REQUIREMENT_MAXDELAY", requirementMaxDelay);
  ParameterConfiguration::getInstance()->setParameter("REQUIREMENT_MAXLOSS", requirementMaxLoss);
  ParameterConfiguration::getInstance()->setParameter("REQUIREMENT_MINBANDWIDTH", requirementMinBandwidth);
  ParameterConfiguration::getInstance()->setParameter("RTT_TIME_TABLE_MAX_DURATION", rttTimeTableMaxDuration);

  // Print parameters
  std::cout << "Variables" << std::endl;
  std::cout << "randomNodeNumber1: " << randomNodeNumber1 << std::endl;
  std::cout << "randomNodeNumber2: " << randomNodeNumber2 << std::endl;
  std::cout << "Parameters" << std::endl;
  std::cout << "logDir: " << logDir << std::endl;
  std::cout << "forwardingStrategy: " << forwardingStrategy << std::endl;
  std::cout << "queue: " << queue << std::endl;
  std::cout << "approach: " << approach << std::endl;
  std::cout << "PI Refresh Frequency: " << piRefreshFrequency << std::endl;
  std::cout << "Link errors: " << linkErrorParam << std::endl;
  std::cout << "appSuffix: " << appSuffix << std::endl;
  std::cout << "probeSuffix: " << probeSuffix << std::endl;
  std::cout << "prefixOffset: " << prefixOffset << std::endl;
  std::cout << "taintingEnabled: " << taintingEnabled << std::endl;
  std::cout << "minNumOfFacesForTainting: " << minNumOfFacesForTainting << std::endl;
  std::cout << "maxTaintedProbesPercentage: " << maxTaintedProbesPercentage << std::endl;
  std::cout << "requirementMaxDelay: " << requirementMaxDelay << std::endl;
  std::cout << "requirementMaxLoss: " << requirementMaxLoss << std::endl;
  std::cout << "requirementMinBandwidth: " << requirementMinBandwidth << std::endl;
  std::cout << "rttTimeTableMaxDuration: " << rttTimeTableMaxDuration << std::endl;
  std::cout << std::endl;

  // Read topology
  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("scenarios/topologies/PIPS-topology.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.InstallAll();

  // Check which approach was chosen
  if (forwardingStrategy.compare("lowest-cost") == 0) 
  {
    // Set strategy
    ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/lowest-cost");

    // TODO write comment

    Ptr<Node> consumer1 = Names::Find<Node>("Cons1");
    Ptr<Node> producer1 = Names::Find<Node>("Prod1");
    Ptr<Node> consumer2 = Names::Find<Node>("Cons2");
    Ptr<Node> producer2 = Names::Find<Node>("Prod2");
  } 

  else if (forwardingStrategy.compare("best-route") == 0) {

  }







  Simulator::Stop(MilliSeconds(simTime));

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
