/**
 * Compile with:
 * ```
 * clang -O2 -S -emit-llvm phifutures.c
 * ```
 * Make all `tail call` be `call` operations (just to be safe).
 * Add `!hlir.task !2` to every call to `ack`.
 * Append `!2 = !{!"launch"}` to the end of the file.
 */
#include <stdio.h>
#include <unistd.h>

#include "ack.h"

int main(int argc, char *argv[]) {
  int x, y, z;
  int ans;

  if(argc < 3) {
    y = ack(4,1);
    z = ack(3,4);
    if(argc == 1) {
      x = ack(4,1);
    } else {
      x = ack(3,4);
    }
    printf("X is %i on branch 1!\n", x);
  } else {
    y = ack(3,3);
    z = ack(3,6);
    if(argc ==3){
      x = ack(3,5);
    } else {
      x = ack(3,6);
    }
    printf("X is %i on branch 2!\n", x);
  }

  ans = x + y + z;
  printf("The answer is %i!\n", ans);
  return 0;
}
