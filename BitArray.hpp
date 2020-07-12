#ifndef BITARRAY_H
#define BITARRAY_H

#include <vector>
#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#else
#include <endian.h>
#endif
#include <assert.h>
#include <cstdint>
#include <functional>
#include <immintrin.h>
#include <iostream>

class BitArray;


/**
* Helper functions to count bits for both hardware enabled and non-hardware enabled platforms.
*/
__attribute__((target("popcnt"))) 
inline int64_t countBits (const uint64_t &a) {  return _mm_popcnt_u64(a); }
__attribute__((target("default")))
inline int64_t countBits (const uint64_t &a) { return __builtin_popcountll(a); }

#include <bitset>
void debugPrint(uint64_t n) {
    std::cout << std::bitset<100>(n);
  }

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
  void operator=(bool b) const { _byte = b ? _byte | (1 << _pos) : _byte & ~(1 << _pos); }

private:
  ProxyBit(uint8_t &byte, size_t pos) : _byte(byte), _pos(pos){};
  uint8_t &_byte;
  size_t _pos;
};


class BitArray {

public:
// TODO: Forward declare everything;

  BitArray(size_t len) : _size(len), _data(_size / 8 + (1 + 7)) {} // Very lazy, extra 7 bytes of zeros.
  BitArray() : _size(0), _data(0) {}
  /**
  * Create a bit Array as such BitArray("10101000 10101110")
  */
  BitArray(const std::string s): _size(0) {
    for (size_t i = 0; i < s.size(); ++i)
      if (s[i] == '1' or s[i] == '0')
        ++_size;
    _data.resize((_size+1)/8);
    size_t idx(0);
    for (size_t i = 0; i < s.size(); ++i)
      if (s[i] == '1' or s[i] == '0') {
        if (s[i] == '1')
          (*this)[idx] = 1;
        ++idx;
      }
  }
  size_t size() const { return _size; };
  uint8_t * data() {return _data.data(); }

  /**
   * This will return a proxy to the bit so that users can set it.
   */
  ProxyBit operator[](size_t i) {
    assert(i < _size);
    return ProxyBit(_data[i / 8], (i & 7));
  }

  /**
   * Performs a dot product on a range of two ProxyBits.
   * Since the two ProxyBits could be offset we need to
   * align them and do the given operator followed by the sum.
   */
  __attribute__((target("default"))) 
  static uint64_t DotProd(const ProxyBit &pb_a, const ProxyBit &pb_b, size_t len) {
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
        a_64t = (a_64t << (64 - len));
        b_64t = (b_64t << (64 - len));
        accum += countBits(a_64t & b_64t);
        break;
      }

      accum += countBits(first_seven_mask & a_64t & b_64t);
    }

    return accum;
  }
  
  __attribute__((target("sse2"))) 
  static uint64_t DotProd(const ProxyBit &pb_a, const ProxyBit &pb_b, size_t len) {
    uint64_t accum(0);
    uint64_t tmp[2];

    // Even with SIMD we still deal with 64-bit integers at a time (for now).
    // We load all we can (2 for SSE2, 4 for AVX etc.) of the first BitArray
    // into the register then bit shift left to align it with the proxy bit in
    // question, do the same with the second BitArray do our and operator; shift
    // right 8 to get rid of the last byte (this is b/c the left shift may have
    // brought 0-7 zero bits in which are not valid), then finally popcount both
    char *p_packedBits_A = (char *)&pb_a._byte;
    char *p_packedBits_B = (char *)&pb_b._byte;

    // We move forward by 112 bytes each time b/c it's 2 sets of 7 bytes.
    size_t i(0);
    for (; len > 112; len -= 112, i += 14) {
      // We deal with 8 bytes at a time but we drop the last byte, so we need
      // the SSE2 register to have first 8 bytes, Plus 1 byte of overlap.
      tmp[1] = *(uint64_t *)&p_packedBits_A[i];
      tmp[0] = *(uint64_t *)&p_packedBits_A[i + 7];
      // Things look good here.
      __m128i a_bitVec = _mm_loadu_si128((__m128i const *)&tmp);

      // Shift both left so it is aligned to zero position
      a_bitVec = _mm_srli_epi64(a_bitVec, pb_a._pos);

      // Load the next
      tmp[1] = *(uint64_t *)&p_packedBits_B[i];
      tmp[0] = *(uint64_t *)&p_packedBits_B[i + 7];
      __m128i b_bitVec = _mm_loadu_si128((__m128i const *)&tmp);

      // Shift so it is also aligned
      b_bitVec = _mm_srli_epi64(b_bitVec, pb_b._pos);

      // bit wise and them
      a_bitVec = _mm_and_si128(a_bitVec, b_bitVec);

      // Pull out the bits for all but the last byte.
      a_bitVec = _mm_slli_epi64(a_bitVec, 8);
      _mm_storeu_si128((__m128i *)&tmp, a_bitVec);
      // _mm_maskmoveu_si128 (a_bitVec, mask, (char*)&tmp); // TODO: I cant get
      // this to work, is it faster than shifts and store?
      accum += countBits(tmp[1]) + countBits(tmp[0]);
    }

    uint64_t first_seven_mask = 0x00FFFFFFFFFFFFFF;

    uint8_t *a_ptr = &pb_a._byte;
    uint8_t *b_ptr = &pb_b._byte;

    // Now we just have to take care of the last, potentially 112 bits.
    // This is just like how we do the non-SSE case.
    for (; len != 0; len -= (7 * 8), i += 7) {
      uint64_t a_64t = (*(uint64_t *)&a_ptr[i]);
      a_64t = (a_64t >> pb_a._pos);

      uint64_t b_64t = (*(uint64_t *)&b_ptr[i]);
      b_64t = (b_64t >> pb_b._pos);

      if (len < 7 * 8) {
        a_64t = (a_64t << (64 - len));
        b_64t = (b_64t << (64 - len));
        accum += countBits(a_64t & b_64t);
        break;
      }

      accum += countBits(first_seven_mask & a_64t & b_64t);
    }
    return accum;
  }

  // TODO: Get these to be const
  __attribute__((target("default"))) 
  static void Convolve(BitArray &taps, BitArray &bits, BitArray &result) {
    if (result.size() < (taps.size() + bits.size() - 1)) {
      throw std::runtime_error("Results of the convolution must be at least large enough to hold the result (taps size + data size - 1)");
    }
    if (taps.size() > 32) { // TODO: Remove this restriction in the future.
      throw std::runtime_error("Taps greater than 32 bits are currently not supported.");
    }

    uint64_t tapsReg = *(uint64_t*) &taps[0]._byte;
    uint32_t *p_bits32 = (uint32_t*) &bits[0]._byte;
    uint64_t bitsReg = (uint64_t(p_bits32[0]) << (taps.size() - 1));
    ++p_bits32;

    size_t ii(0), i(1);
    while (ii < (bits.size()-31)) {
      for (; i < 32; ++i, bitsReg >>= 1) {
        std::cout << "Taps: "; debugPrint(tapsReg); std::cout << std::endl;
        std::cout << "bits: "; debugPrint(bitsReg); std::cout << std::endl;
        std::cout << "ii: " << ii << " bits len: " << bits.size() << " taps size: " << taps.size() << std::endl;
        result[ii++] = countBits(tapsReg & bitsReg) & 0x01; // mod 2
        std::cout << "Result: " << result[ii - 1] << std::endl;
      }
      std::cout << "Trying to load in the next 32 bits" << std::endl;
      bitsReg |= (uint64_t(*(p_bits32++)) << taps.size());
      i=0;
      std::cout << "bits: "; debugPrint(bitsReg); std::cout << std::endl;
    }

    for (size_t i = 0; (bits.size()+taps.size()-1) != ii; ++i, bitsReg >>= 1) {
        std::cout << "Taps: "; debugPrint(tapsReg); std::cout << std::endl;
        std::cout << "bits: "; debugPrint(bitsReg); std::cout << std::endl;
        std::cout << "ii: " << ii << "res: " << result.size() << " bits len: " << bits.size() << " taps size: " << taps.size() << std::endl;
      result[ii++] = countBits(tapsReg & bitsReg) & 0x01; // mod 2
    }
    std::cout << "ii is: " << ii << " expected is one more than: " << (bits.size() + taps.size()) << std::endl;
  }

private:
  size_t _size;
  std::vector<uint8_t> _data;
};
#endif
