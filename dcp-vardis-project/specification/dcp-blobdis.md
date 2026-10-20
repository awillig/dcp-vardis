
[comment]: # Copyright (C) 2026 Andreas Willig, University of Canterbury
[comment]: # 
[comment]: # This work is licensed under a Creative Commons Attribution 
[comment]: # 4.0 International License (CC BY).

---

# The BlobDis (Binary large object Dissemination) Protocol {#chap-blobdis-protocol}

This chapter describes the Blob Dissemination (BlobDis) protocol, a BP
client protocol to reliably disseminate larger blocks of data (binary
large objects -- blobs) to nodes.


## Purpose

A blob (binary large object) refers to finite-sized block of
contiguous binary data with no discernible internal structure, which
belongs to a particular application.  The fundamental aim of the
BlobDis protocol is to disseminate a blob reliably (in-order and
complete), to preserve its integrity (making sure the copy in a
participant is identical to the original blob), and to achieve
reasonably efficient transfer of a blob from a **source node** (or
simply source) to a subset of nodes in the network, the so-called
**listening nodes** (or listeners), which are running the same
application. To achieve this, BlobDis uses the services of the
underlying BP protocol. We refer to the set of all involved nodes
(source and listeners) as the **participants** in the dissemination of
a blob. The set of participants can vary between different blob
transfers, depending on which node runs which applications.

A blob can be substantially larger than the maximum size of a beacon,
and therefore the participants will need to transmit the blobs over
multiple beacon payloads. A key but not immediately quantifiable
limitation of BlobDis is that blobs must be able to fit into the main
(or random access) memory of all participants, so that during the blob
transfer each participant is able to quickly access arbitrary parts of
the blob to re-transmit it upon request. Participants will need to
reserve the required amount of memory for the entire duration of the
blob transfer, including some extra time at the end where blob data is
kept in memory to satisfy retransmission requests from neighbored
participants.

The BlobDis protocol is built under the assumption that blobs can be
quite large, and their dissemination may take substantial amounts of
time due to data transmissions being restricted to periodic
beacons. This means that data corruption can occur not only while blob
data is in transit between participants (during which it is usually
protected by frame checksums offered by the UWB, see [The Underlying
Wireless Bearer](#sec-architecture-uwb)), but it can also occur while
the blob data resides in the memory of participants. A key feature of
BlobDis is that blob data is accompanied by suitable hashes, allowing
each participant to verify the integrity of the stored blob data and
request its re-transmission if needed.

The BlobDis protocol supports application multiplexing, each blob is
associated with exactly one application. Applications can be
dynamically registered and de-registered with a BlobDis instance
running in a node, using unique application identifiers. A node that
has no registered application for a given blob does not become a
participant, i.e. it does not engage in its dissemination. A node that
does have a registered application is always assumed to be a willing
participant, assuming it has sufficient memory available for the
blob. An example of a potential application is file dissemination,
where a blob might include file metadata like the file and path name,
creation timestamp or permissions, as well as the actual file data,
and the application might even manage the transfer of very large files
by partitioning it into multiple blobs and staging their dissemination
in a suitable manner. Note, however, that such application protocols
are outside the scope of BlobDis.

The BlobDis protocol supports parallel dissemination of multiple
blobs, from one or from multiple sources.


## High-level Overview

A key design target for BlobDis was to avoid any topological or
routing state in the dissemination of a blob, hence no spanning tree
or (connected) dominating set is constructed for the participants in a
blob transfer. This limitation requires participants to be be
sufficiently densely deployed in a network, but provides the most
flexibility in dealing with drone mobility.

As stated above, key requirements for BlobDis are reliability and
integrity of a blob transfer. Reliability refers to ensuring that
participants obtain the blob completely and in the right order, and
that no errors are introduced during transmission. This is achieved by
UWB frame checksums as well as by a retransmission mechanism within
BlobDis. Integrity is established by having the source node not only
transmit the actual blob data, but by pre-computing hashes for the
blob data and disseminating these hashes along with the blob
data. Listeners are required to periodically verify received blob data
against these hashes while the blob resides in memory, and request
retransmissions of both hashes and their covered blob data when this
verification fails.

A blob belongs to an application. It is transmitted by one node, and
received by all other nodes that can be reached and have the
corresponding application currently registered with BlobDis
(participants). It is assumed that all participants engage in the blob
transfer, assuming they have sufficient memory for the blob data.

After an application has handed over a new blob for transmission on
the source node, the local BlobDis entity first determines its
**metadata**, which includes amongst other things a suitably chosen
unique **blob identifier**, the length of the blob in bytes, the
application identifier, and other values. The blob identifiers are
chosen such that they are unique, even if an application on a source
requests two blob transmissions in short succession, or if it does
both before and after a crash-and-reboot event. The blob identifier
includes the node identifier of the transmitter node (of type
`NodeIdentifierT`, see Section [Basic Transmissible Data
Types](#chap-datatypes-basic-transmissible)) and a timestamp.
  
A blob is partitioned into contiguous and equal-sized **parts** (the
last part may be smaller). A part is characterized by its starting
offset within the blob and its size. The sender computes for each part
a separate **partial hash** (or phash), and disseminates these phashes
along with the actual blob data. A listener will only be able to
complete the reception of a blob if it has all blob data and all
phashes, and if all phashes indeed verify their associated part data.

A part in turn is sub-divided into an integral number of equal-sized
**chunks**, and the size of a full-sized part must be an integer
multiple of the chunk size. The last chunk of the last part can have a
smaller chunk size. A chunk is the basic unit of transmission of blob
data, the application must choose the chunk size such that at least
the chunk data and all relevant BlobDis and BP headers can be
accommodated in a BlobDis payload. The chunks within a blob are
numbered from zero onwards, so that the starting byte offset of a
chunk within the blob is given by its chunk number multiplied by its
chunk number. Similarly, parts are numbered as well and the starting
byte offset of a part is given by its part number times the part size
chosen for a blob.
  
To disseminate a blob, all participants transmit its metadata, all
phashes and all blob data. After a listener has received the metadata
of a blob, it allocates the memory required to store the blob and
starts to collect chunks and phashes. Once all the data and the phash
for a part are available, the integrity of the part is verified. If
this verification fails, both the phash and the part data are dropped
and a retransmission is requested. Once all blob data has been
verified, the listener hands over the blob data to the local
application for further processing. The completed blob will remain in
memory for a while so it can be used to serve retransmissions of blob
data or phashes to other listeners.

Any participant periodically checks whether it has all required blob
data and phashes, and if not it requests (re-)transmissions of these
from neighbors. These periodic checks also include integrity checks
for completely received parts for which also their phash is available,
and the request of retransmissions for those parts where the integrity
check fails.

In the BlobDis protocol neighboured participants interact with each
other through exchanging **instruction records**, which include
records for requesting and transmitting metadata, phashes and chunks.

## Key Data Types and Definitions

### Basic Transmissible Data Types {#blobdis-basic-transmissible-data-types}

We start by defining some transmissible data types used by BlobDis
(see Section [Naming and Typesetting
Conventions](#chap-datatypes-conventions)). The types for instruction
records are described in Section [Transmissible Data Types for
Instruction Records](#subsubsec-instruction-records).

#### `BlobOffsetT`

The type `BlobOffsetT` is an unsigned integer with an
implementation-defined length in bytes (which must be at least as many
bytes as used for the BP type `BPLengthT`, see Section [BP Data
Types](#subsubsec-beaconing-protocol-data-types). It is used to
represent:

- a byte index,
- a part number,
- a chunk number,
- or a size within the blob in bytes (e.g. the size of a part).

A part is identified by its part number, and it covers the byte
indices with the starting index given by the part number times the
part size, and the end index (including) given by the start index plus
the part size minus one (with obvious adjustments for the last part in
the blob).

Similarly, a chunk is identified by its chunk number and it covers the
byte indices with the starting index given by the chunk number times
the chunk size, and the end index (including) given by the start index
plus the chunk size minus one (with adjustments for the last chunk in
the blob).


#### `BlobAppIdentifierT`

An application is identified by a value of type `BlobAppIdentifierT`,
an unsigned integer with an implementation-dependent number of bytes
(at least two bytes). BlobDis assumes that different applications have
different application identifiers. This has to be ensured through
mechanisms outside the BlobDis protocol. BlobDis does not contain
mechanisms to ensure uniqueness or consistency of application
identifiers, other than a BlobDis instance refusing to register two
different applications under the same identifier.

Note that the value zero is referred to as the `nullAppIdentifier` and
indicates that no particular app is specified.

#### `BlobIdentifierT`

The type `BlobIdentifierT` aims to provide a globally unique
identifier for a blob, unique across time and nodes. A value of this
type has two fields:

- `nodeId` of type `NodeIdentifierT` contains the node identifier of
  the source node for a blob.

- `tstamp` of type `TimeStampT` is a timestamp representing the time
  at which the application on the source requested the dissemination
  of the blob. Implementations must ensure that different transmission
  requests on the same source generate different values for the
  `tstamp` field, for example by waiting for some time after the time
  stamp value has been set.

The types `NodeIdentifierT` and `TimeStampT` are discussed in Section
[Basic Transmissible Data Types](#chap-datatypes-basic-transmissible).

Two values of type `BlobIdentifierT` are considered equal when both
their `nodeId` and `tstamp` fields are equal. The internal structure
of a value of type `BlobIdentifierT` is irrelevant for the BlobDis
entity, only the ability to compare two values for equality, and the
ability to order two values are being used (the latter only for
efficient lookup in container data structures).


#### `BlobCrcT` and `BlobHashT`

There are two datatypes concerned with reliable transmission and
preservation of in-memory integrity. Both types can be used to
calculate a checksum or a hash value over a memory block of arbitrary
size. The type `BlobCrcT` represents a CRC checksum of either sixteen
or thirty-two bits of length (using the CRC-16-CCITT or CRC-32
polynomials, depending on the implementation. See [Cyclic Redundancy
Check /
Wikipedia](https://en.wikipedia.org/wiki/Cyclic_redundancy_check)), to
be used to protect metadata and similarly small structures. The type
`BlobHashT` contains two parts:

  - `hash`: an MD5 hash covering a part of a blob
  - `crc` of type `BlobCrcT` as a checksum for the `hash` value.

Furthermore, the following operations are defined on a value of type
`BlobHashT`:

  - `calc_crc(): BlobHashT -> BlobHashT`: calculates CRC value over
    the `hash` field and stores the result in the `crc` field, returns
    updated phash.
  - `verify_crc(): BlobHashT -> Bool`: calculates CRC value over
    the `hash` field and compares to `crc` value, and checks for
    equality.
  - `verify_hash(): BlobHashT -> BlobOffsetT -> BlobOffsetT ->
    MemRef -> Bool`: takes a blob hash, a starting offset, a length
    value, and a memory reference, calculates a fresh MD5 hash over
    the given memory contents (given by the starting offset and
    length, over the data available behind the memory reference), and
    returns whether the freshly calculated hash matches the `hash`
    value in the given `BlobHashT`.

Note that a `BlobHashT` value has its own CRC checksum. Without this,
in case of a disagreement between a phash and the part data referenced
by that phash both have to be dropped and their retransmission
requested. In the rare case where just the phash is wrong, having it
protected separately by a checksum allows to avoid requesting
retransmissions of part data.


#### `BlobMetadataT`

The type `BlobMetadataT` contains all relevant data that a participant
will need during the transfer of a blob, and no memory will be
allocated for blob data storage before valid metadata is
available. This type has the following fields:

  - `appId` of type `BlobAppIdentifierT` indicates the application
    this blob belongs to. It has to be a value different from
    `nullAppIdentifier`.
  - `blobId` of type `BlobIdentifierT` contains the blob identifier.
  - `length` of type `BlobOffsetT` gives the total length of the
    blob in bytes.
  - `chunkSize` of type `BlobOffsetT` indicates the length of one
    full-sized chunk. Its value must be strictly positive and it must
    not exceed the value of the `BLOBDISPAR_MAX_CHUNK_SIZE`
    configuration parameter, see Section [Configurable
    Parameters](#blobdis-configurable-parameters). If a node
    receives a metadata record with a larger `chunkSize` value, it
    does not become a participant in this blob transfer.
  - `partSize` of type `BlobOffsetT` indicates how large a part of a
    blob should be. Parts are protected by phashes. This value must be
    strictly positive, it must be an integer multiple of the
    `chunkSize` parameter, and it must not exceed the value of the
    `length` field. Implementations are allowed to introduce
    additional restrictions, e.g. allowing only multiples of a certain
    base size.
  - `keepaliveTime`, a 16-bit unsigned integer specifying a time
    interval in units of seconds. Once a node has a completed blob
    available (either because it is the sender of the blob or after
    complete reception of all data and verification of all parts) it
    keeps the blob in memory for a while to serve retransmission
    requests for phashes and blob data. The blob can be removed from
    memory when, after being completely available, no request for
    phashes or blob data have been received for at least the keepalive
    time. The keepalive time is strictly positive and given in units
    of seconds. For the case of receiving a blob, it is assumed that
    by this time the relevant application on the participant has
    processed the received blob.
  - `timeoutTime`, 16-bit unsigned integer specifying a time interval
    in units of seconds. This value indicates how much time a
    participant waits for receiving any new data for an incomplete
    blob before it is allowed to declare the blob transmission failed
    and notifying its local application (if any) accordingly. This
    value must be strictly positive.
  - `crc` of type `BlobCrcT` is a checksum calculated over the
    metadata (with the `crc` field assumed to be zero), which
    allows a participant to verify the integrity of the metadata while
    it resides in memory.

Furthermore, the following operations are defined on a value of type
`BlobMetadataT`:

  - `calc_crc()`: calculates CRC value over all fields except the
    `crc` field (which is assumed to have zero value) and stores the
    result in the `crc` field, returns updated metadata.
  - `verify_crc()`: calculates the CRC value over the all fields but
    the `crc` field (which enters the calculation as if it had zero
    value) and compares the result to the value of the `crc`
    field. Returns `true` if both values are equal, and `false`
    otherwise. A return value of `false` hence indicates a wrong
    checksum.


#### `BlobdisHeaderT`

A Blobdis payload starts with a single record of type
`BlobdisHeaderT`, followed by one or more blob containers, where each
blob container starts with a record of type `BlobHeaderT` (see Section
[`BlobHeaderT'](#blobdis-data-type-blob-header-t)), following by one
or more instruction records.

The type `BlobdisHeaderT` includes the following fields:

  - `majorVersion` is an eight-bit unsigned integer denoting the major
    version of the BlobDis implementation that the sender of the
    payload uses. Any payload processed by a receiving BlobDis entity
    must have a major version matching the major version of the 
    BlobDis implementation in the receiving node.
  - `minorVersion` is an eight-bit unsigned integer denoting the minor
    version of the BlobDis implementation that the sender of the
    payload uses. Implementations can react in an
    implementation-dependent way if the `minorVersion` field of an
    incoming BlobDis payload does not match the minor version of the
    local BlobDis entity.


#### `BlobHeaderT` {#blobdis-data-type-blob-header-t}

The data type `BlobHeaderT` is used in a BlobDis payload. The BlobDis
payload can include instructions for multiple blob transfers. The
instruction records for each blob transfer are grouped in **blob
containers**. Each such container starts with a header of type
`BlobHeaderT` which mainly identifies the current blob, followed by a
number of instruction records that pertain to this very blob. This
header has the following fields:

  - `blobId` of type `BlobIdentifierT` gives the common blob
    identifier that the following instruction records apply to.
  - `length` of type `BPLengthT` gives the total length in bytes for
    the remainder of the blob container (i.e. all instruction records
    subsumed by the same `BlobHeaderT`). This value must be a strictly
    positive number. An implementation needs to check that all
    contained instruction records are complete.


### Transmissible Data Types for Instruction Records {#subsubsec-instruction-records}

In the following we describe the different types of instruction
records. Note that for all instruction record types defined here it is
possible to infer the length of the complete instruction record after
reading the instruction record header.

#### Instruction Record Type `IRTypeT` {#subsubsubsec:instruction-record-types}

The instruction records for one specific blob follow a header of type
`BlobHeaderT`, which provides the common `blobId` value for all
related instruction records. Therefore, the instruction record types
themselves do not include a `blobId` field.

The transmissible data type `IRTypeT` (Instruction Record Type) is a
single byte indicating the type of an instruction record. The
following values are defined:

  - `IRTYPE-METADATA-REQUEST = 1`
  - `IRTYPE-METADATA-RESPONSE = 2`
  - `IRTYPE-CHUNK = 3`
  - `IRTYPE-GAP = 4`
  - `IRTYPE-PHASH-REQUEST = 5`
  - `IRTYPE-PHASH-RESPONSE = 6`
  - `IRTYPE-ABORT = 7`

#### `IRMetadataRequestT` {#subsubsubsec:instruction-record-metadata-request}

The transmissible data type `IRMetadataRequestT` is an instruction
record used by a participant to request transmission of the
`BlobMetadataT` record for a previously overheard unknown
`BlobIdentifierT` value. This instruction record includes the
following field:

  - `irType` of type `IRTypeT` contains the value `IRTYPE-METADATA-REQUEST`.


#### `IRMetadataResponseT` {#subsubsubsec:instruction-record-metadata-response}
	
The transmissible data type `IRMetadataResponseT` is used by a
participant to transmit blob metadata, either unsolicited
(e.g. after the sender got a fresh metadata record itself) or in
response to a previously received `IRMetadataRequestT` request. This
instruction record contains the following fields:

  - `irType` of type `IRTypeT` contains the value `IRTYPE-METADATA-RESPONSE`.
  - `metadata` of type `BlobMetadataT` contains a blob metadata
    record.

#### `IRChunkT` {#subsubsubsec:instruction-record-chunk}

The transmissible data type `IRChunkT` is used by a participant to
transmit a chunk of data (i.e. a contiguous block of data) of the
blob. This instruction record includes the following fields:

  - `irType` of type `IRTypeT` contains the value `IRTYPE-CHUNK`.
  - `chunkNumber` of type `BlobOffsetT` contains the chunk number of this
    chunk. The starting index within the blob data is obtained by
    multiplying the value of this field with the `chunkSize` field of
    the blob metadata.
  - `length` of type `BPLengthT` contains the size of the chunk. The
    chunk must lie entirely within the blob, and the value of the
    `length` field must be identical to the `chunkSize` field in the
    blob metadata (with obvious adjustments for the last chunk). This
    field must have a value strictly larger than zero.
  - `data` is a contiguous block of bytes of size `length`
	containing the actual chunk data.

#### `IRGapT` {#subsubsubsec:instruction-record-gap}

The transmissible data type `IRGapT` is used by a participant to let
other participants know about blocks of data it is still missing
(ranges of missing chunks, identified by their chunk numbers), and
asking them for transmission of suitable chunks covering some or all
of the gap. This instruction record includes the following fields:

  - `irType` of type `IRTypeT` contains the value `IRTYPE-GAP`.
  - `chunkNumber` of type `BlobOffsetT` contains the chunk number of
    the first chunk that the sender is missing. 
  - `size` of type `BlobOffsetT` contains the requested number of
    chunks.


#### `IRPHashRequestT` {#subsubsubsec:instruction-record-phash-request}

The transmissible data type `IRPHashRequestT` is used by a participant
to request a partial hash for a given part (of size `partSize`
indicated in the blob metadata, except for the last part). This
instruction record includes the following fields:

  - `irType` of type `IRTypeT` contains the value `IRTYPE-PHASH-REQUEST`.
  - `partNumber` of type `BlobOffsetT` identifies the part number of
	the blob part for which the partial hash is to be requested.


#### `IRPHashResponseT` {#subsubsubsec:instruction-record-phash-response}

The transmissible data type `IRPHashResponseT` is used by a
participant to send a partial hash for a given blob. The instruction
record includes the following fields:

  - `irType` of type `IRTypeT` contains the value `IRTYPE-PHASH-RESPONSE`.
  - `partNumber` of type `BlobOffsetT` gives the part number of the
    blob part covered by this partial hash.
  - `phash` of type `BlobHashT` is a partial hash value.


#### `IRAbortT` {#subsubsubsec:instruction-record-abort}

The transmissible data type `IRAbortT` is used by a source to abort
the dissemination of a blob sent by that source node. It is used to
alert other nodes that they can signal their relevant applications and
release any resources used by the blob. The instruction record has the
following fields:

  - `irType` of type `IRTypeT` contains the value `IRTYPE-ABORT`.


### Data Types for Runtime Data

We now describe some non-transmissible data types that a BlobDis
entity uses at runtime.

#### `BlobDisApplication` {#subsubsec-runtime-types-blobdis-application}

This data type contains data describing a BlobDis application. It
contains at minimum the following fields:

  - `appId` of type `BlobAppIdentifierT` contains the identifier of
    the application. This value has to differ from
    `nullAppIdentifier`.
  - `description` of type `String` is a human-readable short
    description of the application. There is no limit on the length of
    this description.

Implementations can add further fields, e.g. to maintain statistics
about the number of blobs in various states of processing that have so
far been observed for this application.


#### `MemRef` {#subsubsec-runtime-types-blobdis-memref}

This type is implementation- and operating-system dependent. It
represents a reference to the memory where the data of a blob is
stored in main memory.

Operations supported:

  - `bdmem_allocate()`: takes as parameter a size value of type
    `BlobOffsetT` and asks the OS to allocate a memory block of that
    size, returns updated `MemRef` value
  - `bdmem_free()`: takes as parameter a `MemRef` value (previously
    obtained with `bdmem_allocate()`) and returns the memory back to
    the OS
  - `bdmem_allocated()`: takes as parameter a `MemRef` value and
    returns a Boolean indicating whether memory has been allocated or
    not.
  - `bdmem_ref()`: takes as parameter a `MemRef` value and a byte offset
    of type `BlobOffsetT`, and provides read access to the blob memory
    at the given offset.


#### `BlobProcessingStatus` {#subsubsec-runtime-types-blobdis-blob-processing-status}

This is an unsigned integer value which describes the processing
status of a blob. We first consider the case of an incoming blob,
i.e. a blob for which the present node is a (potential) listener. As a
first step, the present node must receive the metadata for the blob --
before it has the metadata it will ignore any other instruction
records (including those containing blob data or partial hashes).
After it has the metadata and allocated the required memory for the
blob data, the listener will accept chunks and partial hashes. When
all partial hashes and chunks are available, the blob will be checked
for integrity. If parts of the blob are incorrect, their
retransmission will be requested (both the data chunks for the part
and the phash). If all parts of the blob are correct, the present node
keeps the blob in memory for a certain amount of time before removing
it entirely. We distinguish the following values for the blob
reception status:

  - `RXSTATUS-NO-METADATA = 1`: the present node has not yet received
    the metadata.
  - `RXSTATUS-INCOMPLETE = 2`: the present node has the metadata but does
    not yet have all phashes or all data chunks.
  - `RXSTATUS-COMPLETE = 3`: the present node has the metadata, all phashes,
    all data chunks and has successfully verified the integrity of all
    data parts.
  - `RXSTATUS-ABORTED = 4`: the blob reception has been aborted.

When we transmit a blob ourselves, we first send the metadata,
followed by the partial hashes, followed by all chunks. We distinguish
the following values for the blob processing status:

  - `TXSTATUS-SENDING = 5`: the present node is in the process of
    initial transmission of the metadata, phashes and blob data.
  - `TXSTATUS-COMPLETED = 6`: the present node has sent the metadata,
    all phashes, and all chunks but keeps the blob in memory for
    a while to respond to retransmission requests for chunks or
    phashes.
  - `TXSTATUS-ABORTED = 7`: This can for example happen when during
    the blob transfer the initially calculated phashes don't match the
    part data anymore, i.e. when either the phashes or the blob data
    have been corrupted while residing in memory.

#### `AbortReason` {#subsubsec-runtime-types-blobdis-abort-reason}

This is an unsigned integer value which describes why the BlobDis
entity aborted the dissemination of a blob (either as a listener or as
a source). We distinguish the following values:

  - `ABORT-REASON-TIMEOUT = 1`
  - `ABORT-REASON-SOURCE-ABORT = 2`


#### `Interval` and `IntervalList`

An interval (within the context of a given blob) is a pair of type
`(BlobOffsetT, BlobOffsetT)` where the first component represents a
starting index inside the blob, and the second component represents
the length of the interval. The interval must lie entirely within the
given blob.

An interval list is a collection of intervals. The intervals inside
the interval list must be maintained to be non-overlapping and
non-adjacent, we refer to this as being a *proper* interval list. An
`IntervalList` supports the following main operations:

  - `is_empty() : IntervalList --> Bool`: checks whether the interval
    list is empty or not.
  - `clear() : IntervalList --> IntervalList`: removes all entries from
    the interval list, it is empty afterwards.
  - `adjoin() : IntervalList --> Interval --> IntervalList`: takes a
    proper interval list and a new interval, and adds the new interval
    to the existing list, to return a proper updated list. This is
    carried out such that after completing this operation:
	   - The updated list is proper.
	   - The new interval is fully contained in one of the intervals
	     of the resulting interval list
	   - all previous intervals of the input list that did not overlap
	     with the new interval are preserved without modification
	   - any index that was in the interval list before adjoining the
	     new one remains in the nesulting interval list
	   - No index that was not previously in the interval list or is
	     contained in the new interval is contained in the resulting
	     interval list
  - `remove() : IntervalList --> Interval --> IntervalList`: takes an
    interval list and an interval and returns an updated interval
    list such that:
	   - The updated interval list is proper.
	   - all indices that are covered by the given interval are not
	     contained anymore in the resulting interval list
	   - all indices that were present in the given interval list and
	     were not contained in the given interval are present in the
	     resulting interval list.
	   - No index that was not previously in the interval list and not
	     included in the interval is contained in the resulting
	     interval list
  - `intersect() : IntervalList --> Interval -> IntervalList`:
    given a proper interval list and an interval, returns a new proper
    interval list containing intervals that form the intersection
    between the given interval list and the given interval, i.e.:
	   - the given interval list is not modified (and thus proper)
	   - the returned interval list is proper
	   - the returned interval list only covers indices which were
	     available in both the given interval list and the given
	     interval (and it covers all of these).
  - `contains(): IntervalList --> Interval --> Bool`: checks whether
    the given interval is fully contained in / covered by the interval
    list. Returns true when every index contained in the interval is
    also contained in the interval list.
  - `clear(): IntervalList --> IntervalList`: clears the given
    interval list and returns an updated interval list that is empty.


#### `BlobRuntimeData` {#blobdis-data-runtime-data}

A value of type `BlobRuntimeData` contains all the bookkeeping data
that BlobDis needs during the ongoing transfer of a blob (in either
transmit or receive direction). The runtime data contains at least the
following fields:

  - `blobId` of type `BlobIdentifierT`: blob identifier of this blob.
  - `status` of type `BlobProcessingStatus`: current processing status.
  - `metadata` of type `BlobMetadataT`: the metadata for the
    blob. The contents of this field is valid only when `status` has
    a value different from `RXSTATUS-NO-METADATA`
  - `memref` of type `MemRef`: memory reference for blob data
  - `phashes` of type `Dictionary<BlobOffsetT,BlobHashT>`: a
    dictionary of received valid phashes, where each phash of type
    `BlobHashT` covers a part of size `metadata.partSize` bytes
    starting at the index given by the `BlobOffsetT` value. The last
    blob covers possibly fewer than `metadata.partSize` bytes. The
    phashes in this dictionary refer either to part for which not yet
    all data has been received, or parts for which all data has been
    received and the phash matches the data.
  - `txq_chunk_new` of type `Queue<(BlobOffsetT,BlobOffsetT)>`: a
    queue of new (previously untransmitted) chunks to transmit (using
    `IRChunkT` instruction records), with each chunk being described
    by its offset `offs` of type `BlobOffsetT` within the blob and
    having length `len` of type `BlobOffsetT` (the length value must
    not exceed `BLOBDISPAR_MAX_CHUNK_SIZE`, see Section [Configurable
    Parameters](#blobdis-configurable-parameters)).
  - `txq_chunk_rtx` of type `Queue<(BlobOffsetT,BlobOffsetT)>`: a
    queue of retransmitted chunks to transmit, otherwise like
    `txq_chunk_new`.
  - `txq_gap` of type `IntervalList` is an interval list of
    gaps to request (using `IRGapT` instruction records), each gap
    being described by its starting offset and length.
  - `txq_phash_request` of type `Queue<BlobOffsetT>` is a queue of
    phash requests to transmit (using `IRPHashRequestT`
    instruction records), where each phash is being specified by its
    starting offset in the blob (must be an integer multiple of
    the `partSize` value contained in the `metadata` of this blob).
  - `txq_phash_response` of type `Queue<BlobOffsetT>`: a queue of
    phashes to transmit (using `IRPHashResponseT` instruction
    records). Each phash is specified by the `BlobOffsetT` value of
    its starting offset in the blob (must be an integer multiple of
    the `partSize` value contained in the `metadata` of this blob).
  - `txq_metadata_request` of type `Queue<Bool>`: when this queue is
    non-empty, a metadata request is to be transmitted (using a
    `IRMetadataRequestT` instruction record).
  - `txq_metadata_response` of type `Queue<Bool>`: when this queue is
    non-empty, a metadata response is sent (using a
    `IRMetadataResponseT` instruction record).
  - `txq_abort` of type `Queue<Bool>`: when this queue is non-empty,
    an abort signal is sent (using a `IRAbortT` instruction record).
  - `stat_chunks_sent` of unsigned integer type: counts the number of
    chunks that have been written into a BlobDis payload for
    transmission.
  - `gaps` of type `IntervalList`: a proper interval list
    containing all the intervals for which we do not yet have the
    data.
  - `blocks` of type `IntervalList`: a proper interval list containing
    all the intervals for we already have the data.
  - `tstamp_lastnew` of type `TimeStampT`: timestamp of the last time
    *new* data was received for this blob (i.e. data that this node
    did not have before and which actually led to a modification of
    `gaps`). This is relevant for managing the timeout time.
  - `tstamp_lastreq` of type `TimeStampT`: timestamp of the last time
    that a chunk or phash request for this blob (of type `IRGapT` or
    type `IRPHashRequestT`) was received for this blob (to be used
    with keepalive time).
  - `tstamp_creation` of type `TimeStampT`: timestamp of the time a
    blob becomes known first to the BlobDis entity and this runtime
    record is created.
  - `tstamp_metadata` of type `TimeStampT`: timestamp of the time the
    metadata for a blob becomes available.


### Runtime data

Global data:

 - `gBlacklist` of type `Dictionary<(BlobIdentifierT,
   BlobAppIdentifierT)>` is a dictionary of `BlobIdentifierT` values and their
   associated `BlobAppIdentifierT` values, representing
   the blob identifiers with which not to engage anymore as they
   have been aborted or eventually completed.
 - `gActiveBlobs` of type `Dictionary<BlobIdentifierT,
   BlobRuntimeData>`:  a dictionary containing the runtime data for
   BlobRuntimeData>all currently active blobs.
 - `gApplications` of type `Dictionary<BlobAppIdentifierT,
   BlobDisApplication>` is  a dictionary of currently registered 
   applications and their descriptions (and --
   implementation-dependent -- possibly other data such
   as statistics)
 - `gBlobdisActive` of type `Bool` is a flag indicating whether or
   not the BlobDis protocol is currently active. If it is inactive,
   the BlobDis entity does not participate in the dissemination of
   blobs, and does not process any services other than related to
   activation or deactivation.
 - `gOwnNodeIdentifier` of type `NodeIdentifierT` stores the own DCP
   node identifier as returned by the BP protocol after registration
   (See [Service
   `BP-RegisterProtocol`](#bp-interface-service-bp-register-protocol)).
  - `gMajorVersion` is an unsigned integer containing the major
    version of the present BlobDis entity.
  - `gMinorVersion` is an unsigned integer containing the minor
    version of the present BlobDis entity.

## Application Service Interface
### Preliminaries
In the following we describe the services offered by BlobDis to
applications. We describe these services abstractly in terms of
service primitives being exchanged. This specification does not
prescribe how exactly this exchange is implemented.

For each service primitive described in the following we specify its
minimum parameters. There can be further parameters, e.g. related to
the implementation of exchanging service primitives.


<!--THERE IS A SIMILAR EXPLANATION FOR VARDIS, OR THERE SHOULD
BE. FACTOR OUT INTO PRECEDING CHAPTER?-->

### Status codes

Usually, for each `.request` primitive for a service there is a
corresponding `.confirmation` primitive, which BlobDis uses to inform
the application about the outcome of a service request. All
`.confirmation` primitives contain a status code value. The following
status codes are possible:

  - `BLOBDIS-STATUS-OK`: the request is valid and blob transmission
    has commenced (or will commence soon). In this case the `blobId`
    value in the `.confirm` primitive contains a valid blob identifier
    for the new blob handed over by the corresponding `.request`
    primitive. For all other status values the `blobId` field does not
    contain a valid value.
  - `BLOBDIS-STATUS-APPLICATION-UNKNOWN`: the `appId` value in the
    request has not been previously registered with the BlobDis entity
    (using the `BDApp-Register` service).
  - `BLOBDIS-STATUS-APPLICATION-ALREADY-REGISTERED`: the `appId` value
    in a `DBApp-Register.request` primitive has already been
    registered previously.
  - `BLOBDIS-STATUS-ILLEGAL-BLOB-LENGTH`: the `length` value is zero.
  - `BLOBDIS-STATUS-ILLEGAL-BLOB-STATUS`: the blob has a processing
    status that is unknown (see Section
    [`BlobProcessingStatus`](#subsubsec-runtime-types-blobdis-blob-processing-status)).
  - `BLOBDIS-STATUS-INACTIVE`: the BlobDis entity is currently
    inactive.
  - `BLOBDIS-STATUS-BLOB-UNKNOWN`: the given blob identifier does not
    correspond to a known blob.
  - `BLOBDIS-STATUS-BLOB-BLACKLISTED`: an operation has been attempted
    on a blacklisted blob. (*OR RETURN BLOBDIS-STATUS-OK WHEN THIS IS ATTEMPTED??*)
  - `BLOBDIS-STATUS-ABORT-ON-TRANSMIT-BLOB`: an attempt to abort blob
    reception has been made on a blob we are transmitting ourselves.
  - `BLOBDIS-STATUS-ABORT-ON-RECEIVE-BLOB`: an attempt to abort blob
    transmission has been made on a blob we are receiving.
  - `BLOBDIS-STATUS-BLOB-ALREADY-ABORTED`: an attempt has been made to
    abort a blob transmission that has already been aborted.
  - `BLOBDIS-STATUS-ILLEGAL-PART-SIZE`: a blob transmission has been
    requested with an invalid part size parameter.
  - `BLOBDIS-STATUS-ILLEGAL-TIME-VALUE`: a blob transmission has been
    requested with an invalid time parameter (keepalive time or
    timeout time).
  - `BLOBDIS-STATUS-ILLEGAL-APP-IDENTIFIER`: an application has
    requested to register an invalid application identifier.

### Handling of Applications
#### Preparations
- A BlobDis instance onley engages in a blob transfer when the
  corresponding application is registered locally
- An application has a unique AppIdentifier, and they can be
  dynamically registered and de-registered


#### Service `BD-AppRegister` (Request, Confirm)

Applications can register themselves dynamically with the local
BlobDis entity, which generally means that the local BlobDis entity
will engage with the dissemination of blobs related to this
application (assuming BlobDis is active). BlobDis will not engage for
un-registered applications.

Applications invoke this service by submitting a
`BD-AppRegister.request` primitive, which carries at least the
following parameters:
   - `appId` of type `BlobAppIdentifierT` gives the unique application
     identifier of the application to be registered.
   - `descr` of type `String` contains a human-readable textual
     description of the application.
A `BD-AppRegister.confirm` primitive is returned after the BlobDis
entity completed processing this request. As a parameter it only
carries a status code.

To process the request, the BlobDis entity performs the following
steps:

~~~
1. If (not gBlobdisActive)
      stop processing
	  return status code BLOBDIS-STATUS-INACTIVE

2. If (gApplications.exists(appId))
      stop processing
	  return status code BLOBDIS-STATUS-APPLICATION-ALREADY-REGISTERED

3. If (appId == nullAppIdentifier)
      stop processing
	  return status code BLOBDIS-STATUS-ILLEGAL-APP-IDENTIFIER

4. Let newapp : BlobDisApplication with
       newapp.appId       =  appId
	   newapp.description =  descr

5. gApplications.adjoin (appId, newapp)

6. return status code BLOBDIS-STATUS-OK
~~~


#### Service `BD-AppDeregister` (Request, Confirm)

Applications can de-register with the local BlobDis entity. After
de-registration the BlobDis entity will not engage anymore in any blob
dissemination activity for this application. Any local data related to
ongoing blob transfers for this applications is removed, so the node
will not engage with these any further. No separation indication
primitives are generated when aborting these blob
transfers. (@@@@@Provide reference to transfer abort indication
primitives relevant here). 

Applications invoke this service by submitting a
`BD-AppDeregister.request` primitive, which carries at least the
following parameters:

  - `appId` of type `BlobAppIdentifierT` gives the unique identifier
    of the application to be de-registered.
	
A `BD-AppDeregister.confirm` primitive is returned after the BlobDis
entity completed processing this request. As a parameter it only
carries a status code.

To process the request, the BlobDis entity performs the following
steps:

~~~
1. If (not gBlobdisActive)
      stop processing
	  return status code BLOBDIS-STATUS-INACTIVE

2. If (not gApplications.contains(appId))
      stop processing
	  return status code BLOBDIS-STATUS-APPLICATION-UNKNOWN

3. Loop over all blobrt : BlobRuntimeData in gActiveBlobs:
      If blobrt.metadata.appId == appId then
	     blobrt.memref.bdmem_free()
		 gBlacklist.adjoin (blobrt.blobId)
		 gActiveBlobs.remove (blobrt.blobId)

4. gApplications.remove(appId)

5. return status code BLOBDIS-STATUS-OK
~~~

When removing the runtime data of a blob belonging to the given
application, all further dynamic data belonging to that blob (like
transmit queues) are to be deleted as well. Note that the blob
identifiers belonging to this application are added to the blacklist,
so that any further blob containers received for such a blob can be
effectively filtered out.


#### Service `BD-AppListRegistered` (Request, Confirm)

This service can be used by applications to query the list of
currently registered applications.

Applications invoke this service by submitting a
`BD-AppListRegistered.request` primitive, which carries no
parameters. A `BD-AppListRegistered.confirm` primitive is returned
after the BlobDis entity completed processing this request. As a
parameter it carries a status code, and the list of all currently
known applications (of type `BlobDisApplication`, carrying the
application identifier of type `BlobAppIdentifierT` and textual
description of type `String`).

To process the request, the BlobDis entity performs the following
steps:

~~~
1. If (not gBlobdisActive)
      stop processing
	  return status code BLOBDIS-STATUS-INACTIVE

2. return confirm primitive with contents of gApplications 
      and status code BLOBDIS-STATUS-OK
~~~

#### Service `BD-AppDescribe` (Request, Confirm)

This service can be used to retrieve information about a particular
application.

Applications invoke this service by submitting a
`BD-AppDescribe.request` primitive, which carries at least the
following parameters:

  - `appId` of type `BlobAppIdentifierT`: the application identifier
    for which to retrieve further information.

A `BD-AppDescribe.confirm` primitive is returned which includes a
status code. If the requested application can be found, then
additional information about the application is added: this includes
at minimum the `BlobDisApplication` data for this application, and
possibly any additional information that the implementation collects
for this application (e.g. statistics).

To process the request, the BlobDis entity performs the following
steps:

~~~
1. If (not gBlobdisActive)
      stop processing
	  return status code BLOBDIS-STATUS-INACTIVE

2. If (not gApplications.contains(appId))
      stop processing
	  return status code BLOBDIS-STATUS-APPLICATION-UNKNOWN

3. return confirm primitive 
      with contents of gApplications.lookup(appId) 
      and status code BLOBDIS-STATUS-OK
~~~


### Blob Reception Services
#### Preparations
#### Service `BD-RxNewBlob` (Indication)

By sending a `BD-RxNewBlob.indication` primitive,
the BlobDis entity informs a relevant application that it started
receiving a new blob belonging to this application. This indication
primitive is sent once the blob metadata for this blob has been
received for the first time.

This primitive includes at least the following parameters:

   - `blobId` of type `BlobIdentifierT` contains the blob identifier
     for the new blob
   - `metadata` of type `BlobMetadataT` contains the metadata for the
     new blob.
	 
This primitive is for informational purposes only.

Applications should **not** access any data belonging to the received
blob before reception has been completed (including verification) and
the application has received a `BD-RxCompletion.indication` primitive.

#### Service `BD-RxCompletion` (Indication)

By sending a `BD-RxCompletion.indication` primitive, the BlobDis
entity informs a relevant application that it has completed receiving
a blob, i.e. it has received and verified all the data. This
indication primitive is sent once the last missing data of the blob
has been received.

This primitive includes at least the following parameters:

   - `blobId` of type `BlobIdentifierT` contains the blob identifier
     for the completed blob
   - `metadata` of type `BlobMetadataT` contains the metadata for the
     completed blob.
   - `memref` of type `MemRef` contains the information about where in
     memory the blob data is stored.
	 
This primitive is for informational purposes only. After receiving
this the application needs to decide how to process the received blob
data. The application should complete processing within the keepalive
time for this blob, as beyond this time it is not guaranteed that the
blob will remain in memory. The application should only read blob data
from memory, releasing back the blob memory to the operating system is
done by the BlobDis entity.


#### Service `BD-RxRelease` (Indication)

By sending a `BD-RxRelease.indication` primitive, the BlobDis entity
informs a relevant application that it has stopped engaging any
further in the dissemination of a blob and has released the memory
used for the blob data. This indication primitive is sent once the
blob has been received completely and verified, and after not hearing
any retransmission requests for this blob for at least the keepalive
time.

This primitive includes at least the following parameters:

   - `blobId` of type `BlobIdentifierT` contains the blob identifier
     for the completed blob
   - `metadata` of type `BlobMetadataT` contains the metadata for the
     completed blob.
	 
This primitive is for informational purposes only. After receiving
this the application should not make any further use of the memory
reference provided earlier through a `BD-RxCompletion.indication`
primitive, as the BlobDis entity will have released the memory used
for the blob data.


#### Service `BD-RxAbort` (Request, Confirm, Indication)

An application can request aborting the reception of a particular
blob. The BlobDis entity will move the blob onto the blacklist and
release the memory used by the blob data. This service is only
effective for blobs that are being received, and not for blobs that
the present node is transmitting. Furthermore, the abort operation has
only local significance, no information about the local use of this
service is sent to other nodes.

Applications invoke this service by submitting a `BD-RxAbort.request`
primitive, which carries at least the following parameters:

  - `blobId` of type `BlobIdentifierT`: identifier of the blob for
    which reception is to be aborted.
  - (*DO WE NEED APPID AS A REQUEST PARAMETER?*)

A `BD-RxAbort.confirm` primitive is returned after the BlobDis entity
completed processing this request. Applications should not access the
memory used for storing blob data after sending the
`BD-RxAbort.request` primitive.(*REVIEW THAT LAST SENTENCE*)

To process the request, the BlobDis entity performs the following
steps:

~~~
1. If (not gBlobdisActive)
      stop processing
	  return status code BLOBDIS-STATUS-INACTIVE

2. If (gBlacklist.exists (blobId))
      stop processing
	  return status code BLOBDIS-STATUS-BLOB-BLACKLISTED

3. If (not gActiveBlobs.exists (blobId))
      stop processing
	  return status code BLOBDIS-STATUS-BLOB-UNKNOWN

4. Let blobrt = gActiveBlobs.lookup(blobId)
       appId  = blobrt.metadata.appId

5. If (blobrt.status not in RXSTATUS-*)
      stop processing
	  return status code BLOBDIS-STATUS-ILLEGAL-BLOB-STATUS

-- 6. blobrt.memref.bdmem_free()

-- 7. Release all other resources used by blob runtime data.
    -- Instead, clear all transmit queues except the one for aborts

-- 8. gActiveBlobs.remove (blobId)

9. gBlacklist.adjoin(blobId, appId)

10. send BD-RxAbort.indication to application indicated by appId

11. return status code BLOBDIS-STATUS-OK
~~~

(*!BY LEAVING BLOCK ACTIVE AND SIMULTANEOUSLY HAVING IT ON BLACKLIST,
NO CHUNKS FOR IT WILL BE PROCESSED ANYMORE, THE TIMEOUT TIMER WILL
EXPIRE AND WE CAN DO THE CLEANUP THEN, GIVING APPLICATIONS ENOUGH
TIME. NEED TO VERIFY THIS BEHAVIOUR AND RE-PHRASE ANY TEXT RELATED TO
WHETHER APPLICATIONS SHOULD ACCESS THE MEMORY AFTER ABORT*)

Besides the option of having an application trigger an abort on
receiving a blob, it can also be triggered by the BlobDis entity
itself, either by not receiving any new data for a certain amount of
time (timeout time), or by receiving an indication from the source
node of a blob that the source has aborted blob transfer. In these
cases, the BlobDis entity informs the relevant application by
submitting a `BD-RxAbort.indication` primitive, which carries at least
the following data:

  - `blobId` of type `BlobIdentifierT`: identifier for the aborted
    blob
  - `metadata` of type `BlobMetadataT`: metadata for the aborted blob
  - `reason` of type `AbortReason`

After the application received this primitive, it should not access
the memory used for storing the data of the aborted blob, as this may
have already been released by BlobDis. 


(*WE REALLY NEED TO GIVE THE APPLICATION SOME TIME AFTER IT GOT
COMPLETION INDICATION AND IS WORKING ON THE DATA*)


### Blob Transmission Services
#### Service `BD-TxTransmit` (Request, Confirm)

By issueing a `BD-TxTransmit.request` primitive, a registered
application requests the transmission of a new blob. The `.request`
service primitive carries the following parameters:

  - `appId` of type `BlobAppIdentifierT` identifies the application
    that this blob belongs to.
  - `length` of type `BlobOffsetT` specifies the length of the blob in
    bytes.
  - `memref` of type `MemRef` specifies the memory area in which the
    blob is stored. This memory has already been allocated by the
    application and will remain available and accessible throughout
    the entire transfer (the application should only release the
    memory once BlobDis has signaled the application that the blob
    transfer is truly over).
  - `keepaliveTime` is a 16-bit unsigned integer specifying the
    keepalive time for this blob in units of seconds. It must be
    strictly positive.
  - `timeoutTime` is a 16-bit unsigned integer specifying the
    timeout time for this blob in units of seconds. It must be
    strictly positive.
  - `partSize` of type `BlobOffsetT` is the size in bytes of one part
    of the blob. It must be strictly positive and not exceed the
    `length` parameter.

The BlobDis entity responds to this primitive by sending a
`BD-TxTransmit.confirm` primitive, which contains a status code
named `status` and a value `blobId` of type `BlobIdentifierT`. 

To process the request, the BlobDis entity performs the following
steps (the variable `txreq` contains the `BD-TxTransmit.request`
structure): 

~~~
1. If (not gBlobdisActive) then 
      stop processing
	  return status code BLOBDIS-STATUS-INACTIVE

2. If (not gApplications.exists (txreq.appId)) then
      stop processing
	  return status code BLOBDIS-STATUS-UNKNOWN-APPLICATION

3. If (txreq.length == 0) then
      stop processing
	  return status code BLOBDIS-STATUS-ILLEGAL-BLOB-LENGTH

4. If ((txreq.partSize == 0) or (txreq.partSize > txreq.length))
      stop processing
	  return status code BLOBDIS-STATUS-ILLEGAL-PART-SIZE

5. If ((txreq.keepaliveTime == 0) or (txreq.timeoutTime == 0))
      stop processing
	  return status code BLOBDIS-STATUS-ILLEGAL-TIME-VALUE

6. Let newmeta : BlobMetadataT with
       newmeta.appId          =  txreq.appId
   	   newmeta.blobId         =  tbd_generate_new_blobId (appId)
	   newmeta.length         =  txreq.length
	   newmeta.partSize       =  txreq.partSize
	   newmeta.keepaliveTime  =  txreq.keepaliveTime
	   newmeta.timeoutTime    =  txreq.timeoutTime
	   newmeta.calc_crc()

7. Let newrt : BlobRuntimeData with
       newrt.blobId     =  newmeta.blobId
	   newrt.status     =  TXSTATUS-SENDING
       newrt.metadata   =  newmeta
	   newrt.memref     =  txreq.memref
	   newrt.phashes    =  tbd_calculate_all_phashes (newmeta.partSize, txreq.memref, txreq.length)
	   newrt.txq_*.clear()
	   newrt.stat_chunks_sent  =  0
       newrt.gaps              =  <empty interval list>
	   newrt.blocks            =  <empty interval list>.adjoin ((0, txreq.length))
	   newrt.tstamp_lastnew    =  current system time
	   newrt.tstamp_lastreq    =  current system time
	   newrt.tstamp_creation   =  current system time

8. newrt.txq_metadata_response.qAppend (true)

9. for all phash : newrt.phashes do:
      newrt.txq_phash_response.qAppend (phash.offset)

10. for all chunks in the data part:
       newrt.txq_chunk_new.qAppend (chunk)

11. gActiveBlobs.adjoin (newrt.blobId, newrt)

12. return BLOBDIS-STATUS-OK
~~~


#### Service `BD-TxCompletion` (Indication)

By sending a `BDT-Completion.indication` primitive, the BlobDis entity
informs the relevant application that the last original chunk of the blob data
has now been transferred into a BlobDis payload for transmission, and
from here onward the BlobDis entity will engage with retransmissions
of chunks. This primitive includes at least the following parameters:

   - `blobId` of type `BlobIdentifierT` contains the blob identifier
     for the transmitted blob
   - `metadata` of type `BlobMetadataT` contains the metadata for the
     transmitted blob.

This primitive is for informational purposes only.
  
#### Service `BD-TxRelease` (Indication)

By sending a `BD-TxRelease.indication` primitive, the BlobDis entity
informs a relevant application that it has stopped all activities for disseminating the
blob to be transmitted. This happens after the keepalive time, which
i.e. the BlobDis entity has not received any retransmission request
for at least this amount of time. The BlobDis entity has released the
memory occupied by this blob and moved it from the list of active
blobs to the blacklist. This primitive includes at least the following
parameters:

   - `blobId` of type `BlobIdentifierT` contains the blob identifier
     for the transmitted blob
   - `metadata` of type `BlobMetadataT` contains the metadata for the
     transmitted blob.

This primitive is for informational purposes only.

#### Service `BD-TxAbort` (Request, Confirm, Indication)

An application can request aborting the transmission of a blob. The
BlobDis entity will move the blob onto the blacklist and eventually
release the memory used by the blob data. This service is only
effective for blobs that are being transmitted by this node. An abort
signal is disseminated into the network to inform other nodes about
the abortion and let them start the process of releasing resources. 

Applications invoke this service by submitting a `BD-TxAbort.request`
primitive, which carries at least the following parameters:

  - `blobId` of type `BlobIdentifierT`: identifier of the blob for
    which transmission is to be aborted.
  - (*DO WE NEED APPID AS A REQUEST PARAMETER?*)

A `BD-TxAbort.confirm` primitive is returned after the BlobDis entity
completed processing this request. Applications should not access the
memory used for storing blob data after sending the
`BD-TxAbort.request` primitive.(*REVIEW THAT LAST SENTENCE*)

To process the request, the BlobDis enityt performs the following
steps:

~~~
1. If (not gBlobdisActive)
      stop processing
	  return status code BLOBDIS-STATUS-INACTIVE

2. If (gBlacklist.exists (blobId))
      stop processing
	  return status code BLOBDIS-STATUS-BLOB-BLACKLISTED

3. If (not gActiveBlobs.exists (blobId))
      stop processing
	  return status code BLOBDIS-STATUS-BLOB-UNKNOWN

4. Let blobrt = gActiveBlobs.lookup(blobId)
       appId  = blobrt.metadata.appId

5. If (blobrt.status not in TXSTATUS-*)
      stop processing
	  return status code BLOBDIS-STATUS-ABORT-ON-RECEIVE-BLOB

6. If (blobrt.status == TXSTATUS-ABORT)
      stop processing
	  return status code BLOBDIS-STATUS-BLOB-ALREADY-ABORTED

7. gBlacklist.adjoin(blobId, appId)

8. with blobrt do:
      blobrt.status = TXSTATUS_ABORTED
	  blobrt.txq_chunk_new.clear()
	  blobrt.txq_chunk_rtx.cleaR()
	  blobrt.txq_gap.clear()
	  blobrt.txq_phash_request.clear()
	  blobrt.txq_phash_response.clear()
	  blobrt.txq_metadata_request.clear()
	  blobrt.txq_metadata_response.clear()
	  blobrt.txq_abort.clear()
	  blobrt.txq_abort.qAppend (true)
      blobrt.tstamp_lastnew = current system time
	  blobrt.tstamp_lastreq = current system time

9. return status code BLOBDIS-STATUS-OK
~~~


(*!BY LEAVING BLOCK ACTIVE AND SIMULTANEOUSLY HAVING IT ON BLACKLIST,
NO CHUNKS OR RETRANSMISSION REQUESTS FOR IT WILL BE PROCESSED ANYMORE,
THE TIMEOUT TIMER WILL EXPIRE AND WE CAN DO THE CLEANUP THEN, GIVING
APPLICATIONS ENOUGH TIME. NEED TO VERIFY THIS BEHAVIOUR AND RE-PHRASE
ANY TEXT RELATED TO WHETHER APPLICATIONS SHOULD ACCESS THE MEMORY
AFTER ABORT*)

Besides the option of having the application trigger an abort while
transmitting a blob, it can also happen that the BlobDis entity makes
this decision itself, for example when during the blob transmission
the phashes do not match the data anymore and therefore the integrity
of the blob is in question. In these cases, the BlobDis entity informs
the relevant application by submitting a `BD-TxAbort.indication`
primitive, which carries at least the following data:

  - `blobId` of type `BlobIdentifierT`: identifier for the aborted
    blob
  - `metadata` of type `BlobMetadataT`: metadata for the aborted blob
  - `reason` of type `AbortReason`

After the application received this primitive, it should not access
the memory used for storing the data of the aborted blob, as this may
have already been released by BlobDis. (*REVIEW THIS SENTENCE*)



### Management
#### Service `BD-MgmtListBlacklist` (Request, Confirm)
#### Service `BD-MgmtListActiveBlobs` (Request, Confirm)
#### Service `BD-MgmtActivate` (Request, Confirm)

This service can be used to activate the local BlobDis entity, so that
afterwards it engages in blob transmissions and receptions.
Applications can invoke it by submitting a `BD-MgmtActivate.request`
primitive, which carries no parameters. The BlobDis entity responds to
this primitive by sending a `BD-MgmtActivate.confirm` primitive, which
contains a status code.

To process the request, the BlobDis entity performs the following
steps:

~~~
1. gBlobdisActive = true

2. return status code BLOBDIS-STATUS-OK
~~~


#### Service `BD-MgmtDeactivate` (Request, Confirm)

This service can be used to deactivate the local BlobDis entity, so
that afterwards it does not engage in blob transmissions or receptions
any longer. Applications can invoke it by submitting a
`BD-MgmtDeactivate.request` primitive, which carries no parameters. The
BlobDis entity responds to this primitive by sending a
`BD-MgmtDeactivate.confirm` primitive, which contains a status
code. Ongoing blob transfers are affected as follows:

  - Transmitted blobs are aborted and eventually put on the blacklist,
    their respective applications are notified.
  - Received blobs are blacklisted and their respective applications
    are notified.
  
To process the request, the BlobDis entity performs the following
steps:

~~~
1. If (not gBlobdisActive)
      stop processing
	  return status code BLOBDIS-STATUS-INACTIVE

2. gBlobdisActive = false

3. for all blobrt : BlobRuntimeData in gActiveBlobs do:
      gBlacklist.adjoin (blobrt.blobId, blobrt.metadata.appId)
	  blobrt.txq_chunk_new.clear()
	  blobrt.txt_chunk_rtx.clear()
	  blobrt.txq_gap.clear()
	  blobrt.txq_phash_request.clear()
	  blobrt.txq_phash_response.clear()
	  blobrt.txq_metadata_request.clear()
	  blobrt.txq_metadata_response.clear()
	  If (blobrt.state in TXSTATE-*)
	     blobrt.txq_abort.append (true)
		 send BD-TxAbort.indication to application given by blobrt.metadata.appId
	  else
	     
4. return status code BLOBDIS-STATUS-OK
~~~

#### Service `BD-MgmtStatus` (Request, Confirm)

- Returns whether active / inactive

### Configurable Parameters {#blobdis-configurable-parameters}

The configurable parameters of BlobDis can be set by applications or
station management entities through a suitably defined
implementation-dependent management interface. Valid parameter changes are
assumed to take effect immediately and will apply to all subsequently
generated beacons and service invocations, but they are not applied
retroactively.

Here we only specify mandatory parameters that any BlobDis
implementation needs to support. These are:

- `BLOBDISPAR_MAX_PAYLOAD_SIZE` of type `BPLengthT` gives the
  maximum allowable length of a payload generated by BlobDis and
  handed over to the underlying BP for transmission. The value must be
  larger than zero, and must not exceed the value
  `BPPAR_MAXIMUM_PACKET_SIZE-(ssizeof(BPHeaderT)+ssizeof(BPPayloadHeaderT))`,
  see Sections [BP Configurable
  Parameters](#bp-interface-configurable-parameters) and [BP Data
  Types](#subsubsec-beaconing-protocol-data-types). The minimum value
  for this parameter is
  `ssizeof(BlobdisHeaderT)+ssizeof(BlobHeaderT)+ssizeof(IRChunkT)+BLOBDISPAR_MAX_CHUNK_SIZE`
  (where `ssizeof(IRChunkT)` only accounts for the fixed parts of the
  `IRChunkT` instruction record). This condition ensures that a
  BlobDis payload can at least accommodate one maximum-sized chunk.

- `BLOBDISPAR_MAX_CHUNK_SIZE` of type `BPLengthT` gives the maximum
  size of payload data in a chunk, must be positive, but must not
  exceed `BLOBDISPAR_MAX_PAYLOAD_SIZE` minus `sizeof(BlobHeaderT)` and
  any separate BlobDis headers. (*BE MORE PRECISE*)

- `BLOBDISPAR_MAX_BLOB_CONTAINER_SIZE` of type `BPLengthT` gives the
  maximum size of a single blob container, must be at least as large as
  `ssizeof(BlobHeaderT)+ssizeof(IRChunkT)+BLOBDISPAR_MAX_CHUNK_SIZE`,
  and must not exceed
  `BLOBDISPAR_MAX_PAYLOAD_SIZE-ssizeof(BlobdisHeaderT)`.

- `BLOBDISPAR_MAX_METADATA_WAIT_TIME` is a strictly positive unsigned
  integer giving the maximum time in seconds for which a BlobDis
  entity waits for reception of metadata after learning the blob
  identifier for the first time, before giving up on the blob reception.

<!--IF I GO FOR MAKING THE CHUNK SIZE PART OF THE METADATA AND THEREFORE
ALL CHUNKS HAVING THE SAME SIZE, WE NEED TO REMOVE THE
BLOBDISPAR_MAX_CHUNK_SIZE PARAMETER, AND WE NEED TO CHECK THE CHUNK
SIZE FOR COMPATIBILITY WITH PAYLOAD SIZE UPON RECEIVING METADATA. THAT
OPENS THE GENERAL QUESTION OF WHAT HAPPENS IF WE RECEIVE FAULTY
METADATA-->


## Payload Format and Payload Construction Process {#blobdis-payload-construction}

No BlobDis payload is generated when either the dictionary of active
blobs `gActiveBlobs` is empty, or when for all active blobs in that
dictionary all their transmit queues (`txq_*` members) are empty.

In the following we assume that a BlobDis payload is to be generated.

The payload starts with a BlobDis header of type `BlobdisHeaderT`,
into which the sending node writes the major and minor version number
of the local BlobDis implementation. Subsequently, the BlobDis entity
selects, in an implementation-specific order, active blobs for which
to generate blob containers -- to be considered for this, an active
blob needs to have at least one non-empty transmit queue (`txq_*`
members) in their runtime data record (see Section
[`BlobRuntimeData`](#blobdis-data-runtime-data)). When a blob
container is to be generated for an active blob and there is
sufficient space left in the Blobdis payload (a blob container can be
up to `BLOBDISPAR_MAX_BLOB_CONTAINER_SIZE` bytes long, see Section
[Configurable Parameters](#blobdis-configurable-parameters) and has a
minimum size of `ssizeof(BlobHeaderT)+ssizeof(IRAbortT)`) then the
BlobDis entity:

  - Generates a blob header of type `BlobHeaderT`, which includes
    the blob identifier and the number of bytes in the blob container.
  - Adds instruction records (as long as there is sufficient space in
    the blob container), by inspecting the transmit queues in the
    following order:
       - When `txq_abort` is non-empty and sufficient space is
         available, include one record of type `IRAbortT` and remove
         oldest entry from `txq_abort`.
	   - When `txq_metadata_response` is non-empty and sufficient space is
         available, include one record of type `IRMetadataResponseT`
         carrying the metadata for this blob, and remove oldest entry
         from `txq_metadata_response`.
	   - When `txq_metadata_request` is non-empty and sufficient space is
         available, include one record of type `IRMetadataRequestT`
         and remove oldest entry from `txq_metadata_request`.
	   - While `txq_phash_response` is non-empty and sufficient space is
         available, remove oldest entry from the queue and add a
         corresponding entry of type `IRPHashResponseT`.
	   - While `txq_phash_request` is non-empty and sufficient space is
         available, remove oldest entry from the queue and add a
         corresponding entry of type `IRPHashRequestT`.
	   - While `txq_chunk_new` is non-empty and sufficient space is
         available, remove oldest entry from the queue and add a
         corresponding entry of type `IRChunkT`.		 
	   - While `txq_chunk_rtx` is non-empty and sufficient space is
         available, remove oldest entry from the queue and add a
         corresponding entry of type `IRChunkT`.
   	   - While `txq_gap` is non-empty and sufficient space is
         available, remove oldest entry from the queue and add a
         corresponding entry of type `IRGapT`.
	   

## Initialization, Runtime and Shutdown
### Preliminaries

In this section we describe the runtime behaviour of BlobDis, covering
initialization, shutdown and periodic activities, including the
generation of payloads. The receive path (i.e. the processing of
incoming payloads and their instruction records) is covered in Section
[Receive Path](#blobdis-receive-path).


### Initialization


In the first step of the initialization process the global variable
`gBlobdisActive` is set to false.

In the next step, the BlobDis entity registers itself with the
underlying Beaconing Protocol (BP) as a client protocol, by submitting
a `BP-RegisterProtocol.request` service primitive (See [Service
`BP-RegisterProtocol`](#bp-interface-service-bp-register-protocol))
with the following parameters:

  - `protId` is set to `BP_PROTID_BLOBDIS` (see [BP Data Types](#subsubsec-beaconing-protocol-data-types))
  - `name` is set to "BlobDis -- Blob Dissemination Protocol Vx.y"
    where 'x' and 'y' refer to the present major and minor version of
    BlobDis. 
  - `maxPayloadSize` is set to the value of the
    `BLOBDISPAR_MAX_PAYLOAD_SIZE` configuration parameter.
  - `queueingMode` is set to `BP_QMODE_QUEUE_DROPTAIL`
  - `maxEntries` is set to an implementation-defined strictly
	positive value.
  - `allowMultiplePayloads` is set to `false`.

The `BP-RegisterProtocol.confirm` service primitive includes the
information about the own node identifier, which is stored in the
global variable `gOwnNodeIdentifier`.

Initialize further global variables as follows:

  - clear the blacklist (`gBlacklist.clear()`)
  - clear the dictionary of active blobs (`gActiveBlobs.clear()`)
  - clear the list of registered applications (`gApplications.clear()`)
  - obtain version information about the BlobDis entity (variables
    `gMajorVersion` and `gMinorVerson`)

Optionally, before registering with the BP, the BlobDis entity can
choose to first submit a `BP-DeregisterProtocol.request` primitive
with `protId` field set to `BP_PROTID_BLOBDIS`, to cover the case that
a previously running BlobDis instance has crashed and the current
instance is re-started, see Section [Recommended behaviour of client
protocols](#bp-recommended-behaviour-client-protocols).

Finally, set `gBlobdisActive` to `true`.


### Runtime {#blobdis-runtime}
#### Preliminaries

At runtime, a BlobDis entity carries out certain activities
periodically. How the period is configured, how strictly it is
observed (or how much jitter is allowed), and how the periodic
activities of BlobDis are phased with respect to the periodic beacon
transmissions of the underlying Beaconing Protocol is left to
implementations. It is, however, assumed that BlobDis carries out its
periodic activities with an (average) period that is within a few
multiples of the average period of the underlying Beaconing Protocol.

We describe the runtime activities in two stages, first the
per-active-blob housekeeping activities (separately for transmitted
and received blobs), and secondly the overall behaviour upon each
periodic invocation of the BlobDis runtime. By an active blob we refer
to a blob that is currently included in the `gActiveBlobs` global
dictionary.

For describing the per-active-blob activities, we assume that the
variable `blobrt` holds the runtime data (of type `BlobRuntimeData`)
for the active blob under consideration.

<!-- Add a time condition to requesting metadata? May be useful, so
that I can also have a timeout for how long to stay in the
RXSTATUS-NO-METADATA state. If I am there for hours, then perhaps it
makes sense to abort. Whom do I send an abort indication to?-->
<!-- Timeout is only considered once we have received at least some data-->
<!-- Note: the notation blobrt.txq_*.clear() means to call clear on
all txq_* variables-->

#### Per-Blob Activities for Received Blobs

The following per-active-blob actions are carried out for a blob that
is being received (i.e. has a status in `RXSTATUS-*`):

~~~
1. Let curr_time = current system time
	   status    = blobrt.status
	   blobId    = blobrt.blobId

-- check whether we had to wait for metadata for too long
2. If (    (status == RXSTATUS-NO-METADATA) 
        && ((curr_time - blobrt.tstamp_creation) >= BLOBDISPAR_MAX_METADATA_WAIT_TIME))
       blobrt.txq_*.clear()
	   gBlacklist.adjoin(blobId, nullAppIdentifier)
	   gActiveBlobs.remove (blobId)
       stop processing

-- request retransmission of metadata if needed
3. If ((status == RXSTATUS-NO-METADATA) && (blobrt.txq_metadata_request.empty()))
       blobrt.txq_metadata_request.qAppend(true)
       stop processing

4. If (status == RXSTATUS-NO-METADATA)
       stop processing

5. Let metadata = blobrt.metadata

-- Check integrity of metadata, start over if incorrect
6. If (not metadata.verify_crc())
      blobrt.status = RXSTATUS-NO-METADATA
      blobrt.phashes.clear()
	  blobrt.gaps.clear()
	  blobrt.blocks.clear()
	  blobrt.txq_*.clear()
      blobrt.txq_metadata_request.qAppend(true)
      stop processing

7. Let appId    = metadata.appId
	   partSize = metadata.partSize
	   memref   = metadata.memref

-- check expiry of timeout time
8. If (    (curr_time - blobrt.tstamp_lastnew > metadata.timeoutTime)
		&& (status != RXSTATUS-NO-METADATA)
		&& (not blobrt.blocks.empty())
	  blobrt.phashes.clear()
	  blobrt.gaps.clear()
	  blobrt.blocks.clear()
	  blobrt.txq_*.clear()
	  blobrt.memref.bdmem_free()
	  gBlacklist.adjoin (blobId, appId)
	  gActiveBlobs.remove (blobId)
	  send BD-RxAbort.indication (blobId) to application given by appId
	  stop processing

-- check expiry of keepalive time
9. If (    (curr_time - blobrt.tstamp_lastreq > metadata.keepaliveTime)
        && (status == RXSTATUS-COMPLETE))
	  -- No retransmission request for a while, deactivate blob,
	  -- release resources, add to blacklist and inform application
      blobrt.phashes.clear()
	  blobrt.gaps.clear()
	  blobrt.blocks.clear()
	  blobrt.txq_*.clear()
	  blobrt.memref.bdmem_free()
	  gBlacklist.adjoin (blobId, appId)
	  gActiveBlobs.remove (blobId)
	  send BD-RxRelease.indication (blobId) to application given by appId
	  stop processing

-- verify integrity of phashes themselves
10. for all (offs,phash) in blobrt.phashes do:
       If (not phash.verify_crc())
	      blobrt.phashes.remove (phash)
		  blobrt.txq_phash_request.qAppend (offs)
	      If (status == RXSTATUS-COMPLETE)
		     blobrt.status = RXSTATUS-INCOMPLETE

-- verify integrity of complete parts with phashes
11. for all (offs,phash) in blobrt.phashes do:
	   If (    (tbd_part_is_completely_received(offs, partSize))
	        && (not phash.verify_hash(offs, partSize, memref))
          blobrt.phashes.remove (phash)
		  blobrt.gaps.adjoin (offs, partSize)
		  blobrt.blocks.remove (offs, partSize)
		  blobrt.txq_gap.adjoin(offs, partSize)
	      If (status == RXSTATUS-COMPLETE)
		     blobrt.status = RXSTATUS-INCOMPLETE
			 
-- check whether we are still missing phashes
12. If (    (blobrt.txq_phash_request.empty())
         && (blobrt.phashes is still missing entries))
	  Add all missing entries from blobrt.pashes to blobrt.txq_phash_request

-- check whether we need to request gaps
13. If (    (blobrt.txq_gap.empty())
         && (not blobrt.gaps.empty()))
	  Add all entries from blobrt.gaps to blobrt.txq_gap
	  -- call tbd_xxx for that, which can possibly break down larger
	  -- gaps into smaller gap requests
~~~


#### Per-Blob Activities for Transmitted Blobs

The following per-active-blob actions are carried out for a blob that
is being transmitted (i.e. has a status in `TXSTATUS-*`):


~~~
1. If (blobrt.status == TXSTATUS-ABORTED)
      stop processing

2. Let metadata = blobrt.metadata
       blobId   = blobrt.blobId

-- Check integrity of metadata, phashes and data, abort transmission
-- if any of these is incorrect. Implementations can choose whether to
-- check all parts/phashes or some subset of parts/phashes during each
-- invocation.
3. If (    (not metadata.verify_crc())
        || (not (forall (offs,phash) in blobrt.phashes : phash.verify_crc()))
		|| (not (forall (offs,phash) in blobrt.phashes : phash.verify_hash(offs, metadata.partSize))))
      blobrt.status = TXSTATUS-ABORTED
      blobrt.phashes.clear()
	  blobrt.gaps.clear()
	  blobrt.blocks.clear()
	  blobrt.txq_*.clear()
	  blobrt.txq_abort.qAppend(true)
	  gBlacklist.adjoin (blobId, nullAppIdentifier)
	  If (metadata.verify_crc())
	     send BD-TxAbort.indication (blobId) to application given by metadata.appId
      stop processing

-- check expiry of keepalive time
4. If (    (curr_time - blobrt.tstamp_lastreq > metadata.keepaliveTime)
        && (status == TXSTATUS-COMPLETED))
      blobrt.phashes.clear()
	  blobrt.gaps.clear()
	  blobrt.blocks.clear()
	  blobrt.txq_*.clear()
	  blobrt.memref.bdmem_free()
	  gBlacklist.adjoin (blobId, appId)
	  gActiveBlobs.remove (blobId)
	  send BD-TxRelease.indication (blobId) to application given by appId
	  stop processing

-- check whether to notify application of completing to send all data
5. If (    blobrt.txq_chunk_new.empty() 
        && (blobrt.status == TXSTATUS-SENDING))
      send BD-TxCompletion.indication to blobrt.metadata.appId
	  blobrt.status = TXSTATUS-COMPLETED
	  blobrt.tstamp_lastnew = current system time
      blobrt.tstamp_lastreq = current system time
	  blobrt.sent_tx_completion_indication = true
~~~

#### Overall Periodic Behaviour

The overall periodic behaviour of a BlobDis entity is described as follows:

~~~

1. Forall blobrt : gActiveBlobs do
     Carry out per-active-blob activities as described above.

2. Prepare BlobDis payload:
     - Select order for active blobs to be considered
	 - Construct payload, update stat_chunks_sent in each blobrt
~~~

The construction of a BlobDis payload is discussed in Section [Payload
Format and Payload Construction
Process](#blobdis-payload-construction).


### Shutdown

The shutdown of a BlobDis entity includes at least the following
steps:

~~~
1. for all blobrt : BlobRuntimeData in gActiveBlobs do:
      gBlacklist.adjoin (blobrt.blobId, blobrt.metadata.appId)
	  blobrt.txq_chunk_new.clear()
	  blobrt.txt_chunk_rtx.clear()
	  blobrt.txq_gap.clear()
	  blobrt.txq_phash_request.clear()
	  blobrt.txq_phash_response.clear()
	  blobrt.txq_metadata_request.clear()
	  blobrt.txq_metadata_response.clear()
	  blobrt.memref.bdmem_free()
	  gActiveBlobs.remove (blobrt.blobId)
~~~

In addition, a BlobDis entity may choose to inform all currently
registered applications about the shutdown in an
implementation-defined way.

After a shutdown has been initiated, no registered application should
access any blob data or metadata anymore.

## Transmit Path

The transmit path is already covered in Sections
@blobdis-payload-construction and @blobdis-runtime.

## Receive Path  {#blobdis-receive-path}

In this section we describe the activities to be carried out by the
BlobDis entity after it has received a BlobDis payload from the
underlying Beaconing Protocol.

### Initial Stages

Assuming the received payload has a sufficient minimum length to
contain a `BlobdisHeaderT` structure and at least one `BlobHeaderT`
structure 

~~~
1. If (payload size < ssizeof(BlobdisHeaderT) + ssizeofBlobHeaderT))
      stop processing

2. Let bdhdr = BlobdisHeaderT value stored at start of payload

3. If (bdhdr.majorVersion != gMajorVersion)
      stop processing

-- iterate over all contained blob containers:
4. While there is still at least ssizeof(BlobHeaderT) data available:
      Let blobhdr = BlobHeaderT value stored at start of remaining payload
          blobid  = blobhdr.blobId
      If (gBlacklist.exists (blobId))
	     stop processing
	  If (not gActiveBlobs.exists(blobId))
	     Let newrt : BlobRuntimeData with
		     newrt.blobId  = blobID
			 newrt.status  = RXSTATUS-NO-METADATA
             newrt.phashes.clear()
			 newrt.txq_*.clear()
			 newrt.txq_metadata_request.qAppend(true)
			 newrt.gaps.clear()
			 newrt.blocks.clear()
			 newrt.tstamp_lastnew      = current system time
			 newrt.tstamp_lastreq      = current system time
			 newrt.tstamp_creation     = current system time
		 gActiveBlobs.adjoin (blobid, newrt)
	  While there is still data available in the blob container:
         extract next instruction record, check if type known
		 If type is not known, stop processing
		 Process instruction record
~~~

There are further checks that are not specified here but need to be
carried out in an implementation. In particular:

- For each instruction record it must be checked whether there is
  still sufficient data available in the payload and within the
  instruction container before extracting the instruction record and
  processing it.
  
- The `irType` values at the start of each instruction record must be
  one of the pre-defined values (see Section [Instruction Record
  Types](#subsubsubsec:instruction-record-types)). 

If any of these conditions fails, payload processing stops, but an
implementation is not required to attempt to roll back any processing
that has been carried out previously while processing the same
payload.

The processing of the known instruction record types is explained in
the next sections. In these sections is assumed that the blob
identifier of the blob container is contained in the input variable
`blobId`.


### Processing `IRMetadataRequestT` instruction record

An `IRMetadataRequestT` instruction record carries no further data,
the blob identifier relevant for this record is contained in the
`blobId` variable. The instruction record is processed as follows:

~~~
1. Let blobrt = gActiveBlobs.lookup (blobId)
       status = blobrt.status
	   
2. If (    (    (    (status in TXSTATUS-*)
                  && (status != TXSTATUS-ABORTED))
		     || (    (status in RXSTATUS-*)
		          && (status != RXSTATUS-NO-METADATA)))
        && (blobrt.txq_metadata_response.empty()))
	  blobrt.txq_metadata_response.qAppend (true)
	  stop processing


~~~

### Processing `IRMetadataResponseT` instruction record

An `IRMetadataResponseT` instruction record carries a metadata record
of type `BlobMetadataT`, which we assume to be available in the
variable `metadata`, and the blob identifier in `blobId`. The
instruction record is processed as follows:

~~~

1. Let blobrt = gActiveBlobs.lookup (blobId)
       status = blobrt.status
	   appId  = metadata.appId
	   
2. If (    (not metadata.verify_crc())
        || (status != RXSTATUS-NO-METADATA))
      stop processing

3. If (not gApplications.exists (appId)
      gBlacklist.adjoin (blobId, appId)
	  blobrt.txq_*.clear()
	  gActiveBlobs.remove (blobId)
      stop processing

4. With blobrt : BlobRuntimeData do:
        blobrt.metadata = metadata
		blobrt.status   = RXSTATUS-INCOMPLETE
		blobrt.memref.bdmem_allocate (metadata.length)
		blobrt.phashes.clear()
		blobrt.gaps.clear()
		blobrt.gaps.adjoin((0, metadata.length))
		blobrt.blocks.clear()
		blobrt.txq_*.clear()
		blobrt.stat_chunks_sent = 0
        blobrt.tstamp_lastnew   = current system time
		blobrt.tstamp_lastreq   = current system time
		blobrt.tstamp_metadata  = current system time

5. send BD-RxNewBlob.indication(blobId, metadata) to app indicated by appId

~~~

Note that we do not yet add any requests for chunks or phashes into
the `txq_gap` or `txq_phash_request` transmit queues. Implementations
can choose to delay such requests for some time.

<!-- Is that the right approach? -->

### Processing `IRChunkT` instruction record

An `IRChunkT` instruction record contains an offset, a length value
and actual data (fields `offset`, `length`, `data`). The blob
identifier of the relevant blob is contained in `blobId`. The
instruction record is processed as follows:

~~~
1. Let blobrt    = gActiveBlobs.lookup (blobId)
       status    = blobrt.status
	   metadata  = blobrt.metadata
	   
2. If (    (status in TXSTATUS-*)
        || (status == RXSTATUS-NO-METADATA)
		|| (status == RXSTATUS-COMPLETE))
	  stop processing

3. If (    (offset+length >= metadata.length)
        || (blobrt.blocks.contains ((offset, length))))
	  stop processing

4. With blobrt : BlobRuntimeData do
      blobrt.memref.bdmem_memcpy (offset, length, data)
      blobrt.gaps.remove ((offset, length))
      blobrt.blocks.adjoin ((offset, length))
	  blobrt.txq_gap.remove ((offset, length))
	  blobrt.tstamp_lastnew = current system time
	  blobrt.txq_chunk_new.qAppend ((offset, length))

5. If (    (phash available for affected part)
        && (affected part is now completely received)
        && (available phash does not match stored part))
      drop phash availabe for affected part
	  drop data available for affected part
	  add requests to blobrt.txq_gap covering affected part
	  add phash request to blobrt.txq_phash_request for affected part
      add affected part to blobrt.gaps
	  remove affected part from blobrt.blocks
      stop processing
	  
6. If (    (blobrt.gaps.empty())
        && (all required phashes are available)
		&& (all stored phashes match their respective parts))
      blobrt.status = RXSTATUS-COMPLETE
	  send BD-RxCompletion.indication to application given by metadata.appId

~~~


### Processing `IRGapT` instruction record

An `IRGapT` instruction record contains an offset and a length value
(in `offset` and `length`). The blob identifier of the relevant blob
is contained in `blobId`. The instruction record is processed as
follows:


~~~

1. Let blobrt = gActiveBlobs.lookup (blobId)
       status = blobrt.status

2. If (    (status == RXSTATUS-NO-METADATA)
        || (status == TXSTATUS-ABORTED)
		|| (offset + length >= blobrt.metadata.length))
	  stop processing

3. blobrt.tstamp_lastrtx = current system time

4. If (    (status in RXSTATUS-*)
        && (blobrt.blocks.intersect((offset, length)) is empty))
	  stop processing

5. Let tx_candidates = a list of chunks for all data requested in the
                       instruction record that is available but not
                       already contained in either
                       blobrt.txq_chunk_new or blobrt.txq_chunk_rtx

6. Let tx_actual = tbd_determine_subset_to_transmit (tx_candidates)

7. If (not tx_actual.empty())
      Add all chunks from tx_actual to blobrt.txq_chunk_rtx

~~~

Note that with this behaviour a BlobDis entity does not change its
behaviour based on its receive state or on whether available data has
already been verified.


### Processing `IRPHashRequestT` instruction record

An `IRPHashRequestT` instruction record contains an offset value (in
`offset`). The blob identifier of the relevant blob is contained in
`blobId`. The instruction record is processed as follows:


~~~

1. Let blobrt = gActiveBlobs.lookup (blobId)
       status = blobrt.status

2. If (    (status == RXSTATUS-NO-METADATA)
        || (status == TXSTATUS-ABORTED)
		|| (offset >= blobrt.metadata.length)
		|| (offset modulo blobrt.metadata.partSize != 0)
		|| (not blobrt.phashes.exists (offset))
		|| (blobrt.txq_phash_response.contains (offset)))
	  stop processing

3. blobrt.txq_phash_response.qAppend (offset)

4. blobrt.tstamp_lastrtx = current system time
~~~

### Processing `IRPHashResponseT` instruction record

An `IRPHashResponseT` instruction record contains an offset value and
a partial hash value (in `offset` and `phash`). The blob identifier of
the relevant blob is contained in `blobId`. The instruction record is
processed as follows:


~~~

1. Let blobrt   = gActiveBlobs.lookup (blobId)
       status   = blobrt.status
	   metadata = blobrt.metadata
	   partSize = metadata.partSize

2. If (    (status == RXSTATUS-NO-METADATA)
        || (status in TXSTATUS-*)
		|| (not phash.verify_crc())
		|| (offset >= blobrt.metadata.length)
		|| (offset modulo partSize != 0))
	  stop processing

3. If (part covered by received phash is not yet fully available)
      If (blobrt.phashes.exists(offset))
	     If (phash != blobrt.phashes.lookup(offset))
		    blobrt.phashes.remove(offset)
			blobrt.txq_phash_request.adjoin(offset)
      else
	     blobrt.phashes.adjoin(offset, phash)
		 blobrt.txq_phash_response.adjoin(offset)
      stop processing

4. If (    (blobrt.phashes.exists(offset))
        && (blobrt.phashes.lookup(offset).verify_crc())
		&& (blobrt.phashes.lookup(offset).verify_phash(offset, partSize)))
      If (phash != blobrt.phashes.lookup(offset))
	     blobrt.txq_phash_response.adjoin(offset)
      stop processing
   else
      blobrt.phashes.remove(offset)
      If (phash.verify_phash(offset, partSize))
	     blobrt.phashes.adjoin(offset, phash)
		 blobrt.txq_phash_response.adjoin(offset)
	  else
         blobrt.gaps.adjoin((offset, partSize))
		 blobrt.blocks.remove((offset, partSize))
		 blobrt.txq_gap.adjoin((offset, partSize))
		 blobrt.txq_phash_request.adjoin(offset)

~~~


### Processing `IRAbortT` instruction record

An `IRAbortT` instruction record contains no further data. The blob
identifier of the relevant blob is contained in `blobId`. The
instruction record is processed as follows:


~~~

1. Let blobrt   = gActiveBlobs.lookup (blobId)
       status   = blobrt.status

2. If (status in TXSTATUS-*)
      stop processing

3. If (status == RXSTATUS-NO-METADATA)
      gBlacklist.adjoin(blobId, nullAppIdentifier)
	  gActiveBlobs.remove(blobId)
	  blobrt.txq_*.clear()
	  blobrt.txq_abort.adjoin(true)
	  stop processing

4.  Let appId = blobrt.metadata.appId
    in
	   send BD-RxAbort.indication to application given by appId
       gBlacklist.adjoin(blobId, appId)
	   blobrt.txq_*.clear()
	   blobrt.txq_abort.adjoin(true)
	   stop processing

~~~


## Limitations

The present BlobDis specification has a number of limitations, which
may or may not be addressed in future versions of this specification:

- A node can only become a participant in a blob dissemination when it
  has the relevant application registered locally, and does not
  otherwise engage. When too few nodes engage in a particular
  application the participants may not form a connected subset and
  blob dissemination may fail. In a future version, BlobDis may add a
  mode allowing a node to engage in blob dissemination without having
  an application registered.

- Currently there is no mechanism allowing a BlobDis entity to inform
  an application about the progress of a blob transfer (either sending
  or receiving a blob).

- No consideration has been given to security concerns.


## Scratch

- What happens if we are in the keepalive time (i.e. the blob is
  completely received, and was correct at least once) and the block
  loses its integrity?
- What is our reaction of the checksum of the metadata fails? We do
  need a periodic checking / scrubbing process that verifies the
  checksum
  Add checksums to instruction records? Probably not, as these are
  meant to be short-lived and get sufficient protection from the frame
  CRC.
- Clarify here or perhaps before the Vardis specification that if a
  transmissible datatype has a certain set of fields, these are to be
  stored in the same order as given in the text.
- I am not entirely clear whether I should use both Crcs and hashes
  for essentially the same purpose (i.e. integrity-protecting stuff
  while it resides in memory)
- How to represent interaction with the surrounding OS / system with
  regards to memory allocation and de-allocation?
- How to manage the risk of a DoS attack from sending tons of metadata
  responses to a node and have them allocate memory? Should metadata
  contain some authentication value and have BlobDis and the relevant
  application exchange some information about that to let BlobDis know
  whether it is ok to engage? Goal is to not add much in the way of
  security to BlobDis proper. But how would that work if I allow for a
  BlobDis instance to participate in *any* blob transfer, even those
  that it has no application registered for?
     - We could perhaps for non-registered applications use a
       simplified mode of operation in which a node simply acts as
       repeater for blob containers it does not know about, without
       maintaining further internal status
- Add logic and services to activate and deactive BlobDis as a whole
   - Perhaps factor out an abstraction of a life cycle of a DCP
     protocol (in particular BP client protocols) and treat all of
     them on a common basis.
- Add management services for activating and de-activating BlobDis as
  a whole
- Add some `repCnt` logic to the transmission of chunks and phashes?
- Clarification: BlobId's are allocated within BlobDis itself, after
  being handed over a valid blob for transmission from a registered
  application.
- Add a section about assumptions on the environment. In particular
  that we have access to a highly precise system clock which
  increments its clock value at a high rate (for the generation of
  unique BlobId's)
- Should we require persistency for blacklist? Or require that any
  blobIds older than a certain age are blacklisted automatically?
  (That requires thab blobIds do indeed carry date and time
  information).
- Receive path: when we do not yet have the blob metadata, do we
  already store chunks? Probably not.
- When defining `TimeStampT` make it clear that such a value contains
  both the date and the time with a certain minimum resolution
- In the metadata response IR, should I also include some status
  information, e.g. that I have blacklisted it?
- Foresee some sort of instruction record for aborts? In response,
  nodes can free the memory while still keeping around the metadata
  for some time. And in general, the memory should be released after
  an abort or after keepalive.
- Give up the distinction between txlst_*_new and txlst_*_rtx? I am
  not sure right now what this is good for.
- Should phashes themselves be protected by a CRC checksum?
- How long will entries be kept in the blacklist? Should it be
  persistent?
- Allow to enable / disable BlobDis on a per-application level?
- Worry about liveness checking of applications / heartbeat?
- When de-registering an application, the current design cancels all
  ongoing blob transfers for this applications. It might be useful at
  some stage to give the option of letting these carry
  on. Furthermore, currently no transfer abort indications are being
  generated.
- Mention somewhere that all processing activities sketched above in
  pseudocode are assumed to be processed atomically, i.e. we do not
  consider issues related to parallel processing of these, like race
  conditions on data.
- I am starting to become quite open to the idea of standardizing the
  chunk size. That makes indexing easier, and it makes it easier to
  track statistics about how often a certain chunk has been received
  (which is next to impossible if we leave it completely to
  implementations to decide which size they want to put in a
  chunk). If I do approach this, the `partSize` parameters should be
  constrained to be an integer multiple of a chunk size. It might even
  be conceivable to either set a chunk size in this specification, or
  to make it a parameter in the metadata of a blob. If I do the
  latter, a node can only participate in a blob if it can support the
  chunk size. 
- When de-activating BlobDis, abort all ongoing blob transmissions and
  receptions, including sending an abort instruction into the network
  and also sending corresponding indication primitives.
- Should all service requests carry an `appId`? How do I (and should
  I) ensure that only the 'owning' application can trigger actions
  like aborting a blob transmission or reception?
- Should `IRAbortT` transmissions be repeated a number of times? And
  should the txqueue for aborts be global? After all, once the abort
  has been performed, the txqueues for the blob are gone, so where to
  keep the abort instruction records?
     - That can be handled by putting a number of copies of the same
       `IRAbortT` into `txq_abort` and, instead of releasing memory
       after receiving an abort, just put the blob on the blacklist
       while keeping it in active blobs, and wait until its timeout
       expires
- Add services for blacklist management by applications?
    - Indicate that a blob has been moved to blacklist to registered
      application
    - Query blacklist
    - Remove blobId from blacklist, clear blacklist
	    (Really, what about blobs that are both on the blacklist and
	    in the list of active blobs?)
- When specifying the construction of outgoing payload, the priority
  order for instruction records to include in a blob container is
  (from highest to lowest):
    - abort
    - metadata response
	- metadata request
	- phash response
	- phash request
	- chunk response
	- gap request
- Question: when BlobDis is de-activated, what should it do about
  timeouts and keepalives? Certainly the relevant timestamps are going
  to be wildly inaccurate.
- Check that TXSTATUS-* states are properly evolved, in particular
  that TXSTATUS-COMPLETED is set at the right time.
- Check that all indications are sent at the right circumstances.
- Keep track of how often each chunk was received, so that we can
  annotate a gap request with something like a probability for a node
  to transmit a chunk
- Check whether we still use `stat_chunks_sent`
- Check whether `BD-TxCompletion.indication` is sent at the right
  place.
- Update the following other chapters of the DCP spec:
    - Changelog
	- Context and aims
	- DCP Architecture and protocol overview
	- Known issues, shortcomings and comments
- Implement handling of `tstamp_metadata` to let a blob wait for some
  time to receive incoming chunks and phashes before starting to send
  requests for gaps. Invent an associated configuration value for
  minimum waiting time.
- I have removed the `verified` flag from `IRChunkT`. Was that a sound
  decision?
- Turn `txq_chunk_*` into `IntervalList` thingies as well, except one
  that does allow neighbored intervals (one interval equals one
  chunk)???
- As a general rule, none of the actual txqueues need to be FIFO
  queues, I can order them, and that should be fine most of the time.
- In the sections where we describe processing the instruction
  records, do we need to repeat the field names of the records? That
  discusses them twice, risk for consistency.
- Can I get into an endless loop where I receive wrong phashes,
  request new copies and they are wrong again? Is there a way to end
  this after some time?
- Maintain the queue semantics, but add an `adjoin` operation?
- Does a phash need a separate checksum? I can throw away a hash
  either when its checksum fails, or when it fails to verify its
  underlying part (though in that case I also throw away data).
- State that there is an assumption that a crash/reboot sequence takes
  *much* longer than the clock resolution, so that blob identifiers
  generated before and after a crash/reboot are guaranteed to be
  distinct.
- Think more about the `nullAppIdentifier` and its use. Is this
  something I could send indications to?
- State that `ownNodeIdentifier` gets initialized after BP
  registration.
- Check in all the right places that the `appId` metadata field for a
  blob is different from `nullAppIdentifier`.
- When getting metadata for a blob (either Tx or Rx) check that the
  `keepaliveTime` and `timeoutTime` parameters are strictly positive,
  otherwise abort transfer with a suitable abort reason.
- Will `BlobdisHeaderT` need a counter for the number of instruction
  containers?
- State that all instruction records are processed atomically,
  payloads are processed atomically.
- When processing received instruction records in a payload, ensure
  that you stop processing when you cannot infer the length of an
  instruction record. Or insure that all instruction records have
  their lengths as an explicit field (in particular chunks). Also
  check that any instruction records other than `IRMetadataResponseT`
  are not processed when no metadata is present, and that in this
- When processing a received chunk, double-check the `length` field
  against `chunkSize` information from the metadata.
- check result of `bdmem_allocate()` using the `bdmem_allocated()`
  function in all the right places.
- dropped the `bdmem_memcpy()` operation, ensure that it is really not
  needed.
- Identify situations where to set status to `RXSTATUS-ABORTED`, which
  is a new status value.
- Add more values to `AbortReason` if needed.

<!--DECISION: THERE WILL BE NO RESTART MANAGEMENT SERVICE, REDUNDANT-->


<!--DECISION: ADOPT FIXED-SIZE CHUNKS, SO THAT EACH RECEIVED CHUNK IS
    EITHER COMPLETELY AVAILABLE OR COMPLETELY NEW -->
	
