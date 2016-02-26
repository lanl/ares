#include <iostream>

#include <ares/frontend.h>

using namespace std;
using namespace ares;

const size_t SIZE = 100;

int main(int argc, char** argv){

  float A[SIZE];
  float B[SIZE];

  for(auto i : Forall(0, SIZE)){
    A[i] = i;
    B[i] = 100*i;
  }

  /*
  for(size_t i = 0; i < SIZE; ++i){
    cout << "A[" << i << "] = " << A[i] << endl;
  }
  */

  return 0;
}
