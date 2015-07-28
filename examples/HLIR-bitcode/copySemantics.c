/**
 * Compile with:
 * ```
 * clang -S -emit-llvm copySemantics.c
 * ```
 * Add `!hlir.task !2` to every call to `ack`.
 * Append `!2 = !{!"launch"}` to the end of the file.
 */
#include <stdio.h>
#include <unistd.h>

#include "ack.h"

void print_msg(int i) {
  // Do some busy computation...
  int x = ack(4, 1);
  printf("I am iteration %i, and got %i!\n", i, x);
}

int main(int argc, char *argv[]) {
  int i;

  for (i = 0; i < 10; i++) {
    print_msg(i);
  }

  sleep(200;

  return 0;
}
