#include <semaphore.h>

struct s {
  sem_t my_thing;
  int x;
};

int main(int argc, char *argv[])
{
  sem_t mutex;
  struct s x;


  sem_init(&mutex, 0, 1);
  sem_wait(&mutex);

  return 0;
}
