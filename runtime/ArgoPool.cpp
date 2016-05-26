// ArgoPool.cpp

#include "ArgoPool.h"
// abt.h does the extern "C" wrapping
#include "abt.h"
#include <assert.h>

namespace ares {

  ABT_xstream ArgoPool::xstreams[2];
  ABT_pool ArgoPool::pools[2];

  ArgoPool::ArgoPool(int argc, char *argv[], size_t numStreams) {
    // can only have one instance of ArgoPool
    assert(ABT_initialized() == ABT_ERR_UNINITIALIZED);
    // start ArgoBots
    assert(ABT_init(argc, argv) == ABT_SUCCESS);
    // we are the main ES and ULT
    assert(ABT_xstream_self(&ArgoPool::xstreams[0]) == ABT_SUCCESS);
    // could use main ES but let's create a new one
    assert(ABT_xstream_create(ABT_SCHED_NULL, &xstreams[1]) == ABT_SUCCESS);
    // get the pools in which to allocate threads
    assert(ABT_xstream_get_main_pools(ArgoPool::xstreams[0], 
				      1, 
				      &ArgoPool::pools[0]) 
	   == ABT_SUCCESS);
    assert(ABT_xstream_get_main_pools(ArgoPool::xstreams[1], 
				      1, 
				      &ArgoPool::pools[1]) 
	   == ABT_SUCCESS);
    // think of main thread as 0
    // threadId = 1;
  }

  ArgoPool::~ArgoPool() {
    // initialized?
    assert(ABT_initialized() == ABT_SUCCESS);

    // join user-created ES with main ES
    assert(ABT_xstream_join(ArgoPool::xstreams[1]) == ABT_SUCCESS);
    // free user-created ES
    assert(ABT_xstream_free(&ArgoPool::xstreams[1]) == ABT_SUCCESS);

    // finalize
    assert(ABT_finalize() == ABT_SUCCESS);
  }

  void ArgoPool::AP_push(FuncPtr func, void* argp, uint32_t priority) {
    // new thread on ES[1] vial POOL[1]
    assert(ABT_thread_create(ArgoPool::pools[1],
			     func,
			     argp,
			     ABT_THREAD_ATTR_NULL,
			     NULL)
	   == ABT_SUCCESS);
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
