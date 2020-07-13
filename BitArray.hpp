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
#include <stdint.h>
#include <functional>
#include <immintrin.h>
#include <stdexcept>
#include <string>
class BitArray;

/**
* Helper functions to count bits for both hardware enabled and non-hardware enabled platforms.
* Not sure why but the mm popcount is only in gcc >= 4.9
*/
#if defined (__GNUC__) && ! defined(__clang__)
#  include <features.h>
#  if __GNUC_PREREQ(4,9)
__attribute__((target("popcnt"))) 
inline int64_t countBits (const uint64_t &a) {  
  return _mm_popcnt_u64(a); 
}
#  else
#  warning Potentially not using the CPU instruction popcount, either due to old version of GCC, not using GCC, or lack of hardware support.
#  endif
#endif


__attribute__((target("default")))
inline int64_t countBits (const uint64_t &a) { return __builtin_popcountll(a); }

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

  /**
  * Assigning a proxybit to another proxybit is just a passthrough to the bool to assignment.
  */
  ProxyBit& operator=(const ProxyBit& other) {
    *this = bool(other);
    return *this;
  }
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
  
  /**
  * Pass through to the vector _data both const and non-const versions
  */
  const uint8_t * data() const {return _data.data(); }
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

  /**
  * Performs a convolution operation on bits using taps and stores it into result. If flush is true
  * zeros will be pushed into the taps at the end resulting in a total output size of taps.size() + 
  * bits.size() - 1. The memory of the registers can optionally be initialized with the initial fill
  * and the final fill will be returned in initialFill.
  */
  __attribute__((target("default"))) 
  static void Convolve(const BitArray &taps, const BitArray &bits, BitArray &result, bool flush=true, uint32_t * pInitialFill=NULL) {
    size_t resultSize = flush ? (taps.size() + bits.size() - 1) : (bits.size());
    uint32_t initialFill = pInitialFill ? *pInitialFill : 0;

    if (result.size() < (resultSize))
      throw std::runtime_error("Results of the convolution must be at least large enough to hold the result");

    if (taps.size() > 32) // TODO: Remove this restriction in the future.
      throw std::runtime_error("Taps greater than 32 bits are currently not supported.");

    uint64_t  tapsReg  = *(uint64_t*) taps.data(); // Never changes or are shifted, these are the taps.
    uint32_t *p_bits32 =  (uint32_t *) bits.data(); // Points to the next 32-bit chunk to copy into the bitsReg
    uint64_t  bitsReg  =  (uint64_t(p_bits32[0]) << (taps.size() - 1)) | initialFill; // Holds the current 64-bits from the input bits, is shifted down until there is room to hold more.
    ++p_bits32;

    size_t ii(0), i(1);
    while (ii < (bits.size()-31)) { // Work through all the bits until we cannot load anymore 32-bit chunks into the register.
      for (; i < 32; ++i, bitsReg >>= 1) { // Shift the bits over, do the AND, then load more!
        result[ii++] = countBits(tapsReg & bitsReg) & 0x01; // mod 2
      }
      bitsReg |= (uint64_t(*(p_bits32++)) << taps.size());
      i=0;
    }

    for (size_t i = 0; resultSize != ii; ++i, bitsReg >>= 1)
      result[ii++] = countBits(tapsReg & bitsReg) & 0x01; // mod 2

    if (pInitialFill)
      *pInitialFill = (uint32_t) bitsReg;
  }

private:
  size_t _size;
  std::vector<uint8_t> _data;
};
#endif
