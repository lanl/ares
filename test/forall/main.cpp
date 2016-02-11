#include <ares/runtime.h>

using namespace std;
using namespace ares;

int main(int argc, char** argv){

  forall(10, [&](int i){
    int j = 9;
  });

  return 0;
}
