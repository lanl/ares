#include <iostream>

#include <ares/frontend.h>

using namespace std;
using namespace ares;

const float MAX_TEMP = 100.0f;
const size_t TIME_STEPS = 100;
const int MESH_DIM = 128;

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

struct Position{
  int x;
  int y;
};

class Mesh{
public:
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

  Position shift(const Position& p, int dx, int dy){
    Position ps;
    ps.x = resolve(p.x, width_);
    ps.y = resolve(p.y, height_);

    return ps;
  }

private:
  int width_;
  int height_;
};

template<class T>
class Field{
public:
  T& operator()(const Position& p){
    return (*this)(p.x, p.y);
  }

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
  HeatMesh m(MESH_DIM, MESH_DIM);
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

  const float dx    = 10.0f / MESH_DIM;
  const float dy    = 10.0f / MESH_DIM;
  const float alpha = 0.00001f;
  const float dt    = 0.5f * (dx * dx+ dy * dy)/4.0f/alpha;
  float u = 0.001f;

  for(size_t i = 0; i < TIME_STEPS; ++i){
    
    for(auto i : Forall(0, m.numCells())){
      auto p = m.position(i);
      auto pw = m.shift(p, -1, 0); 
      auto pe = m.shift(p, 1, 0); 
      auto pn = m.shift(p, 0, 1); 
      auto ps = m.shift(p, 0, -1); 

      float ddx = 0.5 * m.h(pe) - m.h(pw)/dx;

      float d2dx2 = m.h(pn) - 2.0f * m.h(p) + m.h(pw);
      d2dx2 /= dx * dx;

      float d2dy2 = m.h(pn) - 2.0f * m.h(p) + m.h(ps);
      d2dy2 /= dy * dy;

      m.h_next(p) = m.mask(p) * dt * 
        (alpha * (d2dx2 + d2dy2) - m.mask(p) * u * ddx) + m.h(p);
    }

    for(auto i : Forall(0, m.numCells())){
      m.h[i] = m.h_next[i];
    }
  }

  return 0;
}
