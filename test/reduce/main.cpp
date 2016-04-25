#include <iostream>

#include <ares/frontend.h>

using namespace std;
using namespace ares;

const size_t SIZE = 100;

int main(int argc, char** argv){

  float sum = 0.0;

  for(auto i : ReduceAll(0, SIZE, sum)){
    sum += 1;
  }

  cout << "sum = " << sum << endl;

  return 0;
}
