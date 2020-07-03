#ifndef BITARRAY_H
#define BITARRAY_H

#include <vector>
#include <endian.h>
#include <assert.h>
#include <functional>
#include <stdint.h>
#include <immintrin.h>
#include <iostream>

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
    operator bool() const {
        return _byte & (1 << _pos);
    }

    /**
     * Sets the ProxyBit given a bool
     */
    void operator=(bool b) const { _byte = b ? _byte | (1<<_pos) : _byte & ~(1<<_pos); }

private:
    ProxyBit(uint8_t &byte, size_t pos) : _byte(byte), _pos(pos) {};
    uint8_t & _byte;
    size_t _pos;
};

class BitArray {

struct SSE2Reg {
    uint64_t upper64;
    uint64_t lower64;
};

public:
    BitArray(size_t len) : _size(len), _data(_size/8 + (1 + 7)) {} // Very lazy, extra 7 bytes of zeros.
    BitArray() : _size(0), _data(0) {}
    size_t size() { return _size; };
    
    /**
     * This will return the value stored in a given index.
     */
    bool operator [](size_t i) const    {
        assert(i < _size);
        return _data[i / 8] & (1 << (i & 7));
    }

    /**
     * This will return a proxy to the bit so that users can set it.
     */
    ProxyBit operator [](size_t i) {
        assert(i < _size);
        return ProxyBit(_data[i / 8], (i & 7));
    }

    /**
     * Performs a dot product on a range of two ProxyBits.
     * Since the two ProxyBits could be offset we need to 
     * align them and do the given operator followed by the sum.
     */
    template<class Func> static uint64_t DotProd(const ProxyBit & pb_a, const ProxyBit & pb_b, size_t len, Func func) {
        uint64_t accum(0);
        uint64_t first_seven_mask = 0x00FFFFFFFFFFFFFF;

        uint8_t * a_ptr = &pb_a._byte;
        uint8_t * b_ptr = &pb_b._byte;

        for (size_t i = 0; len != 0; len -= (7 * 8), i+=7) {

            uint64_t a_64t = (*(uint64_t *) &a_ptr[i]);
            a_64t = (a_64t >> pb_a._pos);

            uint64_t b_64t = (*(uint64_t *) &b_ptr[i]);
            b_64t = (b_64t >> pb_b._pos);

            if (len < 7*8) {
                a_64t = (a_64t << (64 - len));
                b_64t = (b_64t << (64 - len));
                accum += __builtin_popcountll(func(a_64t, b_64t));
                break;
            }

            accum += __builtin_popcountll(first_seven_mask & func(a_64t, b_64t));
        }

        return accum;
     }

     __attribute__ ((target ("sse2", "popcnt")))
     static uint64_t DotProd(const ProxyBit & pb_a, const ProxyBit & pb_b, size_t len) {
        uint64_t accum(0);
        SSE2Reg tmp;
        // __m128i mask = _mm_set_epi8(255, 255, 255, 255, 255, 255, 255, 0, 255, 255, 255, 255, 255, 255, 255, 0);

        // Even with SIMD we still deal with 64-bit integers at a time (for now).
        // We load all we can (2 for SSE2, 4 for AVX etc.) of the first BitArray into the register then bit shift left to
        // align it with the proxy bit in question, do the same with the second BitArray
        // do our and operator; shift right 8 to get rid of the last byte (this is b/c the left shift may have brought
        // 0-7 zero bits in which are not valid), then finally popcount both
        char * p_packedBits_A = (char*) &pb_a._byte;
        char * p_packedBits_B = (char*) &pb_b._byte;

        for (size_t i = 0; len != 0; len -= 2*(7 * 8), i+=14) {
            // We deal with 8 bytes at a time but we drop the last byte, so we need the SSE2 register to have first 8 bytes, 
            // Plus 1 byte of overlap.
            tmp.lower64 = *(uint64_t*) &p_packedBits_A[i];
            tmp.upper64 = *(uint64_t*) &p_packedBits_A[i+7];
            // Things look good here.
            __m128i a_bitVec = _mm_loadu_si128 ((__m128i const *) &tmp);

            // Shift both left so it is aligned to zero position
            a_bitVec = _mm_srli_epi64(a_bitVec, pb_a._pos);


            // Load the next
            tmp.lower64 = *(uint64_t*) &p_packedBits_B[i];
            tmp.upper64 = *(uint64_t*) &p_packedBits_B[i+7];
            __m128i b_bitVec = _mm_loadu_si128 ((__m128i const *) &tmp);

            // Shift so it is also aligned
            b_bitVec = _mm_srli_epi64(b_bitVec, pb_b._pos);

            // bit wise and them
            a_bitVec = _mm_and_si128 (a_bitVec, b_bitVec);

            if (len < 2*7*8) {
                // Get rid of the stuff at the end by shifting it away.
                a_bitVec = _mm_srli_epi64(a_bitVec, (64 - len));
                b_bitVec = _mm_srli_epi64(b_bitVec, (64 - len));
                
                // Bitwise and what is left.
                a_bitVec = _mm_and_si128 (a_bitVec, b_bitVec);
                _mm_storeu_si128 ((__m128i *)&tmp, a_bitVec);
                accum += _mm_popcnt_u64(tmp.lower64);
                accum += _mm_popcnt_u64(tmp.upper64);
                break;
            }

            // Pull out the bits for all but the last byte.
            a_bitVec = _mm_slli_epi64(a_bitVec, 8);
            _mm_storeu_si128 ((__m128i *)&tmp, a_bitVec);
            // _mm_maskmoveu_si128 (a_bitVec, mask, (char*)&tmp); // TODO: I cant get this to work, is it faster than shifts and store?
            accum += _mm_popcnt_u64(tmp.lower64) + _mm_popcnt_u64(tmp.upper64);
        }
        return accum;
     } 

    __attribute__ ((target ("default")))
    static uint64_t DotProd(const ProxyBit & pb_a, const ProxyBit & pb_b, size_t len) {
        return DotProd(pb_a, pb_b, len, std::bit_and<uint64_t>());
    }

private:
    size_t _size;
    std::vector<uint8_t> _data;
};
#endif
