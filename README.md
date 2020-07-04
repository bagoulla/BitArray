<div style="text-align:center">

`BitArray`
==========

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](.github/CONTRIBUTING.md#pull-requests)
![C/C++ CI](https://github.com/sayguh/BitArray/workflows/C/C++%20CI/badge.svg)

</div>

## Example

```c++
#include <cstdlib>
#include "BitArray.hpp"

int main() {
  size_t size(5290), start_a(17), start_b(3), num(2370);
  BitArray testArray1(size), testArray2(size);
  srand(8);
  for (size_t i = 0; i < testVec1.size(); ++i) {
    testArray1[i] = rand() % 2;
    testArray2[i] = rand() % 2;
  }

  uint64_t my_value = BitArray::DotProd(testArray1[start_a], testArray2[start_b], num);
  return 0;
}
```

## 🚴 Usage

### 🐑 Use `git` to Clone this Repository

```
git clone https://github.com/sayguh/BitArray.git BitArray
```

### 🛠️ Build

```
mkdir -p build
cd build
cmake ../
```

### 🔬 Test

```
cd build
ctest
```
