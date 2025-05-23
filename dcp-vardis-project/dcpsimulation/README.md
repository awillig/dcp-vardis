
DCP/Vardis Simulation for OMNeT++
=================================

The DCP/Vardis simulator is an open-source simulation model for the
DCP/Vardis protocol stack as described in this [arXiv
report](https://arxiv.org/abs/2404.01570) and its enhanced and revised
[journal version](https://doi.org/10.1016/j.comcom.2024.108021). It
aims to be a reasonably (though not completely) faithful
implementation of the DCP/VarDis protocol stack specification, which
can be found [here](https://github.com/awillig/dcp-vardis.git). The
simulator has been developed for the OMNeT++ discrete-event simulation
framework ([website](https://omnetpp.org)) and in addition uses the
INET module library for OMNeT++
([website](https://inet.omnetpp.org/)). The simulator shares
substantial amounts of code with the DCP implementation (which can be
found in the directory `dcpimplementation` adjacent to the current
directory `dcpsimulation`).

IMPORTANT: The DCP/Vardis specification, implementation and simulation
model are being continuously developed, bugs are corrected, features
are added et cetera. We do not guarantee that the implementation or
simulator code fully conforms to the specification or that the
specification as such is free of errors or ambiguities.

We welcome any contributions! If you are interested in contributing to
this project please get in touch with [Andreas
Willig](mailto:andreas.willig@canterbury.ac.nz).


Versions
========

Presently, the code is developed under Ubuntu Linux 24.04 on an
AARCH64 platform. It works together with OMNeT++ in version 6.1 and
builds on the INET framework in version 4.5.4. Both OMNeT++ and the
INET framework have been built with the `gcc` toolchain in version
13.3. The code itself uses features of the C++20 standard.


Build process
=============


## Building on the command line

The following description assumes that you are running the `bash`
shell under Linux. For other shells, you will need to modify some
commands accordingly.

To install and build the code, first make sure that OMNeT++ in version
6.0 or higher, and the INET framework in version 4.5 or higher are
installed and their `setenv` files are included in (or called from)
your shell startup file (file `~/.bashrc`). Test that this is the case
by running
```shell
echo $INET_ROOT
``` 
which should output the installation directory of
the INET source files. Furthermore, ensure that: 
  - INET is built (i.e. that the file `libINET.so` and/or
    `libINET_dbg.so` exists in `$INET_ROOT/src/`),
  - the `bin/` sub-directory of your OMNeT++ installation directory is
    in your `PATH` environment variable, and
  - the `lib/` sub-directory of your OMNeT++ installation directory is
    in your `LD_LIBRARY_PATH` environment variable.
  - the environment variable `__omnetpp_root_dir` points to the root
    directory of your OMNeT++ installation. This is needed by the
    build process for the DCP implementation.

Before building the simulator you will have to build the DCP
implementation to create suitable libraries that can be linked to the
simulator. This can be achieved as follows:
``` shell
cd dcpimplementation/src
rm -rf ../_build/
cmake -S . -B ../_build
cmake --build ../_build
```
You can follow this by running the available unit tests:

``` shell
cd ../_build/ && ctest && cd ../src/
```

To build the DCP/Vardis code run the following commands
```shell
cd ../../dcpsimulation
opp_makemake -f -r -M release --deep -I$INET_ROOT/src -I./src -I../dcpimplementation/src -L$INET_ROOT/src -L../dcpimplementation/_build -lINET -ldcplib-common-sim -ldcplib-bp-sim -ldcplib-vardis-sim
make -j$nproc
```
This invocation builds the simulation in the OMNeT++ release mode and
uses `$nproc` processor cores for compilation. If you want to build in
the OMNeT++ debug mode, then replace the `-M release` option by `-M
debug` and replace `-lINET` by `-lINET_dbg`.

To run the simulations make sure that your `NEDPATH` environment
variable points to the `src` sub-directory of the `dcpsimulation`
directory. Then change into the `simulation/` sub-directory and run
the generated executable with the provided example simulation:
```shell
../out/gcc-release/dcpsimulation -u Cmdenv -c TestNetwork
```


## Setting up in the OMNeT++ IDE
This should be as simple as pointing the IDE at the project directories.
Currently it is not set up to compile and run examples through the IDE.



Directory Structure
===================

The directory structure follows the standard structure for an OMNeT++
project:

* `simulations/`: contains two simple example simulations
* `src/`
    + `src/dcp/applications/`: implements two related VarDis
      applications, one for a variable producer and one for a variable
      consumer. Both use the VarDis interface and operate on the same
      type of variables.
    + `src/dcp/bp/`: implements the beaconing protocol (BP), additionally
      offers a class from which BP client protocols should be derived.
	+ `src/dcp/common/`: offers a base class from which any DCP
      protocol should be derived (directly or indirectly), as well as
      some global types and variables / constants.
	+ `src/dcp/networks/`: contains two NED top-level network modules
      for the test simulations.
	+ `src/dcp/node/`: contains NED definitions for the DCP stack and
      a network node including the DCP stack (built on INET's
      `AdhocHost`).
	+ `src/dcp/srp/`: contains the implementation of the State
      Reporting Protocol (SRP).
	+ `src/dcp/vardis/`: contains the implementation of the VarDis
      protocol, additionally offers a class from which VarDis
      applications can be derived (directly or indirectly).



Restrictions
============

The simulator does not provide a complete implementation of the
DCP/Vardis specification. Some of the missing functionalities include:

* VarDis management services (activate/deactivate) are not implemented.
* BP management services (activate/deactivate) are not implemented.

Also, currently the codebase of the simulator is completely disjoint
from the code base of the implementation, and the latter is more
recent and more complete. A short-to-medium term aim is to base both
on a shared code basis.
