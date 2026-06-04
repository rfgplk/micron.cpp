//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../simd/aliases.hpp"
#include "../simd/intrin.hpp"
#include "../simd/types.hpp"

#include "../hash/hash.hpp"
#include "../memory/actions.hpp"
#include "../memory/addr.hpp"

#include "../allocator.hpp"
#include "../memory/allocation/resources.hpp"

#include "../concepts.hpp"
#include "../except.hpp"
#include "../tags.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

namespace __impl
{

#if defined(__micron_x86_avx2)
__attribute__((always_inline)) static inline usize
ctrl_scan_avx2(const u8 *__restrict__ p, usize plen) noexcept
{
  __m256i cv = simd::avx::loadu_i256(reinterpret_cast<const __m256i_u *>(p));
  u8 bv = plen <= 255u ? static_cast<u8>(plen) : 255u;
  // static to prevent reconstruction on each loop
  static const __m256i incr = simd::avx::set_i8(31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9,
                                                8, 7, 6, 5, 4, 3, 2, 1, 0);
  static const __m256i flip = simd::avx::splat_i8(static_cast<char>(0x80u));
  __m256i base = simd::avx::splat_i8(static_cast<char>(bv));
  __m256i thres = simd::avx2::add_sat_u8(base, incr);
  __m256i cv_s = simd::avx2::xor_i256(cv, flip);
  __m256i th_s = simd::avx2::xor_i256(thres, flip);
  __m256i gt = simd::avx2::gt_i8(cv_s, th_s);
  uint32_t mask = static_cast<uint32_t>(simd::avx2::movemask_i8(gt));
  if ( mask == 0xFFFF'FFFFu ) return 32;
  return static_cast<usize>(__builtin_ctz(~mask));
}

#elif defined(__micron_x86_sse2)
__attribute__((always_inline)) static inline usize
ctrl_scan_sse2(const u8 *__restrict__ p, usize plen) noexcept
{
  __m128i cv = simd::sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p));
  u8 bv = plen <= 255u ? static_cast<u8>(plen) : 255u;
  // static to prevent reconstruction on each loop
  static const __m128i incr = simd::sse::set_i8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
  static const __m128i flip = simd::sse::splat_i8(static_cast<char>(0x80u));
  __m128i base = simd::sse::splat_i8(static_cast<char>(bv));
  __m128i thres = simd::sse::add_sat_u8(base, incr);
  __m128i cv_s = simd::sse::xor_i128(cv, flip);
  __m128i th_s = simd::sse::xor_i128(thres, flip);
  __m128i gt = simd::sse::gt_i8(cv_s, th_s);
  uint32_t mask = static_cast<uint32_t>(static_cast<uint16_t>(simd::sse::movemask_i8(gt)));
  if ( mask == 0xFFFFu ) return 16;
  return static_cast<usize>(__builtin_ctz(~mask));
}

#elif defined(__micron_arm_neon)
// NEON unsigned compare is direct so no signed-flip trick needed
__attribute__((always_inline)) static inline usize
ctrl_scan_neon(const u8 *__restrict__ p, usize plen) noexcept
{
  uint8x16_t cv = simd::neon::load_u8(p);
  u8 bv = plen <= 255u ? static_cast<u8>(plen) : 255u;
  uint8x16_t base = simd::neon::splat_u8(bv);
  static const u8 incr_arr[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
  uint8x16_t thres = simd::neon::qadd(base, simd::neon::load_u8(incr_arr));
  uint8x16_t gt = simd::neon::gt(cv, thres);
  uint16_t mask = simd::neon::movemask_u8(gt);
  if ( mask == 0xFFFFu ) return 16;
  return static_cast<usize>(__builtin_ctz(~static_cast<uint32_t>(mask)));
}
#endif

// if __ctrl_window=0 revert to scalar
#if defined(__micron_x86_avx2)
static constexpr usize __ctrl_window = 32;

__attribute__((always_inline)) static inline usize
ctrl_scan(const u8 *p, usize plen) noexcept
{
  return ctrl_scan_avx2(p, plen);
}
#elif defined(__micron_x86_sse2)
static constexpr usize __ctrl_window = 16;

__attribute__((always_inline)) static inline usize
ctrl_scan(const u8 *p, usize plen) noexcept
{
  return ctrl_scan_sse2(p, plen);
}
#elif defined(__micron_arm_neon)
static constexpr usize __ctrl_window = 16;

__attribute__((always_inline)) static inline usize
ctrl_scan(const u8 *p, usize plen) noexcept
{
  return ctrl_scan_neon(p, plen);
}
#else
static constexpr usize __ctrl_window = 0;
#endif

#if defined(__micron_x86_avx2)
static constexpr usize __node_alignment = 32;
#else
static constexpr usize __node_alignment = 16;
#endif

#if defined(__micron_x86_avx2)
__attribute__((always_inline)) static inline u32
ctrl_occ_mask_avx2(const u8 *__restrict__ p) noexcept
{
  __m256i cv = simd::avx::loadu_i256(reinterpret_cast<const __m256i_u *>(p));
  __m256i zero = simd::avx::splat_i8(0);
  __m256i eq = simd::avx2::eq_i8(cv, zero);
  u32 zero_mask = static_cast<u32>(simd::avx2::movemask_i8(eq));
  return ~zero_mask;
}
#endif

#if defined(__micron_arch_x86_any)
__attribute__((always_inline)) static inline u32
ctrl_occ_mask_sse2(const u8 *__restrict__ p) noexcept
{
  __m128i cv = simd::sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p));
  __m128i zero = simd::sse::splat_i8(0);
  __m128i eq = simd::sse::eq_i8(cv, zero);
  u32 zero_mask = static_cast<u32>(static_cast<u16>(simd::sse::movemask_i8(eq)));
  return (~zero_mask) & 0xFFFFu;
}
#elif defined(__micron_arm_neon)
__attribute__((always_inline)) static inline u32
ctrl_occ_mask_neon(const u8 *__restrict__ p) noexcept
{
  uint8x16_t cv = simd::neon::load_u8(p);
  uint8x16_t zero = simd::neon::splat_u8(0);
  uint8x16_t eq = simd::neon::eq(cv, zero);
  u32 zero_mask = static_cast<u32>(simd::neon::movemask_u8(eq));
  return (~zero_mask) & 0xFFFFu;
}
#endif

};      // namespace __impl

// NOTE: hash collisions will yield same map entry - this is by design
// stores only (key, value)
// metadata lives in ctrl
template<typename K, typename V> struct alignas(__impl::__node_alignment) robin_map_node {
  hash64_t hash;      // pre-computed hash: fast integer filter
  K key;              // original key: definitive equality after hash match
  V value;

  ~robin_map_node() = default;

  robin_map_node() = default;

  robin_map_node(hash64_t h, K &&k, V &&v) : hash(h), key(micron::move(k)), value(micron::move(v)) { }

  robin_map_node(hash64_t h, const K &k, V &&v) : hash(h), key(k), value(micron::move(v)) { }

  template<typename... Args>
  robin_map_node(hash64_t h, K &&k, Args &&...args) : hash(h), key(micron::move(k)), value(micron::forward<Args>(args)...)
  {
  }

  robin_map_node(const robin_map_node &) = default;
  robin_map_node(robin_map_node &&) = default;
  robin_map_node &operator=(const robin_map_node &) = default;
  robin_map_node &operator=(robin_map_node &&) = default;
};

// robin_map
//
// robin hood style open addressing hash map
// fixed capacity, can't resize, not threadsafe
// capacity is always rounded to the nearest pow2
// load factor capped at 7/8 to keep probe distances short
template<typename K, typename V, class Alloc = micron::allocator_serial<>, typename Nd = robin_map_node<K, V>>
  requires micron::is_move_constructible_v<V>
class robin_map: public __immutable_memory_resource<Nd, Alloc>
{
  using __mem = __immutable_memory_resource<Nd, Alloc>;

  u8 *ctrl_ = nullptr;
  usize n_slots_ = 0;
  usize mask_ = 0;

  static constexpr usize __load_num = 7;
  static constexpr usize __load_denom = 8;
  static constexpr float __max_load = static_cast<float>(__load_num) / static_cast<float>(__load_denom);
  static constexpr usize __max_probe = 512;
  static constexpr usize __min_cap = 16;
  // insert_hash throws if a probe distance would exceed this
  static constexpr usize __max_stored_dist = 253;

  static usize
  round_pow2(usize n) noexcept
  {
    if ( n <= __min_cap ) return __min_cap;
    usize p = 1;
    while ( p < n ) {
      usize np = p << 1;
      if ( np <= p ) return p;      // overflow: saturate (the subsequent alloc will throw)
      p = np;
    }
    return p;
  }

  static usize
  default_n_slots() noexcept
  {
    usize auto_elems = Alloc::auto_size() / sizeof(Nd);
    return round_pow2(auto_elems < __min_cap ? __min_cap : auto_elems);
  }

  __attribute__((always_inline)) Nd &
  node_at(usize i) noexcept
  {
    return __mem::memory[i];
  }

  __attribute__((always_inline)) const Nd &
  node_at(usize i) const noexcept
  {
    return __mem::memory[i];
  }

  // needed because operator& might be overloaded
  __attribute__((always_inline)) Nd *
  node_ptr(usize i) noexcept
  {
    return micron::addressof(__mem::memory[i]);
  }

  __attribute__((always_inline)) const Nd *
  node_ptr(usize i) const noexcept
  {
    return micron::addressof(__mem::memory[i]);
  }

  __attribute__((always_inline)) V *
  value_ptr(usize i) noexcept
  {
    return micron::addressof(__mem::memory[i].value);
  }

  __attribute__((always_inline)) const V *
  value_ptr(usize i) const noexcept
  {
    return micron::addressof(__mem::memory[i].value);
  }

  __attribute__((always_inline)) static u8
  encode_dist(usize d) noexcept
  {
    usize v = d + 1;
    // guard preemptively
    return static_cast<u8>(v > 254u ? 254u : v);
  }

  __attribute__((always_inline)) bool
  occupied(usize i) const noexcept
  {
    return ctrl_[i] != 0;
  }

  // precondition: occupied(i)
  __attribute__((always_inline)) usize
  stored_dist(usize i) const noexcept
  {
    return static_cast<usize>(ctrl_[i]) - 1;
  }

  void
  alloc_ctrl(usize n_slots)
  {
    ctrl_ = new u8[n_slots];
    micron::memset(ctrl_, 0u, n_slots);
  }

  void
  free_ctrl() noexcept
  {
    delete[] ctrl_;
    ctrl_ = nullptr;
  }

  __attribute__((always_inline)) void
  destroy_at(usize i) noexcept
  {
    if constexpr ( !micron::is_trivially_destructible_v<Nd> ) node_at(i).~Nd();
    ctrl_[i] = 0;
  }

  // element at slot j with distance d had ideal = j - d
  // after moving to slot j-1 new distance becomes (j-1) - (j-d) = d-1
  void
  backward_shift(usize i) noexcept
  {
    for ( ;; ) {
      usize j = (i + 1u) & mask_;

      if ( !occupied(j) || stored_dist(j) == 0u ) break;

      // prefetch 2 ctrl bytes ahead ctrl is 1 byte so +2 = next iter + 1
      __builtin_prefetch(&ctrl_[(j + 2u) & mask_], 0, 1);

      usize new_dist = stored_dist(j) - 1;      // capture before any ctrl write

      if constexpr ( micron::is_trivially_copyable_v<Nd> ) {
        micron::bytecpy(reinterpret_cast<byte *>(node_ptr(i)), reinterpret_cast<const byte *>(node_ptr(j)), sizeof(Nd));
      } else {
        new (node_ptr(i)) Nd(micron::move(node_at(j)));
        node_at(j).~Nd();
      }
      ctrl_[i] = encode_dist(new_dist);
      ctrl_[j] = 0;
      i = j;
    }
  }

  V *
  insert_hash(hash64_t kh, K &&orig_key, V &&value)
  {
    usize index = static_cast<usize>(kh) & mask_;
    usize plen = 0;
    usize total_steps = 0;
    usize result_index = static_cast<usize>(-1);      // -1 not yet placed

    for ( ;; ) {
      if ( __builtin_expect(plen > __max_stored_dist, 0) )
        exc<except::library_error>("robin_map: stored distance saturated (>253); table needs larger capacity or better hash");

      __builtin_prefetch(&ctrl_[(index + 4u) & mask_], 1, 1);
      __builtin_prefetch(node_ptr((index + 2u) & mask_), 1, 0);

      if ( !occupied(index) ) {
        new (node_ptr(index)) Nd(kh, micron::move(orig_key), micron::move(value));
        ctrl_[index] = encode_dist(plen);
        ++__mem::length;
        return value_ptr(result_index == static_cast<usize>(-1) ? index : result_index);
      }

      if ( node_at(index).hash == kh && node_at(index).key == orig_key ) {
        node_at(index).value = micron::move(value);
        return value_ptr(index);
      }

      usize sd = stored_dist(index);
      bool steal = (sd < plen);
      hash64_t hmask = -hash64_t(steal);
      hash64_t dh = (kh ^ node_at(index).hash) & hmask;
      kh ^= dh;
      node_at(index).hash ^= dh;
      if ( steal ) micron::swap(orig_key, node_at(index).key);
      if ( steal ) micron::swap(value, node_at(index).value);
      ctrl_[index] = steal ? encode_dist(plen) : ctrl_[index];
      plen = steal ? sd : plen;
      if ( steal && result_index == static_cast<usize>(-1) ) result_index = index;

      if ( __builtin_expect(++total_steps > __max_probe, 0) )
        exc<except::library_error>("robin_map: probe limit exceeded (table is full or hash is pathological)");

      ++plen;
      index = (index + 1u) & mask_;
    }
  }

  V *
  probe_find(hash64_t kh, const K &orig_key) noexcept
  {
    if ( __builtin_expect(!ctrl_ || !n_slots_, 0) ) return nullptr;

    usize index = static_cast<usize>(kh) & mask_;
    usize plen = 0;

#if defined(__micron_x86_avx2) || defined(__micron_x86_sse2) || defined(__micron_arm_neon)
    constexpr usize W = __impl::__ctrl_window;
    // plen <= __max_stored_dist guard: above that bound no element exists (insert throws)
    while ( __builtin_expect(plen <= __max_stored_dist && index + W <= n_slots_, 1) ) {
      __builtin_prefetch(&ctrl_[index + 16u], 0, 1);
      __builtin_prefetch(node_ptr(index + 4u), 0, 0);

      usize stop = __impl::ctrl_scan(&ctrl_[index], plen);

      for ( usize i = 0; i < stop; ++i ) {
        if ( node_at(index + i).hash == kh && __builtin_expect(node_at(index + i).key == orig_key, 0) ) return value_ptr(index + i);
      }

      if ( __builtin_expect(stop < W, 0) ) return nullptr;

      plen += W;
      index = (index + W) & mask_;
    }
#endif

    // scalar fallback
#if defined(__micron_compiler_gcc_compat)
#pragma GCC unroll 4
#endif
    while ( ctrl_[index] > plen ) {
      __builtin_prefetch(&ctrl_[(index + 16u) & mask_], 0, 1);
      __builtin_prefetch(node_ptr((index + 4u) & mask_), 0, 0);

      if ( node_at(index).hash == kh && node_at(index).key == orig_key ) return value_ptr(index);
      ++plen;
      index = (index + 1u) & mask_;
    }
    return nullptr;
  }

  const V *
  probe_find(hash64_t kh, const K &orig_key) const noexcept
  {
    if ( __builtin_expect(!ctrl_ || !n_slots_, 0) ) return nullptr;

    usize index = static_cast<usize>(kh) & mask_;
    usize plen = 0;

#if defined(__micron_x86_avx2) || defined(__micron_x86_sse2) || defined(__micron_arm_neon)
    constexpr usize W = __impl::__ctrl_window;
    while ( __builtin_expect(plen <= __max_stored_dist && index + W <= n_slots_, 1) ) {
      __builtin_prefetch(&ctrl_[index + 16u], 0, 1);
      __builtin_prefetch(node_ptr(index + 4u), 0, 0);

      usize stop = __impl::ctrl_scan(&ctrl_[index], plen);

      for ( usize i = 0; i < stop; ++i ) {
        if ( node_at(index + i).hash == kh && __builtin_expect(node_at(index + i).key == orig_key, 0) ) return value_ptr(index + i);
      }

      if ( __builtin_expect(stop < W, 0) ) return nullptr;

      plen += W;
      index = (index + W) & mask_;
    }
#endif

#if defined(__micron_compiler_gcc_compat)
#pragma GCC unroll 4
#endif
    while ( ctrl_[index] > plen ) {
      __builtin_prefetch(&ctrl_[(index + 16u) & mask_], 0, 1);
      __builtin_prefetch(node_ptr((index + 4u) & mask_), 0, 0);

      if ( node_at(index).hash == kh && node_at(index).key == orig_key ) return value_ptr(index);
      ++plen;
      index = (index + 1u) & mask_;
    }
    return nullptr;
  }

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using key_type = K;
  using mapped_type = V;
  typedef usize size_type;
  typedef Nd value_type;
  typedef Nd &reference;
  typedef Nd &ref;
  typedef const Nd &const_reference;
  typedef const Nd &const_ref;
  typedef Nd *pointer;
  typedef const Nd *const_pointer;

  class iterator
  {
    Nd *base_;
    const u8 *ctrl_;
    usize i_;
    usize n_;

    __attribute__((always_inline)) void
    skip_empty() noexcept
    {
      while ( i_ < n_ && ctrl_[i_] == 0u ) ++i_;
    }

  public:
    __attribute__((always_inline))
    iterator(Nd *base, const u8 *ctrl, usize i, usize n) noexcept
        : base_(base), ctrl_(ctrl), i_(i), n_(n)
    {
      skip_empty();
    }

    __attribute__((always_inline)) Nd &
    operator*() const noexcept
    {
      return base_[i_];
    }

    __attribute__((always_inline)) Nd *
    operator->() const noexcept
    {
      return base_ + i_;
    }

    __attribute__((always_inline)) iterator &
    operator++() noexcept
    {
      ++i_;
      skip_empty();
      return *this;
    }

    __attribute__((always_inline)) bool
    operator==(const iterator &o) const noexcept
    {
      return i_ == o.i_;
    }

    __attribute__((always_inline)) bool
    operator!=(const iterator &o) const noexcept
    {
      return i_ != o.i_;
    }
  };

  class const_iterator
  {
    const Nd *base_;
    const u8 *ctrl_;
    usize i_;
    usize n_;

    __attribute__((always_inline)) void
    skip_empty() noexcept
    {
      while ( i_ < n_ && ctrl_[i_] == 0u ) ++i_;
    }

  public:
    __attribute__((always_inline))
    const_iterator(const Nd *base, const u8 *ctrl, usize i, usize n) noexcept
        : base_(base), ctrl_(ctrl), i_(i), n_(n)
    {
      skip_empty();
    }

    __attribute__((always_inline)) const Nd &
    operator*() const noexcept
    {
      return base_[i_];
    }

    __attribute__((always_inline)) const Nd *
    operator->() const noexcept
    {
      return base_ + i_;
    }

    __attribute__((always_inline)) const_iterator &
    operator++() noexcept
    {
      ++i_;
      skip_empty();
      return *this;
    }

    __attribute__((always_inline)) bool
    operator==(const const_iterator &o) const noexcept
    {
      return i_ == o.i_;
    }

    __attribute__((always_inline)) bool
    operator!=(const const_iterator &o) const noexcept
    {
      return i_ != o.i_;
    }
  };

  // dest always first
  ~robin_map()
  {
    if ( !__mem::memory ) return;
    clear();
    free_ctrl();
  }

  robin_map() : __mem(default_n_slots()), n_slots_(default_n_slots()), mask_(default_n_slots() - 1u) { alloc_ctrl(n_slots_); }

  explicit robin_map(usize n) : __mem(round_pow2(n)), n_slots_(round_pow2(n)), mask_(round_pow2(n) - 1u) { alloc_ctrl(n_slots_); }

  robin_map(const robin_map &) = delete;

  robin_map(robin_map &&o) noexcept : __mem(micron::move(o)), ctrl_(o.ctrl_), n_slots_(o.n_slots_), mask_(o.mask_)
  {
    o.ctrl_ = nullptr;
    o.n_slots_ = 0u;
    o.mask_ = 0u;
  }

  robin_map &operator=(const robin_map &) = delete;

  robin_map &
  operator=(robin_map &&o) noexcept
  {
    if ( this == &o ) return *this;
    if ( __mem::memory ) {
      clear();
      __mem::free();
    }
    free_ctrl();
    __mem::memory = o.memory;
    __mem::length = o.length;
    __mem::capacity = o.capacity;
    ctrl_ = o.ctrl_;
    n_slots_ = o.n_slots_;
    mask_ = o.mask_;
    o.memory = nullptr;
    o.length = 0u;
    o.capacity = 0u;
    o.ctrl_ = nullptr;
    o.n_slots_ = 0u;
    o.mask_ = 0u;
    return *this;
  }

  void reserve() = delete;      // fixed capacity cannot grow

  usize
  size() const noexcept
  {
    return __mem::length;
  }

  usize
  max_size() const noexcept
  {
    return n_slots_;
  }      // element count, not bytes

  bool
  empty() const noexcept
  {
    return __mem::length == 0;
  }

  float
  load_factor() const noexcept
  {
    return n_slots_ > 0u ? static_cast<float>(__mem::length) / static_cast<float>(n_slots_) : 0.0f;
  }

  void
  swap(robin_map &o) noexcept
  {
    micron::swap(__mem::memory, o.memory);
    micron::swap(__mem::length, o.length);
    micron::swap(__mem::capacity, o.capacity);
    micron::swap(ctrl_, o.ctrl_);
    micron::swap(n_slots_, o.n_slots_);
    micron::swap(mask_, o.mask_);
  }

  void
  clear() noexcept
  {
    if constexpr ( !micron::is_trivially_destructible_v<Nd> ) {
      for ( usize i = 0u; i < n_slots_; ++i )
        if ( occupied(i) ) node_at(i).~Nd();
    }
    if ( ctrl_ ) micron::memset(ctrl_, 0u, n_slots_);
    __mem::length = 0;
  }

  V *
  find_hash(hash64_t kh, const K &k)
  {
    return probe_find(kh, k);
  }

  const V *
  find_hash(hash64_t kh, const K &k) const
  {
    return probe_find(kh, k);
  }

  V *
  find(const K &k)
  {
    return probe_find(hash<hash64_t>(k), k);
  }

  const V *
  find(const K &k) const
  {
    return probe_find(hash<hash64_t>(k), k);
  }

  bool
  contains(const K &k) const
  {
    return probe_find(hash<hash64_t>(k), k) != nullptr;
  }

  V &
  at(const K &k)
  {
    V *v = probe_find(hash<hash64_t>(k), k);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("Key not found in robin_map");
    return *v;
  }

  const V &
  at(const K &k) const
  {
    const V *v = probe_find(hash<hash64_t>(k), k);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("Key not found in robin_map");
    return *v;
  }

  V &
  operator[](const K &k)
  {
    V *v = probe_find(hash<hash64_t>(k), k);
    if ( v ) [[likely]]
      return *v;
    return *insert(k, V{});
  }

  V *
  insert(const K &k, V &&value)
  {
    if ( __mem::length >= n_slots_ * __load_num / __load_denom ) [[unlikely]]
      exc<except::library_error>("robin_map: load factor limit reached; container is fixed-size");
    K kc = k;
    return insert_hash(hash<hash64_t>(k), micron::move(kc), micron::move(value));
  }

  V *
  insert(K &&k, V &&value)
  {
    if ( __mem::length >= n_slots_ * __load_num / __load_denom ) [[unlikely]]
      exc<except::library_error>("robin_map: load factor limit reached; container is fixed-size");
    hash64_t kh = hash<hash64_t>(k);
    return insert_hash(kh, micron::move(k), micron::move(value));
  }

  V *
  insert(const K &k, const V &value)
  {
    V copy = value;
    return insert(k, micron::move(copy));
  }

  template<typename... Args>
  V *
  emplace(const K &k, Args &&...args)
  {
    return insert(k, V(micron::forward<Args>(args)...));
  }

  V &
  add(const K &k, V value)
  {
    V *result = insert(k, micron::move(value));
    if ( !result ) [[unlikely]]
      exc<except::library_error>("Failed to add to robin_map");
    return *result;
  }

  bool
  erase_hash(hash64_t kh, const K &orig_key)
  {
    if ( __builtin_expect(!ctrl_ || !n_slots_, 0) ) return false;
    usize index = static_cast<usize>(kh) & mask_;
    usize plen = 0;
    while ( ctrl_[index] > plen ) {
      if ( node_at(index).hash == kh && node_at(index).key == orig_key ) {
        destroy_at(index);
        --__mem::length;
        backward_shift(index);
        return true;
      }
      ++plen;
      index = (index + 1u) & mask_;
    }
    return false;
  }

  bool
  erase(const K &k)
  {
    return erase_hash(hash<hash64_t>(k), k);
  }

  usize
  exists(const K &k) const
  {
    return probe_find(hash<hash64_t>(k), k) != nullptr ? 1 : 0;
  }

  usize
  count(const K &k) const
  {
    return exists(k);
  }

  iterator
  begin() noexcept
  {
    return iterator(__mem::memory, ctrl_, 0u, n_slots_);
  }

  const_iterator
  begin() const noexcept
  {
    return const_iterator(__mem::memory, ctrl_, 0u, n_slots_);
  }

  const_iterator
  cbegin() const noexcept
  {
    return const_iterator(__mem::memory, ctrl_, 0u, n_slots_);
  }

  iterator
  end() noexcept
  {
    return iterator(__mem::memory, ctrl_, n_slots_, n_slots_);
  }

  const_iterator
  end() const noexcept
  {
    return const_iterator(__mem::memory, ctrl_, n_slots_, n_slots_);
  }

  const_iterator
  cend() const noexcept
  {
    return const_iterator(__mem::memory, ctrl_, n_slots_, n_slots_);
  }

  Nd *
  raw_begin() noexcept
  {
    return node_ptr(0);
  }

  const Nd *
  raw_begin() const noexcept
  {
    return node_ptr(0);
  }

  Nd *
  raw_end() noexcept
  {
    return node_ptr(n_slots_);
  }

  const Nd *
  raw_end() const noexcept
  {
    return node_ptr(n_slots_);
  }

  const u8 *
  ctrl() const noexcept
  {
    return ctrl_;
  }

  bool
  slot_occupied(usize i) const noexcept
  {
    return ctrl_ && i < n_slots_ && occupied(i);
  }

  __attribute__((always_inline)) bool
  slot_occupied_unsafe(usize i) const noexcept
  {
    return ctrl_[i] != 0;
  }

  // SIMD-scanning walk over occupied slots
  template<typename Fn>
  __attribute__((always_inline)) void
  for_each(Fn &&fn)
  {
    if ( __builtin_expect(!ctrl_ || !n_slots_, 0) ) return;
    usize i = 0;
#if defined(__micron_x86_avx2)
    if ( n_slots_ >= 32 ) {
      for ( ; i + 32 <= n_slots_; i += 32 ) {
        u32 m = __impl::ctrl_occ_mask_avx2(&ctrl_[i]);
        while ( m ) {
          u32 b = static_cast<u32>(__builtin_ctz(m));
          fn(node_at(i + b));
          m &= m - 1u;
        }
      }
    }
#endif
#if defined(__micron_arch_x86_any)
    for ( ; i + 16 <= n_slots_; i += 16 ) {
      u32 m = __impl::ctrl_occ_mask_sse2(&ctrl_[i]);
      while ( m ) {
        u32 b = static_cast<u32>(__builtin_ctz(m));
        fn(node_at(i + b));
        m &= m - 1u;
      }
    }
#elif defined(__micron_arm_neon)
    for ( ; i + 16 <= n_slots_; i += 16 ) {
      u32 m = __impl::ctrl_occ_mask_neon(&ctrl_[i]);
      while ( m ) {
        u32 b = static_cast<u32>(__builtin_ctz(m));
        fn(node_at(i + b));
        m &= m - 1u;
      }
    }
#endif
    // scalar tail
    for ( ; i < n_slots_; ++i )
      if ( ctrl_[i] != 0 ) fn(node_at(i));
  }

  template<typename Fn>
  __attribute__((always_inline)) void
  for_each(Fn &&fn) const
  {
    if ( __builtin_expect(!ctrl_ || !n_slots_, 0) ) return;
    usize i = 0;
#if defined(__micron_x86_avx2)
    if ( n_slots_ >= 32 ) {
      for ( ; i + 32 <= n_slots_; i += 32 ) {
        u32 m = __impl::ctrl_occ_mask_avx2(&ctrl_[i]);
        while ( m ) {
          u32 b = static_cast<u32>(__builtin_ctz(m));
          fn(node_at(i + b));
          m &= m - 1u;
        }
      }
    }
#endif
#if defined(__micron_arch_x86_any)
    for ( ; i + 16 <= n_slots_; i += 16 ) {
      u32 m = __impl::ctrl_occ_mask_sse2(&ctrl_[i]);
      while ( m ) {
        u32 b = static_cast<u32>(__builtin_ctz(m));
        fn(node_at(i + b));
        m &= m - 1u;
      }
    }
#elif defined(__micron_arm_neon)
    for ( ; i + 16 <= n_slots_; i += 16 ) {
      u32 m = __impl::ctrl_occ_mask_neon(&ctrl_[i]);
      while ( m ) {
        u32 b = static_cast<u32>(__builtin_ctz(m));
        fn(node_at(i + b));
        m &= m - 1u;
      }
    }
#endif
    for ( ; i < n_slots_; ++i )
      if ( ctrl_[i] != 0 ) fn(node_at(i));
  }

  template<typename Fn>
    requires micron::is_invocable_v<Fn, const K &, V &>
  __attribute__((always_inline)) void
  for_each(Fn &&fn)
  {
    for_each([&](Nd &n) { fn(n.key, n.value); });
  }

  template<typename Fn>
    requires micron::is_invocable_v<Fn, const K &, const V &>
  __attribute__((always_inline)) void
  for_each(Fn &&fn) const
  {
    for_each([&](const Nd &n) { fn(n.key, n.value); });
  }
};

template<typename K, typename V> using robin = robin_map<K, V>;
};      // namespace micron
