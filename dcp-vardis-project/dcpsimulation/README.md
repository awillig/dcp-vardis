
DCP/Vardis Simulation for OMNeT++
=================================

The DCP/Vardis simulator is an open-source simulation model for the
DCP/Vardis protocol stack as described in this [arXiv
paper](https://arxiv.org/abs/2404.01570). It aims to be a reasonably
(though not completely) faithful implementation of the DCP/VarDis
protocol stack specification, which can be found
[here](https://github.com/awillig/dcp-vardis.git).

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

The following description assumes that you are running the `bash`
shell. For other shells, you will need to modify some commands
accordingly.

To install and build the code, first make sure that OMNeT++ in version
6.0 or higher, and the INET framework in version 4.5 or higher are
installed and their `setenv` files are included in (or called from)
your shell startup file (file `~/.bashrc`). Test that this is the case
by running
```shell
echo $INET_ROOT
``` 
which should output the installation directory of
the INET source files.  Ensure INET is built (i.e. that the
`libINET.so` exists in `$INET_ROOT/src/inet`).

To build the DCP/Vardis code run the following commands
```shell
cd dcpsimulation
make makefiles
make -j$nproc
```

To run the simulations go to the appropriate simulation directory and
call the `run` script using `source`. Make sure that your `NEDPATH`
environment variable points to the `src` sub-directory of the
`dcpsimulation` directory.

# Setting up in the OMNeT++ IDE
This should be as simple as pointing the IDE at the project directories.
Currently it is not setup to compile and run examples through the IDE.



Organisation
============



Contributors
============

- Andreas Willig: DCP/Vardis specification, initial version of this simulator
- Sam Pell wrote an earlier version of a DCP/Vardis simulator
