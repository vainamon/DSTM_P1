/*
 * dummy_loadbalancer.h
 *
 *  Created on: 15.09.2011
 *      Author: igor
 */

#ifndef DUMMY_LOADBALANCER_H_
#define DUMMY_LOADBALANCER_H_

#include "loadbalancer.h"

class DummyLoadBalancer : public LoadBalancer
{
public:
    DummyLoadBalancer(int _nodesNum) : nodesNum(_nodesNum), lastNode(-1) {}

	int getVictim();
private:
	int nodesNum;
	int lastNode;
};

#endif /* DUMMY_LOADBALANCER_H_ */
