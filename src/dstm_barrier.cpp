#include "dstm_barrier.h"

#include <sys/time.h>

void barrier_init(barrier_t *b, int n)
{
  gasnett_cond_init(&b->complete);
  gasnett_mutex_init(&b->mutex);
  b->count = n;
  if (gasnet_mynode() == 0)
    b->count++;
  b->crossing = 0;

//  fprintf(stderr,"bar_init %p count %d node %d \n",b,b->count,gasnet_mynode());
}

int barrier_calculate_threads(int tnum)
{
	int nodes = gasnet_nodes();

	int mynode = gasnet_mynode() + 1;

	int div = tnum / nodes;

	int mod = tnum % nodes;

	int result = 0;

	if (div > 0){
		if (mod > 0){
			if (mynode > mod)
				result = div;
			else
				result = div + 1;
		}else
			result = div;
	}else{
		if (mynode > tnum)
			result = 0;
		else
			result = 1;
	}
	return result;
}

void barrier_cross(barrier_t *b,int local_wait)
{
//	fprintf(stderr,"bar_crs %p node %d - 1\n",b,gasnet_mynode());
	gasnett_mutex_lock(&b->mutex);
//	fprintf(stderr,"bar_crs %p node %d - 2\n",b,gasnet_mynode());

	b->crossing++;
//	fprintf(stderr,"bar_crs %p crs %d node %d \n",b,b->crossing,gasnet_mynode());
	if (b->crossing < b->count) {
		if (local_wait) {
			gasnett_cond_wait(&b->complete, &b->mutex);
			if (b->crossing != 0)
				b->crossing--;
		}else{
			if (b->crossing == 1)
				gettimeofday(&b->start, NULL);
		}
	} else {
//		fprintf(stderr,"bar_crs %p crs %d node %d \n",b,b->crossing,gasnet_mynode());
		gasnet_barrier_notify(b->id, 0);
		GASNET_Safe(gasnet_barrier_wait(b->id,0));

		b->count = b->crossing;

		if (local_wait) {
			b->crossing--;
		} else
			b->crossing = 0;

		gasnett_cond_broadcast(&b->complete);
	}

	gasnett_mutex_unlock(&b->mutex);
}

int barrier_canbe_destroyed(barrier_t *b)
{
	if (!b->crossing)
		return 1;

	return 0;

}

int barrier_elapsed(barrier_t *b)
{
	struct timeval end;

	gettimeofday(&end,NULL);

	return ((end.tv_sec*1000+end.tv_usec/1000) - (b->start.tv_sec*1000+b->start.tv_usec/1000));
}
