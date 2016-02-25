#include <iostream>

#include <ares/frontend.h>

using namespace std;
using namespace ares;

int main(int argc, char** argv){

  for(auto i : Forall(0, 10)){
    int x = i;
  }

  return 0;
}
