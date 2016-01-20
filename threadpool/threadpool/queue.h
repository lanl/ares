#ifndef queue_h
#define queue_h

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
  typedef void *Queue_Work;
  Queue_Work newQueue_Work(size_t);
#ifdef __cplusplus
}
#endif

#endif // ifndef queue_h
