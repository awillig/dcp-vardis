---

# Change Log


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

- Added queueing mode `BP_QMODE_QUEUE_DROPTAIL`

- Fixed various typos


### Changes in `dcp-datatypes.md`

- Introduced convention that names of transmissible data types end
  with `T`


### Changes in `dcp-srp.md`

- Introduced convention that names of transmissible data types end
  with `T`


### Changes in `dcp-vardis.md`

- Transmissible data types have now names ending with `T`


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
