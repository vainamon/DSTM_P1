/*
 * dstm_stats.h
 *
 *  Created on: 26.04.2012
 *      Author: igor
 */

#ifndef DSTM_STATS_H_
#define DSTM_STATS_H_

#include "dstm_lib_p1/dstm_node_stats.h"

void init_nodes_stats(int nodes_num);

void add_node_to_stat(int node_num, node_stats_t node_stat);

void print_nodes_stats();

#endif /* DSTM_STATS_H_ */
