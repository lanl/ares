#include <iostream>

#include <ares/frontend.h>

using namespace std;
using namespace ares;

const float MAX_TEMP = 100.0f;

int resolve(int d, int dim){
  if(d < 0){
    return dim - (-d % dim);
  }
  else if(d < dim){
    return d;
  }
  else{
    return d % dim;
  }
}

class Mesh{
public:
  struct Position{
    int x;
    int y;
  };

  Mesh(int width, int height)
  : width_(width),
  height_(height){}

  int width(){
    return width_;
  }

  int height(){
    return width_;
  }

  int numCells(){
    return width_ * height_;
  }

  Position position(int i){
    Position p;
    p.x = i / width_;
    p.y = i % height_;
    return p;
  }

private:
  int width_;
  int height_;
};

template<class T>
class Field{
public:
  T& operator()(int x, int y){
    int xn = resolve(x, width_);
    int yn = resolve(y, height_);
    return data_[yn * width_ + xn];
  }

  T& operator[](int i){
    return data_[i];
  }

  void init(Mesh& m){
    width_ = m.width();
    height_ = m.height();
    data_ = new T[width_ * height_];
  }

private:
  int width_;
  int height_;
  T* data_;
}; 

class HeatMesh : public Mesh{
public:
  HeatMesh(int width, int height)
  : Mesh(width, height){}

  void init(){
    h.init(*this);
    h_next.init(*this);
    mask.init(*this);
  }

  Field<float> h;
  Field<float> h_next;
  Field<float> mask; 
};

int main(int argc, char** argv){
  HeatMesh m(128, 128);
  m.init();

  for(auto i : Forall(0, m.numCells())){
    auto p = m.position(i);

    m.h[i] = 0.0f;
    m.h_next[i] = 0.0f;
    m.mask[i] = 1.0;

    if(p.y == 0 || p.y == m.height() - 1){
      m.h[i] = MAX_TEMP;
      m.h_next[i] = MAX_TEMP;
      m.mask[i] = 0.0;
    }
  }

  return 0;
}
