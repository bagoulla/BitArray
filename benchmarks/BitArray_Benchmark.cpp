#define PICOBENCH_IMPLEMENT_WITH_MAIN
#include "BitArray.hpp"
#include "picobench.hpp"
#include <chrono>
#include <iostream>

using namespace std::chrono;

PICOBENCH_SUITE("Load BitArray vs Bool Vector");

static void load_bool_vector(picobench::state &s) {
  size_t size(1024 * 1024 * 100), start_a(17), start_b(3);
  std::vector<bool> boolArray1(size), boolArray2(size);
  BitArray testArray1(size);
  srand(7);
  for (auto _ : s) {
    for (size_t i = 0; i < testArray1.size(); ++i) {
      boolArray1[i] = rand() % 2;
    }
  }
}
PICOBENCH(load_bool_vector);

static void load_bitarray(picobench::state &s) {
  size_t size(1024 * 1024 * 100);
  BitArray testArray1(size), testArray2(size);
  srand(7);
  for (auto _ : s) {
    for (size_t i = 0; i < testArray1.size(); ++i) {
      testArray1[i] = rand() % 2;
    }
  }
}
PICOBENCH(load_bitarray);

PICOBENCH_SUITE("Dot-product BitArray vs bool vector");

static void dotprod_bool_vector(picobench::state &s) {
  size_t size(1024 * 1024 * 100), start_a(17), start_b(3),
      num(size - start_a - start_b);
  std::vector<bool> boolArray1(size), boolArray2(size);
  BitArray testArray1(size);
  srand(7);
  for (size_t i = 0; i < testArray1.size(); ++i) {
    boolArray1[i] = rand() % 2;
    boolArray2[i] = rand() % 2;
  }

  uint64_t expected_value(0);
  for (auto _ : s) {
    for (size_t i = 0; i < num; ++i) {
      expected_value += (boolArray1[i + start_a] & boolArray2[i + start_b]);
    }
  }
  s.set_result(expected_value);
}
PICOBENCH(dotprod_bool_vector);

static void dotprod_bitarray(picobench::state &s) {
  size_t size(1024 * 1024 * 100), start_a(17), start_b(3),
      num(size - start_a - start_b);
  BitArray testArray1(size), testArray2(size);
  srand(7);
  for (size_t i = 0; i < testArray1.size(); ++i) {
    testArray1[i] = rand() % 2;
    testArray2[i] = rand() % 2;
  }
  uint64_t my_value(0);
  for (auto _ : s) {
    my_value = BitArray::DotProd(testArray1[start_a], testArray2[start_b], num);
  }
  s.set_result(my_value);
}
PICOBENCH(dotprod_bitarray);
