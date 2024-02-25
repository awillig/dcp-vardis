# SRP/RDDP simulation code

Disclaimer (February 2024): This code is provided for inspection. It has been
used to generate simulation results but is not supported. It requires INET4.4 
and will not run with INET4.5.

To install and build the code, first make sure that INET4.4 is
installed and its it's `setenv` file is included in your `.bashrc`
file. Test that this is the case by running
```shell
echo $INET_ROOT
```
which should output the installation directory of the INET source files.
Ensure INET is built (i.e. that the `libINET.so` exists in
`$INET_ROOT/src/inet`).

To build the SwarmStack and SRP/RDDP code run the following commands
```shell
cd SwarmStack
make makefiles
make -j$nproc
cd ../srp_rddp
make makefiles
make -j$nproc
```

To run the simulations go to the appropriate simulation directory and
call the `run` script using `source`.

# Setting up in the OMNeT++ IDE
This should be as simple as pointing the IDE at the project directories.
Currently it is not setup to compile and run examples through the IDE.

