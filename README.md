`BitArray`
==========

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](.github/CONTRIBUTING.md#pull-requests)
![C/C++ CI](https://github.com/bagoulla/BitArray/workflows/C/C++%20CI/badge.svg)
[![codecov](https://codecov.io/gh/bagoulla/BitArray/branch/develop/graph/badge.svg?token=3QO0OXSUW6)](https://codecov.io/gh/bagoulla/BitArray)

A packed bit container with utility functions that have SSE2 and AVX2 backing utilizing the GCC target attribute to provide runtime
implementation selection.

## Example

Create a feed forward register and perform a convolution. Users have the option to flush or 
not flush the registers and can store/load the register's final/initial state.

```c++
  bool flush = true;
  BitArray taps("1011011101111011111");
  BitArray input(1024*1024);

  for (size_t i = 0; i < input.size(); ++i)
    input[i] = rand() %2;

  BitArray output(taps.size() + input.size() - 1);
  BitArray::Convolve(taps, input, actualOutput, flush);
```

Perform a dot product across slices of two BitArray's.
The example performs a dot product from `start_a` bit and `start_b`
bit of `testArray1` and `testArray2` respectively for `num` bits.

```c++
  size_t size(1024*1024), start_a(17), start_b(3), num(1024);
  BitArray testArray1(size), testArray2(size);

  // Fill with random bits
  for (size_t i = 0; i < testVec1.size(); ++i) {
    testArray1[i] = rand() % 2;
    testArray2[i] = rand() % 2;
  }

  uint64_t result = BitArray::DotProd(testArray1[start_a], testArray2[start_b], num);
```

## 🚴 Installation

Download the header file from https://github.com/bagoulla/BitArray/releases.

## 🚀 Contributing

### 🐑 Use `git` to Clone this Repository

```
git clone https://github.com/bagoulla/BitArray.git BitArray
```

To check code coverage, make sure you also have
[lcov](http://ltp.sourceforge.net/coverage/lcov.php) installed.

### 🛠️ Build

```
mkdir -p build
cd build
cmake -DBUILD_TESTS=on -DBUILD_BENCHMARKS=on ../
```

### 🔬 Test

```
cd build
ctest
```
