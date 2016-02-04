if(APPLE)
  include_directories(/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include) 

set(CMAKE_CXX_COMPILER ${PROJECT_BINARY_DIR}/frontend/hlir-clang/llvm/bin/clang++)

add_executable(barrier main.cpp)

link_directories(${PROJECT_BINARY_DIR}/runtime)

target_link_libraries(barrier ares_runtime)
