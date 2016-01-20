project(ARES)

cmake_minimum_required(VERSION 3.0)

set(CMAKE_BUILD_TYPE "DEBUG" CACHE STRING "Debug build")

set(LLVM_TARGETS_TO_BUILD "X86;NVPTX" CACHE STRING "LLVM targets")
set(LLVM_ENABLE_CXX1Y ON CACHE BOOL "Enable C++ 14")
set(LLVM_ENABLE_EH ON CACHE BOOL "Enable exception handling")
set(LLVM_ENABLE_RTTI ON CACHE BOOL "Enable RTTI")

include_directories(${CMAKE_SOURCE_DIR}/hlir/include) 

add_subdirectory(frontend/hlir-clang/llvm)
add_subdirectory(runtime)
