#ifndef _SIMD_HELPERS
#define _SIMD_HELPERS
#include <immintrin.h>
/**
 * Taken from: https://stackoverflow.com/questions/17354971/fast-counting-the-number-of-set-bits-in-m128i-register
 * Performs bitcount on 128 bit register.
 */
__attribute__((target("sse2", "popcnt")))
static inline int popcnt128(__m128i n) {
    const __m128i n_hi = _mm_unpackhi_epi64(n, n);
    return _mm_popcnt_u64(_mm_cvtsi128_si64(n)) + _mm_popcnt_u64(_mm_cvtsi128_si64(n_hi));
}

// These babies are from https://stackoverflow.com/questions/9980801/looking-for-sse-128-bit-shift-operation-for-non-immediate-shift-value

typedef __m128i XMM;
#define xmbshl(x,n)  _mm_slli_si128(x,n) // xm <<= 8*n  -- BYTE shift left
#define xmbshr(x,n)  _mm_srli_si128(x,n) // xm >>= 8*n  -- BYTE shift right
#define xmshl64(x,n) _mm_slli_epi64(x,n) // xm.hi <<= n, xm.lo <<= n
#define xmshr64(x,n) _mm_srli_epi64(x,n) // xm.hi >>= n, xm.lo >>= n
#define xmand(a,b)   _mm_and_si128(a,b)
#define xmor(a,b)    _mm_or_si128(a,b)
#define xmxor(a,b)   _mm_xor_si128(a,b)
#define xmzero       _mm_setzero_si128()

__attribute__((target("sse2")))
XMM xm_shl(XMM x, unsigned nbits)
{
    // These macros generate (1,2,5,6) SSE2 instructions, respectively:
    #define F1L(n) case 8*(n): x = xmbshl(x, n); break;
    #define F2L(n) case n: x = xmshl64(xmbshl(x, (n)>>3), (n)&15); break;
    #define F5L(n) case n: x = xmor(xmshl64(x, n), xmshr64(xmbshl(x, 8), 64-(n))); break;
    #define F6L(n) case n: x = xmor(xmshl64(xmbshl(x, (n)>>3), (n)&15),\
                                  xmshr64(xmbshl(x, 8+((n)>>3)), 64-((n)&155))); break;
    // These macros expand to 7 or 49 cases each:
    #define DO_7L(f,x) f((x)+1) f((x)+2) f((x)+3) f((x)+4) f((x)+5) f((x)+6) f((x)+7)
    #define DO_7x7L(f,y) DO_7L(f,(y)+1*8) DO_7L(f,(y)+2*8) DO_7L(f,(y)+3*8) DO_7L(f,(y)+4*8) \
                                        DO_7L(f,(y)+5*8) DO_7L(f,(y)+6*8) DO_7L(f,(y)+7*8)
    switch (nbits) {
    case 0: break;
    DO_7L(F5L, 0) // 1..7
    DO_7L(F1L, 0) // 8,16,..56
    DO_7L(F1L, 7) // 64,72,..120
    DO_7x7L(F6L, 0) // 9..15 17..23 ... 57..63 i.e. [9..63]\[16,24,..,56]
    DO_7x7L(F2L,56) // 65..71 73..79 ... 121..127 i.e. [65..127]\[64,72,..,120]
    default: x = xmzero;
    }
    return x;
}

__attribute__((target("sse2")))
XMM xm_shr(XMM x, unsigned nbits)
{
    // These macros generate (1,2,5,6) SSE2 instructions, respectively:
    #define F1R(n) case 8*(n): x = xmbshr(x, n); break;
    #define F2R(n) case n: x = xmshr64(xmbshr(x, (n)>>3), (n)&15); break;
    #define F5R(n) case n: x = xmor(xmshr64(x, n), xmshr64(xmbshr(x, 8), 64-(n))); break;
    #define F6R(n) case n: x = xmor(xmshr64(xmbshr(x, (n)>>3), (n)&15),\
                                  xmshr64(xmbshr(x, 8+((n)>>3)), 64-((n)&155))); break;
    // These macros expand to 7 or 49 cases each:
    #define DO_7R(f,x) f((x)+1) f((x)+2) f((x)+3) f((x)+4) f((x)+5) f((x)+6) f((x)+7)
    #define DO_7x7R(f,y) DO_7R(f,(y)+1*8) DO_7R(f,(y)+2*8) DO_7R(f,(y)+3*8) DO_7R(f,(y)+4*8) \
                                        DO_7R(f,(y)+5*8) DO_7R(f,(y)+6*8) DO_7R(f,(y)+7*8)
    switch (nbits) {
    case 0: break;
    DO_7R(F5R, 0) // 1..7
    DO_7R(F1R, 0) // 8,16,..56
    DO_7R(F1R, 7) // 64,72,..120
    DO_7x7R(F6R, 0) // 9..15 17..23 ... 57..63 i.e. [9..63]\[16,24,..,56]
    DO_7x7R(F2R,56) // 65..71 73..79 ... 121..127 i.e. [65..127]\[64,72,..,120]
    default: x = xmzero;
    }
    return x;
}
#endif