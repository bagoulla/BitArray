#ifndef BITARRAY_H
#define BITARRAY_H

#include <assert.h>
#include <immintrin.h>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

class BitArray;

/**
 * Helper methods to count bits with both hardware and non-hardware versions.
 */
__attribute__((target("popcnt"))) 
inline long long countBits(long long x) { return _mm_popcnt_u64(x); }
__attribute__((target("default"))) 
inline long long countBits(long long x) { return __builtin_popcountll(x); }

/**
 * A class to act as a proxy to a bit in an array.
 * Used to set, and get bits as well as pass a reference
 * to a bit for DotProd
 */
class ProxyBit {
  friend class BitArray;

public:
  virtual ~ProxyBit();
  /**
   * Turns a ProxyBit into a bool
   */
  operator bool() const;

  /**
   * Sets the ProxyBit given a bool
   */
  void operator=(bool b) const;

  /**
   * Assigning a proxybit to another proxybit is just a passthrough to the bool to assignment.
   */
  ProxyBit &operator=(const ProxyBit &other);

private:
  ProxyBit(uint8_t &byte, size_t pos);
  uint8_t &_byte;
  size_t _pos;
};

class BitArray {
public:
  /**
   * Construct a zero filled proxybit of length len bits
   */
  BitArray(size_t len);

  /**
   * An empty, zero length array.
   */
  BitArray();

  /**
   * Create a bit Array as such BitArray("10101000 10101110")
   */
  BitArray(const std::string s);

  /**
   * Returns the size, in bits, of the array.
   */
  size_t size() const;

  /**
   * Pass through to the vector _data both const and non-const versions
   */
  const uint8_t *data() const;
  uint8_t *data();

  /**
   * This will return a proxy to the bit so that users can set it.
   */
  ProxyBit operator[](size_t i);

  /**
   * Performs a dot product on a range of two ProxyBits.
   * Since the two ProxyBits could be offset they get aligned first
   * Both a default, and an SSE2 version exist
   */
  __attribute__((target("default"))) 
  static uint64_t DotProd(const ProxyBit &pb_a, const ProxyBit &pb_b, size_t len);
  __attribute__((target("sse2"))) 
  static uint64_t DotProd(const ProxyBit &pb_a, const ProxyBit &pb_b, size_t len);

  /**
   * Performs a convolution operation on bits using taps and stores it into result. If flush is true
   * zeros will be pushed into the taps at the end resulting in a total output size of taps.size() +
   * bits.size() - 1. The memory of the registers can optionally be initialized with the initial fill
   * and the final fill will be returned in initialFill.
   */
  __attribute__((target("default"))) 
  static void Convolve(const BitArray &taps, const BitArray &bits, BitArray &result, bool flush = true, uint32_t *pInitialFill = NULL);
  __attribute__((target("sse2"))) 
  static void Convolve(const BitArray &taps, const BitArray &bits, BitArray &result, bool flush = true, uint32_t *pInitialFill = NULL);
  __attribute__((target("avx2")))
  static void Convolve(const BitArray &taps, const BitArray &bits, BitArray &result, bool flush = true, uint32_t *pInitialFill = NULL);

private:
  size_t _size;               // Size in bits
  std::vector<uint8_t> _data; // The underlying container holding the bits, TODO: make aligned and perhaps support a constructor that takes a pointer to data.
};

// Private constructor
ProxyBit::ProxyBit(uint8_t &byte, size_t pos) : _byte(byte), _pos(pos) {}

ProxyBit::~ProxyBit() {}

ProxyBit::operator bool() const { return _byte & (1 << _pos); }

void ProxyBit::operator=(bool b) const { _byte = b ? _byte | (1 << _pos) : _byte & ~(1 << _pos); }

ProxyBit &ProxyBit::operator=(const ProxyBit &other) {
  *this = bool(other);
  return *this;
}

// Very lazy, extra 7 bytes of zeros so I do not have to do extra work at the tail end.
BitArray::BitArray(size_t len) : _size(len), _data((_size - 1) / 8 + 1 + 7) {}

BitArray::BitArray() : _size(0), _data(0) {}

BitArray::BitArray(const std::string s) : _size(0) {
  for (size_t i = 0; i < s.size(); ++i)
    if (s[i] == '1' or s[i] == '0')
      ++_size;
  _data.resize((_size - 1) / 8 + 1 + 7);
  size_t idx(0);
  for (size_t i = 0; i < s.size(); ++i)
    if (s[i] == '1' or s[i] == '0') {
      if (s[i] == '1')
        (*this)[idx] = 1;
      ++idx;
    }
}

size_t BitArray::size() const { return _size; }

/**
 * Pass through to the vector _data both const and non-const versions
 */
const uint8_t *BitArray::data() const { return _data.data(); }

uint8_t *BitArray::data() { return _data.data(); }

ProxyBit BitArray::operator[](size_t i) {
  assert(i < _size);
  return ProxyBit(_data[i / 8], (i & 7));
}

__attribute__((target("default"))) 
uint64_t BitArray::DotProd(const ProxyBit &pb_a, const ProxyBit &pb_b, size_t len) {
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
uint64_t BitArray::DotProd(const ProxyBit &pb_a, const ProxyBit &pb_b, size_t len) {
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

    // Shift both right so it is aligned to zero position
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
void BitArray::Convolve(const BitArray &taps, const BitArray &bits, BitArray &result, bool flush, uint32_t *pInitialFill) {
  size_t resultSize = flush ? (taps.size() + bits.size() - 1) : (bits.size());
  uint32_t initialFill = pInitialFill ? *pInitialFill : 0;

  if (result.size() < (resultSize))
    throw std::runtime_error("Results of the convolution must be at least large enough to hold the result");

  if (taps.size() > 32) // TODO: Remove this restriction in the future.
    throw std::runtime_error("Taps greater than 32 bits are currently not supported.");

  uint64_t tapsReg = *(uint64_t *)taps.data();  // Never changes or are shifted, these are the taps.
  uint32_t *p_bits32 = (uint32_t *)bits.data(); // Points to the next 32-bit chunk to copy into the bitsReg
  uint64_t bitsReg = (uint64_t(p_bits32[0]) << (taps.size())) |
                     initialFill; // Holds the current 64-bits from the input bits, is shifted down until there is room to hold more.
  ++p_bits32;

  size_t ii(0);
  while (ii < (bits.size() - 31)) {   // Work through all the bits until we cannot load anymore 32-bit chunks into the register.
    for (size_t i = 0; i < 32; ++i) { // Shift the bits over, do the AND, then load more!
      bitsReg >>= 1;
      result[ii++] = countBits(tapsReg & bitsReg) & 0x01; // mod 2
    }
    bitsReg |= (uint64_t(*(p_bits32++)) << taps.size());
  }

  for (; resultSize != ii; ++ii) {
    bitsReg >>= 1;
    result[ii] = countBits(tapsReg & bitsReg) & 0x01; // mod 2
  }

  if (pInitialFill)
    *pInitialFill = (uint32_t)bitsReg;
}

__attribute__((target("sse2"))) 
void BitArray::Convolve(const BitArray &taps, const BitArray &bits, BitArray &result, bool flush, uint32_t *pInitialFill) {
  uint64_t tapsRegTmp[2], bitsRegTmp[2], resTmp[2];

  size_t resultSize = flush ? (taps.size() + bits.size() - 1) : (bits.size());
  uint32_t initialFill = pInitialFill ? *pInitialFill : 0;

  if (result.size() < (resultSize))
    throw std::runtime_error("Results of the convolution must be at least large enough to hold the result");

  if (taps.size() > 32) // TODO: Remove this restriction in the future.
    throw std::runtime_error("Taps greater than 32 bits are currently not supported.");

  tapsRegTmp[0] = *(uint64_t *)taps.data(); // Never changes or are shifted, these are the taps.
  tapsRegTmp[1] = *(uint64_t *)taps.data();
  __m128i tapsReg128 = _mm_loadu_si128((__m128i const *)&tapsRegTmp);

  uint32_t *p_bits32 = (uint32_t *)bits.data(); // Points to the next 32-bit chunk to copy into the bitsReg

  // We do the first 64 bits seperate so that we can handle the initial fill
  bitsRegTmp[0] = (uint64_t(p_bits32[0]) << (taps.size())) |
                  initialFill; // Holds the current 64-bits from the input bits, is shifted down until there is room to hold more.
  bitsRegTmp[1] = (uint64_t(p_bits32[1]) << (taps.size())) | (p_bits32[0] >> (32 - taps.size()));
  __m128i bitsReg128 = _mm_loadu_si128((__m128i const *)&bitsRegTmp);
  p_bits32 += 2; // Move along the pointer

  // TODO: Check to make sure that the results should be >= 64 bits.
  for (size_t i = 0; i < 32; ++i) { // Shift the bits over, do the AND
    bitsReg128 = _mm_srli_epi64(bitsReg128, 1);
    // print128(bitsReg128);
    __m128i res = _mm_and_si128(tapsReg128, bitsReg128);
    _mm_storeu_si128((__m128i *)&resTmp, res);
    result[i] = countBits(resTmp[0]) & 0x01; // mod 2
    result[i + 32] = countBits(resTmp[1]) & 0x01;
  }

  size_t ii(64);
  for (; ii < bits.size() - 63; ii += 64, p_bits32 += 2) { // Work through all the bits until we cannot load anymore 32-bit chunks into the registers.
    bitsRegTmp[0] = (uint64_t(p_bits32[0]) << (taps.size())) |
                    (p_bits32[-1] >> (32 - taps.size())); // Holds the current 64-bits from the input bits, is shifted down until there is room to hold more.
    bitsRegTmp[1] = (uint64_t(p_bits32[1]) << (taps.size())) | (p_bits32[0] >> (32 - taps.size()));
    bitsReg128 = _mm_loadu_si128((__m128i const *)&bitsRegTmp);
    for (size_t i = 0; i < 32; ++i) { // Shift the bits over, do the AND, then load more!
      bitsReg128 = _mm_srli_epi64(bitsReg128, 1);
      __m128i res = _mm_and_si128(tapsReg128, bitsReg128);
      _mm_storeu_si128((__m128i *)&resTmp, res);
      result[ii + i] = countBits(resTmp[0]) & 0x01; // mod 2
      result[ii + i + 32] = countBits(resTmp[1]) & 0x01;
    }
  }

  // Now we just do the non-SSE version
  uint64_t tapsReg = *(uint64_t *)taps.data(); // Never changes or are shifted, these are the taps.
  uint64_t bitsReg = (uint64_t(p_bits32[0]) << (taps.size())) | (p_bits32[-1] >> (32 - taps.size()));
  for (; ii < resultSize; ii += 32, p_bits32++) {
    bitsReg = (uint64_t(p_bits32[0]) << (taps.size())) | (p_bits32[-1] >> (32 - taps.size()));
    for (size_t i = 0; i < 32; ++i) {
      if ((ii + i) == resultSize)
        break;
      bitsReg >>= 1;
      result[ii + i] = countBits(tapsReg & bitsReg) & 0x01; // mod 2
    }
  }

  if (pInitialFill) {
    *pInitialFill = (uint32_t)bitsReg;
  }
}

__attribute__((target("avx2"))) 
void BitArray::Convolve(const BitArray &taps, const BitArray &bits, BitArray &result, bool flush, uint32_t *pInitialFill) {
  uint64_t tapsRegTmp[4], bitsRegTmp[4], resTmp[4];

  size_t resultSize = flush ? (taps.size() + bits.size() - 1) : (bits.size());
  uint32_t initialFill = pInitialFill ? *pInitialFill : 0;

  if (result.size() < (resultSize))
    throw std::runtime_error("Results of the convolution must be at least large enough to hold the result");

  if (taps.size() > 32) // TODO: Remove this restriction in the future.
    throw std::runtime_error("Taps greater than 32 bits are currently not supported.");

  tapsRegTmp[0] = *(uint64_t *)taps.data(); // Never changes or are shifted, these are the taps.
  tapsRegTmp[1] = *(uint64_t *)taps.data();
  tapsRegTmp[2] = *(uint64_t *)taps.data();
  tapsRegTmp[3] = *(uint64_t *)taps.data();

  __m256i tapsReg256 = _mm256_loadu_si256((__m256i const *)&tapsRegTmp);

  uint32_t *p_bits32 = (uint32_t *)bits.data(); // Points to the next 32-bit chunk to copy into the bitsReg

  // We do the first 64 bits seperate so that we can handle the initial fill
  // Holds the current 64-bits from the input bits, is shifted down until there is room to hold more.
  bitsRegTmp[0] = (uint64_t(p_bits32[0]) << (taps.size())) | initialFill;
  bitsRegTmp[1] = (uint64_t(p_bits32[1]) << (taps.size())) | (p_bits32[0] >> (32 - taps.size()));
  bitsRegTmp[2] = (uint64_t(p_bits32[2]) << (taps.size())) | (p_bits32[1] >> (32 - taps.size()));
  bitsRegTmp[3] = (uint64_t(p_bits32[3]) << (taps.size())) | (p_bits32[2] >> (32 - taps.size()));

  __m256i bitsReg256 = _mm256_loadu_si256((__m256i const *)&bitsRegTmp);
  p_bits32 += 4; // Move along the pointer

  // TODO: Check to make sure that the results should be >= 64 bits.
  for (size_t i = 0; i < 32; ++i) { // Shift the bits over, do the AND
    bitsReg256 = _mm256_srli_epi64(bitsReg256, 1);
    __m256i res = _mm256_and_si256(tapsReg256, bitsReg256);
    _mm256_storeu_si256((__m256i *)&resTmp, res);
    result[i] = countBits(resTmp[0]) & 0x01; // mod 2
    result[i + 32] = countBits(resTmp[1]) & 0x01;
    result[i + 64] = countBits(resTmp[2]) & 0x01;
    result[i + 96] = countBits(resTmp[3]) & 0x01;
  }

  size_t ii(128);
  for (; ii < bits.size() - 127; ii += 128, p_bits32 += 4) { // Work through all the bits until we cannot load anymore 32-bit chunks into the registers.
    bitsRegTmp[0] = (uint64_t(p_bits32[0]) << (taps.size())) |
                    (p_bits32[-1] >> (32 - taps.size())); // Holds the current 64-bits from the input bits, is shifted down until there is room to hold more.
    bitsRegTmp[1] = (uint64_t(p_bits32[1]) << (taps.size())) | (p_bits32[0] >> (32 - taps.size()));
    bitsRegTmp[2] = (uint64_t(p_bits32[2]) << (taps.size())) | (p_bits32[1] >> (32 - taps.size()));
    bitsRegTmp[3] = (uint64_t(p_bits32[3]) << (taps.size())) | (p_bits32[2] >> (32 - taps.size()));

    bitsReg256 = _mm256_loadu_si256((__m256i const *)&bitsRegTmp);
    for (size_t i = 0; i < 32; ++i) { // Shift the bits over, do the AND, then load more!
      bitsReg256 = _mm256_srli_epi64(bitsReg256, 1);
      __m256i res = _mm256_and_si256(tapsReg256, bitsReg256);
      _mm256_storeu_si256((__m256i *)&resTmp, res);
      result[ii + i] = countBits(resTmp[0]) & 0x01; // mod 2
      result[ii + i + 32] = countBits(resTmp[1]) & 0x01;
      result[ii + i + 64] = countBits(resTmp[2]) & 0x01;
      result[ii + i + 96] = countBits(resTmp[3]) & 0x01;
    }
  }

  // Now we just do the non-SSE version
  uint64_t tapsReg = *(uint64_t *)taps.data(); // Never changes or are shifted, these are the taps.
  uint64_t bitsReg = (uint64_t(p_bits32[0]) << (taps.size())) | (p_bits32[-1] >> (32 - taps.size()));
  for (; ii < resultSize; ii += 32, p_bits32++) {
    bitsReg = (uint64_t(p_bits32[0]) << (taps.size())) | (p_bits32[-1] >> (32 - taps.size()));
    for (size_t i = 0; i < 32; ++i) {
      if ((ii + i) == resultSize)
        break;
      bitsReg >>= 1;
      result[ii + i] = countBits(tapsReg & bitsReg) & 0x01; // mod 2
    }
  }

  if (pInitialFill) {
    *pInitialFill = (uint32_t)bitsReg;
  }
}
#endif
