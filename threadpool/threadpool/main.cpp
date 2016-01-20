#include "nodepool.h"
#include <stdio.h>

int main() {
  fprintf(stderr, "start\n");
  Pointer<int> ptr, exp, des;
  Node<int> node;
  fprintf(stderr, "sizeof(Pointer<int>) = %zu\n", sizeof(Pointer<int>));
  int i;
  ptr.ptr = &node;
  ptr.count = 99;
  des.ptr = NULL;
  des.count = 101;
  exp = ptr;
  exp.count++;
  if (cas<Pointer<int>, __int128>(&ptr, exp, des)) {
    fprintf(stderr, "cas succeed and ptr is %p %lu\n", ptr.ptr, ptr.count);
  } else {
    fprintf(stderr, "cas failed\n");
  }

  NodePool<int> nodepool;
  fprintf(stderr, "nodepool created\n");
  Node<int> *np = nodepool.take();
  fprintf(stderr, "nodepool.take()\n");
  np->value = 55;
  np->next.ptr = NULL;
  np->next.count = 88;
  fprintf(stderr, "new node %p given values\n", np);
  nodepool.release(np);
  fprintf(stderr, "nodepool.release(..)\n");
  /*
  Queue<int> q;
  q.enqueue(1);
  q.enqueue(2);
  int i;
  q.dequeue(i);
  printf("%d\n", i);
  q.dequeue(i);
  printf("%d\n", i);
  return 0;
  */
}
