cmake_minimum_required(VERSION 3.0)

project(ares_runtime)

find_package (Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

include_directories(${PROJECT_SOURCE_DIR}/../argobots/install/include)

add_library (ares_runtime runtime.cpp)
target_link_libraries (ares_runtime ${CMAKE_THREAD_LIBS_INIT})
