#include "queue.h"
#include <stdint.h>

template <class T>
class Queue {
public:
  Queue(size_t _size);
  bool enqueue(

private:
  Queue();
  size_t size;

  struct _Pointer;
  typedef struct _Pointer Pointer;
  struct _Node;
  typedef struct _Node Node;
  struct _Queue;
  typedef struct _Queue Queue;

  struct _Pointer {
    Node *ptr;
    uintptr_t count;
  };

  struct _Node {
    T value;
    Pointer next;
  };

  struct _queue {
    Pointer head, tail;
  };


  //  T element;
  //  T fun(T x) {element = x;}
};

template <class T> Queue<T>::Queue(size_t _size) : size(_size) {};

void *newIntQueue(size_t size) {
  return new Queue<int>(size);
}
