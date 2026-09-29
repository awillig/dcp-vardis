// Copyright (C) 2024 Andreas Willig, University of Canterbury
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#pragma once

#include <stack>
#include <source_location>
#include <omnetpp.h>
#include <inet/networklayer/common/InterfaceTable.h>
#include <dcp/common/global_types_constants.h>


// --------------------------------------------------------------------------
namespace dcp {


/**
 * This is the base class for all DCP protocols. It provides some
 * simple debug/logging facilities and some common operations (e.g.
 * finding out the own MAC address / node identifier, and keeping
 * a pointer to the interface used).
 *
 * All DCP protocols should be derived from this.
 */

class DcpProtocol : public omnetpp::cSimpleModule {

public:
    virtual int numInitStages() const override;
    virtual void initialize(int stage) override;

    // return the node identifier (MAC address) of this node
    NodeIdentifierT getOwnNodeId () const { return _ownNodeId; };

private:

    // --------------------------------------------
    // data members
    // --------------------------------------------

    // Node id of this node
    NodeIdentifierT          _ownNodeId      = nullNodeIdentifier;

    // Identifying the network interface to be used, needed for packet transmission
    inet::NetworkInterface  *_wlanInterface  = nullptr;
    std::string              _interfaceName;

    // Supporting debug output: current module name
    std::string              _dbg_module_name;


protected:

    // --------------------------------------------
    // Miscellaneous helpers
    // --------------------------------------------

    void  determineOwnNodeId (std::string interfaceName);
    inet::NetworkInterface  *getWlanInterface() const { return _wlanInterface; };


  // --------------------------------------------
  // Debug helpers
  // --------------------------------------------

  // Declare one such object at the start of a method used in the simulator
  class EvTracing {
  private:
    DcpProtocol&  _prot;
    std::string   _methname;
  public:
    
    EvTracing (DcpProtocol& prot, const std::string methname = std::source_location::current().function_name()) : _prot(prot), _methname (methname)
    {
      prefix();
      the_stream() << "entering" << std::endl;
    };

    ~EvTracing ()
    {
      prefix();
      the_stream() << "leaving" << std::endl;
    };

    std::ostream& the_stream ()
    {
      return OPP_LOGPROXY (_prot.getThisPtr(), omnetpp::LOGLEVEL_INFO, nullptr).getStream();
    };

    void prefix ()
    {
      the_stream() << "t = " << omnetpp::simTime()
		   << " - " << _prot._dbg_module_name
		   << "[Id=" << _prot._ownNodeId
		   << "]." << _methname
		   << ": ";
    };

    void text (std::string str)
    {
      prefix();
      the_stream() << str << std::endl;
    };

    template <typename T1>
    void val(std::string vname, T1 v1)
    {
      prefix();
      the_stream() << vname << " = " << v1 << std::endl;
    };

    template <typename T1, typename T2>
    void val(std::string vname1, T1 v1, std::string vname2, T2 v2)
    {
      prefix();
      the_stream() << vname1 << " = " << v1
		   << " , " << vname2 << " = " << v2
		   << std::endl;
    };

    template <typename T1, typename T2, typename T3>
    void val(std::string vname1, T1 v1, std::string vname2, T2 v2, std::string vname3, T3 v3)
    {
      prefix();
      the_stream() << vname1 << " = " << v1
		   << " , " << vname2 << " = " << v2
		   << " , " << vname3 << " = " << v3
		   << std::endl;
    };

    template <typename T1, typename T2, typename T3, typename T4>
    void val(std::string vname1, T1 v1,
	     std::string vname2, T2 v2,
	     std::string vname3, T3 v3,
	     std::string vname4, T4 v4)
    {
      prefix();
      the_stream() << vname1 << " = " << v1
		   << " , " << vname2 << " = " << v2
		   << " , " << vname3 << " = " << v3
		   << " , " << vname4 << " = " << v4
		   << std::endl;
    };

    template <typename T1, typename T2, typename T3, typename T4, typename T5>
    void val(std::string vname1, T1 v1,
	     std::string vname2, T2 v2,
	     std::string vname3, T3 v3,
	     std::string vname4, T4 v4,
	     std::string vname5, T5 v5)
    {
      prefix();
      the_stream() << vname1 << " = " << v1
		   << " , " << vname2 << " = " << v2
		   << " , " << vname3 << " = " << v3
		   << " , " << vname4 << " = " << v4
		   << " , " << vname5 << " = " << v5
		   << std::endl;
    };
  };
  
  
    // --------------------------------------------
    // Debug helpers
    // --------------------------------------------

    /**
     * Sets the current module name to use, will show up in every line
     */
    void dbg_setModuleName(std::string name);
};


} // namespace

