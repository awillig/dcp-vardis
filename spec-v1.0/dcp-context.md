---

# Context and Aims


When operating a (connected) swarm or formation of unmanned aerial
vehicles (UAVs) or drones, the swarm members will have to communicate
wirelessly with each other for purposes of coordination (e.g. to
decide where the formation should move), or avoidance of physical
drone collisions. To address the requirement of collision avoidance we
assume that each drone frequently transmits messages carrying its own
position, speed, heading and possibly other operational information to
its local neighbourhood, typically using a local
broadcast. Neighboured drones can use this information to estimate the
trajectory of the sender and predict impending collisions. This mode
of operation is very similar to the case of vehicular networks, where
each vehicle frequently transmits basic safety messages to its
neighbourhood, often at a rate of 10 Hz, to reduce uncertainty about
the position of neighbours to a tolerable level.

Under this arrangement, the network carries a constant background load
of periodically transmitted packets, which from hereon we refer to as
**beacons**. The Drone Coordination Protocol stack (DCP) described in
this document provides such a functionality of frequent beacon
transmission, and builds on it to not only let drones broadcast safety
information to single-hop neighbours, but also to provide a mechanism
allowing the replication of information throughout the entire
(generally multi-hop) network by _piggybacking_ it onto beacons. In
other words, we use the periodic beacons as a vehicle for a flooding
process instead of flooding each piece of information in separate
packets through the network. These separate packets would otherwise
compete with beacons for channel access and channel resources, and
would also lead to significantly higher packet overheads, particularly
for small information payloads as is often the case for
coordination-related information. Having each node repeating
piggybacked information a configurable number of times leads, together
with the inherently available spatial diversity, to a high degree of
redundancy and therefore high reliability. This overall functionality
of information replication throughout the entire network is very
useful for global coordination and task allocation purposes.

The VarDis (Variable Dissemination) protocol, which is a key part of
DCP, provides this beacon-based flooding mechanism and offers to
applications the abstraction of a **variable**. Such a variable is
intended to be a piece of information to be shared amongst all drones
in a swarm in a reliable (and hopefully fast) fashion, the goal being
to make sure that all stations very quickly have the same view on a
variable after it has changed. Variables can be created, read, updated
and deleted (CRUD). Currently, the variables provided by VarDis are
restricted in that only the station that created a variable is also
allowed to update and delete it, whereas any other node is only
allowed to read the current value of the variable (and query some of
its attributes). Variables can be dynamically created and removed. We
refer to the collection of all variables as the **real-time
database**.  A key property of the variable abstraction provided by
VarDis is that a node reading a variable will only have access to the
most recent variable value received through an update, but not to the
history of all updates that have happened in the past.

The DCP and in particular the VarDis protocol can operate over a
general wireless multi-hop network. Its design is largely independent
of the underlying wireless technology, as long as it provides a local
broadcast facility and each station can be allocated a unique
identifier like for example its MAC address. For better reading flow,
we will often use WiFi as the underlying technology, which clearly
fulfills these two foundational requirements.

This document describes the DCP and VarDis in a way that is largely
ignorant of implementation matters, like for example the allocation of
activities to processes, interprocess communication and management of
race conditions / mutual exclusion, memory management, or the precise
way in which service primitives are being exchanged between different
protocol entities.
