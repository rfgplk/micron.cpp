//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../types.hpp"
#include "__vector_types_amd64.hpp"

#if !defined(__micron_arch_x86_any)
#error "__asm_blocks_amd64.hpp included on a non-x86 build"
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// inline-asm leaf blocks for memcpy/memset/memcmp
namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wpsabi"
#pragma GCC diagnostic ignored "-Wpedantic"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline
#define __inline_g_T(...) [[gnu::always_inline, gnu::artificial, gnu::target(__VA_ARGS__)]] static inline

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte copy (SSE2)
__inline_g void
__block_copy_16(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  __m128i t;
  __asm__("movdqu %1, %0" : "=x"(t) : "m"(*reinterpret_cast<const __m128i *>(s)));
  __asm__("movdqu %1, %0" : "=m"(*reinterpret_cast<__m128i *>(d)) : "x"(t));
}

__inline_g void
__block_copy_16_a(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  __m128i t;
  __asm__("movdqa %1, %0" : "=x"(t) : "m"(*reinterpret_cast<const __m128i *>(s)));
  __asm__("movdqa %1, %0" : "=m"(*reinterpret_cast<__m128i *>(d)) : "x"(t));
}

__inline_g void
__block_copy_16_nt(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  __m128i t;
  __asm__("movdqu %1, %0" : "=x"(t) : "m"(*reinterpret_cast<const __m128i *>(s)));
  __asm__("movntdq %1, %0" : "=m"(*reinterpret_cast<__m128i *>(d)) : "x"(t));
}

__inline_g void
__block_move_16(u8 *d, const u8 *s) noexcept
{
  __m128i t;
  __asm__("movdqu %1, %0" : "=x"(t) : "m"(*reinterpret_cast<const __m128i *>(s)));
  __asm__("movdqu %1, %0" : "=m"(*reinterpret_cast<__m128i *>(d)) : "x"(t));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte broadcast (SSE2)

__inline_g __m128i
__broadcast_byte_16(u8 b) noexcept
{
  const u32 v32 = static_cast<u32>(b) * 0x01010101u;
  __m128i v;
  __asm__("movd %1, %0\n\t"
          "pshufd $0, %0, %0"
          : "=x"(v)
          : "r"(v32));
  return v;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte word broadcast (SSE2)
__inline_g __m128i
__broadcast_word_16(u64 w) noexcept
{
  __m128i v;
#if defined(__micron_arch_width_64)
  __asm__("movq %1, %0\n\t"
          "punpcklqdq %0, %0"
          : "=x"(v)
          : "r"(w));
#else
  // no 64-bit GPR for movq r64, xmm; build the duplicated-qword vector directly
  v = __extension__(__m128i)(__v2di){ (long long)w, (long long)w };
#endif
  return v;
}

__inline_g void
__block_set_16(u8 *__restrict d, __m128i v) noexcept
{
  __asm__("movdqu %1, %0" : "=m"(*reinterpret_cast<__m128i *>(d)) : "x"(v));
}

__inline_g void
__block_set_16_a(u8 *__restrict d, __m128i v) noexcept
{
  __asm__("movdqa %1, %0" : "=m"(*reinterpret_cast<__m128i *>(d)) : "x"(v));
}

__inline_g void
__block_set_16_nt(u8 *__restrict d, __m128i v) noexcept
{
  __asm__("movntdq %1, %0" : "=m"(*reinterpret_cast<__m128i *>(d)) : "x"(v));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ERMS rep-string leaves
__inline_g void
__rep_movsb(u8 *d, const u8 *s, usize n) noexcept
{
  __asm__ __volatile__("rep movsb" : "+D"(d), "+S"(s), "+c"(n) : : "memory");
}

__inline_g void
__rep_stosb(u8 *d, u8 v, usize n) noexcept
{
  __asm__ __volatile__("rep stosb" : "+D"(d), "+c"(n) : "a"(v) : "memory");
}

__inline_g void
__sfence(void) noexcept
{
  __asm__ __volatile__("sfence" ::: "memory");
}

// no outputs must be volatile
__inline_g void
__prefetch_t0(const u8 *p) noexcept
{
  __asm__("prefetcht0 %0" : : "m"(*p));
}

__inline_g void
__block_copy_16_sa(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  __m128i t;
  __asm__("movdqu %1, %0" : "=x"(t) : "m"(*reinterpret_cast<const __m128i *>(s)));
  __asm__("movdqa %1, %0" : "=m"(*reinterpret_cast<__m128i *>(d)) : "x"(t));
}

__inline_g void
__block_move_16_sa(u8 *d, const u8 *s) noexcept
{
  __m128i t;
  __asm__("movdqu %1, %0" : "=x"(t) : "m"(*reinterpret_cast<const __m128i *>(s)));
  __asm__("movdqa %1, %0" : "=m"(*reinterpret_cast<__m128i *>(d)) : "x"(t));
}

// %%%%%%%%%%%%%%%%%%%%
// 16-byte memcmp early-exit (SSE2)
__inline_g u32
__block_neq_mask_16(const u8 *a, const u8 *b) noexcept
{
  __m128i va, vb;
  __asm__("movdqu %1, %0" : "=x"(va) : "m"(*reinterpret_cast<const __m128i *>(a)));
  __asm__("movdqu %1, %0" : "=x"(vb) : "m"(*reinterpret_cast<const __m128i *>(b)));
  __asm__("pcmpeqb %1, %0" : "+x"(va) : "x"(vb));
  u32 eq_mask;
  __asm__("pmovmskb %1, %0" : "=r"(eq_mask) : "x"(va));
  return (~eq_mask) & 0xFFFFu;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte byte-search mask (SSE2) - powers memchr/memrchr/memmem
__inline_g u32
__block_eq_mask_16(const u8 *p, u8 c) noexcept
{
  const u32 v32 = static_cast<u32>(c) * 0x01010101u;
  __m128i bc, v;
  __asm__("movd %1, %0\n\t"
          "pshufd $0, %0, %0"
          : "=x"(bc)
          : "r"(v32));
  __asm__("movdqu %1, %0" : "=x"(v) : "m"(*reinterpret_cast<const __m128i *>(p)));
  __asm__("pcmpeqb %1, %0" : "+x"(v) : "x"(bc));
  u32 m;
  __asm__("pmovmskb %1, %0" : "=r"(m) : "x"(v));
  return m & 0xFFFFu;
}

__inline_g u32
__block_eq_mask_16_a(const u8 *p, u8 c) noexcept
{
  const u32 v32 = static_cast<u32>(c) * 0x01010101u;
  __m128i bc, v;
  __asm__("movd %1, %0\n\t"
          "pshufd $0, %0, %0"
          : "=x"(bc)
          : "r"(v32));
  __asm__("movdqa %1, %0" : "=x"(v) : "m"(*reinterpret_cast<const __m128i *>(p)));
  __asm__("pcmpeqb %1, %0" : "+x"(v) : "x"(bc));
  u32 m;
  __asm__("pmovmskb %1, %0" : "=r"(m) : "x"(v));
  return m & 0xFFFFu;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 32-byte leaves (AVX2)
#if defined(__micron_x86_avx2)

__inline_g_T("avx2") void __block_copy_32(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  __m256i t;
  __asm__("vmovdqu %1, %0" : "=v"(t) : "m"(*reinterpret_cast<const __m256i *>(s)));
  __asm__("vmovdqu %1, %0" : "=m"(*reinterpret_cast<__m256i *>(d)) : "v"(t));
}

__inline_g_T("avx2") void __block_copy_32_a(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  __m256i t;
  __asm__("vmovdqa %1, %0" : "=v"(t) : "m"(*reinterpret_cast<const __m256i *>(s)));
  __asm__("vmovdqa %1, %0" : "=m"(*reinterpret_cast<__m256i *>(d)) : "v"(t));
}

__inline_g_T("avx2") void __block_copy_32_nt(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  __m256i t;
  __asm__("vmovdqu %1, %0" : "=v"(t) : "m"(*reinterpret_cast<const __m256i *>(s)));
  __asm__("vmovntdq %1, %0" : "=m"(*reinterpret_cast<__m256i *>(d)) : "v"(t));
}

__inline_g_T("avx2") void __block_move_32(u8 *d, const u8 *s) noexcept
{
  __m256i t;
  __asm__("vmovdqu %1, %0" : "=v"(t) : "m"(*reinterpret_cast<const __m256i *>(s)));
  __asm__("vmovdqu %1, %0" : "=m"(*reinterpret_cast<__m256i *>(d)) : "v"(t));
}

// unaligned load + ALIGNED store: bulk-loop bodies (dst pre-aligned)
__inline_g_T("avx2") void __block_copy_32_sa(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  __m256i t;
  __asm__("vmovdqu %1, %0" : "=v"(t) : "m"(*reinterpret_cast<const __m256i *>(s)));
  __asm__("vmovdqa %1, %0" : "=m"(*reinterpret_cast<__m256i *>(d)) : "v"(t));
}

__inline_g_T("avx2") void __block_move_32_sa(u8 *d, const u8 *s) noexcept
{
  __m256i t;
  __asm__("vmovdqu %1, %0" : "=v"(t) : "m"(*reinterpret_cast<const __m256i *>(s)));
  __asm__("vmovdqa %1, %0" : "=m"(*reinterpret_cast<__m256i *>(d)) : "v"(t));
}

__inline_g_T("avx2") __m256i __broadcast_byte_32(u8 b) noexcept
{
  const u32 v32 = static_cast<u32>(b) * 0x01010101u;
  __m256i v;
  __asm__("vmovd %1, %x0\n\t"
          "vpbroadcastb %x0, %0"
          : "=v"(v)
          : "r"(v32));
  return v;
}

// 32-byte word broadcast (AVX2)
__inline_g_T("avx2") __m256i __broadcast_word_32(u64 w) noexcept
{
  __m256i v;
#if defined(__micron_arch_width_64)
  __asm__("vmovq %1, %x0\n\t"
          "vpbroadcastq %x0, %0"
          : "=v"(v)
          : "r"(w));
#else
  // no 64-bit GPR for vmovq r64, xmm; build the broadcasted vector directly
  v = __extension__(__m256i)(__v4di){ (long long)w, (long long)w, (long long)w, (long long)w };
#endif
  return v;
}

__inline_g_T("avx2") void __block_set_32(u8 *__restrict d, __m256i v) noexcept
{
  __asm__("vmovdqu %1, %0" : "=m"(*reinterpret_cast<__m256i *>(d)) : "v"(v));
}

__inline_g_T("avx2") void __block_set_32_a(u8 *__restrict d, __m256i v) noexcept
{
  __asm__("vmovdqa %1, %0" : "=m"(*reinterpret_cast<__m256i *>(d)) : "v"(v));
}

__inline_g_T("avx2") void __block_set_32_nt(u8 *__restrict d, __m256i v) noexcept
{
  __asm__("vmovntdq %1, %0" : "=m"(*reinterpret_cast<__m256i *>(d)) : "v"(v));
}

__inline_g_T("avx2") u32 __block_neq_mask_32(const u8 *a, const u8 *b) noexcept
{
  __m256i va, vb;
  __asm__("vmovdqu %1, %0" : "=v"(va) : "m"(*reinterpret_cast<const __m256i *>(a)));
  __asm__("vmovdqu %1, %0" : "=v"(vb) : "m"(*reinterpret_cast<const __m256i *>(b)));
  __asm__("vpcmpeqb %2, %1, %0" : "=v"(va) : "v"(va), "v"(vb));
  u32 eq_mask;
  __asm__("vpmovmskb %1, %0" : "=r"(eq_mask) : "v"(va));
  return ~eq_mask;
}

__inline_g_T("avx2") u32 __block_eq_mask_32(const u8 *p, u8 c) noexcept
{
  const u32 v32 = static_cast<u32>(c) * 0x01010101u;
  __m256i bc, v;
  __asm__("vmovd %1, %x0\n\t"
          "vpbroadcastb %x0, %0"
          : "=v"(bc)
          : "r"(v32));
  __asm__("vmovdqu %1, %0" : "=v"(v) : "m"(*reinterpret_cast<const __m256i *>(p)));
  __asm__("vpcmpeqb %2, %1, %0" : "=v"(v) : "v"(v), "v"(bc));
  u32 m;
  __asm__("vpmovmskb %1, %0" : "=r"(m) : "v"(v));
  return m;
}

__inline_g_T("avx2") u32 __block_eq_mask_32_a(const u8 *p, u8 c) noexcept
{
  const u32 v32 = static_cast<u32>(c) * 0x01010101u;
  __m256i bc, v;
  __asm__("vmovd %1, %x0\n\t"
          "vpbroadcastb %x0, %0"
          : "=v"(bc)
          : "r"(v32));
  __asm__("vmovdqa %1, %0" : "=v"(v) : "m"(*reinterpret_cast<const __m256i *>(p)));
  __asm__("vpcmpeqb %2, %1, %0" : "=v"(v) : "v"(v), "v"(bc));
  u32 m;
  __asm__("vpmovmskb %1, %0" : "=r"(m) : "v"(v));
  return m;
}

#endif      // __micron_x86_avx2

#undef __inline_g
#undef __inline_g_T

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron
