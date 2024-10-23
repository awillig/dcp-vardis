---

# The VarDis (Variable Dissemination) Protocol {#chap-vardis}

This chapter describes the Variable Dissemination (VarDis) protocol.

## Purpose

On the highest level, VarDis is concerned with the maintenance of a
**real-time database**, which is simply defined to be a collection of
**variables** and is not in any way related to be confused with
relational or non-relational database management systems. A variable
to VarDis is simply a contiguous block of bytes of given length
without any further discernible structure. The variable concept of
VarDis supports the four key operations of create, read, update and
delete (CRUD). Each variable is created by exactly one node, and that
same node is the only node that is allowed to update/modify or delete
the variable -- we frequently refer to this node as the **producer**
of that variable. All other nodes are only allowed to read the current
variable value, we refer to them as the **consumers**. The identity of
the producer does not change during the lifetime of the variable.

A key responsibility of VarDis is to disseminate updates to the
variable as quickly and reliably as possible in the network and to
keep the real-time database in a consistent state across all
nodes. The key mechanism for that is that dissemination of variable
updates and other variable operations relies on piggybacking them onto
frequently transmitted beacons (with beacon transmission provided by
the underlying BP, for which VarDis is a client protocol) instead of
flooding updates separately.

The real-time database itself is dynamic in the sense that variables
can be created or removed at runtime.

The real-time database is not tied to any notion of persistency, and
there is no guarantee that any node will be able to track all updates
made to a variable. Furthermore, the moniker "real-time" should not be
interpreted strictly, as no guarantees on dissemination speed or
reliability will and can be given. It merely captures an aspiration to
be both fast and reliable.


## High-level Overview

A central role in the VarDis protocol plays the variable database or
real-time database (RTDB), which stores the values and accompanying
meta- and control data for the currently known variables. This
database can be modified and queried by higher-layer applications
through the *application service interface* (described in Section
[Application service
interface](#vardis-application-service-interface)) by submitting
*service requests* for creating, updating, and deleting variables, as
well as reading their values.

In response to any change made to the variable database (creating or
deleting a variable, updating its value), the VarDis transmit path
creates a BP payload made up of *instruction containers*, which in
turn contain one or more *instruction records* (or simply
*instructions*) of a given type, for example instructions to create,
update or delete a variable (there are further types of instructions).
For those instructions that include the value of a variable (create,
update instructions), the transmit path will always use the latest
available value of that variable, as stored in the database.  In the
receive path, the VarDis instance receives instructions from
neighboured nodes, processes them and updates the variable database as
necessary. When any change to the variable database has been made in
response to received instructions, the VarDis transmit path will in
turn generate matching instructions and insert them into outgoing
payloads for further dissemination.

If there is sufficient space in a beacon, each node includes
*summaries* of the variables in its internal real-time database
into its beacons.  A summary for a variable includes its
variable identifier and its sequence numbers and is only sent to
immediate neighbours.

When a node `A` receives a variable summary from a neighbour, it first
checks the variable identifier. When `A` does not have that variable
in its local real-time database, it includes a special request
instruction in one of its next beacons. This request instruction
("create-request") carries the variable identifier of the missing
variable and any neighbour receiving this includes variable-creation
instructions in future beacons. This mechanism gives `A` a way to
learn the specification and current value of a variable it did not
previously have.

If on the other hand `A` does have the variable in its real-time
database, but in a strictly outdated version (as recognizable from the
sequence numbers), then `A` includes another special request
instruction in a future beacon ("update-request"). In this
update-request instruction, `A` includes the variable identifier and
the sequence number from its real-time database. Any neighbour hearing
this update-request will include variable update instructions in
future beacons, provided they have strictly more recent updates than
the requesting node.

It is important to note that VarDis does not guarantee that a consumer
node will receive every update instruction ever issued by the producer
--- if updates are generated too quickly, newer values can
"over-write" older ones during dissemination, as outgoing VarDis
instructions always only include the most recent value of a
variable. This can lead to "update gaps".



## Key Data Types and Definitions

### Basic Transmissible Data Types

We start by defining some basic data types which will be used as
building blocks for the data types describing instruction records,
instruction containers and are also used inside the real-time
database. When for any of these types no indication of the amount of
memory required to store a variable of that type in memory is given,
then this amount of memory can be set by the implementation. However,
it must be an integer multiple of one byte.

- The transmissible data type `VarIdT` (short for variable identifier)
  is an unsigned integer. Each variable needs to have a unique
  identifier, but uniqueness must be ensured by application
  developers, it is not strongly enforced by VarDis.

- The transmissible data type `VarLenT` (short for variable length) is
  an unsigned integer describing the length of a variables value in
  memory as a number of bytes. VarDis treats a variable as just a
  contiguous block of bytes of given length. Note that the length of
  (the values of) a variable is not fixed over time, at different
  times a variable can have values of different lengths. The variable
  length must not exceed a configurable maximum variable length
  (parameter `VARDISPAR_MAX_VALUE_LENGTH`, see [Configurable
  Parameters](#vardis-configurable-parameters)).

- The transmissible data type `VarRepCntT` is an unsigned integer,
  which specifies for a variable in how many distinct beacons a node
  should repeat information pertaining to that variable (create or
  delete operations, updates).

- The transmissible data type `VarSeqnoT` is an unsigned integer
  representing sequence numbers, with a width of $n$ bytes. Sequence
  numbers are used in a circular fashion. The sequence number space
  ranges from 0 to `SEQNO-MODULUS` minus one, where `SEQNO-MODULUS`
  equals $2^{8n}$. Each producer of a variable maintains a separate
  local sequence number for that variable. This sequence number is
  initialized upon creation of a variable and incremented upon every
  new update operation and included in the disseminated
  update. Sequence numbers are used by nodes to check whether they
  have the most recent value of a variable.

- The transmissible data type `VarValueT` is represented as a
  `MemBlockT` (see Section [Basic transmissible data
  types](#chap-datatypes-basic-transmissible)). It contains the value
  of a variable (its `length` field of type `VarLenT` specifies the
  number of bytes, and its `data` field contains the actual contiguous
  byte block).

- The transmissible type `VarSpecT` (short for variable specification)
  represents a record collecting the static  attributes describing a
  variable that must be known to all nodes in the network. All these
  attributes remain fixed throughout its lifetime. Furthermore, VarDis
  does not keep track of the type of a variable, it just treats the
  value of a variable as a block of bytes without further
  structure. Any interpretation of a variable (including its
  association with a datatype) is handled by the applications. A
  `VarSpecT` has the following fields:
	- `varId` of type `VarIdT`:  unique numerical variable identifier.
	- `prodId` of type `NodeIdentifierT`: the node identifier of
	  the variable producer.
	- `repCnt` of type `VarRepCntT`: its repetition count.
	- `descr` of type `StringT`: a human-readable description of the
	  variable.

### Data Types for Instruction Records

The following transmissible data types represent the available
instruction records. A number of such records can be stored in an
instruction container, and there are as many types of instruction
records as there are types of instruction containers.

- The transmissible data type `VarUpdateT` (short for variable update) is
  the instruction used to represent an update to the value of a
  variable. This instruction record is to be disseminated into the
  entire network and  includes the following fields:
  - `varId` of type `VarIdT`: identifier of the variable
  - `seqno` of type `VarSeqnoT`: a sequence number as managed by the
    producer, such that successive update instructions have distinct
    sequence numbers.
  - `value` of type `VarValueT` contains the value of the variable.

- The transmissible data type `VarCreateT` (short for variable
  creation) collects all the information the producer of a variable
  disseminates upon creation of a variable. This instruction record is
  to be disseminated into the entire network and contains the following
  fields:
  - `spec` of type `VarSpecT` contains all the metadata describing the
    variable.
  - `upd` of type `VarUpdateT` contains the initial value of the variable.

- The transmissible data type `VarDeleteT` (short for variable
  deletion) is the instruction generated by the producer of a variable
  when it wishes to delete the variable from the network. This
  instruction is to be disseminated into the entire network and
  contains the following fields:
  - `varId` of type `VarIdT`: identifier of the variable to be deleted.

- The transmissible data type `VarSummT` (short for variable summary)
  is used to let nodes include variable summaries in their beacons to
  let neighbours know what is the most recent update to a variable
  they have received (either from applications if it is the producer,
  or from other nodes if it is a consumer). This instruction record is
  only transmitted to immediate neighbours and includes the
  following fields:
  - `varId` of type `VarIdT`: identifier for a variable that the
    sending node has in its real-time database (i.e. an existing
    variable).
  - `seqno` of type `VarSeqnoT`: the seqno of the most recent variable
    update (of type `VarUpdateT`) that the sender has received.

- The transmissible data type `VarReqUpdateT` (short for request an
  variable update instruction) is used by a node to ask an immediate
  neighbour for a newer variable value (contained in an `VarUpdateT`
  instruction record) when it has realized it only has an outdated
  variable value (as determined from the variable sequence
  numbers). This instruction record has the same representation as
  `VarSummT` (i.e. it contains the fields `varId` of type `VarIdT` and
  `seqno` of type `VarSeqnoT`) and is only disseminated to immediate
  neighbours.

- The transmissible data type `VarReqCreateT` (short for request
  variable create instruction) is used by a node to ask an immediate
  neighbour to send the `VarCreateT` instruction record for this
  variable, after it has realized that others use a variable
  identifier that it has not yet heard about. This instruction record
  has the same representation as `VarDeleteT` (i.e. it only contains
  the `varId` field of type `VarIdT`) and is only disseminated
  to immediate neighbours.


### Instruction Containers {#vardis-definitions-instruction-containers}

The VarDis payload of a beacon packet is made up of one or more
**instruction containers** (IC), which hence are transmissible data
types. Each instruction container consists of an `ICHeaderT`, followed
by a list of instruction records of the same type, which depends on
the type of instruction container -- we refer to this list as the
`ICList`. Note that the individual instruction records of an
instruction container can have different lengths (e.g. `VarUpdateT`
records for different variables).

The `ICHeaderT` is a transmissible data type with the following
fields:

- The field `icType` (for instruction container type) is an unsigned
  integer of one byte width. It specifies the particular type of
  instruction container being considered. The allowed values are:
    - `icType = 1` for `ICTYPE-SUMMARIES`
    - `icType = 2` for `ICTYPE-UPDATES`
    - `icType = 3` for `ICTYPE-REQUEST-VARUPDATES`
    - `icType = 4` for `ICTYPE-REQUEST-VARCREATES`
    - `icType = 5` for `ICTYPE-CREATE-VARIABLES`
    - `icType = 6` for `ICTYPE-DELETE-VARIABLES`

- The field `icNumRecords` is an unsigned integer of one byte
  width. specifies the number of instruction records in this
  instruction container. Note that each instruction container must
  contain at least one instruction record.


A VarDis payload transmitted in a beacon can contain one or more of
each of the following types of instruction containers, as long as
their combined size fits within the allowed payload size
(cf. parameter `VARDISPAR_MAX_PAYLOAD_SIZE`, see Section [Configurable
Parameters](#vardis-configurable-parameters)).

In the following we describe the key instruction containers used by
VarDis.


#### `ICTYPE-SUMMARIES` (`icType` = 1)

The `ICList` is a list of `VarSummT` instruction records, where each
`VarSummT` instruction includes a variable identifier (of type
`VarIdT`) and a sequence number (of type `VarSeq`), representing the
latest sequence number received for that variable. A neighbour
receiving this instruction container can compare the received summaries
with the contents of its own real-time database and check whether it
misses any variables, only has them in outdated versions, or has
itself newer versions than the sending neighbour (and sends them to
the neighbour).


#### `ICTYPE-UPDATES` (`icType` = 2)

The `ICList` is a list of `VarUpdateT` instruction records. In each such
record the transmitter includes the latest variable values and
sequence numbers it has stored in its real-time database (but it does
not necessarily include instruction records for all the variables in
the database, subject to packet size limitations). The intention is
that the receiver will itself repeat that update instruction as many
times in distinct future beacons as indicated by the `repCnt` field of
the corresponding variable specification (type `VarSpecT`), to further
propagate the update in the network. This is conditional on this
update being truly new to the receiver, i.e. having a sequence number
strictly larger than the sequence number it previously had stored for
the same variable.


#### `ICTYPE-REQUEST-VARUPDATES` (`icType` = 3)

The `ICList` is a list of `VarReqUpdateT` instruction records, where
each such record contains a variable identifier (of type `VarIdT`) and
a sequence number (of type `VarSeqnoT`). The intention is that the
sending node requests any / all of its neighbours to send `VarUpdateT`
instruction records for the requested `VarIdT` some time in the
future, in as many distinct future beacons as given by the `repCnt`
field of the corresponding variable specification (`VarSpecT`),
provided that the neighbours have strictly more recent versions of the
requested variables than indicated by the included sequence number.


#### `ICTYPE-REQUEST-VARCREATES` (`icType` = 4)

The `ICList` is a list of `VarReqCreateT` instruction records, where
each such record is simply a `VarIdT` value. The intention is that the
sending node requests any / all of its neighbours to send `VarCreateT`
instruction records for each of the requested variables (each such
`VarCreateT` instruction record combines a `VarSpecT` field and a
`VarUpdateT` field), which they will embed into `ICTYPE-CREATE-VARIABLES`
instruction containers some time in the future, in as many distinct
beacons as indicated by the `repCnt` parameter of the corresponding
variable specifications (of type `VarSpecT`). The variable value will
be the most recent value that the neighbour has received.


#### `ICTYPE-CREATE-VARIABLES` (`icType` = 5)

The `ICList` is a list of `VarCreateT` instruction records, where each
such record contains a variable specification (of type `VarSpecT`) and
the current variable value and sequence numbers (combined in type
`VarUpdateT`). The intention is that the sending node notifies its
neighbour that new variables (together with their initial values) have
been added to the real-time database, and for the neighbour to further
repeat the `VarCreateT` instruction records in as many distinct future
beacons as indicated in the `repCnt` field of the variable
specification (`VarSpecT`).


#### `ICTYPE-DELETE-VARIABLES` (`icType` = 6)

The `ICList` is a list of `VarDeleteT` instruction records, where each
such record is a variable identifier (of type `VarIdT`). The intention
is that the sending node notifies the neighbour that the given
variables are to be deleted from the real-time database, and for the
neighbour to further repeat the `VarIdT` values in as many distinct
future beacons as indicated in the `repCnt` field of the variable
specification (`VarSpecT`).


#### Global vs. Local Dissemination

The three instruction containers `ICTYPE-SUMMARIES`,
`ICTYPE-REQUEST-VARUPDATES` and `ICTYPE-REQUEST-VARCREATES` have
single-hop scope, i.e. the sender wishes to reach only its immediate
neighbours. These instruction containers are not included by their
receivers in their own beacons for further dissemination.

In contrast, the `ICTYPE-UPDATES`, `ICTYPE-CREATE-VARIABLES` and
`ICTYPE-DELETE-VARIABLES` instruction containers have global scope. In
particular, when a receiver recognizes that the instruction records
contained in any of these instruction containers contain truly new
data (either a new variable or a truly newer value), then it will in
turn include the received instructions (of type `VarUpdateT`,
`VarCreateT` or `VarDeleteT`) in as many distinct future beacons as
indicated by the `repCnt` parameter of the corresponding variable
specification (`VarSpecT`), to help with further propagation in the
network.


### Real-Time Database

The real-time database is a collection of variables, with values of
type `VarIdT`'s used as a key. For each currently known variable
identifier there exists a database entry of the non-transmissible type
`DBEntry`, containing the following fields:

- `spec` of type `VarSpecT`: contains the variable specification
  for the variable, as received in an `ICTYPE-CREATE-VARIABLES`
  instruction container. The `VarSpecT` includes the identifier of the
  variable (of type `VarIdT`).
- `value` of type `VarValueT` contains the most recently received
  value for this variable.
- `seqno` of type `VarSeqnoT`: the sequence number corresponding to the
  current value of the variable. The `value` and `seqno`
  fields have been received in the last valid `VarUpdateT` or
  `VarCreateT` instruction records received for that variable.
- `tStamp` of type `TimeStamp`: gives the last time (in local system
  time) that a valid `VarUpdateT` or `VarCreateT`  for this variable has
  been received and processed.
- A counter `countUpdate` of type `VarRepCntT`, indicating how many
  more times a `VarUpdateT` instruction record for this variable (with
  its most recent value) is to be included into future distinct
  beacons.
- A counter `countCreate` of type `VarRepCntT`, indicating how many
  more times a `VarCreateT` instruction record for this variable (with
  its most recent value) is to be included into future distinct beacons.
- A counter `countDelete` of type `VarRepCntT`, indicating how many
  more times a `VarDeleteT` instruction record for this variable
  (it's `VarIdT` field) is to be included into future distinct beacons.
- A flag `toBeDeleted` of type `Bool` to indicate whether the variable is
  marked for deletion (`true`) or not (`false`). A variable  deletion
  is not immediately carried out in the moment a `VarDeleteT`
  instruction record found in an `ICTYPE-DELETE-VARIABLES` instruction
  container is processed, but only after the deletion instruction has
  been repeated in beacons sufficiently often. This flag is to be
  initialized with `false`.

The real-time database supports three main operations:

- `RTDB.lookup()`, which takes as parameter a value of type `VarIdT`
  and either returns the unique database entry `ent` (of type
  `DBEntry`) for which `ent.spec.varId` equals the given `VarIdT`
  value, or an indication that no such entry exists.
- `RTDB.remove()`, which takes as parameter a value of type
  `VarIdT`. If the database contains an entry `ent` for which
  `ent.spec.varId` equals the given `VarIdT` value, then the entry
  is removed from the database, otherwise nothing happens.
- `RTDB.update()`, which takes as parameter a value of type
  `DBEntry`, referred to as `newent`. If the database contains no
  entry `ent` for which `ent.spec.varId == newent.spec.varId`, then
  `newent` is added to the database, otherwise the old entry `ent`
  (the unique entry sharing the variable identifier with `newent`) is
  replaced by `newent`.



### Queues {#vardis-definitions-queues}

To manipulate and specify the contents of the instruction containers
in future beacons, for each container type (see Section [Instruction
Containers](#vardis-definitions-instruction-containers)), the VarDis
instance maintains a number of variables of type `Queue<VarIdT>` (see
Section [Queues and Lists](#sec-queues-lists)). In addition to the
standard operations on queues, in VarDis we make use of the following
additional operations:

- `qDropNonexistingDeleted()` drops all `VarIdT` values from the given
  queue for which either `RTDB.lookup()` indicates that the
  corresponding variable does not exist in the real-time database, or
  the database entry for the variable (of type `DBEntry`) has the
  `toBeDeleted` flag set to `true`.
- `qDropNonexisting()` drops all `VarIdT` values from the given queue
  for which `RTDB.lookup()` indicates that the corresponding variable
  does not exist in the real-time database.
- `qDropDeleted()` drops all `VarIdT` values from the given queue
  for which `RTDB.lookup()` indicates that the corresponding variable
  exist in the real-time database but has its `toBeDeleted` flag set
  to `true`.
- `qAdjoin()` checks whether the variable identifier given as
  parameter is already contained in the queue -- if so, the queue
  remains unchanged. If not, the variable identifier is appended to
  the queue.

A VarDis entity maintains the following queues at runtime, all of type
`Queue<VarIdT>`:

- `createQ`: contains `VarIdT` values for all the variables for
  which further repetitions of matching `VarCreateT` instruction records
  have to be included in future beacons (inside of
  `ICTYPE-CREATE-VARIABLES` instruction containers).
- `deleteQ`: contains `VarIdT` values for all the variables for
  which further repetitions of matching `VarDeleteT` instruction
  records have to be included into  future beacons (inside of
  `ICTYPE-DELETE-VARIABLES` instruction containers).
- `updateQ`: contains `VarIdT` values for all the variables for
  which further repetitions of matching `VarUpdateT` instruction records
  have to be included in future beacons (inside of `ICTYPE-UPDATES`
  instruction containers).
- `summaryQ`: contains `VarIdT` values for all the variables for
  which `VarSummT` instruction records have to be included in future
  beacons (inside of `ICTYPE-SUMMARIES` instruction containers).
- `reqUpdQ`: contains `VarIdT` values for all the variables for
  which `VarReqUpdateT` instruction records have to be included in
  future beacons (inside an `ICTYPE-REQUEST-VARUPDATES` instruction
  container), to request a `VarUpdateT` from neighboured nodes with a
  more recent variable value.
- `reqCreateQ`: contains `VarIdT` values for all the variables for
  which `VarReqCreateT` instruction records have to be included in
  future beacons (inside an `ICTYPE-REQUEST-VARCREATES` instruction
  container), to request a matching `VarCreateT` from neighboured
  nodes.



## Application Service Interface {#vardis-application-service-interface}

An application is mainly concerned with maintaining and accessing the
real-time database (RTDB), which is a collection of variables. The
interaction between an application and the real-time database happens
through the following pre-defined services. For each service we
specify a request primitive, which the application uses to request a
service, and a confirm primitive, through which the local VarDis
entity provides its response to the service request to the local
application. We furthermore sketch how the VarDis entity processes a
service request.

Whenever an application submits a service request primitive, VarDis
must check whether it is currently registered as a client protocol
with BP. If not, then VarDis must reject the service request
primitive, i.e. send back a confirm primitive with status code
`VARDIS-STATUS-INACTIVE` to the application.

Furthermore, the implementation of the VarDis application service
interface must support application multiplexing, i.e. it must be
possible to run multiple separate applications (or multiple instances
of the same application) at the same time on top of a VarDis instance,
and the VarDis instance must be able to return confirm primitives
always to the single application instance that submitted the
corresponding request primitive.



### Describing Database Contents {#vardis-service-database-contents}

#### Service `RTDB-DescribeDatabase` {#vardis-service-database-contents-describe-db}

The application issues a `RTDB-DescribeDatabase.request`
primitive. This primitive carries no further parameters. The intention
is that the application is being provided with a list of `VarSpecT`
records for all variables currently in the RTDB.

The VarDis entity responds with a `RTDB-DescribeDatabase.confirm`
primitive. If the global variable `vardisActive` has value `false`,
then this confirm primitive contains status code
`VARDIS-STATUS-INACTIVE`. Otherwise, the confirm primitive will carry
status code `VARDIS-STATUS-OK`, and will carry as further data a list
of the `VarSpecT` records of all variables currently in the real-time
database, including those that have their `toBeDeleted` flag set in
their respective `DBEntry` records. Implementations can choose to add
additional fields from the `DBEntry` record of a variable, for example
the `toBeDeleted` field or its timestamp field `tStamp`.


#### Service `RTDB-DescribeVariable`

The application issues a `RTDB-DescribeVariable.request` primitive,
which carries a value of type `VarIdT` as a parameter. The intention is
that the application is being provided with the full entry of the RTDB
for that variable (of type `DBEntry`), or with a signal that the
variable does not exist.

The VarDis entity responds with a `RTDB-DescribeDatabase.confirm`
primitive. If the global variable `vardisActive` has value `false`,
then this confirm primitive contains status code
`VARDIS-STATUS-INACTIVE`. If the requested variable does not exist in
the real-time database, then the VarDis entity responds with a
`RTDB-DescribeVariable.confirm` primitive carrying the status code
`VARDIS-STATUS-VARIABLE-DOES-NOT-EXIST`. If the variable does exist,
the VarDis entity responds with the same primitive, but now carrying
the status code `VARDIS-STATUS-OK` and the `DBEntry` record for this
variable as parameters.


### CRUD Operations on Variables

#### Service `RTDB-Create`

An application wishes to create a variable and provides an initial
value. If successful, then the current node automatically becomes the
producer for this variable.

The application issues a `RTDB-Create.request` primitive, which
carries the following parameters:

- `spec` of type `VarSpecT`: the variable specification of the new
  variable. Its identifier (`spec.varId`) must be unique, i.e. there
  should be no active variable with the same `VarIdT` value
  currently in the real-time database.
- `value` of type `VarValueT`: The actual initial variable value

The `RTDB-Create.confirm` primitive is returned after the VarDis
entity has finished processing the request. As a parameter it only
includes a status code.

To process the request, the VarDis entity performs the following
steps:

~~~
1.     If (vardisActive == false) then
          stop processing, return status code VARDIS-STATUS-INACTIVE
2.     If (RTDB.lookup(spec.varId) == true) then
          stop processing, return status code VARDIS-STATUS-VARIABLE-EXISTS
3.     If (spec.descr.length > (VARDISPAR_MAX_DESCRIPTION_LENGTH)) then
          stop processing, return status code VARDIS-STATUS-VARIABLE-DESCRIPTION-TOO-LONG
4.     If (value.length > VARDISPAR_MAX_VALUE_LENGTH) then
          stop processing, return status code VARDIS-STATUS-VALUE-TOO-LONG
5.     If (value.length == 0) then
          stop processing, return status code VARDIS-STATUS-EMPTY-VALUE
6.     If (    (spec.repCnt <= 0)
            || (spec.repCnt > VARDISPAR_MAX_REPETITIONS)) then
		  stop processing, return status code VARDIS-STATUS-ILLEGAL-REPCOUNT

7.     Let newent : DBEntry with
           newent.spec         =  spec
		   newent.value        =  value
		   newent.seqno        =  0
		   newent.tStamp       =  current system time
		   newent.countUpdate  =  0
		   newent.countCreate  =  spec.repCnt
		   newent.countDelete  =  0
		   newent.toBeDeleted  =  False

8.     createQ.qRemove (spec.varId)
9.     deleteQ.qRemove (spec.varId)
10.    updateQ.qRemove (spec.varId)
11.	   summaryQ.qRemove (spec.varId)
12.	   reqUpdQ.qRemove (spec.varId)
13.	   reqCreateQ.qRemove (spec.varId)

14.    RTDB.update(newent)
15.    createQ.qAppend (spec.varId)
16.    summaryQ.qAppend (spec.varId)
17.    return status code VARDIS-STATUS-OK
~~~


A successful completion of this service request only means that the
variable has now been added to the real-time database **on the
producer node**. It may take an indefinite amount of time before the
new variable is known to any other node in the network.

The checking of uniqueness of a variable identifier is quite
limited. We can reliably discover the case where a variable is to be
created twice on the same producer node, but there are other
pathological cases an aspiring producer cannot immediately discover,
and the protocol makes no attempt to prevent, resolve or report such a
situation. One example of such a situation is when two nodes at far
away ends of a large multi-hop network want to create the same
variable at the same time. None of the two nodes will be aware of the
other one's efforts, and quite likely it will be some intermediate
nodes noticing a `VarCreateT` instruction record for an already
existing variable (and coming from a different producer node).



#### Service `RTDB-Delete` {#vardis-service-database-contents-delete-var}

An application wishes to delete a variable. To be effective the
deletion service request must be issued on the producer node, other
nodes cannot delete a variable.

To request deletion of a variable, an application uses the
`RTDB-Delete.request` primitive, which carries as a parameter the
`VarIdT` value of the variable to be deleted, referred to as `varId`
below.

The `RTDB-Delete.confirm` primitive is returned after the VarDis
entity has finished processing the request. As a parameter it only
carries a status code.

To process the request, the VarDis entity performs the following
steps:

~~~
1.     If (vardisActive == false) then
          stop processing, return status code VARDIS-STATUS-INACTIVE
2.     If (RTDB.lookup(varId) == false) then
           stop processing, return status code VARDIS-STATUS-VARIABLE-DOES-NOT-EXIST
3.     Let ent = RTDB.lookup(varId)
4.     If (ent.spec.prodId != ownNodeIdentifier) then
           stop processing, return status code VARDIS-STATUS-NOT-PRODUCER
5.     If (ent.toBeDeleted == true) then
           stop processing, return status code VARDIS-STATUS-VARIABLE-BEING-DELETED
6.     deleteQ.qAppend(varId)
7.     createQ.qRemove(varId)
8.     summaryQ.qRemove(varId)
9.     updateQ.qRemove(varId)
10.    reqUpdQ.qRemove(varId)
11.    reqCreateQ.qRemove(varId)
12.    ent.toBeDeleted   =  True
13.    ent.countDelete   =  ent.spec.repCnt
14.    ent.countCreate   =  0
15.    ent.countUpdate   =  0
16     RTDB.update(ent)
17.    return status code VARDIS-STATUS-OK
~~~

A successful completion of this primitive only means that now the
deletion process for the chosen variable has started **on the producer
node**. It takes an unspecified amount of time before the variable is
actually deleted from the real-time databases of all nodes (including
the producer node). To check for deletion on the producer node, an
application can issue the `RTDB-DescribeDatabase.request` service
primitive at a later point in time (see [Service
`RTDB-DescribeDatabase](#vardis-service-database-contents-describe-db)). There
is currently no way for the application on the producer to confirm
whether the variable has been deleted from every node in the network.


#### Service `RTDB-Update`

An application on the producer node wishes to write a new value to
the variable.

To achieve that, the application issues the `RTDB-Update.request`
primitive, which carries three parameters:

- `varId` of type `VarIdT`: the identifier of the variable to be
  updated 
- `value` of type `VarValueT`: The actual new variable value.

The `RTDB-Update.confirm` primitive is returned after the VarDis
entity has finished processing the request. As a parameter it only
carries a status code.

To process the request, the VarDis entity performs the following
steps:

~~~
1.     If (vardisActive == false) then
          stop processing, return status code VARDIS-STATUS-INACTIVE
2.     If (RTDB.lookup(varId) == false) then
           stop processing, return status code VARDIS-STATUS-VARIABLE-DOES-NOT-EXIST
3.     Let ent = RTDB.lookup(varId)
4.     If (ent.spec.prodId != ownNodeIdentifier) then
           stop processing, return status code VARDIS-STATUS-NOT-PRODUCER
5.     If (ent.toBeDeleted == True) then
           stop processing, return status code VARDIS-STATUS-VARIABLE-BEING-DELETED
6.     If (value.length > VARDISPAR_MAX_VALUE_LENGTH) then
          stop processing, return status code VARDIS-STATUS-VALUE-TOO-LONG
7.     If (value.length == 0) then
          stop processing, return status code VARDIS-STATUS-EMPTY-VALUE
8.     Increment ent.seqno modulo SEQNO-MODULUS
9.     Set ent.value        = value
		   ent.countUpdate  = ent.spec.repCnt
		   ent.tStamp       = current system time
10.    RTDB.update(ent)
11.    if (not updateQ.contains(varId))
        updateQ.qAppend(varId)
12.    return status code VARDIS-STATUS-OK
~~~


A successful completion of this primitive only means that the variable
has been updated **on the producer node**. It may take an indefinite
amount of time before it has been updated on any other node in the
network. Furthermore, there is no guarantee of completeness: if the
producer node updates a variable at time t~1~ with value v~1~ and a
short time later at time t~2~ it updates it with value v~2~, a given
consumer node may receive both update instruction records in the same
order and apply both updates to its RTDB, or it receives the second
update instruction record before the first one, in which case it will
ignore the first update instruction completely (as from the sequence
numbers it is recognized as outdated after the second update record
has been processed).


#### Service `RTDB-Read`

An application wishes to inquire the current value of a variable
stored in its local real-time database.

The application issues the `RTDB-Read.request` primitive, which
carries the `VarIdT` value of the variable as its only parameter.

The `RTDB-Read.confirm` primitive is returned after the VarDis entity
has finished processing the request. As a parameter it carries a
status code and, if the status code is `VARDIS-STATUS-OK`, also the
value of the variable (of type `VarValueT`) and the time stamp of
the last update of the variable.

To process the request, the VarDis entity performs the following
steps:

~~~
1.     If (vardisActive == false) then
          stop processing, return status code VARDIS-STATUS-INACTIVE
2.     If (RTDB.lookup(varId) == false) then
           stop processing, return status code VARDIS-STATUS-VARIABLE-DOES-NOT-EXIST
3.     Let ent = RTDB.lookup(varId)
4.     If (ent.toBeDeleted == True) then
           stop processing, return status code VARDIS-STATUS-VARIABLE-BEING-DELETED
5.     return status code VARDIS-STATUS-OK, ent.value, ent.tStamp
~~~



### Configurable Parameters {#vardis-configurable-parameters}

The configurable parameters of VarDis can be set by applications or
station management entities through a suitably defined
implementation-dependent management interface. Parameter changes are
assumed to take effect immediately and will apply to all subsequently
generated beacons and service invocations, but they are not applied
retroactively. For example, if the value of the parameter
`VARDISPAR_MAX_DESCRIPTION_LENGTH` is reduced, variables that have
been conforming at the time of their creation but are not anymore will
be retained in the real-time database.

Here we only specify mandatory parameters that any VarDis
implementation needs to support. These are:

- `VARDISPAR_MAX_VALUE_LENGTH` is the maximum allowable length of a
  variable value. Default value is 32. The value must be larger than
  zero and must not exceed the minimum of the following values:
    - the maximum value representable in the `VarLenT` data type.
    - `VARDISPAR_MAX_PAYLOAD_SIZE` - `ssizeof(ICHeaderT)`
  
- `VARDISPAR_MAX_DESCRIPTION_LENGTH` is the maximum length of a
  textual variable description (of type `StringT`) contained in a
  value of type `VarSpecT`. Default value is 32. The value must be
  larger than zero and must not exceed the value 
  `VARDISPAR_MAX_PAYLOAD_SIZE` - (`ssizeof(ICHeaderT) +  ssizeof(VarSpecT) + ssizeof(VarUpdateT) + VARDISPAR_MAX_VALUE_LENGTH`)),
  where `ssizeof(VarSpecT)` only includes the `length` component of
  the `descr` field and `ssizeof(varUpdT)` only includes the `length`
  component of the `value` field.

- `VARDISPAR_MAX_REPETITIONS` gives the maximally allowed number of
  repetitions for a variable update, variable creation or variable
  deletion that a node may include into its outgoing beacons. The
  value must be at least one.

- `VARDISPAR_MAX_PAYLOAD_SIZE` is the maximum allowable length of a
  payload generated by VarDis and handed over to the underlying BP for
  transmission. The value must be larger than zero, and must not
  exceed the value
  `BPPAR_MAXIMUM_PACKET_SIZE-(ssizeof(BPHeaderT)+ssizeof(BPPayloadHeaderT))`,
  see Sections [BP Configurable
  Parameters](#bp-interface-configurable-parameters) and [BP Data
  Types](#subsubsec-beaconing-protocol-data-types).

- `VARDISPAR_MAX_SUMMARIES` is the maximum number of variable
  summaries (of type `VarSummT`) to be included in a single payload in
  an `ICTYPE-SUMMARIES` instruction container. It is a non-negative
  integer, which must not exceed
  `(VARDISPAR_MAX_PAYLOAD_SIZE-ssizeof(ICHeaderT))/ssizeof(VarSummT)`.
  If this value is zero, then no `ICTYPE_SUMMARIES` instruction
  container is being created.

- `VARDISPAR_BUFFER_CHECK_PERIOD` specifies the period of time between
  checks of the number of payloads in the BP buffer for VarDis (see
  Section [Runtime](#vardis-runtime)). This value must be larger than
  zero.



## Payload Format and Payload Construction Process {#vardis-payload-format-construction}

We discuss how a node (the "current node") composes the VarDis payload
to be handed down for transmission to the underlying BP. The payload
is composed by appending instruction containers until either no further
containers are to be added, or the maximum allowed payload size
(parameter `VARDISPAR_MAX_PAYLOAD_SIZE`, see Section [Configuruable
Parameters](#vardis-configurable-parameters)) will be exceeded.

For each of the available types of instruction containers (see
[Instruction Containers](#vardis-definitions-instruction-containers))
one or more instances can be inserted into a payload (this is
implementation-dependent, restricting to one instance is
permissible). Each of the inserted instruction containers must contain
at least one matching instruction record -- instruction containers
with the `icNumRecords` field being zero will be treated as an error
by the receive path. Insertion is considered in the following order
and under the following conditions:

- First step: When `createQ.qIsEmpty()` indicates a non-empty queue,
  then generate an `ICTYPE-CREATE-VARIABLES` instruction container with
  as many distinct `VarCreateT` instruction records as are available
  and may fit into the VarDis payload.
- Second step: When `deleteQ.qIsEmpty()` indicates a non-empty queue
  and there is sufficient space left in the VarDis payload, then
  generate an `ICTYPE-DELETE-VARIABLES` instruction container with as
  many distinct `VarDeleteT` instruction records as are available and
  may fit into the remaining VarDis payload.
- Third step: When `reqCreateQ.qIsEmpty()` indicates a non-empty queue
  and there is sufficient space left in the VarDis payload, then
  generate an `ICTYPE-REQUEST-VARCREATES` instruction container with as
  many distinct `VarRecCreateT` instruction records as are available
  and may fit into the remaining VarDis payload.
- Fourth step: When `summaryQ.qIsEmpty()` indicates a non-empty queue,
  `VARDISPAR_MAX_SUMMARIE` is larger than zero and there is sufficient
  space left in the VarDis payload, then generate an
  `ICTYPE-SUMMARIES` instruction container with as many distinct
  `VarSummT` instruction records as are needed and may fit into the
  remaining VarDis payload, but not exceeding the value of the
  `VARDISPAR_MAX_SUMMARIES` parameter.
- Fifth step: When `updateQ.qIsEmpty()` indicates a non-empty queue
  and there is sufficient space left in the VarDis payload, then
  generate an `ICTYPE-UPDATES` instruction container with as many
  distinct `VarUpdateT` instruction records as are available and may
  fit into the remaining VarDis payload.
- Sixth step: When `reqUpdQ.qIsEmpty()` indicates a non-empty queue
  and there is sufficient space left in the VarDis payload, then
  generate an `ICTYPE-REQUEST-VARUPDATES` instruction container with
  as many distinct `VarReqUpdateT` instruction records pairs as are
  available and may fit into the remaining VarDis payload.

In these steps, the word `distinct` means: for different variable
identifiers (of type `VarIdT`).

When none of these steps produces an instruction container, then
VarDis will not generate a BP payload.

As described in Section [Instruction
Containers](#vardis-definitions-instruction-containers), each such
instruction container starts with an `ICHeaderT`, followed by a list
of one or more distinct instruction records specific to the type of
instruction container (the `ICList`). We next describe the process of
composing each of these types of instruction containers. We make use
of a pre-defined function `numberFittingRecords` which takes as
parameters one of the pre-defined VarDis runtime queues (see Section
[Queues](#vardis-definitions-queues)) and an indication of the type of
records, and calculates how many distinct records of the corresponding
record type can still fit into the remaining BP payload (also
accounting for the size of the `ICHeaderT` and for the maximum number
of instruction records that can fit into the instruction container,
which is equal to the maximum value representable in the
`icNumRecords` field of `ICHeaderT`). In this calculation, when `q` is
the queue under consideration (which has entries of type `VarIdT`),
each entry of `q` is considered at most once (entries are considered
in first-in-first-out order), and the function `numberFittingRecords`
knows how to calculate the serialized size of a record for a given
`VarIdT` (this size is calculated based on the current contents of the
RTDB for the given `VarIdT`).


### Composing the `ICTYPE-SUMMARIES` Container {#vardis-composing-ictype-summaries}

The process of composing the `ICTYPE-SUMMARIES` instruction container
includes the following steps:

~~~
1.     summaryQ.qDropNonexistingDeleted()
2.     If (    (summaryQ.qIsEmpty()) 
            || (no space to add ICHeaderT and one VarSummT)
			|| (VARDISPAR_MAX_SUMMARIES == 0)) then
           stop processing, return empty ICTYPE-SUMMARIES container
3.     Let numRecordsToAdd0 = numberFittingRecords(summaryQ,VarSummT)
           numRecordsToAdd  = min(numRecordsToAdd0, VARDISPAR_MAX_SUMMARIES)
           ieHdr : ICHeaderT
4.     ieHdr.icType        = ICTYPE-SUMMARIES
       ieHdr.icNumRecords  = numRecordsToAdd
5.     Add ieHdr to ICTYPE-SUMMARIES container
6.     for (i=0 ; i<numRecordsToAdd ; i++)
           Let nextVarId = summaryQ.qPeek()
		       nextVar   = RTDB.lookup (nextVarId)
		   summaryQ.qTake()
		   summaryQ.qAppend(nextVarId)
		   Add VarSummT using (nextVarId, nextVar.seqno) to ICTYPE-SUMMARIES container
7.     return the ICTYPE-SUMMARIES container
~~~


The preceding procedure adds an `ICHeaderT` and `VarSummT` records to
the `ICTYPE-SUMMARIES` container as long as:

 - the number of added records does not exceed
   `VARDISPAR_MAX_SUMMARIES`, and
 - the records fit into the remaining BP payload, and
 - no record is added twice.
 
A record to be added is taken from the head of `summaryQ` and then
appended to it. At the start, we remove those `VarIdT` values from
`summaryQ` for which either the underlying variable does not exist in
the real-time database or has been marked for deletion.


### Composing the `ICTYPE-CREATE-VARIABLES` Container

The process of composing the `ICTYPE-CREATE-VARIABLES` instruction
container includes the following steps:

~~~
1.     createQ.qDropNonexistingDeleted()
2.     If (    (createQ.qIsEmpty())
            || (no space to add ICHeaderT and VarCreateT for createQ.qPeek())) then
           stop processing, return empty ICTYPE-CREATE-VARIABLES container
3.     Let numRecordsToAdd = numberFittingRecords(createQ, VarCreateT)
           ieHdr : ICHeaderT
4.     ieHdr.icType        = ICTYPE-CREATE-VARIABLES
       ieHdr.icNumRecords  = numRecordsToAdd
5.     Add ieHdr to ICTYPE-CREATE-VARIABLES container
6.     for (i=0 ; i<numRecordsToAdd ; i++)
           Let nextVarId = createQ.qPeek()
		       nextVar   = RTDB.lookup(nextVarId)
		   createQ.qTake()
		   decrement nextVar.countCreate
		   RTDB.update(nextVar)
		   Add VarCreateT using (nextVar.spec, nextVar.value) to ICTYPE-CREATE-VARIABLES
           if (nextVar.countCreate > 0)
		      createQ.qAppend(nextVarId)
7.     Return the constructed ICTYPE-CREATE-VARIABLES container
~~~


The preceding procedure adds an `ICHeaderT` and `VarCreateT` records
to the `ICTYPE-CREATE-VARIABLES` instruction container as long as:

 - the records fit into the BP payload, and
 - no record is added twice.

The repetition counter for each variable is decremented, and the
`VarIdT` is appended to `createQ` again if the repetition counter is
still larger than zero.


### Composing the `ICTYPE-UPDATES` Container

This follows exactly the same process as for the
`ICTYPE-CREATE-VARIABLES` instruction container, with the following
substitutions:

- Replace `createQ` by `updateQ`
- Replace `ICTYPE-CREATE-VARIABLES` by `ICTYPE-UPDATES`
- Replace `countCreate` by `countUpdate`
- Replace `VarCreateT` by `VarUpdateT`


### Composing the `ICTYPE-DELETE-VARIABLES` Container

The process of composing the `ICTYPE-DELETE-VARIABLES` instruction
container includes the following steps:

~~~
1.     deleteQ.qDropNonexisting()
2.     If (    (deleteQ.qIsEmpty())
            || (no space to add ICHeaderT and VarDeleteT for deleteQ.qPeek())) then
           stop processing, return empty ICTYPE-DELETE-VARIABLES container
3.     Let numRecordsToAdd = numberFittingRecords(deleteQ, VarDeleteT)
           ieHdr : ICHeaderT
4.     ieHdr.icType        = ICTYPE-DELETE-VARIABLES
       ieHdr.icNumRecords  = numRecordsToAdd
5.     Add ieHdr to ICTYPE-DELETE-VARIABLES container
6.     for (i=0 ; i<numRecordsToAdd ; i++)
           Let nextVarId = deleteQ.qPeek()
		       nextVar   = RTDB.lookup(nextVarId)
		   deleteQ.qTake()
		   decrement nextVar.countDelete
		   RTDB.update(nextVar)
		   Add VarDeleteT using nextVarId to ICTYPE-DELETE-VARIABLES
           if (nextVar.countDelete > 0)
		      deleteQ.qAppend(nextVarId)
		   else
		      RTDB.remove(nextVarId)
7.     Return the constructed ICTYPE-DELETE-VARIABLES container
~~~

The preceding procedure adds an `ICHeaderT` and `VarIdT` records to the
`ICTYPE-DELETE-VARIABLES` instruction container as long as:
 
 - the records fit into the BP payload, and
 - no record is added twice.

The repetition counter for each variable is decremented, and the
`VarIdT` is appended to `deleteQ` again if the repetition counter is
still larger than zero. Once the repetition counter has reached zero,
the variable is also deleted from the local RTDB.



### Composing the `ICTYPE-REQUEST-VARUPDATES` Container

The process of composing the `ICTYPE-REQUEST-VARUPDATES` instruction
container includes the following steps:

~~~
1.     reqUpdQ.qDropNonexistingDeleted()
2.     If (    (reqUpdQ.qIsEmpty())
            || (no space to add ICHeaderT and VarReqUpdateT for reqUpdQ.qPeek())) then
           stop processing, return empty ICTYPE-REQUEST-VARUPDATES container
3.     Let numRecordsToAdd = numberFittingRecords(reqUpdQ, VarSummT)
           ieHdr : ICHeaderT
4.     ieHdr.icType        = ICTYPE-REQUEST-VARUPDATES
       ieHdr.icNumRecords  = numRecordsToAdd
5.     Add ieHdr to ICTYPE-REQUEST-VARUPDATES container
6.     for (i=0 ; i<numRecordsToAdd ; i++)
           Let nextVarId = reqUpdQ.qPeek()
		       nextVar   = RTDB.lookup(nextVarId)
		   reqUpdQ.qTake()
		   Add VarSummT using (nextVarId, nextVar.seqno) to ICTYPE-REQUEST-VARUPDATES
7.     Return the constructed ICTYPE-REQUEST-VARUPDATES container
~~~

The preceding procedure adds an `ICHeaderT` and `VarIdT` records to the
`ICTYPE-REQUEST-VARUPDATES` instruction container as long as:

- the records fit into the BP payload, and
 - no record is added twice.

Note that a request for a VarUpdateT is only transmitted once.


### Composing the `ICTYPE-REQUEST-VARCREATES` Container

This follows exactly the same process as for the
`ICTYPE-REQUEST-VARUPDATES` instruction container, with the following
substitutions:

  - Replace `reqUpdQ` by `reqCreateQ`
  - Replace `ICTYPE-REQUEST-VARUPDATES` by `ICTYPE-REQUEST-VARCREATES`
  - Replace `VarSummT` by `VarIdT`
  - Replace `qDropNonexistingDeleted` by `qDropDeleted`


## Initialization, Runtime and Shutdown {#vardis-init-runtime-shutdown}

VarDis will have to perform certain actions at runtime besides
processing received packets, preparing beacon payloads or responding
to service requests from applications. 


### Non-Configurable Parameters and Global Variables


A VarDis instance will obtain the own node identifier (of type
`NodeIdentifierT`) during initialization and store it in a global
variable named `ownNodeIdentifier`, which will not be changed any
further during the lifetime of the VarDis instance.

The global variable `vardisActive` of type `Bool` indicates whether
the VarDis instance is currently active (`vardisActive=true`) or
not. If the instance is not active, then the VarDis instance will not
generate outgoing beacon payloads, it will not process incoming beacon
payloads, and it will not perform any action on a received application
service primitive (primitives `RTDBxxx.request`, see Section
[Application Service
Interface](#vardis-application-service-interface)) other than sending
back a corresponding confirmation primitive indicating the
condition. The variable `vardisActive` is only set to `true` once the
initialization has finished completely, and it is set to `false` once
the shutdown procedure starts.


### Initialization Steps {#vardis-init-runtime-shutdown-init}

As the very first step of the initialization process, the VarDis
entity will set the variable `vardisActive` to `false`.

During initialization, VarDis will need to register itself as a client
protocol with the underlying BP. To achieve this registration, VarDis
prepares a `BP-RegisterProtocol.request` service primitive (see
[Service
BP-RegisterProtocol](#bp-interface-service-bp-register-protocol)) with
the following parameters:

- `protId` is set to `BP_PROTID_VARDIS`.
- `name` is set to "VarDis -- Variable Dissemination Protocol Vx.y"
  where 'x' and 'y' refer to the present version of VarDis. Currently,
  the version number is "V1.1".
- `maxPayloadSize` is set to the value of the
  `VARDISPAR_MAX_PAYLOAD_SIZE` configuration parameter.
- `queueingMode` is set to `BP_QMODE_QUEUE_DROPTAIL`.
- `maxEntries` is set to an implementation-defined value.
- `allowMultiplePayloads` is set to `false`.

The `BP-RegisterProtocol.confirm` service primitive includes the
information about the own node identifier, which is stored in the
global variable `ownNodeIdentifier`.

Optionally, before registering with the BP, the VarDis entity can
choose to first submit a `BP-DeregisterProtocol.request` primitive
with `protId` field set to `BP_PROTID_VARDIS`, to cover the case that
a previously running VarDis instance has crashed and the current
instance is re-started, see Section [Recommended behaviour of client
protocols](#bp-recommended-behaviour-client-protocols).


Furthermore, VarDis performs the following additional actions during
initialization:

- The real-time database `RTDB` is initialized to be empty, i.e. not
  holding any variables.
  
- All queues (`createQ`, `deleteQ`, `updateQ`, `summaryQ`, `reqUpdQ`,
  `reqCreateQ`) are initialized to be empty.

- The own node identifier (of type `NodeIdentifierT`) is determined
  from the `BP-RegisterProtocol.confirm` primitive received in
  response to the own `BP-RegisterProtocol.request` primitive and
  stored in the global variable `ownNodeIdentifier`.
  
After all previous steps of the initialization have been successfully
completed, the variable `vardisActive` is set to `true`. 
  

### Runtime {#vardis-runtime}

One key responsibility of VarDis at runtime is the generation of
beacon payloads, which however is only done when the global variable
`vardisActive` has value `true`.


The default process by which VarDis generates payloads to transmit is
of a reactive nature: VarDis waits until the underlying BP has
indicated that a payload has just been included in a beacon (see
Section [Service
BP-PayloadTransmitted](#bp-service-payload-transmitted)), upon which
VarDis receives the `BP-PayloadTransmitted.indication` primitive. Note
that in this primitive the underlying BP indicates the point in time
it generates its next beacon, so that the VarDis implementation can
generate and hand over its own payload to BP ahead of time. In
response to this primitive, VarDis then generates the next payload by
following the process described in Section [Payload
Construction](#vardis-payload-format-construction), and submitting the
resulting payload (if any) to the BP by invoking the
`BP-TransmitPayload.request` primitive as follows:

- The `protId` parameter is set to `BP_PROTID_VARDIS`.
- The `length` parameter is set to the length of the new payload in bytes.
- The `payload` parameter is set to be the new payload.

However, an additional process is needed in case a
`BP-PayloadTransmitted.indication` is being lost or in order to
initiate the transmission of the very first payload. To achieve this,
VarDis checks periodically (with the period given by the
`VARDISPAR_BUFFER_CHECK_PERIOD` parameter, see Section [Configurable
Parameters](#vardis-configurable-parameters)) the number of payloads
that the BP currently holds for VarDis. It does so by invoking the
`BP-QueryNumberBufferedPayloads.request` service primitive with the
`protId` parameter set to `BP_PROTID_VARDIS`. If the value received in
the `BP-QueryNumberBufferedPayloads.confirm` primitive is valid (the
primitive has status code `BP-STATUS-OK`) and equal to zero, then
VarDis attempts to generate a beacon payload and, if there is any,
hands it over to the BP by submitting a `BP-TransmitPayload.request`
primitive as above.



### Shutdown

When the operation of VarDis is ending, VarDis performs at least the
following actions:

- As its very first step, it sets the variable `vardisActive` to
  `false`. The intended effect is that from this time onwards the
  VarDis instance will not generaty any new payloads for transmission,
  will not process any received payloads, and will also not respond to
  any application service request other than with an indication that
  VarDis is inactive.

- It de-registers itself as a client protocol with BP, by preparing a
  `BP-DeregisterProtocol.request` primitive with the `protId`
  parameter set to `BP_PROTID_VARDIS`.

- The real-time database `RTDB` is completely cleared.

- All queues (`createQ`, `deleteQ`, `updateQ`, `summaryQ`, `reqUpdQ`,
  `reqCreateQ`) are completely cleared.



## Transmit Path

The process of creating VarDis payloads and ensuring that they are
being generated at appropriate times is described in Section
[Runtime](#vardis-runtime). Note that the generation of payloads is
only allowed when the global variable `vardisActive` has value `true`.


## Receive Path

We discuss how a node (the "current node") processes the VarDis
payload of a beacon received from a neighbour (the "neighbour node")
and handed to the VarDis instance by the BP.

When the global variable `vardisActive` has value `false`, then the
received payload is discarded without any further processing.


### Processing Order of Instruction Containers {#vardis-receive-path-ic-processing-order}

The received VarDis payload contains a number of instruction
containers of the various types introduced before (Section
[Instruction
Containers](#vardis-definitions-instruction-containers)). As a very
first step, the current node sequentially extracts all instruction
containers from the payload, and checks for each container whether its
type is one of the types of instruction containers supported by the
implementation (see Section [Instruction
Containers](#vardis-definitions-instruction-containers)). If not, the
unknown instruction container and the remaining VarDis payload are
silently discarded, but the already completely and successfully
retrieved containers will still be processed. The receiver must be
prepared to find the instruction containers in any order within the
payload, and can also contain two or more containers of the same type.

The valid instruction containers included in the payload must be
processed in the following order:

- First step: process all `ICTYPE-CREATE-VARIABLES` containers.
- Second step: process all `ICTYPE-DELETE-VARIABLES` containers.
- Third step: process all `ICTYPE-UPDATES` containers.
- Fourth step: process all `ICTYPE-SUMMARIES`, `ICTYPE-REQUEST-VARCREATES`
  and `ICTYPE-REQUEST-VARUPDATES` containers in any convenient order.

The rationale for processing `ICTYPE-CREATE-VARIABLES` and
`ICTYPE-DELETE-VARIABLES` containers before `ICTYPE-UPDATES` is that a
producer node might in the same payload not only include the
instruction record for creating the variable, but also an update
instruction record for that same variable. However, as updates are
only allowed to be carried out for existing variables, the 'Create'
instruction should be processed first, otherwise the update will be
rejected. For a similar reason `ICTYPE-UPDATES` containers should be
processed after `ICTYPE-DELETE-VARIABLE` containers, as updates are
not carried out while a variable is in the process of deletion.

Furthermore, it is also helpful to process `ICTYPE-UPDATES` before
`ICTYPE-SUMMARIES`. To see why, imagine that a neighbour includes both
a summary and an update instruction record for the same variable in
the same payload, showing the same sequence number. Assume that the
receiver only possesses an older sequence number at the time of
reception. If the receiver were to process the summary instruction
record first, it would notice that it only has that variable in an
outdated version and might itself instigate transmission of a
`VarReqUpdateT` instruction record for that variable. But that would
be un-necessary if the `VarUpdateT` instruction is processed first.

The receive path extracts the instruction containers (header of type
`ICHeaderT`, followed by a list of instruction records of the same
type) out of the received payload in a sequential manner. When
processing an individual container or record, certain error conditions
may arise that prohibit further processing, including the following:

- A record has a larger length than there are bytes available in the
  as-yet-unprocessed part of the payload.
- The `icType` field of the `ICHeaderT` header contains an unknown
  value for the type of instruction container (see Section
  [Instruction Containers](#vardis-definitions-instruction-containers))
- The `icNumRecords` field of the `ICHeaderT` header contains the
  value zero.

In these cases the erroneous instruction container or record and the
remaining payload shall remain un-processed, but the effects of the
already correctly processed instruction containers and instruction
records are not being reversed.


### Processing `ICTYPE-CREATE-VARIABLES` Containers {#vardis-receive-path-ic-create-variables}

The contents of an `ICTYPE-CREATE-VARIABLES` instruction container is
a list of `VarCreateT` instruction records. For a specific such record
`rcvdVC` of type `VarCreateT` the receiving VarDis instance performs
the following steps:

~~~
1.     Let spec : VarSpecT    = rcvdVC.spec
           upd : VarUpdateT   = rcvdVC.upd
		   varId : VarIdT     = spec.varId
2.     If (RTDB.lookup (varId) == true) then
          stop processing
3.     If (spec.prodId == ownNodeIdentifier) then
          stop processing
4.     If (    (upd.value.length > VARDISPAR_MAX_VALUE_LENGTH) 
            || (spec.descr.length > VARDISPAR_MAX_DESCRIPTION_LENGTH))
	      stop processing
5.     Let newent : DBEntry with
           newent.spec         =  spec
		   newent.value        =  upd.value
		   newent.seqno        =  upd.seqno
		   newent.tStamp       =  current system time
		   newent.countUpdate  =  0
		   newent.countCreate  =  spec.repCnt
		   newent.countDelete  =  0
		   newent.toBeDeleted  =  False

6.     createQ.qRemove (spec.varId)
7.     deleteQ.qRemove (spec.varId)
8.     updateQ.qRemove (spec.varId)
9.	   summaryQ.qRemove (spec.varId)
10.	   reqUpdQ.qRemove (spec.varId)
11.	   reqCreateQ.qRemove (spec.varId)


12.    RTDB.update(newent)
13.    createQ.qAppend(varId)
14.    summaryQ.qAppend(varId)
~~~



### Processing `ICTYPE-DELETE-VARIABLES` Containers

The contents of an `ICTYPE-DELETE-VARIABLES` instruction container is
a list of `VarDeleteT` instruction records, each of which includes a
`VarIdT` value. For each such `VarIdT` value `varId` the receiving
VarDis instance performs the following steps:

~~~
1.     If (RTDB.lookup(varId) == false) then
           stop processing
2.     Let ent = RTDB.lookup(varId)
3.     If (ent.toBeDeleted == True) then
           stop processing
4.     If (ent.spec.prodId == ownNodeIdentifier) then
           stop processing
5.     ent.toBeDeleted = True
6.     ent.countUpdate = 0
7.     ent.countCreate = 0
8.     ent.countDelete = ent.spec.repCnt
9.     RTDB.update(ent)
10.    updateQ.qRemove(varId)
11.    createQ.qRemove(varId)
12.    reqUpdQ.qRemove(varId)
13.    reqCreateQ.qRemove(varId)
14.    summaryQ.qRemove(varId)
15.    deleteQ.qRemove(varId)
16.    deleteQ.qAppend(varId)
~~~





### Processing `ICTYPE-UPDATES` Containers {#vardis-receive-path-ic-updates}

The contents of an `ICTYPE-UPDATES` instruction container is a list of
`VarUpdateT` instruction records. For each included `VarUpdateT`
record `upd` the receiving VarDis instance performs the following
steps:

~~~
1.     Let varId = upd.varId
2.     If (RTDB.lookup(varId) == false) then
           If (reqCreateQ.qExists(varId) == false) then
               reqCreateQ.qAppend(varId)
		   stop processing
3.     Let ent = RTDB.lookup(varId)
4.     If (ent.toBeDeleted == True) then
           stop processing
5.     If (ent.spec.prodId == ownNodeIdentifier) then
           stop processing
6.     If (upd.value.length > VARDISPAR_MAX_VALUE_LENGTH)
	       stop processing
7.     If (upd.seqno == ent.seqno) then
           stop processing
8.     If (upd.seqno is strictly older than ent.seqno) then
           if (updateQ.qExists(varId) == false) then
		       updateQ.qAppend(varId)
			   ent.countUpdate = ent.spec.repCnt
			   RTDB.update(ent)
	       stop processing
9.     ent.value         =  upd.value
10.    ent.seqno         =  upd.seqno
11.    ent.tStamp        =  current system time
12.    ent.countUpdate   =  ent.spec.repCnt
13.    RTDB.update(ent)
14.    if (updateQ.qExists(varId) == false) then
           updateQ.qAppend(varId)
15.    reqUpdQ.qRemove(varId)
~~~



### Processing `ICTYPE-SUMMARIES` Containers

The contents of an `ICTYPE-SUMMARIES` instruction container is a list
of `VarSummT` instruction records. For each included `VarSummT` record
`summ` the receiving VarDis instance performs the following steps:

~~~
1.     Let varId      = summ.varId
           rcvdseqno  = summ.seqno
2.     If (RTDB.lookup(varId) == false) then
           If (reqCreateQ.qExists(varId) == false) then
               reqCreateQ.qAppend(varId)
		   stop processing
3.     Let ent = RTDB.lookup(varId)
4.     If (ent.toBeDeleted == true) then
           stop processing
5.     If (ent.spec.prodId == ownNodeIdentifier) then
           stop processing
6.     If (rcvdseqno == ent.seqno) then
           stop processing
7.     If (rcvdseqno is strictly older than ent.seqno) then
           if (updateQ.qExists(varId) == false) then
		       updateQ.qAppend(varId)
			   ent.countUpdate = ent.spec.repCnt
			   RTDB.update(ent)
	       stop processing
8.     If (reqUpdQ.qExists(varId) == false) then
           reqUpdQ.qAppend(varId)
~~~




### Processing `ICTYPE-REQUEST-VARUPDATES` Containers

The contents of an `ICTYPE-REQUEST-VARUPDATES` instruction container
is a list of `VarReqUpdateT` instruction records, which each contain a
variable identifier `varId` of type `VarIdT` and a sequence number
`seqno` of type `VarSeqnoT`. For each such `VarReqUpdateT` record the
receiving VarDis instance performs the following steps:

~~~
1.     If (RTDB.lookup(varId) == false) then
           If (reqCreateQ.qExists(varId) == false) then
               reqCreateQ.qAppend(varId)
           stop processing
2.     Let ent = RTDB.lookup(varId)
3.     If (ent.toBeDeleted == True) then
           stop processing
4.     If (ent.seqno is not strictly larger than seqno) then
           stop processing
5.     ent.countUpdate  =  ent.spec.repCnt
6.     RTDB.update(ent)
7.     If (updateQ.qExists(varId) == false) then
           updateQ.qAppend(varId)
~~~




### Processing `ICTYPE-REQUEST-VARCREATES` Containers

The contents of an `ICTYPE-REQUEST-VARCREATES` instruction container
is a list of `VarReqCreateT` instruction records, each of which
includes a variable identifier `varId` of type `VarIdT`. For each such
`VarReqCreateT` record the receiving VarDis instance performs the
following steps:

~~~
1.     If (RTDB.lookup(varId) == false) then
           If (reqCreateQ.qExists(varId) == false) then
               reqCreateQ.qAppend(varId)
           stop processing
2.     Let ent = RTDB.lookup(varId)
3.     If (ent.toBeDeleted == true) then
           stop processing
4.     ent.countCreate  =  ent.spec.repCnt
5.     RTDB.update(ent)
6.     If (createQ.qExists(varId) == false) then
           createQ.qAppend(varId)
~~~




