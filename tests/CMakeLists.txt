################################
# Doctest.h
################################
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(runUnitTests BitArray_Test.cpp)
target_include_directories(runUnitTests PUBLIC ${CMAKE_SOURCE_DIR}/)
target_include_directories(runUnitTests PUBLIC ${CMAKE_SOURCE_DIR}/third_party)
target_link_libraries(runUnitTests pthread)
add_test(runUnitTests runUnitTests)
