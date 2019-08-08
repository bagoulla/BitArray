################################
# GTest
################################
add_executable( runUnitTests Main_TestAll.cpp BitArray_Test.cpp )

target_include_directories(runUnitTests PUBLIC ${CMAKE_SOURCE_DIR}/)
target_link_libraries(runUnitTests gtest pthread)

add_test( runUnitTests runUnitTests )
