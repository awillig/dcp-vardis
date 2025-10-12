/**
 * Copyright (C) 2025 Andreas Willig, University of Canterbury
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */


#include <dcp/vardis/vardis_configuration.h>
#include <dcp/vardis/vardis_constants.h>
#include <dcp/vardis/vardis_transmissible_types.h>


namespace dcp::vardis {

  // ------------------------------------------------------------------------------

  
  void VardisConfigurationBlock::add_options (po::options_description& cfgdesc)
  {
    cfgdesc.add_options()
      
      // Vardis parameters
      (opt("maxValueLength").c_str(),         po::value<size_t>(&maxValueLength)->default_value(defaultValueMaxValueLength), txt("maximum length of a variable value (bytes)").c_str())
      (opt("maxDescriptionLength").c_str(),   po::value<size_t>(&maxDescriptionLength)->default_value(defaultValueMaxDescriptionLength), txt("maximum length of a variable description (bytes)").c_str())
      (opt("maxRepetitions").c_str(),         po::value<uint8_t>(&maxRepetitions)->default_value(defaultValueMaxRepetitions), txt("maximum number of repetitions (repCnt)").c_str())
      (opt("maxPayloadSize").c_str(),         po::value<uint16_t>(&maxPayloadSize)->default_value(defaultValueMaxPayloadSize), txt("maximum length of a Vardis payload (bytes)").c_str())
      (opt("maxSummaries").c_str(),           po::value<uint16_t>(&maxSummaries)->default_value(defaultValueMaxSummaries), txt("maximum number of summaries in a Vardis payload").c_str())
      (opt("scrubbingPeriodMS").c_str(),      po::value<uint16_t>(&scrubbingPeriodMS)->default_value(defaultValueScrubbingPeriodMS),  txt("scrubbing period for soft-state mechanism (in ms)").c_str())
      (opt("payloadGenerationIntervalMS").c_str(),  po::value<uint16_t>(&payloadGenerationIntervalMS)->default_value(defaultValuePayloadGenerationIntervalMS),  txt("interval for checking payload generation (in ms)").c_str())
      (opt("pollRTDBServiceIntervalMS").c_str(),    po::value<uint16_t>(&pollRTDBServiceIntervalMS)->default_value(defaultValuePollRTDBServiceIntervalMS),  txt("interval for checking RTDB service requests in shared memory (in ms)").c_str())
      (opt("queueMaxEntries").c_str(),              po::value<uint16_t>(&queueMaxEntries)->default_value(defaultValueQueueMaxEntries), txt("maximum entries in BP queue for Vardis").c_str())

      (opt("lockingIndividualContainers").c_str(),              po::value<bool>(&lockingForIndividualContainers)->default_value(defaultValueLockingForIndividualContainers), txt("Locking protocol data for processing individual containers (instead of one lock per received payload)").c_str())

      ;
  }

  
  // ------------------------------------------------------------------------------

  
  void VardisConfigurationBlock::validate ()
  {
    if (maxValueLength <= 0) throw ConfigurationException ("VardisConfigurationBlock", "maxValueLength <= 0");
    if (maxValueLength > MAX_maxValueLength) throw ConfigurationException ("VardisConfigurationBlock", "maxValueLength too large");
    if (maxValueLength > (maxPayloadSize - InstructionContainerT::fixed_size()))  throw ConfigurationException ("VardisConfigurationBlock", "maxValueLength too large");

    if (maxDescriptionLength <= 0) throw ConfigurationException ("VardisConfigurationBlock", "maxDescriptionLength <= 0");
    if (maxDescriptionLength > MAX_maxDescriptionLength) throw ConfigurationException ("VardisConfigurationBlock", "maxDescriptionLength too large");
    if (maxDescriptionLength > maxPayloadSize - (InstructionContainerT::fixed_size() + VarSpecT::fixed_size() + VarUpdateT::fixed_size() + maxValueLength))
      throw ConfigurationException ("VardisConfigurationBlock", "maxDescriptionLength too large");

    if (maxRepetitions <= 0) throw ConfigurationException ("VardisConfigurationBlock", "maxRepetitions <= 0");
    if (maxRepetitions > 15) throw ConfigurationException ("VardisConfigurationBlock", "maxRepetitions > 15");

    if (maxPayloadSize <= 0) throw ConfigurationException ("VardisConfigurationBlock", "maxPayloadSize <= 0");
    /* upper bound on maxPayloadSize is checked by BP upon registration */

    if (maxSummaries > ((maxPayloadSize - InstructionContainerT::fixed_size())/(VarSummT::fixed_size())))
      throw ConfigurationException ("VardisConfigurationBlock", "maxSumaries too large");

    if (scrubbingPeriodMS <= 0) throw ConfigurationException ("VardisConfigurationBlock", "scrubbing period must be strictly positive");
    if (scrubbingPeriodMS > 65000) throw ConfigurationException ("VardisConfigurationBlock", "scrubbing period must not exceed 65000 (ms)");
    
    if (pollRTDBServiceIntervalMS <= 0) throw ConfigurationException ("VardisConfigurationBlock", "period for checking RTDB service requests in shared memory must be strictly positive");
    if (payloadGenerationIntervalMS <= 0) throw ConfigurationException ("VardisConfigurationBlock", "payload generation interval must be strictly positive");

    if (queueMaxEntries <= 0) throw ConfigurationException ("VardisConfigurationBlock", "maximum entries in BP queue for Vardis must be strictly positive");
  }

  
  // ------------------------------------------------------------------------------

  
  std::ostream& operator<< (std::ostream& os, const dcp::vardis::VardisConfiguration& cfg)
  {
    os << "VardisConfiguration { "
       << "loggingToConsole = " << cfg.logging_conf.loggingToConsole
       << " , logfileNamePrefix = " << cfg.logging_conf.logfileNamePrefix
       << " , logAutoFlush = " << cfg.logging_conf.logAutoFlush
       << " , minimumSeverityLevel = " << cfg.logging_conf.minimumSeverityLevel
       << " , rotationSize = " << cfg.logging_conf.rotationSize

       << " , commandSocketFile[BP] = " << cfg.bp_cmdsock_conf.commandSocketFile
       << " , commandSocketTimeoutMS[BP] = " << cfg.bp_cmdsock_conf.commandSocketTimeoutMS

       << " , shmAreaNameBP = " << cfg.bp_shm_conf.shmAreaName

       << " , shmAreaNameVarStore = " << cfg.vardis_shm_vardb_conf.shmAreaName
      
       << " , maxValueLength = " << cfg.vardis_conf.maxValueLength
       << " , maxDescriptionLength = " << cfg.vardis_conf.maxDescriptionLength
       << " , maxRepetitions = " << cfg.vardis_conf.maxRepetitions
       << " , maxPayloadSize = " << cfg.vardis_conf.maxPayloadSize
       << " , maxSummaries = " << cfg.vardis_conf.maxSummaries
       << " , scrubbingPeriodMS = " << cfg.vardis_conf.scrubbingPeriodMS
       << " , pollRTDBServiceIntervalMS = " << cfg.vardis_conf.pollRTDBServiceIntervalMS
       << " , payloadGenerationIntervalMS = " << cfg.vardis_conf.payloadGenerationIntervalMS
       << " , queueMaxEntries = " << cfg.vardis_conf.queueMaxEntries
       << " , lockingforindividualcontainers = " << cfg.vardis_conf.lockingForIndividualContainers
    
       << " }";
    return os;
  }
  
};  // namespace dcp::vardis
