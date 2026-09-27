#pragma once

#include <dcp/common/global_types_constants.h>


#pragma pack(push, 1)
typedef struct VardisTestVariable {
  uint64_t           seqno;
  double             value;
  dcp::TimeStampT    tstamp;
} VardisTestVariable;
#pragma pack(pop)
