/* -*- Mode: C; c-basic-offset:2 ; indent-tabs-mode:nil ; -*- */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for sleep()
#include <time.h>   // for nanosleep()
#include "abt.h"
#include "abttest.h"

#include<stdbool.h>

#define DEFAULT_NUM_XSTREAMS    4
#define DEFAULT_NUM_THREADS     4

int g_sharedResource = 0;
ABT_mutex queueMutex;
#define queueSize  128
int queue[queueSize];
// head chases tail, head points to head item, tail points to first empty
int queueHead = 0, queueTail = 0;  // empty


bool queuePush(int item) {
  int tp1 = (queueTail + 1) % queueSize;
  if (tp1 == queueHead) return false;  // failed, full
  queue[queueTail] = item;
  queueTail = tp1;
  return true;
}

bool queueEmpty() {
  return (queueHead == queueTail);
}

bool queuePop(int *item) {
  if (queueEmpty()) return false; // failed, empty
  *item = queue[queueHead];
  queueHead = (queueHead + 1) % queueSize;
  return true;
}

void pushOrYield(size_t my_id, int item) {
  int ret;
  bool success = false;
  while (!success) {
    while ((ret = ABT_mutex_trylock(queueMutex)) == ABT_ERR_MUTEX_LOCKED) {
      ABT_test_printf(1, "[TH%lu]: before yield for PUSH\n", my_id);
      ABT_thread_yield();
    }
    assert(ret == ABT_SUCCESS && "error ABT_mutex_trylock(queueMutex)");
    
    success = queuePush(item);
    assert(ABT_mutex_unlock(queueMutex) == ABT_SUCCESS);
    /* end mutex region */
    ABT_test_printf(1, "[TH%lu]: after unlock for PUSH %s\n", 
                    my_id, success ? "succeeded" : "full failed");
  }
}

void pushNoYield(size_t my_id, int item) {
  int ret;
  bool success = false;
  while (!success) {
    ret = ABT_mutex_lock(queueMutex);
    /* start mutex region */
    assert(ret == ABT_SUCCESS && "error ABT_mutex_trylock(queueMutex)");
    success = queuePush(item);
    assert(ABT_mutex_unlock(queueMutex) == ABT_SUCCESS);
    /* end mutex region */
    ABT_test_printf(1, "[TH%lu]: after unlock for PUSH %s\n", 
                    my_id, success ? "succeeded" : "full failed");
  }
}

void popOrYield(size_t my_id, int *item) {
  struct timespec req, rem;
  req.tv_sec = 0;
  req.tv_nsec = 10000;

  int ret;
  bool success = false;
  while (!success) {
    while ((ret = ABT_mutex_trylock(queueMutex)) == ABT_ERR_MUTEX_LOCKED) {
      ABT_test_printf(1, "[TH%lu]: before yield for POP\n", my_id);
      // worker threads only pop so let master have a chance to push work
      nanosleep(&req, &rem);
      ABT_thread_yield();
    }
    assert(ret == ABT_SUCCESS && "error ABT_mutex_trylock(queueMutex)");
    
    success = queuePop(item);
    if (success) g_sharedResource++;
    assert(ABT_mutex_unlock(queueMutex) == ABT_SUCCESS);
    /* end mutex region */
    ABT_test_printf(1, "[TH%lu]: after unlock for POP %s\n", 
                    my_id, success ? "succeeded" : "empty failed");
  }
}


void thread_func(void *arg)
{
  size_t my_id = (size_t)arg;
  int ret;
  
  int item;
  do {
    popOrYield(my_id, &item);
    ABT_test_printf(1, "[TH%lu]: got a %d\n", my_id, item);
  } while (item != -1);
  ABT_test_printf(1, "[TH%lu]: exiting\n", my_id);
}

int main(int argc, char *argv[])
{
  int i, j;
  int ret;
  int num_xstreams = DEFAULT_NUM_XSTREAMS;
  int num_threads = DEFAULT_NUM_THREADS;
  if (argc > 1) num_xstreams = atoi(argv[1]);
  assert(num_xstreams >= 0);
  if (argc > 2) num_threads = atoi(argv[2]);
  assert(num_threads >= 0);
  
  ABT_xstream *xstreams;
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_xstreams);
  
  /* Initialize */
  ABT_test_init(argc, argv);
  
  g_sharedResource = 0;
  /* mutex for g_sharedResource */
  ret = ABT_mutex_create(&queueMutex);
  ABT_TEST_ERROR(ret, "ABT_mutex_create(queueMutex)");
  
  // get primary xstream
  ret = ABT_xstream_self(&xstreams[0]);
  /* create new xtreams */
  ABT_TEST_ERROR(ret, "ABT_xstream_self");
  for (i = 1; i < num_xstreams; i++) {
    ret = ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
    ABT_TEST_ERROR(ret, "ABT_xstream_create");
  }
  
  // get first main pool attached to each execution streams
  ABT_pool *pools;
  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_xstreams);
  for (i = 0; i < num_xstreams; i++) {
    ret = ABT_xstream_get_main_pools(xstreams[i], 1, pools+i);
    ABT_TEST_ERROR(ret, "ABT_xstream_get_main_pools");
  }
  
  // Create threads IN NEW STREAMS
  for (i = 1; i != num_xstreams; i++) {
    for (j = 0; j != num_threads; j++) {
      size_t tid = i * num_threads + j + 1;
      ret = ABT_thread_create(pools[i],
                              thread_func, 
                              (void *)tid, ABT_THREAD_ATTR_NULL,
                              NULL);
      ABT_TEST_ERROR(ret, "ABT_thread_create");
    }
  }
  
  // main thread make work
  for (i = 0; i != 1000; i++) {
    pushNoYield(1000, i);
  }
  /* tell threads to exit */
  for (i = 0; i != num_threads * (num_xstreams - 1); i++) {
    pushNoYield(1000, -1);
  }
  
  /* Join Execution Streams */
  for (i = 1; i < num_xstreams; i++) {
    ret = ABT_xstream_join(xstreams[i]);
    ABT_TEST_ERROR(ret, "ABT_xstream_join");
  }

  ABT_test_printf(1, "final value is %d, should be %d\n", g_sharedResource, num_threads * num_xstreams);
  
  /* Free Execution Streams */
  for (i = 1; i < num_xstreams; i++) {
    ret = ABT_xstream_free(&xstreams[i]);
    ABT_TEST_ERROR(ret, "ABT_xstream_free");
  }
  
  /* free mutex */
  ret = ABT_mutex_free(&queueMutex);
  ABT_TEST_ERROR(ret, "ABT_mutex_free(&queueMutex)");
  
  /* Finalize */
  ret = ABT_test_finalize(0);
  
  free(pools);
  free(xstreams);
  
  return ret;
}
