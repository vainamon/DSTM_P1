/*
 * dstm_transaction_stats.h
 *
 *  Created on: 26.04.2012
 *      Author: igor
 */

#ifndef DSTM_TRANSACTION_STATS_H_
#define DSTM_TRANSACTION_STATS_H_

#include <stdint.h>

typedef struct transaction_stats {
	/// aborts
	uint32_t total_aborts;
	uint32_t rsval_aborts;
	uint32_t grablocks_aborts;
	uint32_t wr_confl_aborts;
	uint32_t ww_confl_aborts;
	uint32_t invmem_r_aborts;
	uint32_t invmem_w_aborts;
	uint32_t unknwn_aborts;
	uint32_t explicit_aborts;
} transaction_stats_t;

inline transaction_stats_t operator +(transaction_stats_t s1,transaction_stats_t s2)
{
	transaction_stats_t result;
	result.total_aborts = s1.total_aborts + s2.total_aborts;
	result.rsval_aborts = s1.rsval_aborts + s2.rsval_aborts;
	result.grablocks_aborts = s1.grablocks_aborts + s2.grablocks_aborts;
	result.wr_confl_aborts = s1.wr_confl_aborts + s2.wr_confl_aborts;
	result.ww_confl_aborts = s1.ww_confl_aborts + s2.ww_confl_aborts;
	result.invmem_r_aborts = s1.invmem_r_aborts + s2.invmem_r_aborts;
	result.invmem_w_aborts = s1.invmem_w_aborts + s2.invmem_w_aborts;
	result.unknwn_aborts = s1.unknwn_aborts + s2.unknwn_aborts;
	result.explicit_aborts = s1.explicit_aborts + s2.explicit_aborts;
	return result;
}

#endif /* DSTM_TRANSACTION_STATS_H_ */
