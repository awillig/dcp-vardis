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

#include <boost/program_options.hpp>


namespace po = boost::program_options;

namespace dcp {

  // =========================================================================
  // =========================================================================
  // =========================================================================
  
  /**
   * @brief Base class for configuration blocks, prescribes methods to
   *        add options to the config file (in its own named section)
   *        and to validate configuration data.
   *
   * Derived classes will add data members for actual configuration
   * data, all of which will be combined in a block / section in the
   * configuration file.
   */
  
  class DcpConfigurationBlock {
    
  protected:

    /**
     * @brief Name of the block/section in the config file (section
     *        starts with '[blockName]')
     */
    std::string blockName;

    
  public:
    DcpConfigurationBlock () = delete;
    DcpConfigurationBlock (std::string bl) : blockName(bl) {};

    /**
     * @brief Abstract method to add the actual options to the BOOST
     *        config file parser
     *
     * @param cfgdesc: The options_description to which to add the
     *        options for this configuration block
     */
    virtual void add_options (po::options_description& cfgdesc) = 0;

    
    /**
     * @brief Abstract method to validate configuration values, should
     *        be used after reading them from config file. Throws an
     *        exception (of type ConfigurationException) if validation
     *        is not successful
     */
    virtual void validate () = 0;


    /**
     * @brief Construct the option name in implementations of
     *        add_options
     *
     * @param opt_name: name of the option, as used in the
     *        configuration file
     */
    std::string opt (std::string opt_name) const { return blockName + "." + opt_name; };


    /**
     * @brief Construct the textual description in implementations of
     *        add_options
     *
     * @param opt_text: textual description of the option
     */
    std::string txt (std::string opt_text) const { return blockName + ": " + opt_text; };
    
  };


  // =========================================================================
  // =========================================================================
  // =========================================================================
  
  /**
   * @brief Base class for actual configurations.
   *
   * A configuration is a container for one or more configuration
   * blocks (i.e. data members of classes derived from
   * DcpConfigurationBlock)
   */
  
  typedef struct DcpConfiguration {

    /**
     * @brief Abstract method being called when textual description of
     *        configuration file options is to be built
     *
     * @param cfgdesc: The options_description to be constructed
     *
     * An implementation of this method will call the add_options
     * method of every configuration block that is a member of the
     * configuration.
     *
     * This method is a helper method for construct_options_description()
     */
    virtual void build_description (po::options_description& cfgdesc) = 0;


    /**
     * @brief Public method to be used to construct description of
     *        configuration options.
     */
    virtual po::options_description construct_options_description ()
    {
      po::options_description cfgdesc ("Allowed options in configuration file");
      build_description (cfgdesc);
      return cfgdesc;
    };


    /**
     * @brief Public method to read configuration file.
     *
     * @param cfgfilename: file name of configuration file
     * @param allow_unregistered: if set to true, then an unknown
     *        option will simply be ignored. If set to false, then an
     *        unknown option raises an exception.
     *
     * Attempts to read configuration file without validating the
     * options. Throws an exception of tpye ConfigurationException in
     * case of an error.
     */ 
    void read_from_config_file (const std::string& cfgfilename, bool allow_unregistered = false);


    /**
     * @brief Abstract method to validate a configuration (after it
     *        has been read from config file). Throws an exception if
     *        configuration is invalid.
     *
     * An implementation of this method in a configuration class will
     * call the validate() methods of all contained configuration
     * blocks (derived from DcpConfigurationBlock)
     */
    virtual void validate () = 0;
    
    
  } DcpConfiguration;


};  // namespace dcp
