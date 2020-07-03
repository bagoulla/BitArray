cmake_minimum_required(VERSION 3.1.0)
project(BitArray)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

enable_testing()
include(CTest)
if(BUILD_TESTING)
  find_package(GTest REQUIRED)
  add_subdirectory(tests)
endif()
