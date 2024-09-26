---

# Change Log

The change log describes changes made from one version to the
next. This is restricted to substantial changes, editorial matters
such as language issues, typos or minor errors are not reported.

## Changes for Version 1.1


### Changes in `dcp-architecture.md`

- Introduced a minimum maximal packet size for UWBs.


### Changes in `dcp-bp.md`

- Service ~BP-RegisterProtocol~: now treats the case where
  maxPayloadSize<=0

- Changed type names of transmissible data types to now end with a `T`

- Clarified in description of `BP_QMODE_QUEUE` that head-of-line
  blocking can happen

- Expanded description of returning `*.confirm` service primitives in
  response to received `*.request` service primitives

- Refined description of configurable parameters, in particular
  allowed values and reaction to illegal values. Also added
  sub-section on non-configurable parameters (max packet size allowed
  by UWB)

- Specified handling of version field in `BPHeaderT`

- Added queueing modes `BP_QMODE_QUEUE_DROPTAIL` and
  `BP_QMODE_QUEUE_DROPHEAD`

- Fixed various typos


### Changes in `dcp-datatypes.md`

- Introduced convention that names of transmissible data types end
  with `T`


### Changes in `dcp-srp.md`

- Introduced convention that names of transmissible data types end
  with `T`


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

- The BP queueing mode of VarDis is now set to `BP_QMODE_QUEUE` (see
  [Initialization](#vardis-init-runtime-shutdown-init)).

- Clarified behaviour upon errors when processing received information
  elements (see [Processing
  order](#vardis-receive-path-ie-processing-order)) to say that if an
  unknown information element is found then the entire remainder of
  the VarDis payload is silently dropped.

- When processing received `IETYPE-CREATE-VARIABLES` and `IETYPE-UPDATES`
  information elements, the length of the received variable value and
  (in case of a `VarCreateT`) length of description is checked against
  maximum allowed values `VARDISPAR_MAX_VALUE_LENGTH` and
  `VARDISPAR_MAX_DESCRIPTION_LENGTH`, and the records are discarded
  when they exceed these lengths (see [Processing
  `IETYPE-CREATE-VARIABLES`](#vardis-receive-path-ie-create-variables)
  and [Processing `IETYPE_UPDATES`](#vardis-receive-path-ie-updates)).

- When composing an outgoing payload, summaries now have precedence
  over updates (see [Payload
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
  `IETYPE-SUMMARIES`](#vardis-composing-ietype-summaries).





