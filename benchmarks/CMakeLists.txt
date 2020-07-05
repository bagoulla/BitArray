################################
# Picobench.hpp
################################
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(runBenchmarks BitArray_Benchmark.cpp)
target_include_directories(runBenchmarks PUBLIC ${CMAKE_SOURCE_DIR}/)
target_include_directories(runBenchmarks PUBLIC ${CMAKE_SOURCE_DIR}/third_party)
target_link_libraries(runBenchmarks)
add_custom_target(bench
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS runBenchmarks
)
add_custom_command(TARGET bench
    POST_BUILD
    COMMAND runBenchmarks --samples=1 --iters=10
)
