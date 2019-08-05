#include <limits.h>
#include "BitArray.hpp"
#include "gtest/gtest.h"
#include <stdlib.h>
#include <vector>

namespace {

TEST(BasicUsage, Creation) {
    size_t size(5290), start_a(17), start_b(3), num(2370);
    std::vector<short> testVec1(size), testVec2(size);
    BitArray testArray1(size), testArray2(size);
    srand(8);
    for (size_t i = 0; i < testVec1.size(); ++i) {
        testVec1[i] = rand()%2;
        testArray1[i] = testVec1[i];
        EXPECT_EQ(testVec1[i], testArray1[i]);
        testVec2[i] = rand()%2;
        testArray2[i] = testVec2[i];
        EXPECT_EQ(testVec2[i], testArray2[i]);
    }

    std::cout << "Hello" << std::endl;
    uint64_t expected_value(0);
    for (size_t i = 0; i < num; ++i) {
        expected_value += (testVec1[i+start_a] & testVec2[i+start_b]);
    }

    uint64_t my_value = BitArray::DotProd(testArray1[start_a], testArray2[start_b], num);

    std::cout << "Expected: " << expected_value << " got: " << my_value << std::endl;
    EXPECT_EQ(my_value, expected_value);
}
}  // namespace

