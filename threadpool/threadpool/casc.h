#ifndef casc_h
#define casc_h
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// -mcx16

typedef unsigned __int128 dqword;
typedef unsigned __int128 dqwordalign __attribute__ ((aligned (16)));

typedef uintptr_t qword;
typedef uintptr_t qwordalign __attribute__ ((aligned (8)));

// we really want memory values to be reasonably aligned and to not cross
// cache lines

// sysconf (_SC_LEVEL1_DCACHE_LINESIZE)
// getconf LEVEL1_DCACHE_LINESIZE
// getconf LEVEL2_CACHE_LINESIZE
// getconf LEVEL3_CACHE_LINESIZE

// #include <stdlib.h>
// 
// The function posix_memalign() allocates size bytes and places the
// address of the allocated memory in *memptr.  The address of the
// allocated memory will be a multiple of alignment, which must be a
// power of two and a multiple of sizeof(void *).  If size is 0, then
// the value placed in *memptr is either NULL, or a unique pointer value
// that can later be successfully passed to free(3).
//
// posix_memalign() returns zero on success, or one of the error values
// listed in the next section on failure.  The value of errno is
// indeterminate after a call to posix_memalign().
//
// or #include <malloc.h>
//
// void *memalign(size_t alignment, size_t size);
//
// 
// On the assumption that for x86_64 (data) cache lines are 64 bytes
// we only need insure that our pointed-to values are aligned on their
// sizes.

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

inline bool cas128(dqword *ptr, dqword expected, dqword desired) {
  assert(!((qword)ptr & (128-1)) && "cas128 pointer not aligned");
  return __atomic_compare_exchange_n( 
	   ptr, 
	   &expected, // per above exp may get written!
	   desired, 
	   false, 
	   __ATOMIC_SEQ_CST, 
	   __ATOMIC_SEQ_CST );
}

inline bool cas64(qword *ptr, qword expected, qword desired) {
  assert(!((qword)ptr & (64-1)) && "cas64 pointer not aligned");
  return __atomic_compare_exchange_n( 
	   ptr, 
	   &expected, // per above exp may get written!
	   desired, 
	   false, 
	   __ATOMIC_SEQ_CST, 
	   __ATOMIC_SEQ_CST );
}



#endif
