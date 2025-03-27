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


#pragma once


#include <dcp/common/configuration.h>




namespace dcp {


  
  /***************************************************************************
   * Configuration block for shared memory area
   **************************************************************************/


  /**
   * @brief Default name for a shared memory block
   */
  const std::string defaultValueShmAreaName    = "dcp-shm";


  /**
   * @brief This class holds all the configuration data for a shared
   *        memory block
   *
   * As a default, the shared memory configuration is a separate
   * section in a config file with section name 'sharedmem'
   */
  class SharedMemoryConfigurationBlock : public DcpConfigurationBlock {
  public:

    std::string shmAreaName; /*!< Name of shared memory area, must be systemwide unique at the time of creation */


    /**
     * @brief Default constructor, setting default section name for config file
     */
    SharedMemoryConfigurationBlock ()
      : DcpConfigurationBlock ("sharedmem")
    {};


    /**
     * @brief Constructors, setting section name for config file to
     *        chosen name
     *
     * @param bname: section name for config file
     */
    SharedMemoryConfigurationBlock (std::string bname)
      : DcpConfigurationBlock (bname)
    {};


    /**
     * @brief Constructors, setting section name for config file to
     *        chosen name and also setting default area name to given
     *        name
     *
     * @param bname: section name for config file
     * @param shmname: name of shared memory area file
     */
    SharedMemoryConfigurationBlock (std::string bname, std::string shmname)
      : DcpConfigurationBlock (bname),
	shmAreaName (shmname)
    {};

    
    /**
     * @brief Add description of configuration options for config file, also taking default area name
     */
    virtual void add_options (po::options_description& cfgdesc)
    {
      add_options (cfgdesc, defaultValueShmAreaName);
    };
    
    
    /**
     * @brief Add description of configuration options for config file, also taking default area name
     */
    virtual void add_options (po::options_description& cfgdesc, std::string default_area_name);


    /**
     * @brief Validates configuration values. Throws exception if invalid.
     */
    virtual void validate ();
    
  };
  
  
};  // namespace dcp
