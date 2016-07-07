if(APPLE)
  include_directories(/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include) 

set(CMAKE_CXX_COMPILER ${PROJECT_BINARY_DIR}/frontend/hlir-clang/llvm/bin/clang++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

add_executable(forall-nested main.cpp)

link_directories(${PROJECT_BINARY_DIR}/runtime)

target_link_libraries(forall-nested ares_runtime /Users/nickm/argo/install/lib/libabt.a)

add_dependencies(forall-nested clang)
