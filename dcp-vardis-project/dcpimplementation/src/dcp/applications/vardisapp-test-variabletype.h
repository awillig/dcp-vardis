#pragma once

#include <dcp/common/global_types_constants.h>

using dcp::TimeStampT;

#pragma pack(push, 1)
typedef struct VardisTestVariable {
    uint64_t      seqno;
    double        value;
    TimeStampT    tstamp;
} VardisTestVariable;
#pragma pack(pop)
