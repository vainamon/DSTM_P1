#include "dstm_lib_p1/dstm_timestamps.h"

int isTimestampsEquivalent(uint64_t _ts1, uint64_t _ts2)
{
    if (NODE_TS(_ts1) == NODE_TS(_ts2))
        if (VERSION_TS(_ts1) == VERSION_TS(_ts2))
            return 1;
    return 0;
}

int isTimestampsEqual(uint64_t _ts1, uint64_t _ts2)
{
    if (VERSION_TS(_ts1) == VERSION_TS(_ts2))
        return 1;
    return 0;
}

int isTimestampGreater(uint64_t _ts1, uint64_t _ts2)
{
    if (VERSION_TS(_ts1) > VERSION_TS(_ts2))
        return 1;
    return 0;
}
