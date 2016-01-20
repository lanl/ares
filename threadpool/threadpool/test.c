#include "nodepool.h"
#include <stdio.h>

int main() {
  IntNodePool queue = newIntNodePool(0);
  printf("got a value %p\n", queue);
}
