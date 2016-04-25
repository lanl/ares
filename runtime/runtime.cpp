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

#include <iostream>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <functional>
#include <cassert>
#include <deque>
#include <queue>

#include "ThreadPool.h"
#include "Barrier.h"

#include "communication.h"

using namespace std;
using namespace ares;

#define np(X) _logMutex.lock(); cout << __FILE__ << ":" << __LINE__ << ": " << \
__PRETTY_FUNCTION__ << ": " << #X << " = " << (X) << std::endl; _logMutex.unlock()

namespace{

  mutex _logMutex;

  static const size_t NUM_THREADS = 32;

  class Synch{
  public:
    Synch(int count)
    : sem_(1 - count){}

    void release(){
      sem_.release();
    }

    void await(){
      sem_.acquire();
    }

  private:
    CVSemaphore sem_;
  };

  struct FuncArg{
    FuncArg(Synch* synch, int n, void* args)
      : n(n),
      synch(synch),
      args(args){}

    Synch* synch;
    uint32_t n;
    void* args;
  };

  struct TaskArg{
    TaskArg(Synch* futureSync)
      : futureSync(futureSync){}

    Synch* futureSync;
    uint32_t depth;
  };

  ThreadPool* _threadPool = new ThreadPool(NUM_THREADS);

  Communicator* _communicator = nullptr;

} // namespace

extern "C"{

  void* __ares_alloc(uint64_t bytes){
    return malloc(bytes);
  }

  void __ares_free(void* ptr){
    free(ptr);
  }

  void* __ares_create_synch(uint32_t count){
    return new Synch(count);
  }

  void* __ares_create_barrier(uint32_t count){
    return new Barrier(count);
  }

  void __ares_wait_barrier(void* barrier){
    auto b = static_cast<Barrier*>(barrier);
    b->wait();
  }

  void __ares_delete_barrier(void* barrier){
    auto b = static_cast<Barrier*>(barrier);
    delete b;
  }

  void __ares_queue_func(void* synch, void* args, void* fp,
                         uint32_t index, uint32_t priority){
    _threadPool->push(reinterpret_cast<FuncPtr>(fp),
      new FuncArg(reinterpret_cast<Synch*>(synch), index, args), priority);
  }

  void __ares_finish_func(void* arg){
    auto a = reinterpret_cast<FuncArg*>(arg);
    a->synch->release();
    delete a;
  }

  void __ares_signal_synch(void* sync){
    auto s = reinterpret_cast<Synch*>(sync);
    s->release();
  }

  void __ares_await_synch(void* synch){
    auto s = reinterpret_cast<Synch*>(synch);
    s->await();
    delete s;
  }

  void __ares_task_queue(void* funcPtr, void* argsPtr){
    auto func = reinterpret_cast<FuncPtr>(funcPtr);
    auto args = reinterpret_cast<TaskArg*>(argsPtr);
    args->futureSync = new Synch(0);
    _threadPool->push(func, args, 0);
  }

  void __ares_task_await_future(void* argsPtr){
    auto args = reinterpret_cast<TaskArg*>(argsPtr);
    args->futureSync->await();
  }

  void __ares_task_release_future(void* argsPtr){
    auto args = reinterpret_cast<TaskArg*>(argsPtr);
    args->futureSync->release();
  }

  void __ares_debug(){
    np(9);
  }

  void __ares_debug_ptr(void* ptr){
  }

  void __ares_debug_i32(int32_t x){
    np(x);
  }

  void __ares_debug_f32(float x){
    np(x);
  }

} // extern "C"

namespace ares{

  bool ares_listen(int port){
    assert(!_communicator);
    auto c = new SocketCommunicator;
    _communicator = c;
    return c->listen(port);
  }

  bool ares_listen(const std::string& sendPath, const std::string& receivePath){
    assert(!_communicator);
    auto c = new FIFOCommunicator;
    _communicator = c;
    return c->listen(sendPath, receivePath);
  }

  bool ares_connect(const char* host, int port){
    assert(!_communicator);
    auto c = new SocketCommunicator;
    _communicator = c;
    return c->connect(host, port);  
  }

  bool ares_connect(const std::string& sendPath, const std::string& receivePath){
    assert(!_communicator);
    auto c = new FIFOCommunicator;
    _communicator = c;
    return c->connect(sendPath, receivePath);  
  }

  void ares_send(char* buf, size_t size){
    auto msg = new MessageBuffer(MessageType::Raw, buf, size, true);
    _communicator->send(msg);
  }

  char* ares_receive(size_t& size){
    MessageBuffer* msg = _communicator->receive();
    size = msg->size();
    char* buf = msg->buffer();
    return buf;
  }

  void ares_init_comm(size_t groupSize){
    assert(_communicator);
    _communicator->init(groupSize);
  }

  void ares_barrier(){
    assert(_communicator);
    _communicator->barrier();
  }

} // namespace ares
