#include <iostream>

#include <cassert>

#include <ares/runtime.h>

using namespace std;
using namespace ares;

int main(int argc, char** argv){
  assert(argc == 2);

  string type = argv[1];

  if(type == "listen"){
    ares_listen("f1", "f2");
    char* buf = strdup("testmsg");
    ares_send(buf, strlen(buf) + 1);
    sleep(1);
  }
  else if(type == "connect"){
    ares_connect("f2", "f1");
    size_t size;
    char* buf = ares_receive(size);
    cout << buf << endl;
    sleep(1);
  }
  else{
    assert(false);
  }

  return 0;
}
