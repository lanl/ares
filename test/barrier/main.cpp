#include <iostream>
#include <cstdlib>

#include <cassert>

#include <ares/runtime.h>

using namespace std;
using namespace ares;

int main(int argc, char** argv){
  assert(argc == 2);

  string type = argv[1];

  if(type == "listen"){
    ares_listen("f1", "f2");
  }
  else if(type == "connect"){
    ares_connect("f2", "f1");
  }
  else{
    assert(false);
  }

  ares_init_comm(2);

  ares_barrier();

  cout << "past barrier 1" << endl;

  if(type == "listen"){
    sleep(2);
  }
  else{
    sleep(5);
  }

  ares_barrier();

  cout << "past barrier 2" << endl;

  return 0;
}
