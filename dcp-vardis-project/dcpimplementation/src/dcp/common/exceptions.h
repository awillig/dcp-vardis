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

#include <exception>
#include <format>

/**
 * @brief This module defines various types of exceptions used in the
 *        DCP implementation
 *
 * Each of these exceptions takes a string in the constructor, and
 * this string is extended by a string representation of the exception
 * type.
 */


#define DCP_EXCEPTION(name_) \
  class name_ : public DcpException { \
  public: \
  name_ (std::string message) : DcpException (#name_, std::string(), message) {}; \
  name_ (std::string modname, std::string message) : DcpException (#name_, modname, message) {}; \
  };


namespace dcp {

  class DcpException : public std::exception {
  private:
    std::string ename_;
    std::string modname_;
    std::string message_;
  public:
    DcpException (std::string exname,
		  std::string modname,
		  std::string& message)
      : ename_ (exname),
	modname_ (modname),
	message_ (message)
    {};
    const char* what() const throw() { return message_.c_str(); };
    const char* ename() const throw() { return ename_.c_str(); };
    const char* modname() const throw() { return modname_.c_str(); };
  };

  DCP_EXCEPTION(RingBufferException)
  DCP_EXCEPTION(AVLTreeException)
  DCP_EXCEPTION(ConfigurationException)
  DCP_EXCEPTION(SocketException)
  DCP_EXCEPTION(ReceiverException)
  DCP_EXCEPTION(TransmitterException)
  DCP_EXCEPTION(BPClientLibException)
  DCP_EXCEPTION(VardisClientLibException)
  DCP_EXCEPTION(VardisStoreException)
  DCP_EXCEPTION(SRPStoreException)
  DCP_EXCEPTION(ManagementException)
  DCP_EXCEPTION(LoggingException)
  DCP_EXCEPTION(AreaException)
  DCP_EXCEPTION(AssemblyAreaException)
  DCP_EXCEPTION(DisassemblyAreaException)
  DCP_EXCEPTION(ShmException)
  DCP_EXCEPTION(VardisReceiveException)
  DCP_EXCEPTION(VardisTransmitException)
 
};  // namespace dcp
