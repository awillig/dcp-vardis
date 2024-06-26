---

# Known Issues, Shortcomings and Comments

## Overall

- There is not yet a disciplined process to change the value of
  configuration parameters at runtime (for adaptation purposes), and
  no statement as to when (and under which conditions) such changes
  would take effect.

## Beaconing Protocol

### Known Issues

- The interface does not yet allow BP to signal memory shortage
  issues, e.g. when a client protocol has chosen a queue as buffering
  mode and no more payload can be added to the queue.


### Potential Future Features

- Allow the BP to dynamically change the allowable beacon size,
  e.g. to respond to congestion.

- When there are several higher-layer protocols and their combined
  payload exceeds the maximum beacon size, we might wish to be able to
  statically prioritize higher layer protocols, to make sure the
  higher-priority payloads are always included. Alternatively we could
  go for round-robin.
  
- Offer a management facility to enable and disable the beacon
  transmission process.

- Offer a management facility to change the selection of inter-beacon
  generation times, in particular the (long-term) rate at which
  beacons are generated

- There is no runtime liveness checking of client protocols and no
  proper cleanup. That could be a soft-state mechanism for registered
  protocols, together with additional status codes

- Add something like a 'network identifier' to the BP header, so that
  we can reliably separate two swarms in close vicinity

- Introduce a finite queue into the interface between client protocols
  and BP, in addition to the queue of indefinite size. This finite
  queue will need additional status codes when the queue is full at
  the arrival of another payload
  
- Allow to control the process of which payloads to include in the
  beacon by introducing explicit priorities for client protocols


## SRP

### Potential Future Features

- Offer a management facility to activate and deactivate the SRP

- The sequence numbers in the extended state records / neighbour table
  entries can potentially be used to form a packet loss rate estimate.


## VarDis


### Known Issues

- The behaviour when two different nodes want to create variables with
  the same VarId is not yet convincing, particularly when this happens
  at about the same time on two far-away nodes, so that neither
  creator knows the intention of the others.

- The VarDis interface does not allow to signal any memory management
  issues (e.g. when it is running out of storage when adding new variables)

- Unclear what should happen when a new VarCreate arrives for a variable
  that is currently in the process of being deleted on a consumer
  node. That could happen when the producer follows a delete
  immediately with another create. Right now a receiving node will
  ignore the new create until the variable has completely
  disappeared. Would we need sequence numbers for variable existence
  periods?

- Changing the `VARDISPAR_MAX_PAYLOAD_SIZE` parameter at runtime might
  require a new registration with BP (which is not currently done), as
  currently this is the only way for client protocols to hand down
  their max payload size to BP.

- It is unclear whether it is necessary to call `qDropNonexisting`
  whenever an information element is created for a payload. 



### Potential Future Features

- The protocol does not contain any mechanism to allow a consumer
  check the liveness of the producer of a variable and remove
  variables for which the producer has died (though a 'dead variable'
  does not require much bandwidth). We could add a separately
  configurable lifetime or timeout to each variable specification,
  such that if a node does not hear from the producer for that amount
  of time then the variable is dropped. A timeout could be infinite to
  forbid dropping. When a node drops a variable, it might flood that
  information into the network to force others to drop it as well, but
  that could be problematic if a disconnected node drops everything
  and then reconnects, asking everyone else to drop variables.

- A more bandwidth-efficient summary mechanism, in particular for
  variables with low update rates.

- Add support for multiple writers to the same variable, who possibly
  write concurrently. When very precise timestamps are available,
  these could be included in updates and precedence being decided
  based on them.

- Transmit update and create requests more than once. And include in
  such requests the node identifier of the preferred sender
  (presumably the node from which we have learned the need to get the
  updates), to avoid having the entire neighbourhood responding.

- Introduce separate information element types for variables with
  fixed length or of well-known and pre-defined type (only included /
  specified in their creation record). For such fixed-length variables
  the IE header does not need to include length information, but if a
  node receives such a fixed-length IE for a variable it does not yet
  know about (in particular: for which it does not yet have
  type/length information), the remainder of the packet may become
  unparse-able.

- Include a mechanism that allows a node not to include recent updates
  when it has overheard them often enough (the update will still be
  reflected in the summaries), or to make a probabilistic decision as
  to whether or not to include updates.

- Create a separate `MAX_SIZE` type of parameter for each of the
  information element types.

- Single-writer scenario: when a consumer node realizes that the same
  VarId has been created by different nodes, it could itself flood an
  "Abort" message into the entire network, deleting both instances of
  the variable.

- Consider the handling of timestamps, allow for updates to carry the
  timestamp of the producer and have consumers being able to use that
  timestamp and their local timestamp to tell propagation time

- While responding to `RTDB-Create.request` and `RTDB-Update.request`
  primitives we currently stop procesing when `length`. We could allow
  this to indicate a NULL value.

- When a node is switched on lately, it will have to learn about all
  variables and their values, and the current mechanism (through
  summaries and the occasional update) might be slow. A database
  synchronization procedure like in OSPF could be added, provided this
  can be more efficient.

- Add services and logic to re-initialize and suspend / resume
  VarDis. During suspension no service requests shall be accepted and
  no beacons be transmitted or received(?).

- Add a mechanism allowing a node to re-create a variable after it has
  been re-started, while other nodes still have it in their memory.

- Revise the specification of when exactly VarDis generates a new
  payload. It could make use of the `nextBeaconGenerationEpoch` value
  of the `BP-PayloadTransmitted` indication sent by BP to generate the
  next VarDis payload just in time before the next beacon generation,
  so that VarDis contents are as 'fresh' as possible.
