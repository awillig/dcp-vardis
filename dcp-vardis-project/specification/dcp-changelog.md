---

# Change Log

The change log describes changes made from one version to the
next. This is restricted to substantial changes, editorial matters
such as language issues, typos or minor errors are not reported.

## Changes for Version 1.1

### Changes affecting multiple sections

- The conventions around transmissible data types (c.f. Section
  [Conventions](#chap-datatypes-conventions)) have now changed such
  that:
    - The names of all transmissible data types now end with `T`.
	- The bit-width of several transmissible data types are not fixed
	  by this specification but rather can be chosen by
	  implementations as integral multiples of one byte.
	  
- Use of `sizeof()` expressions have now been replaced with
  `ssizeof()` to clarify that this expression has its own meaning
  within DCP/VarDis that differs from its meaning in other programming
  languages such as C++.

### Changes in `dcp-version-contributors.md`

- Added an explanation around the version policy


### Changes in `dcp-datatypes.md`

- Introduced convention that names of transmissible data types end
  with `T` and that transmissible data types, when not of fixed width,
  are to have a width that is an integer multiple of a byte.

- Clarified that the accuracy of `TimeStampT` is mainly relevant for
  applications, whereas DCP/VarDis has more modest accuracy
  requirements.

- The length of a string of type `StringT` is now limited to 255
  bytes, and strings are not zero-terminated anymore but rather are
  preceded by a length byte.

- Removed subsection on versioning (redundant with `dcp-version-contributors.md`)

- New operation `qClear()` for queues.


### Changes in `dcp-architecture.md`

- Introduced a minimum maximal packet size for UWBs.


### Changes in `dcp-bp.md`

- Service ~BP-RegisterProtocol~: now treats the case where
  maxPayloadSize<=0

- Changed type names of transmissible data types to now end with a `T`

- Expanded description of returning `*.confirm` service primitives in
  response to received `*.request` service primitives

- Refined description of configurable parameters, in particular
  allowed values and reaction to illegal values. Also added
  sub-section on non-configurable parameters (max packet size allowed
  by UWB)

- Specified handling of version field in `BPHeaderT`

- Renamed type `BPPayloadBlockHeaderT` to `BPPayloadHeaderT`

- Added queueing modes `BP_QMODE_QUEUE_DROPTAIL` and
  `BP_QMODE_QUEUE_DROPHEAD`, removed queueing mode
  `BP_QMODE_QUEUE`. Clarified that head-of-line blocking can happen


- The `BPClientProtocol` type and the parameters of the
  `BP-RegisterProtocol.request` primitive now include a field
  `allowMultiplePayloads`, with the intention to let BP client
  protocols control whether the BP implementation may include several
  client payloads into the same outgoing beacon. It is left to
  implementations whether they respect this flag or not, there is no
  requirement to implement this behaviour.

- Added new service `BP-ClearBuffer` so that client protocols can
  clear their buffer / queue. Also made handing over a payload of zero
  length in service `BP-TransmitPayload.request` an error.

- In the transmit path, the current value of `bpSequenceNumber` is now
  incremented after filling the `seqno` field in an outgoing
  `BPHeaderT`.

- Revised condition for maximum value of `maxPayloadSize` when
  processing a `BP-RegisterProtocol.request` service primitive.

- Field `timeStamp` of type `BPClientProtocol` is now named
  `timeStampRegistration`.

- Confirmation primitive `BP-RegisterProtocol.confirm` now includes
  the own node identifier, so that client protocols have a defined way
  of learning the own node identifier.

- Clarified return value of service `BP-ListRegisteredProtocols`, in
  particular current buffer / queue contents are not included.
  
- Clarified that initialization must be completed before accepting any
  registrations from client protocols, and introduced global
  parameter `ownNodeIdentifier` to hold the own node identifier
  (obtained during initialization).

- Added a comment on protocol multiplexing in Section
  [Non-configurable parameters](#bp-interface-non-configurable-parameters).

- Fixed various typos


### Changes in `dcp-srp.md`

- Introduced convention that names of transmissible data types end
  with `T`

- Clarified initialization to state that the own node identifier is
  learned from the `BP-RegisterProtocol.confirm` primitive, and that
  implementations can choose to submit a
  `BP-DeregisterProtocol.request` primitive before submitting a
  `BP-RegisterProtocol.request` primitive.

### Changes in `dcp-vardis.md`

- Transmissible data types have now names ending with `T`

- Clarified semantics of `RTDB-Create.confirm` and
  `RTDB-Update.confirm` primitives to stress that they only report
  about local success, not about success on any other node

- Removed requirement that types `VarIdT`, `VarLenT`, `VarRepCntT`,
  and `VarSeqnoT` have a length of eight bits. The length of these
  data types is now an implementation-dependent choice.

- Introduced constant `SEQNO-MODULUS` to represent sequence number
  space.
  
- Added new subsection 'Implementation requirements' to comment on
  mutual exclusion issues in case an implementation uses
  multi-threading.

- Added further clarification on allowed ranges for the values of
  configurable parameters (Section [Configurable
  Parameters](#vardis-configurable-parameters)).

- Expanded description of `RTDB-DescribeDatabase` service (see
  [service description](#vardis-service-database-contents-describe-db))

- The BP queueing mode of VarDis is now set to
  `BP_QMODE_QUEUE_DROPTAIL` (see
  [Initialization](#vardis-init-runtime-shutdown-init)) with an
  implementation-defined number of entries.

- Clarified behaviour upon errors when processing received information
  elements (see [Processing
  order](#vardis-receive-path-ic-processing-order)) to say that if an
  unknown information element is found then the entire remainder of
  the VarDis payload is silently dropped.

- When processing received `IETYPE-CREATE-VARIABLES` and `IETYPE-UPDATES`
  information elements, the length of the received variable value and
  (in case of a `VarCreateT`) length of description is checked against
  maximum allowed values `VARDISPAR_MAX_VALUE_LENGTH` and
  `VARDISPAR_MAX_DESCRIPTION_LENGTH`, and the records are discarded
  when they exceed these lengths (see [Processing
  `IETYPE-CREATE-VARIABLES`](#vardis-receive-path-ic-create-variables)
  and [Processing `IETYPE_UPDATES`](#vardis-receive-path-ic-updates)).

- When composing an outgoing payload, constructing `IETYPE-SUMMARIES`
  and `IETYPE-REQUEST-VARCREATES` now have precedence over
  constructing `IETYPE-UPDATES` (see [Payload
  construction](#vardis-payload-format-construction)).

- Clarified semantics of the confirmation to a `RTDB-Delete.request`
  service primitive (see [Service
  `RTDB-Delete`](#vardis-service-database-contents-delete-var)).

- Changed name of `VARDIS_STATUS_INVALID_VALUE` to
  `VARDIS_STATUS_EMPTY_VALUE`

- When processing an `IETYPE-REQUEST-VARUPDATES` or an
  `IETYPE_REQUEST_VARCREATES` information element, we now add the
  received `varId` to `reqCreateQ` when the variable is unknown.

- When processing a `RTDB-Create.request` primitive from the local
  higher layers, the new `varId` is now removed from all queues before
  it is inserted into the relevant ones (`summaryQ` and
  `createQ`). The same applies to the processing of a received
  `IETYPE-CREATE-VARIABLES` record.

- Added clarification to the explanation of the `numberFittingRecords`
  function (see [Payload
  Construction](#vardis-payload-format-construction)) that the number
  of records must not exceed the maximum number representable in the
  `ieNumRecords` member of `IEHeaderT`.

- Added a global runtime variable `vardisActive` to indicate whether
  or not the VarDis instance is active. When it is not active, it will
  not generate any outgoing beacon payloads, will not process any
  received beacon payloads and will not process any application
  service requests.

- At the start of the description of the VarDis application service
  interface it is now stated that a VarDis instance must be able to
  support multiple concurrent application / application instances.

- Added a section with a high-level overview at the start.

- Re-organised description of data types. We now refer to *instruction
  containers* instead of information elements, and the entries in such
  an instruction container are called *instruction records* or simply
  *instructions*. There is also now a distinction between basic
  (transmissible) data types that serve as building blocks, then data
  types describing instructions (each type of instruction now has its
  own data type) and data types for the instruction
  containers. Furthermore, to represent variable values and
  descriptions there is now a basic data type called `MemoryBlockT`.
  
- Added clarification that the length of basic data types such as
  `VarIdT` or `VarLenT` can be chosen by implementation but must be an
  integer multiple of one byte.

- Configurable parameters: added clarification that changes to a
  parameter are not applied retroactively.

- Removed hard upper bound of 15 for `VARDISPAR_MAX_REPETITIONS`.

## September 8, 2024: Publication of Version 1.0

- Various minor cleanup changes


## Changes since publication of (preliminary version of) Version 1.0

- January 27, 2024:
   - Various changes to the specification after implementing a version
     for simulation
   - Removed this changelog from the actual specification document

- October 17, 2022: VarDis: Changed meaning of
  `VARDISPAR_MAX_SUMMARIES` configuration variable to now include an
  option to disable generation of summaries when this variable is set
  to zero, see [Configurable
  Parameters](#vardis-configurable-parameters), [Payload
  Construction](#vardis-payload-format-construction), [Composing
  `IETYPE-SUMMARIES`](#vardis-composing-ictype-summaries).





