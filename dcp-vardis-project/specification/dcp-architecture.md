---

# DCP Architecture and Protocol Overview {#chap-architecture}

We break the DCP system down into a number of sub-protocols. Here we
introduce these sub-protocols and the environment they operate in.

The **underlying wireless bearer** (UWB) is not part of the DCP stack
proper, but refers to an underlying wireless technnology implementing
at least a physical (PHY) and medium access control (MAC) sublayer,
and offering a local broadcast facility. 

The DCP stack itself consists of three protocols. The **beaconing
protocol** (BP) sits on top of the UWB, whereas the **State Reporting
Protocol** (SRP) and the **Variable Dissemination Protocol** (VarDis)
operate in parallel on top of BP. In this section we give a high-level
overview over each of these four protocols, their detailed
specifications are then given in Sections
[BP](#chap-beaconing-protocol), [SRP](#chap-state-reporting-protocol)
and [VarDis](#chap-vardis).


Finally, by **applications** we refer to any entity using services
offered by the BP, SRP or VarDis protocols. These entities can be
applications or other protocols, and they use BP / SRP / VarDis
services through their respective interfaces.


## The Underlying Wireless Bearer (UWB) {#sec-architecture-uwb}

The UWB includes as a minimum a physical and a MAC layer. We do not
prescribe any specific technology or protocol stack for the UWB but
only make a reasonably minimal set of assumptions about its
capabilities and behaviour:

- It provides a local broadcast capability, i.e. it is possible to
  send a single packet in one transmission to an entire local
  single-hop neighbourhood. We do not assume the UWB to provide any
  kind of control over the size of this neighbourhood or the
  particular set of neighbours reachable -- this is in general
  influenced by physical layer parameters such as the transmit power,
  the modulation and coding scheme, antenna directivity and other
  factors outside the scope of the DCP. These broadcast packets are
  not acknowledged. A UWB entity also has the ability to receive such
  broadcasts from neighboured nodes and handing them over to the BP.

- The UWB provides a facility for protocol multiplexing, i.e. it is
  possible for the UWB to distinguish packets belonging to DCP (the
  BP, to be precise) from packets belonging to other protocol stacks,
  e.g. IPv4 or IPv6.

- The UWB has a known maximum packet size, which is available to the
  BP. At the discretion of DCP the actual size of beacon packets can
  be varied dynamically, e.g. to strike a balance between maintaining
  a small overhead ratio and avoiding channel congestion. This maximum
  packet size of the UWB shall be no smaller than 256 bytes.

- The UWB protects packets with an error-detecting or error-correcting
  code. We assume that the error-detection capability is practically
  perfect. As a result, none of the DCP protocols (BP, SRP, VarDis)
  will need to include own checksum mechanisms. 



## The Beaconing Protocol (BP)

At the lowest level of the DCP we have the **Beaconing Protocol**
(BP), which is responsible for periodically sending and receiving
beacon packets through the UWB, and for exchanging **client payloads**
(or simply payloads) as byte arrays with arbitrary BP client
protocols. Such client protocols operate on top of BP and use its
services, examples include the SRP and VarDis. The BP can include
payloads from several client protocols, or multiple payloads for the
same client protocol, in the same beacon.

The BP sits on top of the UWB and uses its ability to locally
broadcast beacons and receiving such local broadcasts from neighboured
nodes.

The BP interface to client protocols allows client protocols to
register and un-register a unique client protocol identifier with the
BP, which the latter uses to identify and distinguish payloads in
beacons. Once such a client protocol identifier has been registered, a
client protocol entity will be able to generate variable-length
payloads (which from the perspective of BP are just blocks of bytes
without any internal structure) for transmission, such that
neighboured nodes will receive these blocks under the same client
protocol identifier. Furthermore, in the reverse direction BP delivers
received payloads to their respective client protocols (as indicated
by their client protocol identifier).



## The State Reporting Protocol (SRP)


The **State Reporting Protocol** (SRP) is a client protocol of the
BP. In the transmit direction, the application frequently retrieves
safety-related information from the system (e.g. position, speed,
heading) and hands them over as a record of type `SafetyData` to the
SRP -- the specifics of this data type are outside the scope of this
document, but it has a fixed and well-known length. When the SRP
prepares a block for transmission by the BP, it always only includes
the information from the most recent `SafetyData` record it has
received. Furthermore, the SRP adjoins meta information like a
timestamp (of type `TimeStampT`) and a SRP sequence number to the
`SafetyData` record, which allows a receiving node to assess how old
the last SRP information received from the sender is.

On the receiving side, the SRP receives extended SRP `SafetyData`
records from neighboured nodes and uses these to build and maintain a
**neighbour table**, which registers all neighboured nodes and their
extended `SafetyData`. The information contained in the neighbour table
can then be used by applications to predict trajectories of other
drones, forecast collisions and so on. The neighbour table entries are
furthermore subject to a soft-state mechanism with configurable
timeout.

  
## The Variable Dissemination Protocol (VarDis)  

  
The **Variable Dissemination** (VarDis) protocol is a second client
protocol of the BP. VarDis allows applications to create, read, update
and delete (CRUD) so-called variables, which are disseminated by
VarDis into the entire network by piggybacking them onto beacons. A
key goal is to achieve a consistent view after any operation modifying
a variable (create, update, delete) across the entire network as
quickly and reliably as possible. To achieve this, the protocol
leverages spatial diversity and also offers the option of having
information about each modifying operation being repeated a
configurable number of times by each node receiving it. 

In the current DCP version update and delete operations are restricted
to the node that created the variable. The protocol also includes
mechanisms for nodes to detect missing information (like new variables
or missed updates) and to query neighbors for this missing
information.

The interface between VarDis and an application offers all operations
necessary for creating, reading, updating and deleting variables, as
well as auxiliary operations like listing the currently known
variables or retrieving meta-information about variables.

