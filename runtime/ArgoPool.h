/*
 * ###########################################################################
 *  Copyright 2015-2016. Los Alamos National Security, LLC. This software was
 *  produced under U.S. Government contract ??? (LA-CC-15-056) for Los
 *  Alamos National Laboratory (LANL), which is operated by Los Alamos
 *  National Security, LLC for the U.S. Department of Energy. The
 *  U.S. Government has rights to use, reproduce, and distribute this
 *  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY,
 *  LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY
 *  FOR THE USE OF THIS SOFTWARE.  If software is modified to produce
 *  derivative works, such modified software should be clearly marked,
 *  so as not to confuse it with the version available from LANL.
 *
 *  Additionally, redistribution and use in source and binary forms,
 *  with or without modification, are permitted provided that the
 *  following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Los Alamos National Security, LLC, Los
 *      Alamos National Laboratory, LANL, the U.S. Government, nor the
 *      names of its contributors may be used to endorse or promote
 *      products derived from this software without specific prior
 *      written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
 *  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 *  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 * ###########################################################################
 *
 * Notes
 *
 * #####
 */

// export PATH=/home/kei/projects/gcc-5.3.0-install/bin:$PATH

// use gcc-5.3.0 libraries?  see /home/kei/project/gcc-5.3.0-install/README-libs.txt

// g++ -c ArgoPool.cpp -std=c++14 -I../argobots/install/include

#ifndef __ARES_ARGO_POOL_H__
#define __ARES_ARGO_POOL_H__

#include "abt.h"
#include <stdint.h>

namespace ares {

  using FuncPtr = void(*)(void*);
  
  class ArgoPool {
  public:
    
    // initialize an argbots pool and perform any argobots initialization needed
    // assume for now that this class is a singleton - the ARES runtime will
    // create exactly one ArgoPool
    ArgoPool(int argc, char *argv[]);
    ~ArgoPool();
    
    // queue a function pointer for execution with args
    // we will ignore priority for now
    static void AP_push(FuncPtr func, void* argp, uint32_t priority);
    
    // called by an argobots thread to yield
    static void AP_yield();
    
    // called when the argobots thread finishes to perform any cleanup needed
    static void AP_finish();
    
  private:
    
    // this could be static since only one instance of ArgoPool may exist
    //    int threadId;
    static ABT_xstream xstreams[2];
    static ABT_pool pools[2];
  };
  
} // namespace ares

#endif // __ARES_ARGO_POOL_H__
