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
#include <iostream>

/**
 * @brief This module defines various types of exceptions used in the
 *        DCP implementation
 *
 * Each of these exceptions takes a string in the constructor, and
 * this string is extended by a string representation of the exception
 * type.
 */


namespace dcp {

  class DcpException : public std::exception {
  private:
    std::string message_;
  public:
    DcpException (const std::string& message) : message_(message) {};
    const char* what() const throw() { return message_.c_str(); };
  };

  class RingBufferException : public DcpException {
  public:
    RingBufferException (const std::string& message) : DcpException("RingBuffer: " + message) {};
  };

  class AVLTreeException : public DcpException {
  public:
    AVLTreeException (const std::string& message) : DcpException("AVLTree: " + message) {};
  };
  
  class ConfigurationException : public DcpException {
  public:
    ConfigurationException (const std::string& message) : DcpException("Configuration: " + message) {};
  };

  class SocketException : public DcpException {
  public:
    SocketException (const std::string& message) : DcpException("Socket: " + message) {};
  };
  
  class ReceiverException : public DcpException {
  public:
    ReceiverException (const std::string& message) : DcpException("Receiver: " + message) {};
  };

  class TransmitterException : public DcpException {
  public:
    TransmitterException (const std::string& message) : DcpException("Transmitter: " + message) {};
  };

  class BPClientLibException : public DcpException {
  public:
    BPClientLibException (const std::string& message) : DcpException("BPCLientLib: " + message) {};
  };

  class VardisClientLibException : public DcpException {
  public:
    VardisClientLibException (const std::string& message) : DcpException("VardisCLientLib: " + message) {};
  };

  class VardisStoreException : public DcpException {
  public:
    VardisStoreException (const std::string& message) : DcpException("VardisStore: " + message) {};
  };

  class SRPStoreException : public DcpException {
  public:
    SRPStoreException (const std::string& message) : DcpException("SRPStore: " + message) {};
  };
  
  class ManagementException : public DcpException {
  public:
    ManagementException (const std::string& message) : DcpException("Management: " + message) {};
  };
  
  class LoggingException : public DcpException {
  public:
    LoggingException (const std::string& message) : DcpException("Logging: " + message) {};
  };


  class AreaException : public DcpException {
  public:
    AreaException (const std::string& message) : DcpException("Area: " + message) {};
  };

  class AssemblyAreaException : public DcpException {
  public:
    AssemblyAreaException (const std::string& message) : DcpException("AssemblyArea: " + message) {};
  };
  
  class DisassemblyAreaException : public DcpException {
  public:
    DisassemblyAreaException (const std::string& message) : DcpException("DisassemblyArea: " + message) {};
  };
  
  class ShmException : public DcpException {
  public:
    ShmException (const std::string& message) : DcpException("Shm: " + message) {};
  };

  class VardisReceiveException : public DcpException {
  public:
    VardisReceiveException (const std::string& message) : DcpException("VardisReceive: " + message) {};
  };

  class VardisTransmitException : public DcpException {
  public:
    VardisTransmitException (const std::string& message) : DcpException("VardisTransmit: " + message) {};
  };

 
};  // namespace dcp
