cmake_minimum_required(VERSION 3.0)

project(ideas_runtime)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

include_directories(/Users/nickm/kokkos/core/src include)

add_library (ideas_runtime runtime.cpp)

