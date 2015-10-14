#include "impl/Kokkos_StaticAssert.hpp"
#include "Kokkos_Threads.hpp"
#include "Threads/Kokkos_ThreadsExec.hpp"

using namespace Kokkos;
using namespace Impl;

namespace{

  using Func = void (*)(char*, char*);
  using ThreadsExecFunc = void (*)(ThreadsExec&, const void*);

  struct FuncArg{
    FuncArg(int n)
      : n(n){}

    int n;
  };

} // namespace

extern "C"{

  void __ideas_queue_func(Func f, int n){
    ThreadsExec::start(reinterpret_cast<ThreadsExecFunc>(f), new FuncArg(n));
  }

  void __ideas_free_func_arg(char* arg){
    delete reinterpret_cast<FuncArg*>(arg);
  }

} // extern "C"
