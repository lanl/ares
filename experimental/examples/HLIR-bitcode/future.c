#include <stdio.h>
#include <unistd.h>

int ack(int m, int n) {
  if(m == 0) {
    return n + 1;
  } else if(n == 0) {
    return ack(m - 1, 1);
  } else {
    return ack(m - 1, ack(m, n - 1));
  }
}

int main(int argc, char *argv[])
{
  int x = ack(3,3);
  int y = ack(4,1);
  int z = ack(4,1);
  int ans = x + y + z;
  printf("The answer is %i\n", ans);
  return 0;
}
