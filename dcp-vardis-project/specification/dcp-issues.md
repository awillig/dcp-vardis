---

# Known Issues, Shortcomings and Comments

## Overall

- There is no consideration of security. This includes a number of
  missing functionalities:
    - Encryption / decryption
	- Limiting certain management actions to proper clients. For
	  example, currently an BP client can de-register any registered
	  protocol and not just itself. Similarly, a VarDis client can
	  delete any variable, including variables created by other VarDis
	  clients.
	  
- Related: There is no liveness checking. For example, BP does not
  currently have a mechanism to discover that a client protocol
  instance is not running anymore. Similarly, VarDis has no way of
  checking whether the application that created a variable is still
  active. If it becomes inactive, a sensible course of action could be
  to remove all variables it created.
  

## Beaconing Protocol

### Known Issues

### Potential Future Features

- Allow the BP to dynamically change the allowable beacon size,
  e.g. to respond to congestion, and to also dynamically change the
  beaconing rate.

- When there are several higher-layer protocols and their combined
  payload exceeds the maximum beacon size, we might wish to be able to
  statically or dynamically prioritize higher layer protocols, to make
  sure the higher-priority payloads are always included.
  
- Offer a service to activate and de-activate BP.

- There is no runtime liveness checking of client protocols and no
  proper cleanup. That could be a soft-state mechanism for registered
  protocols, together with additional status codes.

- Add something like a 'network identifier' to the BP header, so that
  co-located drone swarms can be separated.


## SRP

### Potential Future Features

- Offer a service to activate and deactivate the SRP.

- The sequence numbers in the extended state records / neighbour table
  entries can potentially be used to form a packet loss rate
  estimate. While this is generally the responsibility of
  applications, SRP could offer a default estimator.


## VarDis


### Known Issues

- The behaviour when two different far-away nodes want to create
  variables with the same variable identifier at the same time is not
  yet convincing. This condition should be detected and the involved
  nodes should be alerted.

- There is no service to activate and deactivate VarDis, and to
  re-initialize it.

- Changing the `VARDISPAR_MAX_PAYLOAD_SIZE` parameter at runtime might
  require a new registration with BP (which is not currently done), as
  currently this is the only way for client protocols to hand down
  their max payload size to BP.

- It is unlikely to be always necessary to call `qDropNonexisting` and
  similar functions whenever an information element is created for a
  payload.

- When a node has produced a variable with a certain identifier, has
  run long enough to count up the sequence number to some large
  nonzero value, and then crashes, restarts fast and initializes the
  sequence number of this variable to zero again, then a number of
  future variable updates will be rejected by other nodes as being
  outdated (they will still have the variable in their local
  database). A mechanism is needed to make sure that consumer nodes
  will re-set their local sequence numbers and accept the new
  `VarCreateT` instruction and any future `VarUpdateT` instructions.

- Just repeating a `VarDeleteT` instruction `repCnt` might easily fail
  when `repCnt` is perhaps just one and the beacon packet sent by the
  producer containing the `VarDeleteT` instruction fails (e.g. due to
  a collision). A mechanism is needed to make it much more likely that
  such an instruction is reliably disseminated and not counter-acted
  by other instructions (such as `VarUpdateT`) for this variable still
  going around.



### Potential Future Features

- The protocol does not contain any mechanism to allow a consumer
  to check the liveness of (the producer of) a variable and remove
  variables for which the producer has died. We could add a separately
  configurable lifetime or timeout to each variable specification,
  such that if a node does not hear from the producer for that amount
  of time then the variable is dropped. A timeout could be infinite to
  forbid dropping. When a node drops a variable, it might flood that
  information into the network to force others to drop it as well, but
  that could be problematic if a disconnected node drops everything
  and then reconnects, asking everyone else to drop variables. This
  would also require a "Keepalive" feature, allowing the producer of a
  variable to send keepalive signals for that variable without
  necessarily changing its value.

- A more bandwidth-efficient summary mechanism, in particular for
  variables with low update rates.

- Add support for multiple writers to the same variable, who possibly
  write concurrently.

- Include a mechanism that allows a node not to include recent update
  instructions when it has overheard them often enough (the update
  will still be reflected in the summaries), or to make a
  probabilistic decision as to whether or not to include updates.

- When a node is switched on much later than the rest, it will have to
  learn about all variables and their values, and the current
  mechanism (through summaries and the occasional update) might be
  slow. A database synchronization procedure like in OSPF could be
  added, provided this can be made efficient.

- Add a mechanism allowing a node to re-create a variable after it has
  been re-started, while other nodes still have it in their memory.
