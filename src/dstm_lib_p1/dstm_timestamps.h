#ifndef DSTM_TIMESTAMPS_H_
#define DSTM_TIMESTAMPS_H_

#include <stdint.h>

#define LOCKBIT_TS(x) ((uint64_t)x >> 63)
#define NODE_TS(x) (((uint64_t)x >> 48) & 0x7fff)
#define VERSION_TS(x) ((uint64_t)x & 0xffffffffffff)

#define TO_NODE_TS(x) (((uint64_t)x & 0x7fff) << 48)

int isTimestampsEquivalent(uint64_t _ts1, uint64_t _ts2);
int isTimestampsEqual(uint64_t _ts1, uint64_t _ts2);
int isTimestampGreater(uint64_t _ts1, uint64_t _ts2);

#endif
