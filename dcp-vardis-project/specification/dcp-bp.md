---

# The Beaconing Protocol (BP) {#chap-beaconing-protocol}

This section describes the beaconing protocol (BP).

## Purpose

The BP entity on a node frequently effects the transmission of
**beacons** to a local neighbourhood, using the services of the
UWB. These beacons can include zero or more **client payloads** or
simply payloads, which refer to variable-length byte blocks generated
by a client protocol entity on the sending node, up to a
(configurable) maximum payload size. Furthermore, the BP receives such
beacons from local neighbours and delivers their payloads to their
respective client protocols.

The BP does not offer any guarantees about successful transmission of
client protocol payloads. Furthermore, as the criteria for deciding
which payload(s) to include in the next outgoing beacon is left to
implementations, there is also no guarantee that any client protocol
payload will be included in a beacon within any fixed amount of time.
Implementers can choose decision criteria that provide such guarantees
at least for some client protocols.


## Key Data Types and Definitions

### Data Types

- The transmissible data type `BPProtocolId` (short for protocol
  identifier) is a 16-bit unsigned integer value uniquely identifying
  a client protocol. The following values are pre-defined:
    - `BP_PROTID_SRP = 0x0001` represents the SRP client protocol.
    - `BP_PROTID_VARDIS = 0x0002` represents the VarDis client
      protocol.

- The transmissible data type `BPLength` is a 16-bit unsigned
  integer value indicating the length of a payload block. A payload is
  assumed to be a contiguous block of bytes without any further
  internal structure of relevance to BP.

- The non-transmissible data type `BPQueueingMode` has three
  distinguishable values, indicating how payloads handed over by
  client protocols for transmission are being buffered in the BP:
    - `BP_QMODE_QUEUE` means that payloads enter a first-in-first-out
      queue of indefinite length. Any payload is transmitted in a
      beacon only once, then dropped from the queue. A beacon can
      contain only one payload from the queue.
	- `BP_QMODE_ONCE` means that the BP places client payloads handed
	  down from client protocols into an internal buffer holding at
	  most one payload. When a new payload arrives before the buffer
	  contents have been transmitted, the buffer is over-written with
	  the new payload. Once a buffered payload has been transmitted,
	  it is removed from the buffer (i.e. it is only transmitted
	  once).
	- `BP_QMODE_REPEAT` is similar to `BP_QMODE_ONCE`, but the payload
	  is not dropped from the buffer after transmission, i.e. it may
	  be transmitted repeatedly (until the client protocol hands over
	  a new payload).
    - No other values are allowed.

- The non-transmissible data type `BPBufferEntry` holds all the data
  needed to describe a payload handed down from a client protocol for
  transmission. It is a record with the following fields:
    - `length` of type `BPLength` contains the length of the payload
      as a number of bytes.
	- `payload` is a byte array containing the actual payload. It has
	  exactly as many bytes as indicated by the `length` field.

- The non-transmissible data type `BPClientProtocol` holds all the
  information the BP maintains about a client protocol. It is a record
  with the following fields:
    - `protocolId` of type `BPProtocolId` contains the unique protocol
      identifier for the client protocol.
	- `protocolName` of type `String` is the human-readable name of
	  the client protocol.
	- `maxPayloadSize` of type `BPLength` specifies the maximum
	  allowable payload size for this client protocol.
	- `queueMode` of type `BPQueueingMode`
	- `timeStamp` of type `TimeStamp` contains the local time at which
	  the client protocol has been registered.
	- `queue` of type `Queue<BPBufferEntry>` is the queue holding
	  payloads needed when `queueMode` equals `BP_QMODE_QUEUE`.
	- `bufferOccupied` of type `Bool` is needed for `queueMode` being
	  either `BP_QMODE_ONCE` or `BP_QMODE_REPEAT`.
	- `bufferEntry` of type `BPBufferEntry` is needed for `queueMode`
	  being either `BP_QMODE_ONCE` or `BP_QMODE_REPEAT`.

- The transmissible data type `BPPayloadBlockHeader` holds all the
  header information for an individual payload. It is a record with
  the following fields:
    - `protocolId` of type `BPProtocolId` contains the unique protocol
      identifier of the registered client protocol generating the
      payload.
	- `length` of type `BPLength` contains the length of the payload
	  as a number of bytes.

- The transmissible data type `BPHeader` is the header for a BP packet
  including one or more payload blocks. It is a record with the
  following fields:
    - `magicNo` is an unsigned 16-bit value. It has to be set to
      `0x497E`.
	- `senderId` is of type `NodeIdentifier` and contains the node
      identifier of the node sending the beacon.
    - `version` is an unsigned 8-bit value containing the version
      number of the beaconing protocol being used. It has to be set to
      `0x01` (corresponding to Version 1 of the DCP protocol stack
      described in this specification).
	- `numPayloads` is an unsigned 8-bit value specifying the number
      of BP payload blocks contained in the remaining BP packet. Each
      payload block includes a `BPPayloadBlockHeader` followed by a
      block of bytes specific for the corresponding BP client
      protocol.
	- `seqno` is an unsigned 32-bit value. A BP instance maintains a
      corresponding local unsigned 32-bit variable `bpSequenceNumber`
      that is initialized to zero and incremented before each
      construction of a new beacon. The updated value is included in
      the `BPHeader`. This allows receiving nodes to estimate beacon
      loss rates towards the sending node.


### Client Protocol List

The BP maintains a variable `currentClientProtocols` of type
`List<BPClientProtocol>`. In addition to the standard operations on
list (see Section [Queues and Lists](#sec-queues-list)) this list also
supports the following operation: 

- `lProtocolExists()`, which takes as parameter a value of type
  `BPProtocolId` and returns a `Bool` indicating whether the list
  contains an entry of type `BPClientProtocol` with `protocolId` field
  equal to the paramter (result is `true`) or not (result `false`).
- `lRemoveProtocol()`, which takes as parameter a value of type
  `BPProtocolId` and removes all entries from the list for which their
  `protocolId` field is equal to the parameter.
- `lLookupProtocol()`, which takes as parameter a value of type
  `BPProtocolId` and returns the first entry from the list for which
  its `protocolId` field is equal to the parameter (if it exists).


## Interface

### Registration and De-Registration of Client Protocols

Before a client protocol entity is able to transmit payloads in
beacons or receive payloads from incoming beacons for its own
processing, it must register itself with the BP. At the end of its
operation the client protocol entity can de-register. Transmitting and
receiving payloads for the registered client protocol is only possible
while the registration is active.

Different client protocols are distinguished through their
`BPProtocolId` values.


#### Service `BP-RegisterProtocol` {#bp-interface-service-bp-register-protocol}

The purpose of this service is to allow a client protocol entity to
register itself with the local BP entity, so that the client entity
can submit and receive payloads.

The service user invokes this service by using the
`BP-RegisterProtocol.request` primitive, which carries the following
parameters: 

- `protId` of type `BPProtocolId` is the protocol identifier under
  which the new client protocol should be registered.
- `name` of type `String` is a human-readable name for the new client
  protocol.
- `maxPayloadSize` of type `BPLength` specifies the maximum allowable
  size for a payload for this client protocol.
- `queueingMode` of type `BPQueueingMode` specifies the queueing mode
  to be applied for this client protocol.
  
After receiving the `BP-RegisterProtocol.request` service primitive,
the BP performs at least the following actions:


~~~
1.     If (currentClientProtocols.lProtocolExists(protId) == true) then
          stop processing, return status code BP-STATUS-PROTOCOL-ALREADY-REGISTERED
2.     If (maxPayloadSize > (BPPAR_MAXIMUM_PACKET_SIZE - (sizeof(BPPayloadBlockHeader) + 4)))
	      stop processing, return status code BP-STATUS-ILLEGAL-MAX-PAYLOAD-SIZE
3.     Let newent : BPClientProtocol with
           newent.protocolId      =  protId
		         .protocolName    =  name
				 .maxPayloadSize  =  maxPayloadSize
				 .queueMode       =  queueingMode
				 .timeStamp       =  current system time
				 .queue           =  empty queue
				 .bufferOccupied  =  false
				 .bufferEntry     =  empty buffer
4.     currentClientProtocols.lInsert(newent)
5.     return BP-STATUS-OK
~~~



#### Service `BP-DeregisterProtocol` {#bp-service-bp-deregister-protocol}

The purpose of this service is to allow a client protocol entity to
revoke a protocol registration with BP, so that no payloads are being
received or transmitted any longer.

The service user invokes this service by using the
`BP-DeregisterProtocol.request` primitive, which carries the following
parameters:

- `protId` of type `BPProtocolId` is the protocol identifier of the
  protocol to be de-registered.
  
After receiving the `BP-DeregisterProtocol.request` service primitive,
the BP performs at least the following actions:

~~~
1.     If (currentClientProtocols.lProtocolExists(protId) == false) then
          stop processing, return status code BP-STATUS-UNKNOWN-PROTOCOL
2.     de-allocate buffers / queues as appropriate for chosen queueMode
3.     currentClientProtocols.lRemoveProtocol(protId)
4.     return BP-STATUS-OK
~~~

An implementation must guarantee that after finishing processing the
`BP-DeregisterProtocol.request` primitive no payloads from the
requesting client protocol is included into beacons anymore.



### Querying Registered Protocols

#### Service `BP-ListRegisteredProtocols`

This service returns a list of all currently registered protocols. For
each protocol its record of type `BPClientProtocol` is returned.

The service user invokes this service by using the
`BP-ListRegisteredProtocols.request` primitive, which carries no
parameters.

After receiving the `BP-ListRegisteredProtocols.request` primitive,
the BP performs returns a `BP-ListRegisteredProtocols.confirm`
primitive, which carries as a parameter the current value of the
`currentClientProtocols` variable (as registered while processing the
`BP-RegisterProtocol.request` primitive).


### Payload Exchange

The following services govern the exchange of payloads between the
BP and a registered client protocol.


#### Service `BP-ReceivePayload` {#bp-service-bp-receive-payload}

With this service the BP indicates to a client protocol that a
payload for this protocol has been received in an incoming beacon. The
payload is handed over as is to the client protocol without
further processing by the BP.

In particular, when the BP instance has received a payload for a
currently registered protocol, it generates a
`BP-ReceivePayload.indication` primitive for the client
protocol, which carries two parameters:

- `protId` of type `BPProtocolId` holds the protocol identifier of the
  payload, which is equal to the protocol identifier registered by the
  client protocol.
- `length` of type `BPLength` holds the length of the received payload
  in bytes.
- `value` is a block of bytes of length `length`, which contains the
  received payload.

Any received payload for a currently registered protocol is handed
over only once to its client protocol and cannot be retrieved
afterwards.


#### Service `BP-TransmitPayload` {#bp-service-bp-transmit-payload}


With this service a registered client protocol can hand down a payload
to BP for transmission in one of the next beacons. No guarantees are
given on when (or if) the payload is transmitted or whether it will be
successfully received by any neighbour.

The client protocol invokes this service by submitting a
`BP-TransmitPayload.request` primitive. This primitive carries the
following parameters:

- `protId` of type `BPProtocolId` holds the protocol identifier of the
  client protocol generating the payload.
- `length` of type `BPLength` holds the length of the payload
  to be transmitted in bytes.
- `payload` is a block of bytes of length `length` containing the
  payload to be transmitted.

After receiving the `BP-TransmitPayload.request` primitive, the BP
performs at least the following actions:

~~~
1.     If (currentClientProtocols.lProtocolExists(protId) == false) then
          stop processing, return status code BP-STATUS-UNKNOWN-PROTOCOL
2.     Let protEntry = currentClientProtocols.lLookupProtocol(protId)
3.     If (length > protEntry.maxPayloadSize) then
          stop processing, return status code BP-STATUS-PAYLOAD-TOO-LARGE
4.     If (protEntry.queueMode == BP_QMODE_QUEUE) then
          If (length > 0) then
             Let newent : BPBufferEntry with
    	         newent.length  =  length
 		               .payload =  payload
             protEntry.queue.qAppend(newent)
			 possibly trigger generation of beacon
			 stop processing, return BP-STATUS-OK
		  else
		     stop processing, return BP-STATUS-EMPTY-PAYLOAD
5.     If (    (protEntry.queueMode == BP_MODE_ONCE) 
            || (protEntry.queueMode == BP_MODE_REPEAT)) then
          If (length > 0) then
		     protEntry.bufferEntry.length   = length
			 protEntry.bufferEntry.payload  = payload
			 protEntry.bufferOccupied       = true
			 possibly trigger generation of beacon
		  else
		     protEntry.bufferOccupied       = false
			 protEntry.bufferEntry.length   = 0
			 protEntry.bufferEntry.payload  = null
  		  stop processing, return BP-STATUS-OK
~~~


### Payload Transmission Indication

#### Service `BP-PayloadTransmitted` {#bp-service-payload-transmitted}

This service is used to allow BP to signal to a currently registered
client protocol that one of its payloads has just been transferred
from their queue or buffer into a bearer payload (see Section [Packet
Format](#bp-packet-format)), generated during the process of
assembling the bearer payload.

When a client payload is added to the bearer payload, then BP issues a
`BP-PayloadTransmitted.indication' primitive with the following
parameters:

- `protId` of type `BPProtocolId` holds the protocol identifier of the
  payload, which is equal to the protocol identifier registered by the
  client protocol.

- `nextBeaconGenerationEpoch` of type `TimeStamp` tells the client
  protocol when the BP will prepare the _next_ beacon, so that the
  client protocol can prepare a new payload in time.


### Querying Number of Buffered Payloads

#### Service `BP-QueryNumberBufferedPayloads` {#bp-service-number-buffered-payloads}

This service allows a client protocol to query how many of its
payloads are currently available for transmission to the BP. 

The client protocol invokes this service by submitting a
`BP-QueryNumberBufferedPayloads.request` service primitive, which
carries only one parameter

- `protId` of type `BPProtocolId` holds the protocol identifier of the
  requesting client protocol.
  
After receiving the `BP-QueryNumberBufferedPayloads.request` service
primitive, the BP performs at least the following actions:

~~~
1.     If (currentClientProtocols.lProtocolExists(protId) == false) then
          stop processing, return status code BP-STATUS-UNKNOWN-PROTOCOL
2.     Let protEntry = currentClientProtocols.lLookupProtocol(protId)
3.     If (protEntry.queueMode == BP_QMODE_QUEUE) then
          stop processing, return status code BP-STATUS-OK and value of protEntry.queue.qLength()
4.     If (protEntry.bufferOccupied == true) then
          stop processing, return status code BP-STATUS-OK and value 1
       else
    	  stop processing, return status code BP-STATUS-OK and value 0
~~~


### Configurable Parameters {#bp-interface-configurable-parameters}

These can be set by client protocols or by station management entities
through a suitably defined management interface. Parameter changes are
assumed to take effect immediately and will apply to all subsequently
generated beacons and service invocations.

Here we specify only mandatory parameters that any BP implementation
needs to support. These are:

- `BPPAR_BEACON_PERIOD`: refers to the (long-term average) beacon period
- `BPPAR_MAXIMUM_PACKET_SIZE`: specifies the maximum packet size of
  beacons (which can include several payloads). This parameter must
  never be larger than the value of the `maxPacketSize` variable,
  which is initialized at startup and contains the maximum packet size
  allowed by the UWB.



## Packet Format {#bp-packet-format}

The BP prepares a block of data to be handed over to the underlying
wireless bearar (UWB, see Section [Architecture](#chap-architecture)),
we refer to this block of data as the **bearer payload**. We refer to
the payloads generated and processed by the client protocols (which
are embedded into the bearer payload) as client payloads or sometimes
simply as payloads. 


The bearer payload is made up of a beacon protocol header of type
`BPHeader` and one or more **payload blocks** following each other
without gap. A payload block wraps a client payload, it consists of a
value of type `BPPayloadBlockHeader`, immediately followed by the
actual client payload as a contiguous sequence of bytes.


The combined length of all payload blocks included in the bearer
payload must not exceed the value given in the
`BPPAR_MAXIMUM_PACKET_SIZE` parameter (see Section [Configurable
Parameters](#bp-interface-configurable-parameters)). When no client
payload is available at the time the BP attempts to construct a
beacon, then no beacon is being generated.


## Initialization, Runtime and Shutdown

During initialization of the BP the list of currently registered
client protocols (variable `currentClientProtocols`) is initialized as
the empty list, and the 32-bit variable `bpSequenceNumber` is
initialized to zero.


Furthermore, BP retrieves from the UWB the maximum allowed packet size
and stores this in the global variable `maxPacketSize`. The value of
the configurable `BPPAR_MAXIMUM_PACKET_SIZE` is initialized to the
`maxPacketSize` value.



## Transmit Path

The timing of beacon transmissions is not specified -- beacons can be
generated by the BP and submitted to the UWB for example strictly
periodically or with some random jitter; their generation might
additionally be triggered by the arrival of a new client
payload. However, for a fixed value of the `BP_BEACON_PERIOD`
parameter it is expected that the number of beacons transmitted during
a time period _T_ converges to (_T_/`BP_BEACON_PERIOD`) as _T_ grows
large.


When preparing a beacon packet, the BP inspects all available payload
queues and buffers (fields `queue`, `bufferOccupied` and `bufferEntry`
of the currently registered protocols in the list
`currentClientProtocols`, see Section [Service
TransmitPayload](#bp-service-bp-transmit-payload)) in an unspecified,
implementation-dependent order. It is also left to the implementation
to decide how many and which payload blocks (see Section [Packet
Format](#bp-packet-format)) are being added into a bearer payload, as
long as the combined length of payload blocks and the `BPHeader` does
not exceed the value of the `BP_MAXIMUM_PACKET_SIZE` parameter.

Let `clientProtocol` of type `BPClientProtocol` be a client protocol
entry in `currentClientProtocols` currently under consideration for
adding a payload to the bearer payload. The following rules apply:

- If `clientProtocol.queueMode` equals `BP_QMODE_ONCE` or
  `BP_QMODE_REPEAT`, then the payload contained in
  `clientProtocol.bufferEntry` can only be added to the bearer
  payload when the `clientProtocol.bufferOccupied` flag is `true` and
  when the payload length `clientProtocol.bufferEntry.length` of the
  payload stored in the buffer is small enough so that adding the
  resulting payload block to the bearer payload does not make the
  latter exceed the maximum bearer payload length
  (`BP_MAXIMUM_PACKET_SIZE`). When the BP does add the payload stored
  in the buffer, it will afterwards set the
  `clientProtocol.bufferOccupied` flag to `false` when the
  `clientProtocol.queueMode` flag supplied has value `BP_QMODE_ONCE`,
  whereas the `bufferOccupied` flag remains `true` when `queueMode` is
  equal to `BP_QMODE_REPEAT`.
- If `clientProtocol.queueMode` equals `BP_QMODE_QUEUE`, a payload can
  only be added if `clientProtocol.queue.qIsEmpty()` returns `false`
  and the `length` field of the head-of-queue element of type
  `BPBufferPayload` (obtained through `clientProtocol.queue.qPeek()`)
  indicates that the payload length is small enough so that adding the
  resulting payload block to the bearer payload does not make the
  bearer payload exceed the maximum bearer payload length
  (`BP_MAXIMUM_PACKET_SIZE`). When the BP indeed does add a payload
  from the queue, then it calls `clientProtocol.queue.qTake()` to
  remove this head-of-line payload from the queue. No more than one
  payload from the queue is added to the beacon.

In any case, if a payload of a client protocol is being added to the
bearer payload, it must inform the client protocol by sending a
`BP-PayloadTransmitted.indication` service primitive (see [Service
BP-PayloadTransmitted](#bp-service-payload-transmitted)), with the
`protId` parameter being set to the value of the `protocolId` field of
the variable `clientProtocol`. If no client protocol has a payload
ready, then the BP does not prepare a beacon. 

If no client payloads are being generated, then the BP will not send a
beacon. Otherwise, the bearer payload will be constructed by first
creating the `BPHeader` and then appending client protocol payloads as
long as space is available and the number of appended client protocol
payloads does not exceed 255. The `BPHeader` is generated as follows:
- First increment the variable `bpSequenceNumber` modulo 2**32.
- Set the `magicNo` field to `0x497E`.
- Set the `senderId` field to the node identifier of the sending node.
- Set the `version` field to `0x01`.
- Set the `numPayloads` field to the number of appended client
  protocol payloads.
- Set the `seqno` field to the current value of `bpSequenceNumber`.



## Receive Path

When the UWB hands over the bearer payload of a received beacon to the
BP, it first parses the `BPHeader` and then parses the contained
payload blocks sequentially. When parsing the `BPHeader`, the
following sanity checks are performed:
- The `magicNo` field is equal to `0x497E`.
- The `version` field is equal to `0x01`.
- The `numPayloads` field is not zero.

If any of these conditions is not satisfied, the received bearer
payload is discarded without further processing. It is at the
discretion of an implemention whether and how the `seqno` field of the
received `BPHeader` is processed. Its main intention is to allow the
receiver to estimate the beacon loss rate towards the sender of the
received beacon.


Recall that each payload block consists
of a header of type `BPPayloadBlockHeader` followed by the actual
payload as a sequence of bytes, without any gap (see Section [Packet
Format](#bp-packet-format)). We initialize a variable `index` to zero,
where generally `index` refers to the byte position in the bearer
payload at which the next payload block starts.

In the following, we refer to the payload block starting at index
`index` as `pblock`, to its header as `pblock.header` (of type
`BPPayloadBlockHeader`) and the actual payload data as
`pblock.payload`. The receiving BP performs at least the following
steps:

~~~
1.     let index      =  0
           bearerlen  =  length of overall bearer payload
		   transId    =  node identifier of node sending the beacon
2.     If (transId == own node identifier) then
          stop processing, drop beacon
3.     While (index < bearerlen)
3.a.      let protId        = pblock.header.protocolId
              plength       = pblock.header.length
		      blocklength   = plength + sizeof(BPPayloadBlockHeader)
3.b.      index = index + blocklength
3.c.      If (currentClientProtocols.lProtocolExists(protId) == true) then
             send BP-ReceivePayload.indication primitive prim to client protocol, with
		         prim.protId  = protId
				 prim.length  = plength
				 prim.value   = pblock.payload
~~~


