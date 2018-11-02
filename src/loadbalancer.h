/*
 * loadbalancer.h
 *
 *  Created on: 15.09.2011
 *      Author: igor
 */

#ifndef LOADBALANCER_H_
#define LOADBALANCER_H_

class LoadBalancer
{
public:
	virtual int getVictim() = 0;
};

typedef LoadBalancer* loadbalancer_ptr_t;

#endif /* LOADBALANCER_H_ */
