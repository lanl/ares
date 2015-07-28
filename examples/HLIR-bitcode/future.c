/**
 * Compile with:
 * ```
 * clang -S -emit-llvm future.c
 * ```
 * Add `!hlir.task !2` to every call to `ack`.
 * Append `!2 = !{!"launch"}` to the end of the file.
 */
#include <stdio.h>
#include <unistd.h>

#include "ack.h"

int main(int argc, char *argv[])
{
  int x = ack(3,3);
  int y = ack(4,1);
  int z = ack(4,1);
  int ans = x + y + z;
  printf("The answer is %i\n", ans);
  return 0;
}
