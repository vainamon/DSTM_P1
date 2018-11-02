/*
 * dstm_node_stats.h
 *
 *  Created on: 26.04.2012
 *      Author: igor
 */

#ifndef DSTM_NODE_STATS_H_
#define DSTM_NODE_STATS_H_

#include "dstm_transaction_stats.h"

typedef struct node_stats {
	uint32_t total_txs;
	transaction_stats_t total_txs_stats;
} node_stats_t;

#endif /* DSTM_NODE_STATS_H_ */
