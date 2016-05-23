// g++ -c ArgoPool.cpp -std=c++14 -I../argobots/install/include -Wall
// g++ -c test.cpp -std=c++14 -I../argobots/install/include  -Wall
// g++ ArgoPool.o test.o -o test -L../argobots/install/lib/ -labt
// ./test


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
  ArgoPool ap(argc, argv);
  for (intptr_t arg = 1; arg != 11; arg++) {
    ArgoPool::AP_push(fun, (void *)(uintptr_t)arg, 0);
  }
}
