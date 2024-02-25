# Overview

  This component mainly centers around applications or network
  scenarios where "beaconing" is important. A key use case is
  in vehicular or flying ad-hoc networks, where each node periodically
  broadcasts its current position, direction, speed, etc. We refer to
  one such transmission as a beacon.
  
  A key property of beacons is that they are "locally broadcast",
  i.e. they go to all receivers within range. In terms of the MAC
  protocol this means that the WiFi MAC frames are addressed to the
  WiFi broadcast address.
  
  This module provides a facility to transmit local broadcasts within
  the OMNeT++ INET framework. INET provides a library of pre-defined
  protocols and components, including mobility support, wireless
  technologies like WiFi, a whole raft of IP protocols (IPv4, IPv6,
  TCP, UDP, ...) and others. Beyond this, INET provides an
  infrastructure and framework for systematically composing and
  decomposing packets from headers, and for multiplexing and
  demultiplexing protocols.
  
  The local broadcast facility is provided by the "Local Broadcast
  Protocol" (LBP). If we call the data that is to be locally broadcast
  a "beacon payload", then the LBP puts its own header before the
  beacon payload (which includes a protocol identifier, a version
  field, a sequence number, and a senderId [the MAC address of the
  transmitting station]), and embeds the result as payload into a WiFi
  data frame addressed to the broadcast address. Most of the code for
  the 'LocalBroadcastProtocol' is concerned with adding and removing
  the LBP header, constructing and deconstructing the resulting
  packets according to the conventions set by INET, and interacting
  with the underlying MAC protocol (WiFi) through a multiplexer. 
  
  The LBP implementation currently supports one higher layer protocol
  that uses local broadcasts. To implement such a higher layer
  protocol, a class 'LBPClientBase' is provided, from which any higher
  layer protocol using LBP should be derived.


# Contents of the Archive

  In the following I describe some parts of the source tree that may
  be more relevant to you. There is more, in various states of
  completion.


## Subdirectory src/

   This contains a few relevant subdirectories.
   
   The subdirectory 'base/' contains some common declarations,
   including shorthand type declarations for MacAddresses and node
   identifiers (same as Mac addresses).
   
   The subdirectory 'lbp/' contains:
     - The actual Local Broadcast Protocol (LBP). The LBP header is
       declared in the file 'LocalBroadcastHeader.msg', the LBP itself
       is contained in LocalBroadcastProtocol.cc / .h / .ned.
     - A base class for LBP clients, i.e. for higher layer protocols
       which want to embed their packets into LBP. It is an abstract
       base class with methods for sending a packet via the LBP, for
       processing packets received via LBP, and for handling other
       messages. Furthermore there are some debug methods.
	 - A NED module interface 'ILbpClient.ned' which every LBP client
	   protocol needs to implement
	 - A NED compound module type 'AdhocLBPHost.ned' which describes
	   the protocol stack that a host may have, including the LBP and
	   the correct wiring of the LBP to the underlying MAC.
	   
   The subdirectory 'beaconing/' contains:
     - Subdirectory 'base/': basic declarations for a specific
       beaconing protocol (including a prescription of the contents of
       beacon messages)
	 - Subdirectory 'renewalbeaconing/' an example beaconing protocol
	   (RenewalBeaconing.cc/.h/.ned) which frequently broadcasts
	   beacons with a programmable inter-generation time, the beacons
	   incude current position and other data extracted from the INET
	   mobility framework.
	 - Subdirectory 'safetycheck/': an attempt at finding out whether
	   two nodes get too close to each other, never tested.

   The file 'RenewalBeaconingNetwork.ned' contains a sample NED
   network building a network nodes using the RenewalBeaconingNode
   node type (cf. the renewal beaconing protocol mentioned above).
   
   The subdirectory 'flooding/' contains a flooding protocol built on
   LBP that is probably of no relevance.
   
   The file 'package.ned' is the top-level package declaration.
  

## Subdirectory simulations/

   Contains an example ini file
  
  
# Limitations

   The LBP itself is not yet able to multiplex higher-layer protocols,
   i.e. it will not be able to cope with situations where different
   higher-layer protocols want to transmit their packets over LBP and
   have them cleanly separated.

   This code is not maintained any longer.

# Installation and Building

   To allow commandline builds, ensure that both the OMNeT++ and INET
   environments are setup. This can be done by adding the respective
   `setenv` scripts to your `.bashrc` folder. For an example, on my
   system this means adding the following to the `.bashrc` file:
   ```
   source $HOME/omnetpp-6.0/setenv -q
   source $HOME/omnetppv6workspace/inet4.4/setenv -q
   ```
   The `-q` argument supresses the print statements when the scripts
   execute. Then the create the source  makefile run
   ```
   make makefile
   ```
   Then to compile the program you can just call
   ```
   make
   ```

   To enable builds in the OMNeT++ IDE, you will need to add the
   INET and SwarmStack source folders to the include paths. This can be
   done by, once the project is imported into the IDE, right clicking
   project directory, selecting properties and then adding the INET
   and `SwarmStack/src` directories to the `Paths and Symbols` menu.
   It is probably best to add it to all configurations at once, and
   the paths must be added for all languages.

