#include "parameterconfiguration.h"

ParameterConfiguration* ParameterConfiguration::instance = NULL;

ParameterConfiguration::ParameterConfiguration()
{
  setParameter("PREFIX_OFFSET", P_PREFIX_OFFSET);
  setParameter("TAINTING_ENABLED", P_TAINTING_ENABLED);
  setParameter("MIN_NUM_OF_FACES_FOR_TAINTING", P_MIN_NUM_OF_FACES_FOR_TAINTING);
  setParameter("MAX_TAINTED_PROBES_PERCENTAGE", P_MAX_TAINTED_PROBES_PERCENTAGE);
  setParameter("REQUIREMENT_MAXDELAY", P_REQUIREMENT_MAXDELAY);
  setParameter("REQUIREMENT_MAXLOSS", P_REQUIREMENT_MAXLOSS);
  setParameter("REQUIREMENT_MINBANDWIDTH", P_REQUIREMENT_MINBANDWIDTH);
  setParameter("RTT_TIME_TABLE_MAX_DURATION", P_RTT_TIME_TABLE_MAX_DURATION);
}


void ParameterConfiguration::setParameter(std::string para_name, double value)
{
  pmap[para_name] = value;
}

double ParameterConfiguration::getParameter(std::string para_name)
{
  return pmap[para_name];
}

ParameterConfiguration *ParameterConfiguration::getInstance()
{
  if(instance == NULL)
    instance = new ParameterConfiguration();

  return instance;
}