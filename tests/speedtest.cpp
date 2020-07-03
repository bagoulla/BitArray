#include <BitArray.hpp>
#include <iostream>
#include <chrono> 

using namespace std::chrono; 

TEST(SpeedTest, speedtest) {
	using namespace std::chrono;

    size_t size(1024*1024*100), start_a(17), start_b(3), num(size - start_a - start_b);

    BitArray testArray1(size), testArray2(size);
    std::vector<bool> boolArray1(size), boolArray2(size);
    srand(7);
    auto load_start_baseline = high_resolution_clock::now();
    for (size_t i = 0; i < testArray1.size(); ++i) {
        boolArray1[i] = rand()%2;
        boolArray2[i] = rand()%2;
    }
    auto load_stop_baseline = high_resolution_clock::now();
    auto load_duration_baseline = duration_cast<milliseconds>(load_stop_baseline - load_start_baseline);

    srand(7);
    auto load_start = high_resolution_clock::now();
    for (size_t i = 0; i < testArray1.size(); ++i) {
        testArray1[i] = rand()%2;
        testArray2[i] = rand()%2;
    }
    auto load_stop = high_resolution_clock::now();
    auto load_duration = duration_cast<milliseconds>(load_stop - load_start);

	for (size_t i = 0; i < testArray1.size(); ++i) {
        ASSERT_EQ(boolArray1[i], testArray1[i]);
        ASSERT_EQ(boolArray2[i], testArray2[i]);
	}

    std::cout << "Loading a bool vector took: " << load_duration_baseline.count() << "ms" << std::endl;
    std::cout << "Loading a BitArray took: " << load_duration.count() << "ms" << std::endl;

    // Make sure that loading a BitArray is faster than loading a bool vector.
    // This just is not a good test, it depends on platform
    // ASSERT_LT(load_duration, load_duration_baseline);

    uint64_t expected_value(0);
    auto dotprod_start_baseline = high_resolution_clock::now();
    for (size_t i = 0; i < num; ++i)
        expected_value += (boolArray1[i+start_a] & boolArray2[i+start_b]);

    auto dotprod_stop_baseline = high_resolution_clock::now();
    auto dotprod_duration_baseline = duration_cast<milliseconds>(dotprod_stop_baseline - dotprod_start_baseline);

    auto dotprod_start = high_resolution_clock::now();
    uint64_t my_value = BitArray::DotProd(testArray1[start_a], testArray2[start_b], num);
    auto dotprod_stop = high_resolution_clock::now();
    auto dotprod_duration = duration_cast<milliseconds>(dotprod_stop - dotprod_start);

    ASSERT_EQ(my_value, expected_value);

    std::cout << "DotProd a bool vector took: " << dotprod_duration_baseline.count() << "ms" << std::endl;
    std::cout << "DotProd a BitArray took: " << dotprod_duration.count() << "ms" << std::endl;

	ASSERT_LT(dotprod_duration, dotprod_duration_baseline);
}

int main() {
    size_t size(1024*1024*1024), start_a(17), start_b(3), num(size - start_a - start_b);

    BitArray testArray1(size), testArray2(size);
    std::vector<bool> boolArray1(size), boolArray2(size);
    srand(7);
    for (size_t i = 0; i < testArray1.size(); ++i) {
        boolArray1[i] = rand()%2;
        boolArray2[i] = rand()%2;
    }
    auto load_start = high_resolution_clock::now(); 
    for (size_t i = 0; i < testArray1.size(); ++i) {
        testArray1[i] = boolArray1[i];
        testArray2[i] = boolArray2[i];
    }
    auto load_stop = high_resolution_clock::now(); 
    auto load_duration = duration_cast<milliseconds>(load_stop - load_start); 

    auto my_start = high_resolution_clock::now(); 
    uint64_t my_value = BitArray::DotProd(testArray1[start_a], testArray2[start_b], num);
    auto my_stop = high_resolution_clock::now(); 
    auto my_duration = duration_cast<milliseconds>(my_stop - my_start); 

    std::cout << "Loading the array took: " << load_duration.count() << "ms" << std::endl;
    std::cout << "My way took: " << my_duration.count() << "ms" << std::endl;
    std::cout << "Value: " << my_value << std::endl;
}
