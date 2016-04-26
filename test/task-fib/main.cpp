#include <iostream>

using namespace std;

task int fib(int i){
  if(i <= 1){
    return i;
  }

  return fib(i - 1) + fib(i - 2);
}

int main(int argc, char** argv){
  int f = fib(9);

  cout << "f = " << f << endl;

  return 0;
}
