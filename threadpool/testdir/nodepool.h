#ifndef nodepool_h
#define nodepool_h

#include <stdlib.h>
#include <stdint.h>

struct Node;

class NodePool {
public:
  NodePool(size_t _size);
  NodePool();

  void push(int value);
  bool pop(int& value);

private:

  size_t size;
  class Node;

  class Pointer {
  public:
    Node *ptr;
    uint64_t count;
  };

  Pointer tosp;

  class Node {
  public:
    Node(int value);
    int value;
    Pointer next;
  };

  //  Node *head;

};

#endif // ifndef nodepool_h
