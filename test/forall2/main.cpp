#include <iostream>

#include <ares/frontend.h>

using namespace std;
using namespace ares;

const size_t N = 10;

int main(int argc, char** argv){

  int a[N];
  int b[N];

  for(int i = 0; i < N; ++i){
    b[i] = i;
  }

  for(auto i : Forall(0, N)){

  }

  for(int i = 0; i < N; ++i){
    cout << a[i] << endl;
  }

  return 0;
}
