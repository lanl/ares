#include <stdio.h>

void print_msg(int i) {
  printf("I am iteration %i!\n", i);
}

int main(int argc, char *argv[])
{
  int i;

  for (i = 0; i < 10; i++) {
    print_msg(i);
  }

  return 0;
}
