cmake_minimum_required(VERSION 3.1.0)
project(BitArray VERSION 0.1.0)

option(BUILD_TESTS "Build test executable" OFF)
option(BUILD_BENCHMARKS "Benchmark BitArray" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as no build type was specified")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type (Debug/Release)" FORCE)
endif (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if (ENABLE_COVERAGE AND NOT CMAKE_CONFIGURATION_TYPES)
    if (NOT BUILD_TESTS)
        message(FATAL_ERROR "Tests must be enabled for code coverage!")
    endif (NOT BUILD_TESTS)

    include(CodeCoverage)

    append_coverage_compiler_flags()
    set(COVERAGE_EXCLUDES "/usr/include/\\*;${CMAKE_SOURCE_DIR}/tests/*;${CMAKE_SOURCE_DIR}/third_party/*")

    setup_target_for_coverage(NAME coverage EXECUTABLE ctest DEPENDENCIES coverage)
endif (ENABLE_COVERAGE AND NOT CMAKE_CONFIGURATION_TYPES)

if (BUILD_TESTS)
  include(CTest)
  add_subdirectory(tests)
endif (BUILD_TESTS)

if (BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif (BUILD_BENCHMARKS)
