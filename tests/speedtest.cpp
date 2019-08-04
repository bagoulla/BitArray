#include <BitArray.hpp>
#include <iostream>
#include <chrono> 

using namespace std::chrono; 

int main() {
    size_t size(1024*1024*1024), start_a(17), start_b(3), num(size - start_a - start_b);
    //std::vector<uint8_t> thing1(size), thing2(size);

    //for (size_t i = 0; i < thing1.size(); ++i)
    //    thing1[i] = rand()%2;
    //for (size_t i = 0; i < thing2.size(); ++i)
    //    thing2[i] = rand()%2;

    BitArray testArray1(size), testArray2(size);
    srand(7);
    auto load_start = high_resolution_clock::now(); 
    for (size_t i = 0; i < testArray1.size(); ++i) {
        testArray1[i] = 1;
        testArray2[i] = 1;
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
