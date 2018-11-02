/*
 * dstm_stats.cpp
 *
 *  Created on: 26.04.2012
 *      Author: igor
 */

#include "dstm_stats.h"

#include <vector>

#include <stdio.h>

std::vector<node_stats_t> nodes_stats;

void init_nodes_stats(int nodes_num)
{
	nodes_stats.resize(nodes_num);
}

void add_node_to_stat(int node_num, node_stats_t node_stat)
{
	nodes_stats[node_num] = node_stat;
}

void print_nodes_stats()
{
	node_stats_t summary = {0,0,0,0,0,0,0,0,0,0};
	printf("\n----------STATISTIC----------\n");
	for (int i = 0; i < nodes_stats.size(); i++) {
		printf("NODE %d\n", i);
		printf("  #TOTAL TXS		: %d\n", nodes_stats[i].total_txs);
		printf("  #TOTAL ABORTS		: %d\n", nodes_stats[i].total_txs_stats.total_aborts);
		printf("  #read-set val aborts	: %d\n", nodes_stats[i].total_txs_stats.rsval_aborts);
		printf("  #grab-locks	aborts	: %d\n", nodes_stats[i].total_txs_stats.grablocks_aborts);
		printf("  #wr-conf	aborts	: %d\n", nodes_stats[i].total_txs_stats.wr_confl_aborts);
		printf("  #ww-conf	aborts	: %d\n", nodes_stats[i].total_txs_stats.ww_confl_aborts);
		printf("  #inv-mem-r	aborts	: %d\n", nodes_stats[i].total_txs_stats.invmem_r_aborts);
		printf("  #inv-mem-w	aborts	: %d\n", nodes_stats[i].total_txs_stats.invmem_w_aborts);
		printf("  #unknown	aborts	: %d\n", nodes_stats[i].total_txs_stats.unknwn_aborts);
		printf("  #explicit	aborts	: %d\n", nodes_stats[i].total_txs_stats.explicit_aborts);
		summary.total_txs += nodes_stats[i].total_txs;
		summary.total_txs_stats = summary.total_txs_stats + nodes_stats[i].total_txs_stats;
	}

	printf("SUMMARY\n");
	printf("  #TOTAL TXS		: %d\n", summary.total_txs);
	printf("  #TOTAL ABORTS		: %d\n", summary.total_txs_stats.total_aborts);
	printf("  #read-set val aborts	: %d\n", summary.total_txs_stats.rsval_aborts);
	printf("  #grab-locks	aborts	: %d\n", summary.total_txs_stats.grablocks_aborts);
	printf("  #wr-conf	aborts	: %d\n", summary.total_txs_stats.wr_confl_aborts);
	printf("  #ww-conf	aborts	: %d\n", summary.total_txs_stats.ww_confl_aborts);
	printf("  #inv-mem-r	aborts	: %d\n", summary.total_txs_stats.invmem_r_aborts);
	printf("  #inv-mem-w	aborts	: %d\n", summary.total_txs_stats.invmem_w_aborts);
	printf("  #unknown	aborts	: %d\n", summary.total_txs_stats.unknwn_aborts);
	printf("  #explicit	aborts	: %d\n", summary.total_txs_stats.explicit_aborts);

	printf("--------END STATISTIC--------\n");
}
