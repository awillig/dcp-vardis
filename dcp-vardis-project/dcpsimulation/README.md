
DCP/Vardis Simulation for OMNeT++
=================================

The DCP/Vardis simulator is an open-source simulation model for the
DCP/Vardis protocol stack as described in this [arXiv
paper](https://arxiv.org/abs/2404.01570). It aims to be a reasonably
(though not completely) faithful implementation of the DCP/VarDis
protocol stack specification, which can be found
[here](https://github.com/awillig/dcp-vardis.git). The simulator has
been developed for the OMNeT++ discrete-event simulation framework
([website](https://omnetpp.org)) and in addition uses the INET module
library for OMNeT++ ([website](https://inet.omnetpp.org/)).

IMPORTANT: The DCP/Vardis specification and simulation model are being
continuously developed, bugs are corrected, features are added et
cetera. We do not guarantee that the simulator code fully conforms to
the specification or that the specification as such is free of errors
or ambiguities.

We welcome any contributions! If you are interested in contributing to
this project please get in touch with [Andreas
Willig](mailto:andreas.willig@canterbury.ac.nz).


Versions
========

Presently, the code is developed under Linux on an AARCH64
platform. It works together with OMNeT++ in version 6.0.1 and builds
on the INET framework in version 4.5. Both OMNeT++ and the INET
framework have been built with the `gcc` toolchain in version 11.4.


Build process
=============


# Building on the command line

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

To build the DCP/Vardis code run the following commands
```shell
cd dcpsimulation
opp_makemake -f -r -M release --deep -I$INET_ROOT/src -I./src -L$INET_ROOT/src -lINET
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


# Setting up in the OMNeT++ IDE
This should be as simple as pointing the IDE at the project directories.
Currently it is not setup to compile and run examples through the IDE.



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


