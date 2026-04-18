//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"

#if defined(__micron_x86_sse2)
#include <immintrin.h>
#endif
#if defined(__micron_arm_neon)
#include <arm_neon.h>
#endif

#include "../hash/hash.hpp"
#include "../memory/actions.hpp"

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
  __m256i cv = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p));
  u8 bv = plen <= 255u ? static_cast<u8>(plen) : 255u;
  // static to prevent reconstruction on each loop
  static const __m256i incr = _mm256_set_epi8(31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8,
                                              7, 6, 5, 4, 3, 2, 1, 0);
  static const __m256i flip = _mm256_set1_epi8(static_cast<char>(0x80u));
  __m256i base = _mm256_set1_epi8(static_cast<char>(bv));
  __m256i thres = _mm256_adds_epu8(base, incr);
  __m256i cv_s = _mm256_xor_si256(cv, flip);
  __m256i th_s = _mm256_xor_si256(thres, flip);
  __m256i gt = _mm256_cmpgt_epi8(cv_s, th_s);
  uint32_t mask = static_cast<uint32_t>(_mm256_movemask_epi8(gt));
  if ( mask == 0xFFFF'FFFFu ) return 32;
  return static_cast<usize>(__builtin_ctz(~mask));
}

#elif defined(__micron_x86_sse2)
__attribute__((always_inline)) static inline usize
ctrl_scan_sse2(const u8 *__restrict__ p, usize plen) noexcept
{
  __m128i cv = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p));
  u8 bv = plen <= 255u ? static_cast<u8>(plen) : 255u;
  // static to prevent reconstruction on each loop
  static const __m128i incr = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
  static const __m128i flip = _mm_set1_epi8(static_cast<char>(0x80u));
  __m128i base = _mm_set1_epi8(static_cast<char>(bv));
  __m128i thres = _mm_adds_epu8(base, incr);
  __m128i cv_s = _mm_xor_si128(cv, flip);
  __m128i th_s = _mm_xor_si128(thres, flip);
  __m128i gt = _mm_cmpgt_epi8(cv_s, th_s);
  uint32_t mask = static_cast<uint32_t>(static_cast<uint16_t>(_mm_movemask_epi8(gt)));
  if ( mask == 0xFFFFu ) return 16;
  return static_cast<usize>(__builtin_ctz(~mask));
}

#elif defined(__micron_arm_neon)
// vshr/vshl movemask emulation
// avoids loading a separate bit_pos array; uses one immediate + one variable shift three vpadd_u8 passes
// collapse 16 byte lanes into two mask bytes (low/high)
__attribute__((always_inline)) static inline usize
ctrl_scan_neon(const u8 *__restrict__ p, usize plen) noexcept
{
  uint8x16_t cv = vld1q_u8(p);
  u8 bv = plen <= 255u ? static_cast<u8>(plen) : 255u;
  uint8x16_t base = vdupq_n_u8(bv);
  static const u8 incr_arr[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
  static const int8_t shl_arr[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7 };
  uint8x16_t thres = vqaddq_u8(base, vld1q_u8(incr_arr));
  uint8x16_t gt = vcgtq_u8(cv, thres);
  // extract MSB of each 0xFF/0x00 byte then shift into unique bit position
  uint8x16_t bits = vshlq_u8(vshrq_n_u8(gt, 7), vld1q_s8(shl_arr));
  // combine low/high halves with vorrq_u8 before padd reduction
  uint8x8_t lo = vget_low_u8(bits);
  uint8x8_t hi = vget_high_u8(bits);
  uint8x8_t r = vpadd_u8(lo, hi);
  r = vpadd_u8(r, r);
  r = vpadd_u8(r, r);
  uint16_t mask = static_cast<uint16_t>(vget_lane_u8(r, 0)) | (static_cast<uint16_t>(vget_lane_u8(r, 1)) << 8u);
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

};     // namespace __impl

// NOTE: hash collisions will yield same map entry - this is by design
// stores only (key, value)
// metadata lives in ctrl
template <typename K, typename V> struct alignas(__impl::__node_alignment) robin_map_node {
  hash64_t hash;     // pre-computed hash: fast integer filter
  K key;             // original key: definitive equality after hash match
  V value;

  ~robin_map_node() = default;

  robin_map_node() = default;

  robin_map_node(hash64_t h, K &&k, V &&v) : hash(h), key(micron::move(k)), value(micron::move(v)) {}

  robin_map_node(hash64_t h, const K &k, V &&v) : hash(h), key(k), value(micron::move(v)) {}

  template <typename... Args>
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
template <typename K, typename V, class Alloc = micron::allocator_serial<>, typename Nd = robin_map_node<K, V>>
  requires micron::is_move_constructible_v<V>
class robin_map : public __immutable_memory_resource<Nd, Alloc>
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

  static usize
  round_pow2(usize n) noexcept
  {
    if ( n <= __min_cap ) return __min_cap;
    usize p = 1;
    while ( p < n ) p <<= 1;
    return p;
  }

  static usize
  cap_bytes(usize n_elems) noexcept
  {
    return round_pow2(n_elems) * sizeof(Nd);
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
    return addr(__mem::memory[i]);
  }

  __attribute__((always_inline)) const Nd *
  node_ptr(usize i) const noexcept
  {
    return addr(__mem::memory[i]);
  }

  __attribute__((always_inline)) V *
  value_ptr(usize i) noexcept
  {
    return addr(__mem::memory[i].value);
  }

  __attribute__((always_inline)) const V *
  value_ptr(usize i) const noexcept
  {
    return addr(__mem::memory[i].value);
  }

  __attribute__((always_inline)) static u8
  encode_dist(usize d) noexcept
  {
    usize v = d + 1;
    return static_cast<u8>(v > 255u ? 255u : v);
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

      usize new_dist = stored_dist(j) - 1;     // capture before any ctrl write

      if constexpr ( micron::is_trivially_copyable_v<Nd> ) {
        micron::memcpy(node_ptr(i), node_ptr(j), sizeof(Nd));
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

    for ( ;; ) {
      __builtin_prefetch(&ctrl_[(index + 4u) & mask_], 1, 1);
      __builtin_prefetch(node_ptr((index + 2u) & mask_), 1, 0);

      if ( !occupied(index) ) {
        new (node_ptr(index)) Nd(kh, micron::move(orig_key), micron::move(value));
        ctrl_[index] = encode_dist(plen);
        ++__mem::length;
        return value_ptr(index);
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
    // plen <= 254 guard: when plen >= 255 every probe stops immediately
    while ( __builtin_expect(plen <= 254u && index + W <= n_slots_, 1) ) {
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
    while ( __builtin_expect(plen <= 254u && index + W <= n_slots_, 1) ) {
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
  typedef usize size_type;
  typedef Nd value_type;
  typedef Nd &reference;
  typedef Nd &ref;
  typedef const Nd &const_reference;
  typedef const Nd &const_ref;
  typedef Nd *pointer;
  typedef const Nd *const_pointer;
  typedef Nd *iterator;
  typedef const Nd *const_iterator;

  // dest always first
  ~robin_map()
  {
    if ( !__mem::memory ) return;
    clear();
    free_ctrl();
  }

  robin_map() : __mem(cap_bytes(default_n_slots())), n_slots_(default_n_slots()), mask_(default_n_slots() - 1u) { alloc_ctrl(n_slots_); }

  explicit robin_map(usize n) : __mem(cap_bytes(n)), n_slots_(round_pow2(n)), mask_(round_pow2(n) - 1u) { alloc_ctrl(n_slots_); }

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
    if ( __mem::memory ) clear();
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

  void reserve() = delete;     // fixed capacity cannot grow

  usize
  size() const noexcept
  {
    return __mem::length;
  }

  usize
  max_size() const noexcept
  {
    return n_slots_;
  }     // element count, not bytes

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

  template <typename... Args>
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
  begin()
  {
    return node_ptr(0);
  }

  const_iterator
  begin() const
  {
    return node_ptr(0);
  }

  const_iterator
  cbegin() const
  {
    return node_ptr(0);
  }

  iterator
  end()
  {
    return node_ptr(n_slots_);
  }

  const_iterator
  end() const
  {
    return node_ptr(n_slots_);
  }

  const_iterator
  cend() const
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
};

template <typename K, typename V> using robin = robin_map<K, V>;
};     // namespace micron
