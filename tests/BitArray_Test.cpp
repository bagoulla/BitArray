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

TEST_CASE("Testing Correlate with flush") {
  BitArray taps("1011011101111011111");
  BitArray input(1024*1024+13);

  srand(7);
  for (size_t i = 0; i < input.size(); ++i)
    input[i] = rand() %2;

  BitArray expectedOutput(taps.size() + input.size() - 1);
  BitArray actualOutput(taps.size() + input.size() - 1);
  auto t1 = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < expectedOutput.size(); ++i) {
    size_t inputIdx = ( (i < taps.size()) ? 0 : i - taps.size() + 1);
    size_t tapsIdx =  ( (i < taps.size()) ? (taps.size() - i - 1): 0);
    size_t len =      ( (i < taps.size()) ? (i + 1) : taps.size());
    expectedOutput[i] = BitArray::DotProd(taps[tapsIdx], input[inputIdx], len) % 2;
  }
  auto t2 = std::chrono::high_resolution_clock::now();

  BitArray::Convolve(taps, input, actualOutput);
  auto t3 = std::chrono::high_resolution_clock::now();

  auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
  auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>( t3 - t2 ).count();

  std::cout << "Using DotProd took: " << float(duration1)/1e6 << "(s) new convovle took: " << float(duration2)/1e6 << "(s)" << std::endl;

  for (size_t i = 0; i < expectedOutput.size(); ++i) {
    CHECK(expectedOutput[i] == actualOutput[i]);
  }
}

TEST_CASE("Testing Correlate with no flush") {
  BitArray taps("1011011101111011111");

  BitArray input(1024*1024+17);
  for (size_t i = 0; i < input.size(); ++i)
    input[i] = rand() %2;

  BitArray expectedOutput(input.size() );
  BitArray actualOutput(input.size() );
  auto t1 = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < expectedOutput.size(); ++i) {
    size_t inputIdx = ( (i < taps.size()) ? 0 : i - taps.size() + 1);
    size_t tapsIdx =  ( (i < taps.size()) ? (taps.size() - i - 1): 0);
    size_t len =      ( (i < taps.size()) ? (i + 1) : taps.size());
    expectedOutput[i] = BitArray::DotProd(taps[tapsIdx], input[inputIdx], len) % 2;
  }
  auto t2 = std::chrono::high_resolution_clock::now();

  BitArray::Convolve(taps, input, actualOutput, false);
  auto t3 = std::chrono::high_resolution_clock::now();

  auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
  auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>( t3 - t2 ).count();

  std::cout << "Using DotProd took: " << float(duration1)/1e6 << "(s) new convovle took: " << float(duration2)/1e6 << "(s)" << std::endl;

  for (size_t i = 0; i < expectedOutput.size(); ++i) {
    CHECK(expectedOutput[i] == actualOutput[i]);
  }
}

TEST_CASE("Testing Continuous Correlate") {
  BitArray taps("1011011101111011111");

  BitArray continuousInput(1024*1024);
  BitArray   partialInput1(1024*1024/2);
  BitArray   partialInput2(1024*1024/2);

  srand(7);
  for (size_t i = 0; i < continuousInput.size(); ++i)
    continuousInput[i] = rand() % 2;
  
  for (size_t i = 0; i < partialInput1.size(); ++i) {
    partialInput1[i] = continuousInput[i];
    partialInput2[i] = continuousInput[partialInput1.size()+i];
  }

  BitArray expectedOutput(taps.size() + continuousInput.size() - 1);
  BitArray actualOutput1(partialInput1.size());
  BitArray actualOutput2(partialInput2.size() + taps.size() - 1);

  BitArray::Convolve(taps, continuousInput, expectedOutput);
  uint32_t state(0);
  BitArray::Convolve(taps, partialInput1, actualOutput1, false, &state);
  BitArray::Convolve(taps, partialInput2, actualOutput2, true, &state);

  for (size_t i = 0; i < partialInput1.size(); ++i)
    CHECK(expectedOutput[i] == actualOutput1[i]);

  for (size_t i = 0; i < partialInput2.size(); ++i) {
    CHECK(expectedOutput[partialInput1.size()+i] == actualOutput2[i]);
  }
}