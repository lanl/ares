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

#ifndef __ARES_THREAD_POOL_H__
#define __ARES_THREAD_POOL_H__

#include <vector>
#include <mutex>
#include <functional>
#include <thread>
#include <queue>
#include <cassert>
#include <cmath>

#include <pthread.h>

#include "CVSemaphore.h"

 //#define np(X) std::cout << __FILE__ << ":" << __LINE__ << ": " << \
 __PRETTY_FUNCTION__ << ": " << #X << " = " << (X) << std::endl

namespace ares{

using Func = std::function<void(void*)>;
using FuncPtr = void (*)(void*);

class ThreadPool{
 public:
   class Queue{
   public:

     class Item{
     public:
       Item(Func func, void* arg, uint32_t priority)
       : func(func),
       arg(arg),
       priority(priority){}

       Func func;
       void* arg;
       uint32_t priority;
     };

     void push(Func func, void* arg, uint32_t priority){
       mutex_.lock();
       queue_.push(new Item(func, arg, priority));
       mutex_.unlock();
       
       sem_.release();
     }
     
     Item* get(){
       if(!sem_.acquire()){
         return nullptr;
       }

       mutex_.lock();
       Item* item = queue_.top();
       queue_.pop();
       mutex_.unlock();
       return item;
     }

   private:
     struct Compare_{
       bool operator()(const Item* i1, const Item* i2) const{
         return i1->priority < i2->priority;
       }
     };
     
     typedef std::priority_queue<Item*, std::vector<Item*>, Compare_> Queue_;

     Queue_ queue_;
     CVSemaphore sem_;
     std::mutex mutex_;
   };

   ThreadPool(size_t numThreads){
     start(numThreads);
   }

   void push(Func func, void* arg, uint32_t priority){
     queue_.push(func, arg, priority);
   }

   void start(size_t numThreads){
     for(size_t i = 0; i < numThreads; ++i){
       threadVec_.push_back(new std::thread(&ThreadPool::run_, this));  
     }
   }

   void run_(){
     for(;;){
       Queue::Item* item = queue_.get();
       assert(item);
       item->func(item->arg);
       delete item;
     }
   }

 private:
   using ThreadVec = std::vector<std::thread*>;

   Queue queue_;

   std::mutex mutex_;

   ThreadVec threadVec_;
 };

} // namespace ares

 #endif // __ARES_THREAD_POOL_H__
