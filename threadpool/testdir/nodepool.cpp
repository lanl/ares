#include "nodepool.h"
#include "cas.h"

 NodePool::NodePool(size_t size) : size(size) {tosp.ptr = NULL; tosp.count = 0;};

 NodePool::NodePool() : size(0) {tosp.ptr = NULL; tosp.count = 0;};

 NodePool::Node::Node(int value) : value(value) {};


void NodePool::push(int value) {
  Node *node = new Node(value);
    do {
      node->next = tosp;
    } while (1); // (!cas128( (__int128 *) &tosp, (__int128) node->next, (__int128)((Pointer){node->next.ptr, node->next.count+1}) ));
}

bool NodePool::pop(int& value) {}

void *newIntNodePool(size_t size) {
  return new NodePool(size);
}
