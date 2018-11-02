/*
 * dstm_transaction.h
 *
 *  Created on: 10.02.2012
 *      Author: igor
 */

#ifndef DSTM_TRANSACTION_H_
#define DSTM_TRANSACTION_H_

//#define VARIANT2

#include <setjmp.h>
#include <boost/unordered_map.hpp>
#include <vector>
#include <map>

#include "dstm_p1.h"

#include "../glue.h"

#include "dstm_transaction_stats.h"

enum {
	TX_IDLE = 0,
	TX_ACTIVE = 1,
	TX_COMMITTED = (1<<1),
	TX_ABORTED = (2<<1)
};

enum {
	ABORT_EXPLICIT = (1 << 3),
	ABORT_IMPLICIT = (1 << 4),
	ABORT_READSET_VAL = ABORT_IMPLICIT | (1 << 5),
	ABORT_GRABBING_LOCKS = ABORT_IMPLICIT | (1 << 6),
	ABORT_RW_CONFLICT = ABORT_IMPLICIT | (0x01 << 7),
    ABORT_WR_CONFLICT = ABORT_IMPLICIT | (0x02 << 7),
    ABORT_WW_CONFLICT = ABORT_IMPLICIT | (0x04 << 7),
    ABORT_INVALID_MEMORY_R = ABORT_IMPLICIT | (0x08 << 7),
    ABORT_INVALID_MEMORY_W = ABORT_IMPLICIT | (0x10 << 7)
};

class Transaction {
private:
    friend void glue::locksRequestProcessed(uintptr_t, uint64_t*, uint32_t);

    virtual void locksRequestProcessed(uint64_t*, uint32_t) = 0;
};

class DSTMTransaction : public Transaction{
public:
	DSTMTransaction();
	~DSTMTransaction();

    bool begin();

    bool commit();

    void rollback();

    void saveCheckpoint();

	template<class T>
    T read(T *addr);

	template<class T>
    void write(T *addr, T value);

#ifdef TRANSACTIONS_STATS
	transaction_stats_t getStats(){
		return stats;
	};
#endif

private:

#ifdef VARIANT1
    typedef boost::unordered_map<uintptr_t, uintptr_t> read_set_t;
    typedef boost::unordered_map<uintptr_t, uintptr_t> write_set_t;
#endif
#ifdef VARIANT2
    typedef boost::unordered_map<uintptr_t, uint64_t> read_set_t;
    typedef boost::unordered_map<uintptr_t, uint64_t> write_set_t;
    typedef boost::unordered_map<uintptr_t, uintptr_t> write_set_values_t;
#endif
//	typedef std::map<uintptr_t,uintptr_t> write_set_t;
    typedef std::vector<uint64_t> VC_t;

private:
    inline void abort(uint64_t reason);

    inline bool getLocks();

//    inline bool validateReadSet();

    inline bool validateAll();

    inline bool validate(uint64_t _ts);

//    inline void invalidateRSValuesInCache();

    inline void commitWriteSetValues();

    inline void releaseLocksWithNewClock();

    inline void releaseLocks(write_set_t::const_iterator);

	void locksRequestProcessed(uint64_t*,uint32_t);

    inline void clearWriteSet();
    inline void clearReadSet();
private:
	sigjmp_buf env;

	int status;

	read_set_t read_set;
	write_set_t write_set;
#ifdef VARIANT2
    write_set_values_t write_set_values;
#endif

	int maxRequestedLocksPerNode;

	gasnett_mutex_t rlMutex;
	gasnett_cond_t  rlCond;

	int						rlNodes;
    std::vector<uint64_t>	rlChangedObjects;

    VC_t VC;

#ifdef TRANSACTIONS_STATS
	transaction_stats_t stats;
#endif
};

#endif /* DSTM_TRANSACTION_H_ */
