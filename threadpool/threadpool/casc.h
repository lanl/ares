#ifndef casc_h
#define casc_h
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// -mcx16

typedef unsigned __int128 mword2;
typedef unsigned __int128 mword2align __attribute__ ((aligned (16)));

typedef uintptr_t mword;
typedef uintptr_t mwordalign __attribute__ ((aligned (16)));

// typedef struct __attribute__ ((aligned (16)))

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

inline bool cas128(mword2 *ptr, mword2 expected, mword2 desired) {
  assert(!((mword)ptr & 0x7F) && "cas128 pointer not aligned");
  return __atomic_compare_exchange_n( 
	   ptr, 
	   &expected, // per above exp may get written!
	   desired, 
	   false, 
	   __ATOMIC_SEQ_CST, 
	   __ATOMIC_SEQ_CST );
}

inline bool cas64(mword *ptr, mword expected, mword desired) {
  assert(!((mword)ptr & 0x3F) && "cas64 pointer not aligned");
  return __atomic_compare_exchange_n( 
	   ptr, 
	   &expected, // per above exp may get written!
	   desired, 
	   false, 
	   __ATOMIC_SEQ_CST, 
	   __ATOMIC_SEQ_CST );
}



#endif
