#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <limits.h>
#include <stdlib.h>
#include <vector>
#include <chrono>
#include "BitArray.hpp"
#include "doctest.h"


namespace {

TEST_CASE("Testing creation") {
    BitArray testEmpty;
    ASSERT_EQ(testEmpty.size(), 0);

    BitArray testNotEmpty(27);
    ASSERT_EQ(testNotEmpty.size(), 27);

    for (size_t i = 0; i < testNotEmpty.size(); i++)
        CHECK(testNotEmpty[i] == 0);
}

TEST_CASE("Testing DotProd") {
    size_t size(5290), start_a(17), start_b(3), num(2370);
    std::vector<short> testVec1(size), testVec2(size);
    BitArray testArray1(size), testArray2(size);
    srand(8);
    for (size_t i = 0; i < testVec1.size(); ++i) {
        testVec1[i] = rand()%2;
        testArray1[i] = testVec1[i];
        CHECK(testVec1[i] == testArray1[i]);
        testVec2[i] = rand()%2;
        testArray2[i] = testVec2[i];
        CHECK(testVec2[i] == testArray2[i]);
    }

    uint64_t expected_value(0);
    for (size_t i = 0; i < num; ++i) {
        expected_value += (testVec1[i+start_a] & testVec2[i+start_b]);
    }

    uint64_t my_value = BitArray::DotProd(testArray1[start_a], testArray2[start_b], num);

    CHECK(my_value == expected_value);
}

}  // namespace

