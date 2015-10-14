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

void print_msg(int i) {
  // Do some busy computation...
  int x = ack(4, 1);
  printf("I am iteration %i, and got %i!\n", i, x);
}

int main(int argc, char *argv[])
{
  int i;

  for (i = 0; i < 10; i++) {
    print_msg(i);
  }

  sleep(200;

  return 0;
}
