# DCP Stack Implementation

(October 2026)

This provides an implementation of the DCP protocol
stack in Version 2.0.


## Tools and Versions

The implementation has been developed using the C++ programming
language under Linux (Ubuntu 24.04 and 26.04 Desktop), and underwent
initial tests on a Raspberry Pi 5 (Ubuntu Server 24.04, note that
Raspberry Pi OS will not work, as it is based on Debian Bookworm,
which uses too old versions of the g++ compiler). The implementation
uses C++-20 features, the GNU `g++` compiler in at least version 13.3,
and `cmake` in at least version 3.28.3 as build system. The module
documentation is built using the `doxygen` package in version 1.9.8,
as well as the `dia` and `mscgen` packages in versions 0.98.0 and
0.20, respectively. These all are the defaults under Ubuntu 24.04. For
unit testing currently GoogleTest in version 1.15.2 is used.

The implementation furthermore makes use of additional libraries:

- The C++ Boost library ([link](https://www.boost.org/)) in Version
  1.83 or later for logging, command line argument processing, random number
  generation and inter-process communication (including shared-memory
  management).
- The `libtins` library ([link](https://libtins.github.io/)) in
  version 4.5 for convenient sending and receiving / filtering packets
  over an interface. Libtins relies on `libpcap`.
- The `ncurses` library for some of the applications.

## Structure

### Separation into Processes

The implementation of the DCP stack is broken down into at least three
separate programs / demons: one demon for the beaconing protocol (BP),
one demon for the State Reporting Protocol (SRP) and one demon for the
Variable Dissemination protocol (VarDis). The SRP and VarDis demons
communicate with BP through two different mechanisms:

- BP Command sockets (implemented as Unix Domain Sockets) are used for
  exchanging all service primitives that are not related to the
  transfer of BP client protocol payloads.
- Shared memory areas: the BP demon maintains a separate shared memory
  area towards each BP client protocol for the transfer of client
  protocol payloads.


Similarly, all VarDis applications (or VarDis client protocols) are
separate processes from the VarDis demon and interface with the VarDis
demon using two similar mechanisms:

- VarDis command sockets (implemented as Unix Domain Sockets) are used
  for all VarDis services that are not related to the CRUD (create,
  read, update, delete) operations on the real-time variable database.
- Client shared memory areas: The VarDis demon maintains a separate
  shared memory area towards each VarDis application / client protocol
  that needs to use CRUD services. This area is used for RTDB-Create,
  RTDB-Update and RTDB-Delete services on variables.
- Global shared memory area: the actual variable database is stored in
  a global shared memory area, and the RTDB-Read service reads
  directly from this memory.

The SRP demon also offers a global shared memory area to its client
applications or client protocols. This global shared memory area
contains the neighbour table that applications can read, and it offers
an area into which applications can write the latest safety data
(position etc) of the node for transmission by the SRP protocol.

The main reason for separating the BP demon into its own process is to
reduce the attack surface, as the BP demon will need elevated rights
to access the wireless interface. Furthermore, this allows for
separate development of the BP demon and its client
protocols. Furthermore, separating out the VarDis and BP demons into
their own processes that interfaces with their clients allows to
support multiple clients in parallel and dynamically.

Logically, the BP, VarDis and SRP demons distinguish between
functionalities that are used in the implementation of the demon, and
functionalities that are needed on the client side. For example, a BP
client protocol will need to include the library `bpclient_lib.h`, and
this library provides all the available BP services. The
implementation of this client library uses either the BP command
socket or a shared memory segment to exchange data related to the
service request (from client to BP demon) and service confirmations
(from BP demon back to client, if required). Similar considerations
apply to the VarDis and SRP demons.



### Directory Structure

* `src`: Top-level directory for implementation, contains
  `CMakeLists.txt` file defining the `cmake` build process. Also
  contains scripts for simplifying development (e.g. generating
  example configuration files, defining shell aliases for building and
  testing, and others).
* `test`: Contains test code (using GoogleTest framework)
* `src/dcp/applications`: contains some VarDis applications, currently
  a test variable producer and a consumer which continuously displays
  all test variables on the terminal, an application to delete a VarDis
  variable, an application to describe a VarDis variable, and an
  application to list all currently known VarDis variables.
* `src/dcp/bp`: contains all the supporting functionality needed by BP
  clients and required for implementing the BP demon. Also contains
  the `dcp-bp` main driver program for the BP demon. This and the
  other driver programs offer a command line interface to let users
  invoke various management commands.
* `src/dcp/common`: contains modules that are required across multiple
  other modules (e.g. support for command sockets, shared memory
  areas, logging).
* `src/dcp/srp`: contains the required functions to implement the SRP
  demon. Also contains
  the `dcp-srp` main driver program for the SRP demon.
- `src/dcp/vardis`: contains all the supporting functionality for
  implementing VarDis clients and for implementing the VarDis demon.
  Also contains the `dcp-vardis` main driver program for the VarDis
  demon.
- `src/dcp/test`: contains unit tests.


## Building

Provided all the pre-requisite packages are installed, and the local
`git` repository has been updated, a build can be produced using the
following commands (assuming we are in the top-level directory of the
DCP stack implementation):

``` shell
cd src
source setalias.sh
do-clean
do-cmake-dbg
do-build
do-test
```

This builds the DCP stack software in the directory `../_build/` in a
debug version (use `do-cmake-rel` to build it in a release version),
including the module documentation. The main output files are:

* `../_build/dcp/bp/dcpmain-bp`: main BP executable, can invoke a
  number of management commands or start the BP demon.
* `../_build/dcp/vardis/dcpmain-vardis`: main VarDis executable, can
  invoke a number of management commands or start the BP demon.
* `../_build/dcp/srp/dcpmain-srp`: main SRP executable.
* `../_build/dcp/bp/libdcp-bp.so`: is a dynamic library containing all
  the modules from `src/dcp/bp`, i.e. all the functionality required to support BP
  clients and support implementation of the BP demon.
* `../_build/dcp/common/libdcp-common.so`: library containing
  functionality common to multiple demons or clients, build from the
  contents of `src/dcp/common`.
* `../_build/dcp/srp/libdcp-srp.so`: library containing functionality
  for the SRP demon and SRP clients. Build from the contents of
  `src/dcp/srp`.
* `../_build/dcp/vardis/libdcp-vardis.so`: library containing all the
  functionality required to support VarDis clients and the support
  implementation of the VarDis demon. This includes all modules from
  `src/dcp/vardis`.

Furthermore, the `../_build/dcp/test` directory contains some
executables containing the tests (using the GoogleTest framework). The
module documentation in HTML and LaTeX formats is generated into
directory `../_build/doc`.


## Missing or Incomplete Functionality

- The build process does not yet include an installation or provision
  of different versions (debug, release).
- No attempt at performance optimization has been carried out yet.
- Much more testing is needed (unit and integration testing).
- No attempt has been made to test this on other Linux versions or
  porting it to other operating systems.


## Known Issues

- The dependence on `libtins` is probably not needed, as only very
  little of its functionality is used and this library is not very
  actively maintained. An implementation directly using `libpcap` is
  preferrable.

- The `dcpmain-bp` demon cannot be easily stopped using
  Ctrl-C. This is a result of the lack of a proper timeout
  functionalityy in `libtins`/`libpcap` under Linux when attempting to
  read packets -- it is not possible to have the library stop reading
  when nothing has been received for a while.

- The separation into separate demons for BP and each client protocol
  likely has performance implications. An alternative could be a split
  into one low-level demon just responsible for sending and receiving
  packets, and a demon combining BP and client protocols. This might allow
  to reduce the number of processing steps.
