/*
 * application.h
 *
 *  Created on: 15.09.2011
 *      Author: igor
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <boost/shared_ptr.hpp>

class Application {
public:
    virtual ~Application(){}

	virtual void init(bool _main = true, int _poolsize = 8) = 0;

	virtual int runMainThread() = 0;
	virtual int runChildThread(void*,int,int) = 0;

	/**
	 * Thread-related types
	 */

	enum {
		RUNNABLE,
		RUNNING,
		DEAD,
		EXITED,
		TERMINATED
	};

	typedef struct _thread_t {
		int ltid;
		int state;
	} ApplicationThread;

	typedef boost::shared_ptr<ApplicationThread> application_thread_ptr_t;
};

typedef boost::shared_ptr<Application> application_ptr_t;

#endif /* APPLICATION_H_ */
