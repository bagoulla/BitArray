#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "BitArray.hpp"
#include "doctest.hpp"
#include <chrono>
#include <limits.h>
#include <stdlib.h>
#include <vector>

TEST_CASE("Testing creation") {
  BitArray testEmpty;
  CHECK(testEmpty.size() == 0);

  BitArray testNotEmpty(27);
  CHECK(testNotEmpty.size() == 27);

  for (size_t i = 0; i < testNotEmpty.size(); i++)
    CHECK(testNotEmpty[i] == 0);
}

TEST_CASE("Testing DotProd") {
  size_t size(5290), start_a(17), start_b(3), num(2370);
  std::vector<short> testVec1(size), testVec2(size);
  BitArray testArray1(size), testArray2(size);
  srand(8);
  for (size_t i = 0; i < testVec1.size(); ++i) {
    testVec1[i] = rand() % 2;
    testArray1[i] = testVec1[i];
    CHECK(testVec1[i] == testArray1[i]);
    testVec2[i] = rand() % 2;
    testArray2[i] = testVec2[i];
    CHECK(testVec2[i] == testArray2[i]);
  }

  uint64_t expected_value(0);
  for (size_t i = 0; i < num; ++i) {
    expected_value += (testVec1[i + start_a] & testVec2[i + start_b]);
  }

  uint64_t my_value =
      BitArray::DotProd(testArray1[start_a], testArray2[start_b], num);

  CHECK(my_value == expected_value);
}

TEST_CASE("Testing Correlate") {
  BitArray taps("1011011101111011111");
  BitArray input("1001100111001111001111100111111001111111001111111110011111111110010011001110011110011111001111110011111110011111111100111111111100100110011100111100111110011111100111111100111111111001111111111001001100111001111001111100111111001111111001111111110011111111110010011001110011110011111001111110011111110011111111100111111111100100110011100111100111110011111100111111100111111111001111111111001");
  // BitArray input("111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
  BitArray expectedOutput(taps.size() + input.size() - 1);
  BitArray actualOutput(taps.size() + input.size() - 1);
  for (size_t i = 0; i < input.size(); ++i)
    expectedOutput[i] = (BitArray::DotProd(taps[0], input[i], taps.size()) % 2);

  BitArray::Convolve(taps, input, actualOutput);

  for (size_t i = 0; i < input.size(); ++i) {
    std::cout << "Checking: " << i << std::endl;
    CHECK(expectedOutput[i] == actualOutput[i+taps.size()-1]);
  }

}
