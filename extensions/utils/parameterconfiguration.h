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

#ifndef PARAMETERCONFIGURATION_H
#define PARAMETERCONFIGURATION_H

#include <cstddef>
#include <map>
#include <string>

//default parameters that can be overriden:
#define P_PREFIX_OFFSET                 1      // number of name components which are considered as prefix
#define P_TAINTING_ENABLED              1      // specifies if probes will be forwarded or not; 1=true, 0=false; 
#define P_MIN_NUM_OF_FACES_FOR_TAINTING 3      // the minimum number of faces a node must have to redirect probes
#define P_MAX_TAINTED_PROBES_PERCENTAGE 10     // percentage of working path probes that may be redirected
#define P_REQUIREMENT_MAXDELAY          200.0  // maximum tolerated delay in milliseconds
#define P_REQUIREMENT_MAXLOSS           0.1    // maximum tolerated loss in percentage
#define P_REQUIREMENT_MINBANDWIDTH      0.0    // minimum tolerated bandwith in Kbps
#define P_RTT_TIME_TABLE_MAX_DURATION   1000   // maximum time (in milliseconds) an entry is kept in the rttMap before being erased

/**
 * @brief The ParameterConfiguration class is used to set/get parameters to configure the lowest-cost-strategy.
 * The class uses a singleton pattern.
 */
class ParameterConfiguration
{
public:
  //string parameters:
  std::string PROBE_SUFFIX = "/probe";
  std::string APP_SUFFIX = "/app";


  /**
   * @brief returns the singleton instance.
   * @return
   */
  static ParameterConfiguration* getInstance();

  /**
   * @brief sets a parameter
   * @param param_name the name of the parameter.
   * @param value the value of the parameter.
   */
  void setParameter(std::string param_name, double value);

  /**
   * @brief gets a parameter.
   * @param para_name the name of the parameter.
   * @return
   */
  double getParameter(std::string para_name);

protected:  
  ParameterConfiguration();

  static ParameterConfiguration* instance;

  std::map<
  std::string /*param name*/,
  double /*param value*/
  > typedef ParameterMap;

  ParameterMap pmap;


};

#endif // PARAMETERCONFIGURATION_H