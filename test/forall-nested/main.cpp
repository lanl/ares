#include <iostream>

#include <ares/frontend.h>

using namespace std;
using namespace ares;

const size_t SIZE = 10;

int main(int argc, char** argv){
  float A[SIZE];
  float B[SIZE][SIZE];

  for(auto i : Forall(0, SIZE)){
    A[i] = i;

    for(auto j : Forall(0, SIZE)){
      B[i][j] = i + j * 100;
    }
  }

  for(size_t i = 0; i < SIZE; ++i){
    cout << "A[" << i << "] = " << A[i] << endl;
    
    for(size_t j = 0; j < SIZE; ++j){
      cout << "B[" << i << "][" << j << "] = " << B[i][j] << endl;
    }
  }

  return 0;
}
