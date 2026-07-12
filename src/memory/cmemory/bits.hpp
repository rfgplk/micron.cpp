#pragma once

#include "../../bits/__arch.hpp"

#include "../../attributes.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../simd/__bits/__vec_ld.hpp"
#include "../../simd/intrin.hpp"
#include "../../simd/memory.hpp"

#include "../stack_constants.hpp"

#if defined(__micron_freestanding)
#include "../stack.hpp"
#endif

#include "../../memory/allocation/abcmalloc/malloc_forward.hpp"

namespace micron
{
// byte threshold below which mem* dispatch uses the scalar fast path
constexpr u64 __simd_dispatch_threshold = 32;

#if !defined(__micron_freestanding)
auto get_stack(void) -> stack_t;
auto get_stack_start(void) -> addr_t *;
auto get_stack_size(void) -> usize;
#endif

inline void
__mem_barrier() noexcept
{
#if defined(__micron_arch_arm32)
  __asm__ __volatile__("dmb ish" ::: "memory");
#elif defined(__micron_arch_arm64)
  __asm__ __volatile__("" ::: "memory");
#elif defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  __asm__ __volatile__("" ::: "memory");
#endif
}

constexpr umax_t
broadcast_byte(byte b) noexcept
{
  if constexpr ( sizeof(umax_t) == 8 ) {
    return static_cast<umax_t>(b) * 0x0101010101010101ULL;
  } else if constexpr ( sizeof(umax_t) == 4 ) {
    return static_cast<umax_t>(b) * 0x01010101UL;
  } else {
    return static_cast<umax_t>(b);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%
// mem* dispatch

// for bulk tiers
#if defined(__micron_x86_avx2)
constexpr u64 __mem_ladder_max = 256;
#else
constexpr u64 __mem_ladder_max = 128;
#endif
constexpr usize __mem_tier_disabled = static_cast<usize>(-1);
// smallest value any rep threshold can probe to
constexpr u64 __mem_rep_min = 2048;
#if !defined(MICRON_MEM_NT_THRESHOLD)
#define MICRON_MEM_NT_THRESHOLD (16u << 20)
#endif
constexpr usize __mem_nt_threshold_default = MICRON_MEM_NT_THRESHOLD;
#if defined(__micron_arch_arm64)
constexpr usize __mem_nt_threshold_arm64 = 8u << 20;
constexpr u64 __mem_zva_min = 256;
#endif

// copy and set want different NT thresholds (intel findings)
struct __mem_tunables {
  usize nt_copy_threshold;
  usize nt_set_threshold;
  usize rep_movsb_threshold;
  usize rep_stosb_threshold;
};

inline constinit __mem_tunables __mem_tun
    = { __mem_nt_threshold_default, __mem_nt_threshold_default, __mem_tier_disabled, __mem_tier_disabled };

// 0 = unprobed
// 1 = a thread is probing
// 2 = published
inline u32 __mem_tun_state = 0;

#if defined(__micron_arch_x86_any) && !defined(MICRON_MEM_NO_PROBE)
[[gnu::cold, gnu::noinline]] inline void
__mem_probe() noexcept
{
  u32 a, b, c, dx;
  __asm__ __volatile__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(dx) : "0"(0u), "2"(0u));
  const u32 max_leaf = a;
  bool erms = false, fsrm = false;
  if ( max_leaf >= 7 ) {
    __asm__ __volatile__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(dx) : "0"(7u), "2"(0u));
    erms = (b >> 9) & 1u;
    fsrm = (dx >> 4) & 1u;
  }
  usize l3 = 0;
  for ( u32 pass = 0; pass < 2 && !l3; ++pass ) {
    const u32 leaf = pass == 0 ? 4u : 0x8000001Du;
    if ( pass == 1 ) {
      __asm__ __volatile__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(dx) : "0"(0x80000000u), "2"(0u));
      if ( a < 0x8000001Du ) break;
    }
    for ( u32 sub = 0; sub < 8; ++sub ) {
      __asm__ __volatile__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(dx) : "0"(leaf), "2"(sub));
      if ( (a & 0x1Fu) == 0 ) break;
      const u32 level = (a >> 5) & 0x7u;
      const usize ways = ((b >> 22) & 0x3FFu) + 1;
      const usize parts = ((b >> 12) & 0x3FFu) + 1;
      const usize line = (b & 0xFFFu) + 1;
      const usize sets = static_cast<usize>(c) + 1;
      if ( level == 3 ) l3 = ways * parts * line * sets;
    }
  }
  const usize floor_1m = 1u << 20;
  const usize nt_copy = l3 ? (((l3 / 4) < floor_1m) ? floor_1m : (l3 / 4)) : static_cast<usize>(__mem_nt_threshold_default);
  const usize nt_set = l3 ? ((((l3 / 4) * 3) < floor_1m) ? floor_1m : ((l3 / 4) * 3)) : static_cast<usize>(__mem_nt_threshold_default);
  usize movsb = __mem_tier_disabled;
  if ( fsrm )
    movsb = 2112;
  else if ( erms )
    movsb = 8192;
  usize stosb = __mem_tier_disabled;
  if ( fsrm )
    stosb = 2048;
  else if ( erms )
    stosb = 16384;
  __mem_tun.nt_copy_threshold = nt_copy;
  __mem_tun.nt_set_threshold = nt_set;
  __mem_tun.rep_movsb_threshold = movsb < nt_copy ? movsb : nt_copy;
  __mem_tun.rep_stosb_threshold = stosb < nt_set ? stosb : nt_set;
  __asm__ __volatile__("" ::: "memory");
}
#endif

[[gnu::always_inline]] static inline const __mem_tunables &
__mem_tun_get() noexcept
{
  // if we're in freestanding mode tunables get primed in _start
#if defined(__micron_arch_x86_any) && !defined(MICRON_MEM_NO_PROBE) && !defined(__micron_freestanding)
  if ( __atomic_load_n(&__mem_tun_state, __ATOMIC_ACQUIRE) != 2u ) [[unlikely]] {
    u32 __exp = 0u;
    if ( __atomic_compare_exchange_n(&__mem_tun_state, &__exp, 1u, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE) ) {
      __mem_probe();
      __atomic_store_n(&__mem_tun_state, 2u, __ATOMIC_RELEASE);
    } else {
      while ( __atomic_load_n(&__mem_tun_state, __ATOMIC_ACQUIRE) != 2u ) __asm__ __volatile__("pause");      // wait for the winner
    }
  }
#endif
  return __mem_tun;
}

struct __mem_route_info_t {
  u64 ladder_max;
  usize rep_movsb;
  usize rep_stosb;
  usize nt_copy;
  usize nt_set;
};

inline __mem_route_info_t
__mem_route_info() noexcept
{
  const __mem_tunables &t = __mem_tun_get();
  return { __mem_ladder_max, t.rep_movsb_threshold, t.rep_stosb_threshold, t.nt_copy_threshold, t.nt_set_threshold };
}

};      // namespace micron

#if defined(__micron_freestanding)
// tunables probe for freestanding
// [[used]] because nothing in our TU references this, might get culled otherwise
extern "C" __attribute__((used)) inline void
__micron_mem_init(void) noexcept
{
#if defined(__micron_arch_x86_any) && !defined(MICRON_MEM_NO_PROBE)
  micron::__mem_probe();
  __atomic_store_n(&micron::__mem_tun_state, 2u, __ATOMIC_RELEASE);
#endif
}
#endif

namespace micron
{

namespace __ml
{

// WARNING:these are cpp loads/stores and not asm
[[gnu::always_inline]] static inline void
__copy_le16(byte *d, const byte *s, const u64 n) noexcept
{
  // n in [1, 16]
  if ( n >= 8 ) {
    const u64 a = __ldu64(s), b = __ldu64(s + n - 8);
    __stu64(d, a);
    __stu64(d + n - 8, b);
  } else if ( n >= 4 ) {
    const u32 a = __ldu32(s), b = __ldu32(s + n - 4);
    __stu32(d, a);
    __stu32(d + n - 4, b);
  } else if ( n >= 2 ) {
    const u16 a = __ldu16(s), b = __ldu16(s + n - 2);
    __stu16(d, a);
    __stu16(d + n - 2, b);
  } else {
    d[0] = s[0];
  }
}

[[gnu::always_inline]] static inline void
__copy_le32(byte *d, const byte *s, const u64 n) noexcept
{
  // n in [0, 32]
  if ( n > 16 ) {
    const __v16 a = __ld16(s), b = __ld16(s + n - 16);
    __st16(d, a);
    __st16(d + n - 16, b);
  } else if ( n ) {
    __copy_le16(d, s, n);
  }
}

[[gnu::always_inline]] static inline void
__copy_33_64(byte *d, const byte *s, const u64 n) noexcept
{
  // n in (32, 64]
#if defined(__micron_x86_avx2)
  const __v32 a = __ld32(s), b = __ld32(s + n - 32);
  __st32(d, a);
  __st32(d + n - 32, b);
#else
  const __v16 a = __ld16(s), b = __ld16(s + 16);
  const __v16 c = __ld16(s + n - 32), e = __ld16(s + n - 16);
  __st16(d, a);
  __st16(d + 16, b);
  __st16(d + n - 32, c);
  __st16(d + n - 16, e);
#endif
}

#if defined(__micron_x86_avx2)
[[gnu::always_inline]] static inline void
__copy_65_128(byte *d, const byte *s, const u64 n) noexcept
{
  // n in (64, 128]
  const __v32 a = __ld32(s), b = __ld32(s + 32);
  const __v32 c = __ld32(s + n - 64), e = __ld32(s + n - 32);
  __st32(d, a);
  __st32(d + 32, b);
  __st32(d + n - 64, c);
  __st32(d + n - 32, e);
}

[[gnu::always_inline]] static inline void
__copy_129_256(byte *d, const byte *s, const u64 n) noexcept
{
  // n in (128, 256]
  const __v32 a = __ld32(s), b = __ld32(s + 32), c = __ld32(s + 64), e = __ld32(s + 96);
  const __v32 f = __ld32(s + n - 128), g = __ld32(s + n - 96), h = __ld32(s + n - 64), i = __ld32(s + n - 32);
  __st32(d, a);
  __st32(d + 32, b);
  __st32(d + 64, c);
  __st32(d + 96, e);
  __st32(d + n - 128, f);
  __st32(d + n - 96, g);
  __st32(d + n - 64, h);
  __st32(d + n - 32, i);
}
#else
[[gnu::always_inline]] static inline void
__copy_65_128(byte *d, const byte *s, const u64 n) noexcept
{
  // n in (64, 128]
  const __v16 a = __ld16(s), b = __ld16(s + 16), c = __ld16(s + 32), e = __ld16(s + 48);
  const __v16 f = __ld16(s + n - 64), g = __ld16(s + n - 48), h = __ld16(s + n - 32), i = __ld16(s + n - 16);
  __st16(d, a);
  __st16(d + 16, b);
  __st16(d + 32, c);
  __st16(d + 48, e);
  __st16(d + n - 64, f);
  __st16(d + n - 48, g);
  __st16(d + n - 32, h);
  __st16(d + n - 16, i);
}
#endif

[[gnu::always_inline]] static inline void
__set_le16(byte *d, const u64 w, const u64 n) noexcept
{
  // n in [1, 16]
  if ( n >= 8 ) {
    __stu64(d, w);
    __stu64(d + n - 8, w);
  } else if ( n >= 4 ) {
    __stu32(d, static_cast<u32>(w));
    __stu32(d + n - 4, static_cast<u32>(w));
  } else if ( n >= 2 ) {
    __stu16(d, static_cast<u16>(w));
    __stu16(d + n - 2, static_cast<u16>(w));
  } else {
    d[0] = static_cast<byte>(w);
  }
}

[[gnu::always_inline]] static inline void
__set_le32(byte *d, const u64 w, const u64 n) noexcept
{
  // n in [0, 32]
  if ( n > 16 ) {
    const __v16 v = { w, w };
    __st16(d, v);
    __st16(d + n - 16, v);
  } else if ( n ) {
    __set_le16(d, w, n);
  }
}

[[gnu::always_inline]] static inline void
__set_33_64(byte *d, const u64 w, const u64 n) noexcept
{
  // n in (32, 64]
#if defined(__micron_x86_avx2)
  const __v32 v = { w, w, w, w };
  __st32(d, v);
  __st32(d + n - 32, v);
#else
  const __v16 v = { w, w };
  __st16(d, v);
  __st16(d + 16, v);
  __st16(d + n - 32, v);
  __st16(d + n - 16, v);
#endif
}

#if defined(__micron_x86_avx2)
[[gnu::always_inline]] static inline void
__set_65_128(byte *d, const u64 w, const u64 n) noexcept
{
  // n in (64, 128]
  const __v32 v = { w, w, w, w };
  __st32(d, v);
  __st32(d + 32, v);
  __st32(d + n - 64, v);
  __st32(d + n - 32, v);
}

[[gnu::always_inline]] static inline void
__set_129_256(byte *d, const u64 w, const u64 n) noexcept
{
  // n in (128, 256]
  const __v32 v = { w, w, w, w };
  __st32(d, v);
  __st32(d + 32, v);
  __st32(d + 64, v);
  __st32(d + 96, v);
  __st32(d + n - 128, v);
  __st32(d + n - 96, v);
  __st32(d + n - 64, v);
  __st32(d + n - 32, v);
}
#else
[[gnu::always_inline]] static inline void
__set_65_128(byte *d, const u64 w, const u64 n) noexcept
{
  // n in (64, 128]
  const __v16 v = { w, w };
  __st16(d, v);
  __st16(d + 16, v);
  __st16(d + 32, v);
  __st16(d + 48, v);
  __st16(d + n - 64, v);
  __st16(d + n - 48, v);
  __st16(d + n - 32, v);
  __st16(d + n - 16, v);
}
#endif

// pattern fill for wordset/typeset
[[gnu::always_inline]] static inline void
__wset_le64(byte *d, const u64 w, const u64 n) noexcept
{
  // n in [1, 64]
  if ( n >= 16 ) {
    const __v16 v = { w, w };
    __st16(d, v);
    if ( n > 16 ) __st16(d + n - 16, v);
    if ( n > 32 ) __st16(d + 16, v);
    if ( n > 48 ) __st16(d + 32, v);
  } else if ( n >= 8 ) {
    __stu64(d, w);
    if ( n > 8 ) __stu64(d + n - 8, w);
  } else {
    const byte *wb = reinterpret_cast<const byte *>(&w);
    for ( u64 i = 0; i < n; i++ ) d[i] = wb[i & 7];
  }
}

};      // namespace __ml

#define stackalloc(x, T) reinterpret_cast<T *>(__builtin_alloca(x));

template<typename F>
constexpr bool
__is_aligned_to(const F *ptr, const u64 alignment) noexcept
{
  return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
};

template<typename F>
constexpr bool
__is_aligned_to_r(const F &ref, const u64 alignment) noexcept
{
  return (reinterpret_cast<uintptr_t>(&ref) % alignment) == 0;
};

template<typename F>
constexpr bool
__is_aligned(const F *ptr) noexcept
{
  return (reinterpret_cast<uintptr_t>(ptr) % alignof(F)) == 0;
};

template<typename F>
constexpr bool
__is_aligned(const F &ref) noexcept
{
  return (reinterpret_cast<uintptr_t>(&ref) % alignof(F)) == 0;
};

template<typename F>
constexpr bool
__is_region_aligned_to(const F *ptr, const u64 size, const u64 alignment) noexcept
{
  return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0 && (size % alignment) == 0;
};

template<typename F>
constexpr bool
__is_aligned_to_s(const F *ptr, const u64 alignment) noexcept
{
  if ( ptr == nullptr ) return false;
  return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
};

template<typename F>
constexpr bool
__is_aligned_s(const F *ptr) noexcept
{
  if ( ptr == nullptr ) return false;
  return (reinterpret_cast<uintptr_t>(ptr) % alignof(F)) == 0;
};

template<typename F>
constexpr bool
__is_region_aligned_to_s(const F *ptr, const u64 size, const u64 alignment) noexcept
{
  if ( ptr == nullptr ) return false;
  return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0 && (size % alignment) == 0;
};

// all of these functions slow you down a lot - be warned!

template<typename F>
bool
__is_at_stack(const F *ptr, const u64 size) noexcept
{
  if ( ptr == nullptr ) return false;
  const uintptr_t a = reinterpret_cast<uintptr_t>(ptr);
  stack_t st = get_stack();
  const uintptr_t lo = reinterpret_cast<uintptr_t>(st.start);
  const uintptr_t hi = lo + st.size;
  if ( a < lo or a > hi or size > static_cast<uintptr_t>(hi - a) ) return false;
  return true;
}

template<typename F>
bool
__is_at_stack(F &ref, const u64 size) noexcept
{
  const uintptr_t a = reinterpret_cast<uintptr_t>(&ref);
  stack_t st = get_stack();
  const uintptr_t lo = reinterpret_cast<uintptr_t>(st.start);
  const uintptr_t hi = lo + st.size;
  if ( a < lo or a > hi or size > static_cast<uintptr_t>(hi - a) ) return false;
  return true;
}

template<typename F>
bool
__is_at_stack(const F &ref, const u64 size) noexcept
{
  const uintptr_t a = reinterpret_cast<uintptr_t>(&ref);
  stack_t st = get_stack();
  const uintptr_t lo = reinterpret_cast<uintptr_t>(st.start);
  const uintptr_t hi = lo + st.size;
  if ( a < lo or a > hi or size > static_cast<uintptr_t>(hi - a) ) return false;
  return true;
}

// rely on within for now
// TODO: implement bounds checking for non abcmalloc allocators

template<typename F>
bool
__is_at_heap(const F *ptr) noexcept
{
  return abc::within(reinterpret_cast<const addr_t *>(ptr));
}

template<typename F>
bool
__is_at_heap(F &ref) noexcept
{
  return abc::within(reinterpret_cast<addr_t *>(ref));
}

template<typename F>
bool
__is_at_heap(const F &ref) noexcept
{
  return abc::within(reinterpret_cast<addr_t *const>(ref));
}

template<typename F>
bool
__is_valid_address(const F *ptr, const u64 size) noexcept
{
  return (__is_at_stack(ptr, size) or __is_at_heap(ptr));
}

template<typename F>
bool
__is_valid_address(F &ref, const u64 size) noexcept
{
  return (__is_at_stack(ref, size) or __is_at_heap(ref));
}

template<typename F>
bool
__is_valid_address(const F &ref, const u64 size) noexcept
{
  return (__is_at_stack(ref, size) or __is_at_heap(ref));
}
};      // namespace micron
