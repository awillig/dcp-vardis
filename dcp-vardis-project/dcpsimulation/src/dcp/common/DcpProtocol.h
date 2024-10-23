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
#include <omnetpp.h>
#include <inet/networklayer/common/InterfaceTable.h>
#include <dcp/common/DcpTypesGlobals.h>

using namespace omnetpp;
using namespace inet;

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

class DcpProtocol : public cSimpleModule {

public:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

    // return the node identifier (MAC address) of this node
    NodeIdentifierT getOwnNodeId () const { return _ownNodeId; };

private:

    // --------------------------------------------
    // data members
    // --------------------------------------------

    // Node id of this node
    NodeIdentifierT          _ownNodeId      = nullIdentifier;

    // Identifying the network interface to be used, needed for packet transmission
    inet::NetworkInterface  *_wlanInterface  = nullptr;
    std::string              _interfaceName;

    // Supporting debug output: current module name, and keeping track of nested dbg_enter/dbg_leave calls
    std::string              _dbg_module_name;
    std::stack<std::string>  _methname_stack;


protected:

    // --------------------------------------------
    // Miscellaneous helpers
    // --------------------------------------------

    void  determineOwnNodeId (std::string interfaceName);
    inet::NetworkInterface  *getWlanInterface() const { return _wlanInterface; };

    // --------------------------------------------
    // Debug helpers
    // --------------------------------------------

    /**
     * Use this before calling dbg_enter in a 'toplevel' method to check
     * consistency of the method name stack (allowing for nested method
     * name logging)
     */
    void dbg_assertToplevel();

    /**
     * Sets the current module name to use, will show up in every line
     */
    void dbg_setModuleName(std::string name);

    /**
     * Outputs the starting part of every log message: timestamp, module name,
     * own node identifier, and the current method name
     */
    void dbg_prefix();

    /**
     * Declare that we now enter the given method; allows for nesting
     */
    void dbg_enter(std::string methname);

    /**
     * Declare that we now leave the method, allows for nesting
     */
    void dbg_leave();

    /**
     * generates log message with prefix and the given string
     */
    void dbg_string(std::string str);

};

// ========================================================================================
// Some debugging / tracing macros, to be used inside a class derived
// from DcpProtocol
// ========================================================================================

#define DBG_VAR1(a)                  \
{                                    \
    dbg_prefix();                    \
    EV << #a << " = " << a           \
    << endl;                         \
}

#define DBG_PVAR1(p,a)               \
{                                    \
    dbg_prefix();                    \
    EV << p << ": "                  \
       << #a << " = " << a           \
       << endl;                      \
}

#define DBG_VAR2(a,b)                \
{                                    \
    dbg_prefix();                    \
    EV << #a << " = " << a           \
       << " , " << #b << " = " << b  \
       << endl;                      \
}

#define DBG_PVAR2(p,a,b)             \
{                                    \
    dbg_prefix();                    \
    EV << p << ": "                  \
       << #a << " = " << a           \
       << " , " << #b << " = " << b  \
       << endl;                      \
}


#define DBG_VAR3(a,b,c)              \
{                                    \
    dbg_prefix();                    \
    EV << #a << " = " << a           \
       << " , " << #b << " = " << b  \
       << " , " << #c << " = " << c  \
       << endl;                      \
}

#define DBG_PVAR3(p,a,b,c)           \
{                                    \
    dbg_prefix();                    \
    EV << p << ": "                  \
       << #a << " = " << a           \
       << " , " << #b << " = " << b  \
       << " , " << #c << " = " << c  \
       << endl;                      \
}

#define DBG_VAR4(a,b,c,d)            \
{                                    \
    dbg_prefix();                    \
    EV << #a << " = " << a           \
       << " , " << #b << " = " << b  \
       << " , " << #c << " = " << c  \
       << " , " << #d << " = " << d  \
       << endl;                      \
}

#define DBG_PVAR4(p,a,b,c,d)         \
{                                    \
    dbg_prefix();                    \
    EV << p << ": "                  \
       << #a << " = " << a           \
       << " , " << #b << " = " << b  \
       << " , " << #c << " = " << c  \
       << " , " << #d << " = " << d  \
       << endl;                      \
}

#define DBG_VAR5(a,b,c,d,e)          \
{                                    \
    dbg_prefix();                    \
    EV << #a << " = " << a           \
       << " , " << #b << " = " << b  \
       << " , " << #c << " = " << c  \
       << " , " << #d << " = " << d  \
       << " , " << #e << " = " << e  \
       << endl;                      \
}

#define DBG_PVAR5(p,a,b,c,d,e)       \
{                                    \
    dbg_prefix();                    \
    EV << p << ": "                  \
       << #a << " = " << a           \
       << " , " << #b << " = " << b  \
       << " , " << #c << " = " << c  \
       << " , " << #d << " = " << d  \
       << " , " << #e << " = " << e  \
       << endl;                      \
}

#define DBG_VAR6(a,b,c,d,e,f)        \
{                                    \
    dbg_prefix();                    \
    EV << #a << " = " << a           \
       << " , " << #b << " = " << b  \
       << " , " << #c << " = " << c  \
       << " , " << #d << " = " << d  \
       << " , " << #e << " = " << e  \
       << " , " << #f << " = " << f  \
       << endl;                      \
}

#define DBG_PVAR6(p,a,b,c,d,e,f)     \
{                                    \
    dbg_prefix();                    \
    EV << p << ": "                  \
       << #a << " = " << a           \
       << " , " << #b << " = " << b  \
       << " , " << #c << " = " << c  \
       << " , " << #d << " = " << d  \
       << " , " << #e << " = " << e  \
       << " , " << #f << " = " << f  \
       << endl;                      \
}

} // namespace

