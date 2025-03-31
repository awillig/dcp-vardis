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



extern "C" {
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
}
#include <dcp/common/command_socket.h>
#include <dcp/common/exceptions.h>
#include <dcp/common/global_types_constants.h>
#include <dcp/common/other_helpers.h>


namespace dcp {

  static const int commandSocketListenBufferBacklog = 20;

  // -----------------------------------------------------------------------------------------

  CommandSocket::CommandSocket (std::string name, uint16_t timeout)
    : the_command_socket(-1),
      data_socket(-1),
      socketName (name),
      socketTimeoutMS (timeout)
  {
    if (name.empty())
      throw SocketException ("CommandSocket::ctor", "name must be nonempty");
    if (timeout <= 0)
      throw SocketException (std::format("{}.CommandSocket::ctor", name),
			     "timeout must be strictly positive");
  }

  // -----------------------------------------------------------------------------------------

  CommandSocket::~CommandSocket ()
    {
      if (the_command_socket >= 0)
	close_owner();
    };

  
  // -----------------------------------------------------------------------------------------

  void CommandSocket::open_owner (logger_type& log)
  {
    // check whether socket name exists
    const char* socket_name = socketName.c_str();
    if (!socket_name)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "No name for command socket given.";
	throw SocketException ("open_owner",
			       "no name for command socket given");
      }
    
    // check whether socket name is too long
    if (std::strlen(socket_name) > max_command_socket_name_length())
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "Path name of command socket is too long";
	throw SocketException ("open_owner",
			       "path name of command socket is too long");
      }

    // unlink / remove socket file in case there is a leftover from previous invocation 
    unlink (socket_name);

    // obtain the socket
    the_command_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (the_command_socket < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "Cannot open command socket, errno = " << errno << " , text = " << strerror(errno);
	throw SocketException ("open_owner",
			       std::format("cannot open command socket, errno = {}", strerror (errno)));
      }


    // bind the socket
    struct sockaddr_un addr;
    std::memset (&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    std::strncpy (addr.sun_path, socket_name, sizeof(addr.sun_path) - 1);

    auto curr_umask = umask (0);
    int ret = bind (the_command_socket, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if (ret < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "Cannot bind command socket, errno = " << errno << " , text = " << strerror(errno);
	close (the_command_socket);
	unlink (socket_name);
	throw SocketException ("open_owner",
			       std::format("cannot bind command socket, errno = {}", strerror (errno)));
      }
    umask (curr_umask);
    
    
    // set socket option to time out after configurable time
    struct timeval tv = milliseconds_to_timeval (socketTimeoutMS);
    ret = setsockopt(the_command_socket, SOL_SOCKET, SO_RCVTIMEO, (void*) &tv, sizeof(struct timeval));
    if (ret < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "Cannot set socket option on command socket, errno = " << errno << " , text = " << strerror(errno);
	close (the_command_socket);
	unlink (socket_name);
	throw SocketException ("open_owner",
			       std::format("cannot set socket option on command socket, errno = {}", strerror (errno)));
      }
    BOOST_LOG_SEV(log, trivial::info) << "Set receive timeout of command socket to " << tv.tv_sec << " seconds and " << tv.tv_usec << " microseconds";

    
    // calling listen on the socket
    ret = listen (the_command_socket, commandSocketListenBufferBacklog);
    if (ret < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "Cannot call listen on command socket, errno = " << errno << " , text = " << strerror(errno);
	close (the_command_socket);
	unlink (socket_name);
	throw SocketException ("open_owner",
			       std::format("cannot call listen on command socket, errno = {}", strerror (errno)));
      }

    if (the_command_socket < 0)
      throw SocketException ("open_owner", "command socket inexplicably not open");
  }

  // -----------------------------------------------------------------------------------------

  void CommandSocket::close_owner ()
  {
    if (the_command_socket >= 0)
      {
	unlink (socketName.c_str());
	close (the_command_socket);
      }
    the_command_socket = -1;
  }

  // -----------------------------------------------------------------------------------------

  int CommandSocket::read_owner (logger_type& log, int& data_socket, byte* buffer, size_t buflen)
  {
    if (the_command_socket < 0)
      {
	return -1;
      }
    
    // we first call select with a timeout before calling accept
    // (because accept does not have timeout parameter)
    struct timeval tv = milliseconds_to_timeval (socketTimeoutMS);
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(the_command_socket, &rfds);
    int rv = select(the_command_socket + 1, &rfds, NULL, NULL, &tv);

    if (rv == 0)
      {
	return 0;
      }

    if (rv < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::read_owner: select() returns error, errno = " << errno << " , text = " << strerror(errno);
	return -1;
      }
    
    // now call accept
    data_socket = accept (the_command_socket, NULL, NULL);
    if (data_socket < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::read_owner: accept() returns error, errno = " << errno << " , text = " << strerror(errno);
	return - 1;
      }
	    
    // read received data, check its validity and process it
    int nbytes = read (data_socket, buffer, buflen);
    
    if (nbytes < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::read_owner: read() returns error, errno = " << errno << " , text = " << strerror(errno);
	close (data_socket);
	return -1;
      }

    return nbytes;
  }

  // -----------------------------------------------------------------------------------------

  int CommandSocket::start_read_command (logger_type& log, byte* buffer, size_t buflen, DcpServiceType& serv_t, bool& exitFlag)
  {
    serv_t = InvalidServiceType;
    
    if (data_socket != (-1))
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::start_read_command: socket still in use, exiting.";
	exitFlag = true;
	return -1;
      }
    
    int nbytes = read_owner (log, data_socket, buffer, buflen);
    
    if (nbytes < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::start_read_command: Error reading from socket, exiting.";
	exitFlag = true;
	if (data_socket >= 0)
	  {
	    close (data_socket);
	    data_socket = -1;
	  }
	return -1;
      }

    if (nbytes == 0)
      return nbytes;
    
    if (((size_t) nbytes) < sizeof(DcpServiceType))
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::start_read_command: truncated service type, nbytes = " << nbytes;
	close (data_socket);
	data_socket = -1;
	exitFlag = true;
	return -1;
      }
    
    // dispatch on actual service type
    serv_t = *((DcpServiceType*) buffer);
    
    return nbytes;
  };
  

  // -----------------------------------------------------------------------------------------

  int CommandSocket::stop_read_command (logger_type& log, bool& exitFlag)
  {
    if (data_socket < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::stop_read_command: un-used data socket";
	exitFlag = true;
	return -1;
      }
    close (data_socket);
    data_socket = -1;
    return 0;
  };


  // -----------------------------------------------------------------------------------------

  void CommandSocket::send_raw_confirmation (logger_type& log, const ServiceConfirm& conf, ssize_t confsize, bool& exitFlag)
  {
    if (data_socket < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::send_raw_confirmation: no data socket";
	exitFlag = true;
	return;
      }

    ssize_t bytessent = write (data_socket, (void*) &conf, confsize);
        
    if (bytessent < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::send_raw_confirmation: Error sending confirmation primitive, error when calling sendto(), errno = " << errno << " , text = " << strerror(errno);
	exitFlag = true;
	return;
      }

    if (bytessent != confsize)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::send_raw_confirmation: Error sending confirmation primitive, wrong number of bytes sent = " << bytessent;
	exitFlag = true;
	return;
      }
  }
  
  // -----------------------------------------------------------------------------------------

  ssize_t CommandSocket::send_raw_data (logger_type& log, byte* buffer, size_t len, bool& exitFlag)
  {
    if (data_socket < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::send_raw_data: no data socket";
	exitFlag = true;
	return -1;
      }

    ssize_t bytessent = write (data_socket, (void*) buffer, len);
    
    if (bytessent < 0)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::send_raw_data: error when calling write(), errno = " << errno << " , text = " << strerror(errno);
	exitFlag = true;
	return -1;
      }
    
    if (((size_t) bytessent) != len)
      {
	BOOST_LOG_SEV(log, trivial::fatal) << "CommandSocket::send_raw_data: error when calling write(), wrong number of bytes sent = " << bytessent;
	exitFlag = true;
	return -1;
      }
    
    return bytessent;
  }
  
  // -----------------------------------------------------------------------------------------

  int CommandSocket::open_client ()
  {
        // check whether socket name is too long
    if (std::strlen(socketName.c_str()) > max_command_socket_name_length())
      {
	throw SocketException ("open_client", "socket file name is too long");
      }

    // open the command socket
    int the_client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (the_client_sock < 0)
      {
	throw SocketException ("open_client",
			       std::format("cannot open socket, errno = {}", strerror (errno)));
      }
    
    
    // connect to socket
    struct sockaddr_un addr;
    std::memset (&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    std::strncpy (addr.sun_path, socketName.c_str(), sizeof(addr.sun_path) - 1);
    int ret = connect (the_client_sock, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    
    if (ret < 0)
      {
	close (the_client_sock);
	throw SocketException ("open_client",
			       std::format("cannot connect to socket {}, errno = {}", socketName, strerror (errno)));
      }
    
    // set socket option to time out after configurable time
    struct timeval tv = milliseconds_to_timeval (socketTimeoutMS);
    ret = setsockopt(the_client_sock, SOL_SOCKET, SO_RCVTIMEO, (void*) &tv, sizeof(struct timeval));
    
    if (ret < 0)
      {
	close (the_client_sock);
	throw SocketException ("open_client",
			       std::format("cannot set receive timeout, errno = {}", strerror (errno)));
      }
    
    return the_client_sock;

  }

  // -----------------------------------------------------------------------------------------
  
  int ScopedClientSocket::read_whole_response (byte* buffer, size_t buffer_len, int max_attempts)
  {
    if (the_sock < 0)
      {
	throw SocketException ("read_whole_response", "invalid socket");
	return -1;
      }
    
    size_t bytes_read = 0;
    int    attempts   = 0;
    
    while (true)
      {
	fd_set set;
	struct timeval timeout;
	FD_ZERO (&set);
	FD_SET (the_sock, &set);
	timeout.tv_sec   = 0;
	timeout.tv_usec  = 500000;
	
	attempts++;
	
	int rv = select (the_sock + 1, &set, NULL, NULL, &timeout);
	if (rv == -1)
	  throw SocketException ("read_whole_response",
				 std::format("select() returns errno = {}", strerror (errno)));
	
	if ((rv == 0) and (attempts >= max_attempts))
	  throw SocketException ("read_whole_response", "exhausted all attempts to read from socket");
	
	int nrcvd = read (the_sock, (void*) (buffer + bytes_read), buffer_len - bytes_read);
	
	if (nrcvd < 0)
	  throw SocketException ("read_whole_response",
				 std::format("read() returns errno = {}", strerror (errno)));
	
	if (nrcvd == 0)
	  {
	    return bytes_read;
	  }
	
	bytes_read += nrcvd;
	
	if (bytes_read > buffer_len)
	  throw SocketException ("read_whole_response",
				 std::format("buffer provided ({} B) is too small (req: {} B", buffer_len, bytes_read));
      }
  }
  

  // -----------------------------------------------------------------------------------------

  void CommandSocketConfigurationBlock::add_options (po::options_description& cfgdesc)
  {
    cfgdesc.add_options()
      (opt("commandSocketFile").c_str(),       po::value<std::string>(&commandSocketFile)->default_value(defaultValueCommandSocketFile), txt("file name of UNIX domain socket for exchanging BP commands with BP demon").c_str())
      (opt("commandSocketTimeoutMS").c_str(),  po::value<uint16_t>(&commandSocketTimeoutMS)->default_value(defaultValueCommandSocketTimeoutMS), txt("socket timeout (in ms)").c_str())
      ;

  }


  // -----------------------------------------------------------------------------------------

  void CommandSocketConfigurationBlock::validate ()
  {
    /***********************************
     * checks for command socket options
     **********************************/
    
    if (commandSocketFile.empty())
      throw ConfigurationException("CommandSocketConfigurationBlock",
				   "no command socket (UNIX domain socket) file name given");
    if (commandSocketFile.capacity() > CommandSocket::max_command_socket_name_length())
      throw ConfigurationException("CommandSocketConfigurationBlock",
				   "file name of command socket (UNIX domain socket) exceeds the maximum allowed length");
    if (commandSocketTimeoutMS <= 0)
      throw ConfigurationException("CommandSocketConfigurationBlock",
				   "command socket timeout (in ms) must be strictly positive");
  }

  
};  // namespace dcp
