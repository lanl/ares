// ArgoPool.cpp

#include "ArgoPool.hpp"
// abt.h does the extern "C" wrapping
#include "abt.h"
#include <assert.h>
#include <malloc.h>

namespace ares {

  ABT_xstream *ArgoPool::xstreams;
  ABT_pool *ArgoPool::pools;
  int ArgoPool::numStreams;
  int ArgoPool::nextPool;

  ArgoPool::ArgoPool(int argc, char *argv[], int _numStreams)
  //    : numStreams(numStreams) // in addition to the "main" stream
  {
    numStreams = _numStreams;

    assert(numStreams >= 1);
    // can only have one instance of ArgoPool
    assert(ABT_initialized() == ABT_ERR_UNINITIALIZED);
    // start ArgoBots
    assert(ABT_init(argc, argv) == ABT_SUCCESS);

    // populate xstreams, numStreams is in addition to "main" stream
    xstreams = 
      (ABT_xstream *)malloc(sizeof(ABT_xstream) * (numStreams + 1));
    // we are the main ES and ULT, stream already exists
    assert(ABT_xstream_self(&xstreams[0]) == ABT_SUCCESS);
    // and numStreams new ones
    for (int i = 1; i != numStreams + 1; i++) {
      assert(ABT_xstream_create(ABT_SCHED_NULL, // round robin?
				&xstreams[i]) == ABT_SUCCESS);
    }

    // populate pools, one per total number of streams
    pools = (ABT_pool *)malloc(sizeof(ABT_pool) * (numStreams + 1));
    // get the pools in which to allocate threads
    for (int i = 0; i != numStreams + 1; i++) {
      assert(ABT_xstream_get_main_pools(xstreams[i], 
					1, // just the first such pool
					&pools[i]) 
	   == ABT_SUCCESS);
    }
    // first pool in which to allocate thread
    nextPool = 1;
  }

  ArgoPool::~ArgoPool() {
    // initialized?
    assert(ABT_initialized() == ABT_SUCCESS);

    // join user-created ES with main ES
    assert(ABT_xstream_join(xstreams[1]) == ABT_SUCCESS);
    // free user-created ES
    assert(ABT_xstream_free(&xstreams[1]) == ABT_SUCCESS);

    // finalize
    assert(ABT_finalize() == ABT_SUCCESS);
  }

  void ArgoPool::AP_push(FuncPtr func, void* argp, uint32_t priority) {
    // new thread on ES[nextPool] via POOL[nextPool]
    assert(ABT_thread_create(pools[nextPool],
			     func,
			     argp,
			     ABT_THREAD_ATTR_NULL,
			     NULL)
	   == ABT_SUCCESS);
    nextPool = ((nextPool + 1) % numStreams) + 1;
    // threadId++;
    // assert(ABT_thread_free... not needed since NULL passed to ABT_thread_create()
  }

  void ArgoPool::AP_yield() {
    assert(ABT_thread_yield() == ABT_SUCCESS);
  }

  void ArgoPool::AP_finish() {
    // nothing to do?  ABT told to free thread on termination, we'll see.
  }

} // namespace ares
