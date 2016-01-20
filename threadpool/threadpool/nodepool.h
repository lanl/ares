#ifndef nodepool_h
#define nodepool_h

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "cascpp.h"

template <typename T>
struct Node;

template <typename T>
class Pointer {
 public:
  Node<T> *ptr;
  uint64_t count;
  bool operator==(Pointer<T> rhs);
} __attribute__ ((aligned (16)));   //otherwise segfault!

template <typename T>
bool Pointer<T>::operator==(Pointer<T> rhs) {
  return ptr == rhs.ptr && count == rhs.count;
}

template <typename T>
struct Node {
  T value;
  Pointer<T> next;
};

// node pool maintained as lock-free stack
// TODO:  destructors
//        something better than "new" to allocate?

template <typename T>
class NodePool {
public:
  NodePool(size_t _size);
  NodePool();

  void release(Node<T> *p);
  Node<T> *take();

private:
  Pointer<T> tosp;
  size_t size;
};

template <typename T> 
NodePool<T>::NodePool(size_t _size) {
  tosp.ptr = NULL; 
  tosp.count = 0;
  size = _size;
}

template <typename T> 
NodePool<T>::NodePool() {
  tosp.ptr = NULL; 
  tosp.count = 0;
  size = 0;
  fprintf(stderr, "NodePool() &tosp = %p tosp.ptr = %p, tosp.count = %lu\n", 
	  &tosp,
	  tosp.ptr, 
	  tosp.count);
}

template <typename T> 
void NodePool<T>::release(Node<T> *node) {
  do {
    fprintf(stderr, "release(%p) tosp.ptr = %p, tosp.count = %lu\n", 
	    node, tosp.ptr, tosp.count);    
    node->next = tosp;
  } while( !cas<Pointer<T>, __int128>(&tosp, 
				      node->next, 
				      (Pointer<T>){node->next.ptr, 
					           node->next.count+1}));
}

template <typename T>
Node<T> *NodePool<T>::take() {
  Pointer<T> current = tosp;
  while (current.ptr) {
    if ( cas< Pointer<T>, __int128 >(&tosp, 
				     current, 
				     (Pointer<T>){current.ptr->next.ptr, 
					          current.count+1} ) )
      return current.ptr;
  }
  // no nodes in pool, create new one, Pointer value doesn't matter  
  return new Node<T>();
}

// node queue
// Michael and Scott "Simple, Fast, and Practical Non-Blocking and Blocking Concurrent
// Queue Algorithms"
// see also
// http://www.drdobbs.com/lock-free-data-structures-with-hazard-po/184401890
// https://www.research.ibm.com/people/m/michael/ieeetpds-2004.pdf
// http://www.cs.technion.ac.il/~mad/publications/ppopp2013-x86queues.pdf

template <typename T>
class Queue {
 public:
  Queue();
  Queue(NodePool<T> np);
  void enqueue(T value);
  bool dequeue(T &value);
 private:
  Pointer<T> head, tail;
  NodePool<T> nodePool;
  void init();
};

template <typename T>
void Queue<T>::init() {
  Node<T> *nodep = nodePool.take();
  nodep->next.ptr = NULL;
  head.ptr = tail.ptr = nodep;
}

template <typename T>
Queue<T>::Queue() {
  init();
}

template <typename T>
Queue<T>::Queue(NodePool<T> np) : nodePool(np) {
  init();
}

template <typename T>
void Queue<T>::enqueue(T value) {
  Pointer<T> tailtmp;
  Node<T> *nodep = nodePool.take();
  nodep->value = value;
  nodep->next.ptr = NULL;
  while (1) {
    tailtmp = tail;
    Pointer<T> next = tailtmp.ptr->next;
    if (tailtmp == tail) {
      if (next.ptr == NULL) {
	if (cas<Pointer<T>, __int128>(&tailtmp.ptr->next,
	 			      next,
				      (Pointer<T>){nodep, next.count+1}))
	  break;
      } else {
	cas<Pointer<T>, __int128>(&tail, 
				  tailtmp, 
				  (Pointer<T>){next.ptr, tailtmp.count+1});
      }
    }
  }
  cas<Pointer<T>, __int128>(&tail, tailtmp, (Pointer<T>){nodep, tailtmp.count+1});
}

template <typename T>
bool Queue<T>::dequeue(T &value) {
  Pointer<T> headtmp;
  while(1) {
    headtmp = head;
    Pointer<T> tailtmp = tail;
    Pointer<T> next = headtmp.ptr->next;
    if (headtmp == head) {
      if (headtmp.ptr == tailtmp.ptr) {
	if (next.ptr == NULL)
	  return false;
	cas<Pointer<T>, __int128>(&tail, 
				  tailtmp, 
				  (Pointer<T>){next.ptr, tailtmp.count+1});
      } else {
	value = next.ptr->value;
	if (cas<Pointer<T>, __int128>(&head, 
				      headtmp,
				      (Pointer<T>){next.ptr, headtmp.count+1}))
	  break;
      }
    }
  }
  nodePool.release(headtmp.ptr);
  return true;
}


#endif // ifndef nodepool_h
