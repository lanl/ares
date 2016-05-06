
export ABT_TEST_VERBOSE=1

export LD_LIBRARY_PATH=/home/kei/projects/argobots/install/lib/:$LD_LIBRARY_PATH

gcc -pthread thread_yield.c abttest.c -o thread_yield -I../install/include -L../install/lib -labt
