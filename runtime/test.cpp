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

using namespace ares;

void fun(void *arg) {
  int i = (int)(intptr_t)arg;
  fprintf(stderr, "fun(%d) before yield\n", i);
  ArgoPool::AP_yield();
  fprintf(stderr, "fun(%d) after yield\n", i);
}

int main(int argc, char *argv[]) {
  ArgoPool ap(argc, argv, /*numStreams*/ 4);
  for (intptr_t arg = 1; arg != 11; arg++) {
    ArgoPool::AP_push(fun, (void *)(uintptr_t)arg, 0);
  }
}
