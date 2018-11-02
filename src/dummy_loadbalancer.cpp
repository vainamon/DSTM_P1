/*
 * dummy_loadbalancer.cpp
 *
 *  Created on: 15.09.2011
 *      Author: igor
 */

#include "dummy_loadbalancer.h"

int DummyLoadBalancer::getVictim()
{
//	static int first = 1;

//	if (!first)
		lastNode<nodesNum-1 ? lastNode++ : lastNode=0;
/*	else{
		first = 0;
		return 0;
	}
*/
	return lastNode;
}
