// g++ -c ArgoPool.cpp -std=c++14 -I../argobots/install/include -Wall
// g++ -c test.cpp -std=c++14 -I../argobots/install/include  -Wall
// g++ ArgoPool.o test.o -o test -L../argobots/install/lib/ -labt
// ./test

/*
  # g++ supporting c++14
  export PATH=/home/kei/projects/gcc-5.3.0-install/bin:$PATH
  # Argobots installation
  export LD_LIBRARY_PATH=/home/kei/projects/argobots/install/lib/:$LD_LIBRARY_PATH 
  g++ -c ArgoPool.cpp -std=c++14 -I../argobots/install/include -Wall
  g++ -c test.cpp -std=c++14 -I../argobots/install/include  -Wall
  g++ test.o ArgoPool.o -o test -L../argobots/install/lib/ -labt
 */


#include "ArgoPool.h"
#include <stdio.h>
#include <unistd.h>

int work1(int i) {
  int k = 0;
  for (int j = 0; j != i; j++)
    k+=j;
  return k;
}

int work2(int i) {
  return i;
}

using namespace ares;

void fun(void *arg) {
  int i = (int)(intptr_t)arg;
  int j = work1(i);
  fprintf(stderr, "fun(%d) before yield %d\n", i, j);
  ArgoPool::AP_yield();
  j = work1(i);
  fprintf(stderr, "fun(%d) after yield %d\n", i, j);
}

int main(int argc, char *argv[]) {
  ArgoPool ap(argc, argv, /*numStreams*/ 16);
  for (intptr_t arg = 1; arg != 1000; arg++) {
    ArgoPool::AP_push(fun, (void *)(uintptr_t)arg, 0);
  }
}
