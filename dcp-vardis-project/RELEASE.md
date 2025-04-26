Version 1.3.6, April 2025
=========================

No specification changes, no changes to the implementation. The
simulator code is now based to a significant degree on code from the
implementation, in particular concerning data types and the
implementation of actual protocol behaviours.


Version 1.3.5, April 2025
=========================

No specification changes, no changes to the simulation. Implementation
changes: substantial re-working of shared memory management, receive
path for SRP and Vardis does not use polling anymore but rather
blocking waits on a shared memory queue.

Version 1.3, March 2025
=======================

This release does not introduce any changes to the specification or
the simulator. The implementation has been revised, in particular the
shared-memory management and the SRP and Vardis receive path
processing. These do not use polling anymore.


Version 1.2, February 2025
==========================

The main feature of this release is an initial implementation of the
DCP/Vardis protocol stack in C++ under Linux, and tested on Raspberry
Pi 5 devices. This implementation is in its early stages and not fully
complete. In the process of preparing this implementation a few
non-major changes to the specification have been made and partially
ported to the simulator as well.


Version 1.1, October 2024
=========================

This is an updated version of the version 1.0 release. The specification has
been updated and clarified in a number of places (chech the file dcp-changelog.md)
but without changing packet structures. Consequential changes have been made
to the reference simulator.



Version 1.0, September 2024
===========================

This is the first release. It contains the specification of the
DCP/VarDis protocol stack and a simulator based on the OMNeT++
discrete-event simulation framework.
