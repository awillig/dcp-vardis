DCP/VarDis Project
==================

The DCP/VarDis protocol stack is designed to support command and
control in drone networks. It works by piggybacking command and
control data onto frequently transmitted beacons. A description of the
protocol stack is available in this [arXiv
preprint](https://arxiv.org/abs/2404.01570) and its enhanced and
revised [journal
version](https://doi.org/10.1016/j.comcom.2024.108021). Currently, the
project contains: 
* A specification of the DCP/VarDis protocol stack (in sub-directory `specification`)
* A simulator based on the OMNeT++ discrete-even simulation framework
  ([website](https://omnetpp.org)) and the INET module library for
  OMNeT++ ([website](https://inet.omnetpp.org/)). The simulator can be
  found in sub-directory `dcpsimulation`.  
* A first implementation of the DCP/Vardis protocol stack in the C++
  programming language under Linux, tested on Raspberry Pi 5
  devices. The implementation can be found in sub-directory
  `dcpimplementation`.

We welcome any contributions! If you are interested in contributing to
this project please get in touch with [Andreas
Willig](mailto:andreas.willig@canterbury.ac.nz).
