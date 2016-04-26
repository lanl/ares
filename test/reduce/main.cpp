#include <iostream>

#include <ares/frontend.h>

using namespace std;
using namespace ares;

const size_t SIZE = 100;

int main(int argc, char** argv){

  float sum = 0.0;

  float x = 2.0;

  for(auto i : ReduceAll(0, SIZE, sum)){
    sum += x;
  }

  cout << "sum = " << sum << endl;

  return 0;
}
