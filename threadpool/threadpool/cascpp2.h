#ifndef cas_h
#define cas_h
#include <stdint.h>
#include <stdbool.h>

// actual type, integral type
template<typename T, typename U>
bool cas(T *ptr, T expected, T desired) {
  union Union {
    T t;
    U u;
  };
  Union exp, des;
  exp.t = expected;
  des.t = desired;
  //    bool __atomic_compare_exchange_n (type *ptr, 
  //  				    	  type *expected, 
  //  				    	  type desired, 
  //  				    	  bool weak, 
  //  				    	  int success_memorder, 
  //  				    	  int failure_memorder)
  // NOTE! 
  // This built-in function implements an atomic compare and exchange
  // operation. This compares the contents of *ptr with the contents of
  // *expected. If equal, the operation is a read-modify-write operation that
  // writes desired into *ptr. If they are not equal, the operation is a read
  // and the current contents of *ptr is written into *expected. weak is true
  // for weak compare_exchange, and false for the strong variation. Many
  // targets only offer the strong variation and ignore the parameter. When in
  // doubt, use the strong variation.


  return __atomic_compare_exchange_n( (U *)ptr, 
				      &exp.u, // per above exp may get written!
				      des.u, 
				      false, 
				      __ATOMIC_SEQ_CST, 
				      __ATOMIC_SEQ_CST );
}

#endif
