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


#include <cstdint>
#include <format>
#include <string>
#include <dcp/common/configuration.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/logging_helpers.h>
#include <dcp/common/services_status.h>


namespace dcp {

  /**
   * Provide support for command sockets, implemented under Linux as
   * Unix Domain Sockets (stream sockets).
   *
   * Command sockets are used to send commands from a 'client' to a
   * 'server' and have the server send responses back to the
   * client. The server is assumed to be active permanently (they
   * accept client connections at any time), whereas clients use
   * ephemeral connections to interact with the server.
   */


  /**
   * @brief Support for command sockets (both client and server).
   */
  class CommandSocket {
  private:
    int           the_command_socket   = -1;     /*!< Socket on which the server accepts connection requests */
    int           data_socket          = -1;     /*!< Current data socket when interacting with a client */
    std::string   socketName           = "";     /*!< Socket name, should be valid filename for Unix Domain Socket */
    uint16_t      socketTimeoutMS      =  200;   /*!< Timeout for reading from command socket */

    /**
     * @brief Waits for an incoming client connection for a limited
     *        amount of time and attempts to read data from it.
     *
     * @param log: the logging object to use
     * @param data_socket: the data socket returned by accept() call
     * @param buffer: buffer in which any read data is stored
     * @param buflen: length of buffer
     *
     * @return -1 in case of any error, 0 when no connection request
     *         came in during timeout, and >0 when data has been read
     *         from incoming connection -- in this case the return
     *         value gives the number of data bytes read.
     */
    int read_owner (logger_type& log, int& data_socket, byte* buffer, size_t buflen);
    
  public:

    CommandSocket () = delete;


    /**
     * @brief Constructor
     *
     * @param name: Socket name, should be valid filename for Unix Domain Socket
     * @param timeout: timeout in ms for reading from command socket and other operations
     *
     * No socket is opened yet.
     */
    CommandSocket (std::string name, uint16_t timeout)
      : the_command_socket(-1),
	data_socket(-1),
	socketName (name),
        socketTimeoutMS (timeout)
      {
	if (timeout <= 0) throw SocketException ("CommandSocket::ctor: timeout must be strictly positive");
	if (name.empty()) throw SocketException ("CommandSocket::ctor: name must be nonempty");
      };


    /**
     * @brief Destructor, closes the command socket if it is open.
     */
    ~CommandSocket ()
    {
      if (the_command_socket >= 0)
	close_owner();
    };

    
    /**
     * @brief Returns maximum length of socket name
     */
    static size_t max_command_socket_name_length () { return dcp::maxUnixDomainSocketPathLength - 1; };


    /**
     * @brief Returns whether the command socket (the one on which
     *        server accepts requests) is open
     */
    inline bool is_open () const { return (the_command_socket >= 0); };


    /**
     * @brief Returns name of the socket
     */
    std::string get_name () const { return socketName; };


    /**
     * @brief Opens the command socket (server side)
     *
     * @param log: Logging object to use
     *
     * First performs socket name validation, deletes socket file,
     * opens and binds the socket, sets socket options (timeout for
     * reading from the socket) and calls listen on the socket.
     *
     * In case of an error, the socket file is removed and an
     * exception is thrown.
     */
    void open_owner  (logger_type& log);


    /**
     * @brief Closes command socket (server side)
     */
    void close_owner  ();


    /**
     * @brief Method for the server to attempt reading a command from
     *        an incoming connection
     *
     * @param log: logging object to use
     * @param buffer: buffer to store read data in
     * @param buflen: size of the buffer
     * @param serv_t: output value storing the type of DCP service (command type)
     * @param exitFlag: output value, will be set to true when processing error
     *        occured and caller is expected to exit the server demon
     */
    int start_read_command (logger_type& log, byte* buffer, size_t buflen, DcpServiceType& serv_t, bool& exitFlag);


    /**
     * @brief Sends a confirmation primitive over the current data socket (server side, responding to a request)
     *
     * @param log: logging object to use
     * @param conf: The confirmation to send (of type ServiceConfirm or derived)
     * @param confsize: number of bytes to send
     * @param exitFlag: output value, will be set to true when processing error
     *        occured and caller is expected to exit the server demon
     */
    void send_raw_confirmation (logger_type& log, const ServiceConfirm& conf, ssize_t confsize, bool& exitFlag);


    /**
     * @brief Template method to create and send a simple confirmation
     *        over the current data socket (server side). A simple
     *        confirmation is one with fixed data size
     *
     * @tparam CT: Type of confirmation to create (and send)
     * @param log: logging object to use
     * @param statcode: The status code to write into the created confirmation
     * @param exitFlag: output value, will be set to true when processing error
     *        occured and caller is expected to exit the server demon
     */
    template <typename CT>
    void send_simple_confirmation (logger_type& log, DcpStatus statcode, bool& exitFlag)
    {
      CT conf;
      conf.status_code = statcode;
      send_raw_confirmation (log, conf, sizeof(CT), exitFlag);
    };

    
    /**
     * @brief Sends a block of raw data over the current data socket
     *        (server side)
     *
     * @param log: logging object to use
     * @param buffer: buffer containing the data to be sent over the socket
     * @param len: length of data to be sent
     * @param exitFlag: output value, will be set to true when processing error
     *        occured and caller is expected to exit the server demon
     */
    ssize_t send_raw_data (logger_type& log, byte* buffer, size_t len, bool& exitFlag);


    /**
     * @brief Used by server to close a data socket after processing command
     *
     * @param log: logging object to use
     * @param exitFlag: output value, will be set to true when processing error
     *        occured and caller is expected to exit the server demon
     */
    int stop_read_command (logger_type& log, bool& exitFlag);

    

    /**
     * @brief Opening a command socket as a client.
     *
     * @return Socket descriptor of the socket to exchange command and response
     *
     * Command socket with socket name is opened, and a read timeout
     * is set. Throws exceptions upon processing error.
     */
    int open_client (); 
    
  };
  

  // -----------------------------------------------------------------------------------


  /**
   * @brief Opens and closes a command socket as a client following
   *        the lifetime of this object.
   *
   * The methods of this class throw exceptions in case of processing
   * errors.
   */
  class ScopedClientSocket {

  private:
    int the_sock = -1;  /*!< Socket descriptor */

  public:
    
    ScopedClientSocket () = delete;


    /**
     * @brief Constructor, opens the given command socket as a client
     *
     * @param cmdsock: The command socket to use, needs to include the socket name
     */
    ScopedClientSocket (CommandSocket& cmdsock)
    {
      the_sock = cmdsock.open_client ();
      if (the_sock < 0)
	throw ManagementException ("ScopedClientSocket: invalid socket");
    };


    /**
     * @brief Destructor, closes the command socket
     */
    ~ScopedClientSocket ()
    {
      if (the_sock >= 0)
	close (the_sock);
    };


    /**
     * @brief Returns the client socket
     */
    int operator()() const { return the_sock; };


    /**
     * @brief Reads response data from the socket and stores them in the given buffer
     *
     * @param buffer: The buffer to store the data into
     * @param buffer_len: Size of the buffer
     *
     * @return the number of bytes read from the command socket (server response)
     */
    int read_response (byte* buffer, size_t buffer_len)
    {
      if (the_sock < 0)
	{
	  throw SocketException ("read_response: invalid socket");
	  return -1;
	}
      
      int nbytes = read (the_sock, buffer, buffer_len);
      if (nbytes < 0) abort ("read_response: socket has no data");
      return nbytes;
    };


    /**
     * @brief Convenience method for the client to send a request to
     *        the server and receive a response
     *
     * @tparam RT: class type of request, must have fixed size
     * @param sReq: the actual request to be sent to the server via the client socket
     * @param buffer: The buffer into which to write the response data
     * @param buffer_len: length of buffer for response data
     *
     * @return the number of bytes read from the command socket (server response)
     */
    template <class RT>
    int sendRequestAndReadResponseBlock (RT& sReq, byte* buffer, size_t buffer_len)
    {
      // send service request
      int ret = write (the_sock, (void*) &sReq, sizeof(sReq));
      
      if (ret < 0)
	abort ("sendRequestAndReadResponseBlock: cannot send request");
      
      // await and check response
      return read_response (buffer, buffer_len);
    };


    /**
     * @brief Just sends a service request without expecting a response
     *
     * @return The number of bytes written
     */
    template <class RT>
    int sendRequest (RT& sReq)
      {
	// send service request
	int ret = write (the_sock, (void*) &sReq, sizeof(sReq));
	
	if (ret < 0)
	  abort ("sendRequest: cannot send request");

	return ret;
    };

    

    /**
     * @brief Closes the client socket and throws an exception
     */
    void abort (const std::string& msg)
    {
      if (the_sock >= 0)
	close (the_sock);
      the_sock = -1;
      throw SocketException (std::format ("ScopedClientSocket::abort: {}", msg));
    };
    
  };
  

  // -----------------------------------------------------------------------------------


  const size_t command_sock_buffer_size = 2000;  /*!< Default buffer size for exchange of requests and responses */
  
  /**
   * @brief Supports definition of ClientRuntime classes that
   *        communicate with a server (another module / executable)
   *        via a command socket.
   *
   * Just to be used as a base class, provides the command socket, a
   * buffer for exchange of commands, and a template method for simple
   * exchange of request and confirm primitives of a service.
   */
  class BaseClientRuntime {
  protected:
    bool _isRegistered = false;                           /*!< Can be used by derived classes to track registration status with server */
    CommandSocket commandSock;                            /*!< The command socket to be used for communicating with server */

  public:

    BaseClientRuntime () = delete;


    /**
     * @brief Constructor, mainly initializes command socket (name and timeout)
     */
    BaseClientRuntime (std::string cmdsock_name, uint16_t cmdsock_timeout)
      : _isRegistered (false),
	commandSock (cmdsock_name, cmdsock_timeout)
    {};


    /**
     * @brief Getter for registration status
     */
    inline bool isRegistered () const { return _isRegistered; };


    /**
     * @brief Template method for carrying out a simple exchange of
     *        request and response (simple means: with request and
     *        response types of fixed size)
     *
     * @tparam RT: (simple) type of service request
     * @tparam CT: (simple) type of service confirm
     * @param methname: name of calling method (for debugging purposes)
     *
     * Creates a request of the given type and with given
     * DcpServiceType value, sends it to server, waits for response,
     * checks its service type for validity and returns the status
     * code contained in the response. Throws exceptions in case of
     * processing errors.
     *
     * Note: it is assumed that the RT is derived from class
     * ServiceRequest and has its s_type (service type) field set
     */
    template <typename RT, typename CT>
    DcpStatus simple_request_confirm_service (const std::string& methname)
    {
      ScopedClientSocket cl_sock (commandSock);
      RT srReq;
      byte buffer [command_sock_buffer_size];
      int nrcvd = cl_sock.sendRequestAndReadResponseBlock<RT> (srReq, buffer, command_sock_buffer_size);
      
      if (nrcvd != sizeof(CT))
	cl_sock.abort (methname + ": response has wrong size");
      
      CT *pConf = (CT*) buffer;
      
      if (pConf->s_type != srReq.s_type)
	cl_sock.abort (methname + ": response has wrong service type");
      
      return pConf->status_code;
    }
  };

  // -----------------------------------------------------------------------------------


  const std::string defaultValueCommandSocketFile        = "/tmp/dcp-command-socket"; /*!< Default name for command socket */
  const uint16_t    defaultValueCommandSocketTimeoutMS   = 100;                       /*!< Default timeout in ms for command socket */
  

  /**
   * @brief Holds configuration data for a command socket, adds
   *        correct options to config file parser and validates config
   *        data
   */
  class CommandSocketConfigurationBlock : public DcpConfigurationBlock {
  public:
    
    /**
     * @brief Filename of the UNIX domain socket used to exchange service
     *        primitives
     */
    std::string    commandSocketFile = defaultValueCommandSocketFile;
    
    
    /**
     * @brief Interval for checking termination condition
     */
    uint16_t commandSocketTimeoutMS  = defaultValueCommandSocketTimeoutMS;


    /**
     * @brief Constructors, setting the name of the block in the
     *        configuration file
     */
    CommandSocketConfigurationBlock () : DcpConfigurationBlock ("commandsock") {} ;
    CommandSocketConfigurationBlock (std::string bname) : DcpConfigurationBlock (bname) {};


    /**
     * @brief Adds descriptions of the options to BOOST config file parser
     */
    virtual void add_options (po::options_description& cfgdesc);


    /**
     * @brief Validates configuration values
     */
    virtual void validate ();

    
    
  };

  
  // -----------------------------------------------------------------------------------
  
  
};  // namespace dcp
