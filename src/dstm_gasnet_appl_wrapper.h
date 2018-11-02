/*
 * dstm_gasnet_appl_wrapper.h
 *
 *  Created on: 05.10.2011
 *      Author: igor
 */

#ifndef DSTM_GASNET_APPL_WRAPPER_H_
#define DSTM_GASNET_APPL_WRAPPER_H_

#include "dstm_gasnet_application.h"

class DSTMGASNetApplWrapper : DSTMGASNetApplication {
public:
	int addLocalThread(int _gtid,int _ltid,int _state) = 0;

	int addRemoteThread(int _gtid,int _ltid,int _state) = 0;

	bool inLocalThreads(int _gtid) = 0;

	bool inRemoteThreads(int _gtid) = 0;

	application_thread_ptr_t getLocalThread(int _gtid) = 0;

	application_thread_ptr_t getRemoteThread(int _gtid) = 0;
};

#endif /* DSTM_GASNET_APPL_WRAPPER_H_ */
