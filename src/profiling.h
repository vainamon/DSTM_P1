/*
 * profiling.h
 *
 *  Created on: 30.11.2011
 *      Author: igor
 */

#ifndef PROFILING_H_
#define PROFILING_H_

#include <iomanip>

#if defined LOGGING || defined PROFILING
#include <glog/logging.h>
#endif

#ifdef PROFILING
#define BEGIN_TIME_PROFILING(); {\
	gasnett_tick_t start;		\
	gasnett_tick_t end;			\
	start = gasnett_ticks_now();
#else
#define BEGIN_TIME_PROFILING();
#endif

#ifdef PROFILING
#define END_TIME_PROFILING(msg);	\
	end = gasnett_ticks_now();	\
	uint64_t time_ns = gasnett_ticks_to_ns(end-start); \
	VLOG(1)<<"PROFILING "<<__PRETTY_FUNCTION__<<" TIME: "<<std::fixed<<std::setw(10)<<std::setprecision(7)<<((double)time_ns)/1000000000<<" >>> "<<msg;\
	}
#else
#define END_TIME_PROFILING(msg);
#endif

#ifdef PROFILING
#define DEFINE_H_HANDLER_TIME_PROFILING(hnd);	\
	extern gasnett_tick_t hnd##_start;	\
	extern gasnett_tick_t hnd##_end;		\
	extern int hnd##_gtid;
#else
#define DEFINE_H_HANDLER_TIME_PROFILING(hnd);
#endif

#ifdef PROFILING
#define DEFINE_HANDLER_TIME_PROFILING(hnd,gtid);	\
	gasnett_tick_t hnd##_start;	\
	gasnett_tick_t hnd##_end;		\
	int hnd##_gtid = gtid;
#else
#define DEFINE_HANDLER_TIME_PROFILING(hnd,gtid);
#endif

#ifdef PROFILING
#define START_HANDLER_TIME_PROFILING(hnd,gtid);	\
	if(hnd##_gtid == gtid)					\
		hnd##_start = gasnett_ticks_now();
#else
#define START_HANDLER_TIME_PROFILING(hnd,gtid);
#endif

#ifdef PROFILING
#define END_HANDLER_TIME_PROFILING(hnd,gtid);	\
	if(hnd##_gtid == gtid){						\
		hnd##_end = gasnett_ticks_now();		\
		uint64_t hnd##_time_ns = gasnett_ticks_to_ns(hnd##_end-hnd##_start);	\
		VLOG(2)<<"PROFILING "<<__PRETTY_FUNCTION__<<" TIME: "<<std::fixed<<std::setw(10)<<std::setprecision(7)<<((double)hnd##_time_ns)/1000000000<<" >>> "<<#hnd;	\
	}
#else
#define END_HANDLER_TIME_PROFILING(hnd,gtid);
#endif

DEFINE_H_HANDLER_TIME_PROFILING(spawnthread);

#endif /* PROFILING_H_ */
