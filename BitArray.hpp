#ifndef BITARRAY_H
#define BITARRAY_H

#include <bitset>
#include <iostream>
#include <vector>
#include <endian.h>
#include <assert.h>

class BitArray;

/**
 * A class to act as a proxy to a bit in an array.
 * Note that the _pos variable is:
 * Byte: [0 0 0 0 0 0 0 0]
 * _pos   7 6 5 4 3 2 1 0 
 *
 * This can be a little confusing b/c BitArray[0] is _pos 7 of the first ProxyBit.
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
    void operator=(bool b) const { _byte = b ? _byte | (1<<_pos) : _byte &= ~(1<<_pos); }

private:
    ProxyBit(uint8_t &byte, size_t pos) : _byte(byte), _pos(pos) {};
    uint8_t & _byte;
    size_t _pos;
};

class BitArray {
public:
    BitArray(size_t len) : _size(len), _data(_size/8 + 1 + 7) {} // Very lazy, extra 7 bytes of zeros.
    BitArray() : _size(0), _data(0) {}
    size_t size() { return _size; };
    
    /**
     * This will return the value stored in a given index.
     */
    bool operator [](size_t i) const    {
        assert(i < _size);
        return _data[i/8] & (1 << (7 - i%8));
    }

    /**
     * This will return a proxy to the bit so that users can set it.
     */
    ProxyBit operator [](size_t i) {
        assert(i < _size);
        return ProxyBit(_data[i/8], (7-i%8));
    }

    void fast_set_bit(size_t i, bool val) {
        if (val)
            _data[i/8] |= (1<<(7-i%8));
        else
            _data[i/8] &= ~(1<<(7-i%8));
    }

    /**
     * Performs a dot product on a range of two ProxyBits.
     * Since the two ProxyBits could be offset we need to 
     * align them an do the AND operator followed by the sum.
     * Note that we perform these operations on 64-bit ints
     * which on intel platform are little endian, we therefore
     * need to convert them to big endian when doing our adjustments.
     */
    static uint64_t DotProd(const ProxyBit & pb_a, const ProxyBit & pb_b, size_t len) {
        uint64_t accum(0);
        uint64_t first_seven_mask = 0xFFFFFFFFFFFFFF00;

        uint8_t * a_ptr = &pb_a._byte;
        uint8_t * b_ptr = &pb_b._byte;

        size_t i(0);
        for (; len > 7*8; len -= (7*8), i+=7) {
            uint64_t a_64t = htobe64(*(uint64_t *) &a_ptr[i]);
            a_64t = (a_64t << (7 - pb_a._pos));

            uint64_t b_64t = htobe64(*(uint64_t *) &b_ptr[i]);
            b_64t = (b_64t << (7 - pb_b._pos));

            accum += __builtin_popcountll(first_seven_mask & a_64t & b_64t);
        }

        if (len) {
            uint64_t a_64t = htobe64(*(uint64_t *) &a_ptr[i]);
            a_64t = (a_64t << (7 - pb_a._pos));
            a_64t &= (0xFFFFFFFFFFFFFFFF << (64 - len));

            uint64_t b_64t = htobe64(*(uint64_t *) &b_ptr[i]);
            b_64t = (b_64t << (7 - pb_b._pos));
            b_64t &= (0xFFFFFFFFFFFFFFFF << (64 - len));

            accum += __builtin_popcountll(first_seven_mask & a_64t & b_64t);
        }

        return accum;
     } 

private:
    size_t _size;
    std::vector<uint8_t> _data;

};


#endif