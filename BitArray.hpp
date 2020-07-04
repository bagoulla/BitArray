#ifndef BITARRAY_H
#define BITARRAY_H

#include <vector>
#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#else
#include <endian.h>
#endif
#include <cstdint>
#include <functional>
#include <sstream>

std::string IntToString(int a) {
  std::ostringstream temp;
  temp << a;
  return temp.str();
}

class BitArray;

/**
 * A class to act as a proxy to a bit in an array.
 * Note that the _pos variable is:
 * Byte: [0 0 0 0 0 0 0 0]
 * _pos   0 1 2 3 4 5 6 7
 *
 */
class ProxyBit {
  friend class BitArray;

public:
  virtual ~ProxyBit(){};
  /**
   * Turns a ProxyBit into a bool
   */
  operator bool() const { return _byte & (1 << _pos); }

  /**
   * Sets the ProxyBit given a bool
   */
  void operator=(bool b) const {
    _byte = b ? _byte | (1 << _pos) : _byte & ~(1 << _pos);
  }

private:
  ProxyBit(uint8_t &byte, size_t pos) : _byte(byte), _pos(pos){};
  uint8_t &_byte;
  size_t _pos;
};

class BitArray {
public:
  BitArray(size_t len)
      : _size(len), _data(_size / 8 + (1 + 7)) {
  } // Very lazy, extra 7 bytes of zeros.
  BitArray() : _size(0), _data(0) {}
  size_t size() { return _size; };

  /**
   * This will return the value stored in a given index.
   */
  bool operator[](size_t i) const {
    if (i >= _size) {
      std::string err = IntToString(i) + " is greater than or equal to size " +
                        IntToString(_size);
      throw std::out_of_range(err);
    }
    return _data[i / 8] & (1 << (i & 7));
  }

  /**
   * This will return a proxy to the bit so that users can set it.
   */
  ProxyBit operator[](size_t i) {
    if (i >= _size) {
      std::string err = IntToString(i) + " is greater than or equal to size " +
                        IntToString(_size);
      throw std::out_of_range(err);
    }
    return ProxyBit(_data[i / 8], (i & 7));
  }

  /**
   * Performs a dot product on a range of two ProxyBits.
   * Since the two ProxyBits could be offset we need to
   * align them and do the given operator followed by the sum.
   */
  template <class Func>
  static uint64_t DotProd(const ProxyBit &pb_a, const ProxyBit &pb_b,
                          size_t len, Func func) {
    uint64_t accum(0);
    uint64_t first_seven_mask = 0x00FFFFFFFFFFFFFF;

    uint8_t *a_ptr = &pb_a._byte;
    uint8_t *b_ptr = &pb_b._byte;

    for (size_t i = 0; len != 0; len -= (7 * 8), i += 7) {
      uint64_t a_64t = (*(uint64_t *)&a_ptr[i]);
      a_64t = (a_64t >> pb_a._pos);

      uint64_t b_64t = (*(uint64_t *)&b_ptr[i]);
      b_64t = (b_64t >> pb_b._pos);

      if (len < 7 * 8) {
        a_64t &= (0xFFFFFFFFFFFFFFFF >> (64 - len));
        b_64t &= (0xFFFFFFFFFFFFFFFF >> (64 - len));
        accum += __builtin_popcountll(first_seven_mask & func(a_64t, b_64t));
        break;
      }

      accum += __builtin_popcountll(first_seven_mask & func(a_64t, b_64t));
    }

    return accum;
  }

  static uint64_t DotProd(const ProxyBit &pb_a, const ProxyBit &pb_b,
                          size_t len) {
    return DotProd(pb_a, pb_b, len, std::bit_and<uint64_t>());
  }

private:
  size_t _size;
  std::vector<uint8_t> _data;
};
#endif
