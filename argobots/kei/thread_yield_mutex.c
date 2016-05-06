/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for sleep()
#include "abt.h"
#include "abttest.h"

#define DEFAULT_NUM_XSTREAMS    4
#define DEFAULT_NUM_THREADS     4

ABT_mutex g_mutex;
int g_sharedResource;

void thread_func(void *arg)
{
    size_t my_id = (size_t)arg;
    int ret;

    while ((ret = ABT_mutex_trylock(g_mutex)) == ABT_ERR_MUTEX_LOCKED) {
        ABT_test_printf(1, "[TH%lu]: before yield\n", my_id);
        ABT_thread_yield();
    }
    assert(ret == ABT_SUCCESS && "error ABT_mutex_trylock(g_mutex)");

    /* mutex region */
    ABT_test_printf(1, "[TH%lu]: after yield\n", my_id);
    g_sharedResource++;
    assert(ABT_mutex_unlock(g_mutex) == ABT_SUCCESS);
    /* end mutex region */

    ABT_test_printf(1, "[TH%lu]: after unlock\n", my_id);
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
    ret = ABT_mutex_create(&g_mutex);
    ABT_TEST_ERROR(ret, "ABT_mutex_create(g_mutex)");

    /* Create Execution Streams */
    ret = ABT_xstream_self(&xstreams[0]);
    ABT_TEST_ERROR(ret, "ABT_xstream_self");
    for (i = 1; i < num_xstreams; i++) {
        ret = ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
        ABT_TEST_ERROR(ret, "ABT_xstream_create");
    }

    /* Get the pools attached to an execution stream */
    ABT_pool *pools;
    pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_xstreams);
    for (i = 0; i < num_xstreams; i++) {
        ret = ABT_xstream_get_main_pools(xstreams[i], 1, pools+i);
        ABT_TEST_ERROR(ret, "ABT_xstream_get_main_pools");
    }

    /* Create threads */
    for (i = 0; i < num_xstreams; i++) {
        for (j = 0; j < num_threads; j++) {
            size_t tid = i * num_threads + j + 1;
            ret = ABT_thread_create(pools[i],
                    thread_func, (void *)tid, ABT_THREAD_ATTR_NULL,
                    NULL);
            ABT_TEST_ERROR(ret, "ABT_thread_create");
        }
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
    ret = ABT_mutex_free(&g_mutex);
    ABT_TEST_ERROR(ret, "ABT_mutex_free(&g_mutex)");

    /* Finalize */
    ret = ABT_test_finalize(0);

    free(pools);
    free(xstreams);

    return ret;
}
