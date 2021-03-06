/**
 * Copyright (c) 2015 Daniel Posch (Alpen-Adria Universität Klagenfurt)
 *
 * This file is part of the ndnSIM extension for Stochastic Adaptive Forwarding (SAF).
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
#include "parameterconfiguration.hpp"
//#include <iostream>

ParameterConfiguration* ParameterConfiguration::instance = NULL;

ParameterConfiguration::ParameterConfiguration()
{
  setParameter("TAINTING_ENABLED", P_TAINTING_ENABLED);
  setParameter("MIN_NUM_OF_FACES_FOR_TAINTING", P_MIN_NUM_OF_FACES_FOR_TAINTING);
  setParameter("MAX_TAINTED_PROBES_PERCENTAGE", P_MAX_TAINTED_PROBES_PERCENTAGE);
  setParameter("REQUIREMENT_MAXDELAY", P_REQUIREMENT_MAXDELAY);
  setParameter("REQUIREMENT_MAXLOSS", P_REQUIREMENT_MAXLOSS);
  setParameter("REQUIREMENT_MINBANDWIDTH", P_REQUIREMENT_MINBANDWIDTH);
  setParameter("RTT_TIME_TABLE_MAX_DURATION", P_RTT_TIME_TABLE_MAX_DURATION);
}


void ParameterConfiguration::setParameter(std::string param_name, double value, std::string prefix)
{
  //std::cout << "Set parameter " << param_name << ": " << value << std::endl;
  prefixMap[prefix][param_name] = value;
}

double ParameterConfiguration::getParameter(std::string param_name, std::string prefix)
{

  // Check if the given prefix is known, otherwise return values of the default prefix "/"
  if (prefixMap.find(prefix) == prefixMap.end()) {
    return prefixMap["/"][param_name];
  }
  //std::cout << "Return Parameter " << param_name << ": " << prefixMap[prefix][param_name] << std::endl;

  return prefixMap[prefix][param_name];
}

ParameterConfiguration *ParameterConfiguration::getInstance()
{
  if(instance == NULL)
    instance = new ParameterConfiguration();

  return instance;
}