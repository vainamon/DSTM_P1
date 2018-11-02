/*
 * dstm_common.h
 *
 *  Created on: 12.09.2011
 *      Author: igor
 */

#ifndef DSTM_COMMON_H_
#define DSTM_COMMON_H_

#include <gasnet.h>
#include <gasnet_config.h>
#include <gasnet_tools.h>

#define GASNET_Safe(fncall) do {                                     \
    int _retval;                                                     \
    if ((_retval = fncall) != GASNET_OK) {                           \
      fprintf(stderr, "ERROR calling: %s\n"                          \
                   " at: %s:%i\n"                                    \
                   " error: %s (%s)\n",                              \
              #fncall, __FILE__, __LINE__,                           \
              gasnet_ErrorName(_retval), gasnet_ErrorDesc(_retval)); \
      fflush(stderr);                                                \
      gasnet_exit(_retval);                                          \
    }                                                                \
  } while(0)

#define BARRIER() do {                                              \
  gasnet_barrier_notify(0,GASNET_BARRIERFLAG_ANONYMOUS);            \
  GASNET_Safe(gasnet_barrier_wait(0,GASNET_BARRIERFLAG_ANONYMOUS)); \
} while (0)

#endif /* DSTM_COMMON_H_ */
