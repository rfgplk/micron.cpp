//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#if defined(MICRON_TREE_VECTOR_SCAN)
#include "../simd/aliases.hpp"
#include "../simd/intrin.hpp"
#endif
#include "../memory/actions.hpp"
#include "../memory/addr.hpp"
#include "../memory/allocation/resources.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shared node storage for our arena backed trees

namespace micron
{
namespace __tree_store
{

using node_idx = u32;
inline constexpr node_idx nil = static_cast<node_idx>(~0u);

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// tagged indices

inline constexpr node_idx leaf_bit = 0x80000000u;
inline constexpr node_idx slot_mask = 0x7FFFFFFFu;
inline constexpr node_idx max_slots = 0x7FFFFFFFu;

[[nodiscard, gnu::always_inline]] inline constexpr bool
is_leaf_idx(node_idx i) noexcept
{
  return (i & leaf_bit) != 0u;
}

[[nodiscard, gnu::always_inline]] inline constexpr node_idx
slot_of(node_idx i) noexcept
{
  return i & slot_mask;
}

[[nodiscard, gnu::always_inline]] inline constexpr node_idx
tag_leaf(node_idx slot) noexcept
{
  return slot | leaf_bit;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// prefetch

[[gnu::always_inline]] inline void
prefetch_node(const void *p) noexcept
{
  __builtin_prefetch(p, 0, 3);
}

template<usize Bytes>
[[gnu::always_inline]] inline void
prefetch_span(const void *p) noexcept
{
  const byte *b = static_cast<const byte *>(p);
  for ( usize off = 0; off < Bytes; off += 64 ) __builtin_prefetch(b + off, 0, 3);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// in-node key search
template<typename Compare>
concept natural_order_compare = requires { typename Compare::__natural_order; };

template<typename K, typename Compare>
inline constexpr bool vector_rankable_v = natural_order_compare<Compare> && micron::is_integral_v<K> && (sizeof(K) == 4 || sizeof(K) == 8);

template<typename K, typename Compare>
inline constexpr bool cheap_compare_v = natural_order_compare<Compare> && (micron::is_arithmetic_v<K> || micron::is_pointer_v<K>);

namespace __scan
{
#if defined(MICRON_TREE_VECTOR_SCAN)

// 128bit even with avx256;
// the 256bit form is slower: b_tree's lower_bound went 276 -> 497 cyc/op with it
#if defined(__micron_x86_sse2)
inline constexpr u16 lanes32 = 4;
#if defined(__micron_x86_sse4_2)
inline constexpr u16 lanes64 = 2;
#else
inline constexpr u16 lanes64 = 0;      // no 64-bit integer compare below SSE4.2
#endif
#elif defined(__micron_arm_neon)
// ARM takes the scalar sequential walk
inline constexpr u16 lanes32 = 0;
inline constexpr u16 lanes64 = 0;
#else
inline constexpr u16 lanes32 = 0;
inline constexpr u16 lanes64 = 0;
#endif

template<typename K> inline constexpr u16 lanes_for = (sizeof(K) == 4) ? lanes32 : ((sizeof(K) == 8) ? lanes64 : 0);

template<typename K, bool Strict> struct probe {
  static constexpr u16 W = lanes_for<K>;
  static constexpr bool sgn = micron::is_signed_v<K>;

#if defined(__micron_x86_sse2)
  __m128i tv;
  __m128i bias;

  [[gnu::always_inline]] explicit probe(const K &key) noexcept
  {
    if constexpr ( sizeof(K) == 8 ) {
      bias = simd::sse::splat_i64(sgn ? 0LL : static_cast<long long>(0x8000000000000000ULL));
      tv = simd::sse::xor_i128(simd::sse::splat_i64(static_cast<long long>(key)), bias);
    } else {
      bias = simd::sse::splat_i32(sgn ? 0 : static_cast<int>(0x80000000u));
      tv = simd::sse::xor_i128(simd::sse::splat_i32(static_cast<int>(key)), bias);
    }
  }

  [[nodiscard, gnu::always_inline]] u16
  count(const K *p) const noexcept
  {
    const __m128i kv = simd::sse::xor_i128(simd::sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p)), bias);
    if constexpr ( sizeof(K) == 8 ) {
      const __m128i cmp = Strict ? simd::sse::gt_i64(tv, kv) : simd::sse::gt_i64(kv, tv);
      u32 m = static_cast<u32>(simd::sse::movemask_f64(simd::sse::cast_i128_to_f64(cmp))) & 0x3u;
      if constexpr ( !Strict ) m = (~m) & 0x3u;
      return static_cast<u16>(m == 0x3u ? 2 : __builtin_ctz(~m));
    } else {
      const __m128i cmp = Strict ? simd::sse::gt_i32(tv, kv) : simd::sse::gt_i32(kv, tv);
      u32 m = static_cast<u32>(simd::sse::movemask_f32(simd::sse::cast_i128_to_f32(cmp))) & 0xFu;
      if constexpr ( !Strict ) m = (~m) & 0xFu;
      return static_cast<u16>(m == 0xFu ? 4 : __builtin_ctz(~m));
    }
  }

#else
  [[gnu::always_inline]] explicit probe(const K &) noexcept { }

  [[nodiscard, gnu::always_inline]] u16
  count(const K *) const noexcept
  {
    return 0;
  }
#endif
};

#endif      // MICRON_TREE_VECTOR_SCAN

};      // namespace __scan

#if defined(MICRON_TREE_VECTOR_SCAN)
template<typename K, typename Compare, bool Strict>
[[nodiscard, gnu::always_inline]] inline u16
__vector_rank(const K *keys, u16 n, const K &key) noexcept
{
  constexpr u16 W = __scan::lanes_for<K>;
  const __scan::probe<K, Strict> pr(key);
  u16 i = 0;
  while ( static_cast<u16>(i + W) <= n ) {
    const u16 got = pr.count(keys + i);
    i = static_cast<u16>(i + got);
    if ( got != W ) return i;
  }
  if constexpr ( Strict ) {
    while ( i < n && Compare::lt(keys[i], key) ) ++i;
  } else {
    while ( i < n && !Compare::lt(key, keys[i]) ) ++i;
  }
  return i;
}
#endif      // MICRON_TREE_VECTOR_SCAN

// log2(n) comparisons, both updates conditional moves
template<typename K, typename Compare, bool Strict>
[[nodiscard, gnu::always_inline]] inline u16
__ladder_rank(const K *keys, u16 n, const K &key) noexcept
{
  u16 lo = 0;
  u16 len = n;
  while ( len > 0 ) {
    const u16 half = static_cast<u16>(len >> 1);
    const u16 mid = static_cast<u16>(lo + half);
    const bool go_right = Strict ? Compare::lt(keys[mid], key) : !Compare::lt(key, keys[mid]);
    lo = go_right ? static_cast<u16>(mid + 1) : lo;
    len = go_right ? static_cast<u16>(len - half - 1) : half;
  }
  return lo;
}

template<typename K, typename Compare, bool Strict>
[[nodiscard, gnu::always_inline]] inline u16
__linear_rank(const K *keys, u16 n, const K &key) noexcept
{
  u16 i = 0;
  if constexpr ( Strict ) {
    while ( i < n && Compare::lt(keys[i], key) ) ++i;
  } else {
    while ( i < n && !Compare::lt(key, keys[i]) ) ++i;
  }
  return i;
}

// count of i in [0,n) with lt(keys[i], key)
template<typename K, typename Compare>
[[nodiscard, gnu::always_inline]] inline u16
lower_bound_scan(const K *keys, u16 n, const K &key) noexcept
{
#if defined(MICRON_TREE_VECTOR_SCAN)
  if constexpr ( vector_rankable_v<K, Compare> && __scan::lanes_for<K> > 0 )
    return __vector_rank<K, Compare, true>(keys, n, key);
  else
#endif
      if constexpr ( cheap_compare_v<K, Compare> )
    return __linear_rank<K, Compare, true>(keys, n, key);
  else
    return __ladder_rank<K, Compare, true>(keys, n, key);
}

// count of i in [0,n) with !lt(key, keys[i])
template<typename K, typename Compare>
[[nodiscard, gnu::always_inline]] inline u16
upper_bound_scan(const K *keys, u16 n, const K &key) noexcept
{
#if defined(MICRON_TREE_VECTOR_SCAN)
  if constexpr ( vector_rankable_v<K, Compare> && __scan::lanes_for<K> > 0 )
    return __vector_rank<K, Compare, false>(keys, n, key);
  else
#endif
      if constexpr ( cheap_compare_v<K, Compare> )
    return __linear_rank<K, Compare, false>(keys, n, key);
  else
    return __ladder_rank<K, Compare, false>(keys, n, key);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// element relocation within a node

template<typename T>
inline constexpr bool is_byte_relocatable_v = micron::is_trivially_copyable_v<T> && micron::is_trivially_destructible_v<T>;

template<typename T>
__attribute__((always_inline)) inline void
shift_right(T *base, usize first, usize last) noexcept
{
  if ( first >= last ) return;
  if constexpr ( is_byte_relocatable_v<T> ) {
    const usize cnt = last - first;
    using __ab = unsigned char __attribute__((may_alias));
    __ab *d = reinterpret_cast<__ab *>(base + first + 1);
    const __ab *s = reinterpret_cast<const __ab *>(base + first);
    for ( usize i = cnt * sizeof(T); i > 0; --i ) d[i - 1] = s[i - 1];
  } else {
    for ( usize i = last; i > first; --i ) {
      new (micron::addr(base[i])) T(micron::move(base[i - 1]));
      base[i - 1].~T();
    }
  }
}

template<typename T>
__attribute__((always_inline)) inline void
erase_at(T *base, usize first, usize last) noexcept
{
  if ( first >= last ) return;
  if constexpr ( is_byte_relocatable_v<T> ) {
    const usize cnt = (first + 1 < last) ? (last - first - 1) : 0;
    if ( cnt ) {
      using __ab = unsigned char __attribute__((may_alias));
      __ab *d = reinterpret_cast<__ab *>(base + first);
      const __ab *s = reinterpret_cast<const __ab *>(base + first + 1);
      for ( usize i = 0; i < cnt * sizeof(T); ++i ) d[i] = s[i];
    }
  } else {
    base[first].~T();
    for ( usize i = first + 1; i < last; ++i ) {
      new (micron::addr(base[i - 1])) T(micron::move(base[i]));
      base[i].~T();
    }
  }
}

template<typename T>
__attribute__((always_inline)) inline void
relocate_n(T *dest, T *src, usize cnt)
{
  if constexpr ( is_byte_relocatable_v<T> ) {
    using __ab = unsigned char __attribute__((may_alias));
    __ab *d = reinterpret_cast<__ab *>(dest);
    const __ab *s = reinterpret_cast<const __ab *>(src);
    for ( usize i = 0; i < cnt * sizeof(T); ++i ) d[i] = s[i];
  } else {
    for ( usize i = 0; i < cnt; ++i ) {
      new (micron::addr(dest[i])) T(micron::move(src[i]));
      src[i].~T();
    }
  }
}

template<usize B, usize Al> struct alignas(Al) raw_slot {
  alignas(Al) byte raw[B];
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// node_arena
//
// NOTE: grow() reallocates the slab, and because raw_slot is trivially
// copyable that goes through __owned_memory_resource's realloc path, meaning the whole slab is moveable
// .. every leaf_node&/internal_node&/K*/V* derived from raw() is invalidated by any allocate() that grows
// .. K and V are byte-relocated even when they are not trivially copyable

template<usize B, usize Al = 64, class Alloc = allocator_serial<>> class node_arena
{
  using slot_t = raw_slot<B, Al>;
  static_assert(B >= sizeof(node_idx), "slot too small to hold the free-list link");

  typedef node_idx __attribute__((may_alias)) __alias_idx;

  __immutable_memory_resource<slot_t, Alloc> mem_;
  node_idx free_head_;

  [[gnu::always_inline]] static node_idx
  link_load(const byte *p) noexcept
  {
    return *reinterpret_cast<const __alias_idx *>(p);
  }

  [[gnu::always_inline]] static void
  link_store(byte *p, node_idx v) noexcept
  {
    *reinterpret_cast<__alias_idx *>(p) = v;
  }

  void
  grow()
  {
    const usize old = mem_.capacity;
    const usize minimum = old == static_cast<usize>(-1) ? old : old + 1;
    if ( minimum == old ) exc<except::length_error>("node_arena: capacity overflow");
    mem_.expand(mem_.recommended_capacity(old, minimum));
  }

public:
  node_arena() : mem_(), free_head_(nil) { }

  node_arena(const node_arena &) = delete;
  node_arena &operator=(const node_arena &) = delete;

  node_arena(node_arena &&o) noexcept : mem_(micron::move(o.mem_)), free_head_(o.free_head_) { o.free_head_ = nil; }

  node_arena &
  operator=(node_arena &&o) noexcept
  {
    if ( this != &o ) {
      mem_ = micron::move(o.mem_);
      free_head_ = o.free_head_;
      o.free_head_ = nil;
    }
    return *this;
  }

  [[gnu::always_inline]] byte *
  raw(node_idx i) noexcept
  {
    return mem_.memory[i].raw;
  }

  [[gnu::always_inline]] const byte *
  raw(node_idx i) const noexcept
  {
    return mem_.memory[i].raw;
  }

  node_idx
  allocate()
  {
    if ( free_head_ != nil ) {
      const node_idx i = free_head_;
      free_head_ = link_load(mem_.memory[i].raw);
      return i;
    }
    if ( mem_.length >= mem_.capacity ) grow();
    if ( mem_.length >= static_cast<usize>(max_slots) ) exc<except::length_error>("node_arena: node index space exhausted");
    return static_cast<node_idx>(mem_.length++);
  }

  void
  deallocate(node_idx i) noexcept
  {
    link_store(mem_.memory[i].raw, free_head_);
    free_head_ = i;
  }

  void
  reset() noexcept
  {
    mem_.length = 0;
    free_head_ = nil;
  }

  void
  swap(node_arena &o) noexcept
  {
    auto tm = micron::move(mem_);
    mem_ = micron::move(o.mem_);
    o.mem_ = micron::move(tm);
    const node_idx th = free_head_;
    free_head_ = o.free_head_;
    o.free_head_ = th;
  }

  void
  reserve(usize n_slots)
  {
    if ( mem_.capacity < n_slots ) mem_.expand(n_slots);
  }

  [[nodiscard, gnu::always_inline]] usize
  slots_used() const noexcept
  {
    return mem_.length;
  }

  [[nodiscard]] usize
  slots_live() const noexcept
  {
    usize freed = 0;
    for ( node_idx i = free_head_; i != nil; ++freed ) i = link_load(mem_.memory[i].raw);
    return mem_.length - freed;
  }

  [[nodiscard, gnu::always_inline]] usize
  slots_reserved() const noexcept
  {
    return mem_.capacity;
  }

  [[nodiscard, gnu::always_inline]] static constexpr usize
  slot_bytes() noexcept
  {
    return sizeof(slot_t);
  }
};

// %%%%%%%%%%%%%%%%%%%%%%
// block_pool

template<typename T, class Alloc = allocator_serial<>, usize MinBytes = 4096, usize MaxBytes = 4096u * 256u> class block_pool
{
  typedef T *__attribute__((may_alias)) __alias_ptr;

  static_assert(sizeof(T) >= sizeof(T *), "a pool slot must be able to hold a chain link");

  static constexpr usize __min_slots = 8;

  T *blocks_;        // block chain; slot 0 of each block is the next-block link, never handed out
  T *bump_;          // next unused slot in the newest block
  T *bump_end_;      // one past the newest block's last slot
  T *free_;          // free list head
  usize block_bytes_;

  [[gnu::always_inline]] static T *
  link_load(const T *slot) noexcept
  {
    return *reinterpret_cast<const __alias_ptr *>(slot);
  }

  [[gnu::always_inline]] static void
  link_store(T *slot, T *v) noexcept
  {
    *reinterpret_cast<__alias_ptr *>(slot) = v;
  }

  void
  add_block()
  {
    usize slots = block_bytes_ / sizeof(T);
    if ( slots < __min_slots ) slots = __min_slots;
    T *blk = __allocator_allocate_array<Alloc, T>(slots);
    link_store(blk, blocks_);
    blocks_ = blk;
    bump_ = blk + 1;
    bump_end_ = blk + slots;
    if ( block_bytes_ < MaxBytes ) block_bytes_ <<= 1;
  }

public:
  block_pool() noexcept : blocks_(nullptr), bump_(nullptr), bump_end_(nullptr), free_(nullptr), block_bytes_(MinBytes) { }

  block_pool(const block_pool &) = delete;
  block_pool &operator=(const block_pool &) = delete;

  block_pool(block_pool &&o) noexcept
      : blocks_(o.blocks_), bump_(o.bump_), bump_end_(o.bump_end_), free_(o.free_), block_bytes_(o.block_bytes_)
  {
    o.blocks_ = nullptr;
    o.bump_ = nullptr;
    o.bump_end_ = nullptr;
    o.free_ = nullptr;
    o.block_bytes_ = MinBytes;
  }

  block_pool &
  operator=(block_pool &&o) noexcept
  {
    if ( this != &o ) {
      release();
      blocks_ = o.blocks_;
      bump_ = o.bump_;
      bump_end_ = o.bump_end_;
      free_ = o.free_;
      block_bytes_ = o.block_bytes_;
      o.blocks_ = nullptr;
      o.bump_ = nullptr;
      o.bump_end_ = nullptr;
      o.free_ = nullptr;
      o.block_bytes_ = MinBytes;
    }
    return *this;
  }

  ~block_pool() { release(); }

  [[nodiscard, gnu::always_inline]] T *
  take()
  {
    if ( free_ ) {
      T *p = free_;
      free_ = link_load(p);
      return p;
    }
    if ( bump_ == bump_end_ ) [[unlikely]]
      add_block();
    return bump_++;
  }

  [[gnu::always_inline]] void
  give(T *p) noexcept
  {
    link_store(p, free_);
    free_ = p;
  }

  void
  release() noexcept
  {
    T *b = blocks_;
    while ( b ) {
      T *nx = link_load(b);
      __allocator_deallocate_array<Alloc>(b);
      b = nx;
    }
    blocks_ = nullptr;
    bump_ = nullptr;
    bump_end_ = nullptr;
    free_ = nullptr;
    block_bytes_ = MinBytes;
  }

  // carve one run big enough for n more objects, so a build of known size stays contiguous
  void
  reserve(usize n)
  {
    const usize have = bump_end_ ? static_cast<usize>(bump_end_ - bump_) : 0;
    if ( n <= have ) return;
    const usize want = (n - have + 1) * sizeof(T);
    while ( block_bytes_ < want && block_bytes_ < MaxBytes ) block_bytes_ <<= 1;
    if ( block_bytes_ < want ) block_bytes_ = want;
    add_block();
    (void)0;
  }

  void
  swap(block_pool &o) noexcept
  {
    micron::swap(blocks_, o.blocks_);
    micron::swap(bump_, o.bump_);
    micron::swap(bump_end_, o.bump_end_);
    micron::swap(free_, o.free_);
    micron::swap(block_bytes_, o.block_bytes_);
  }

  [[nodiscard]] usize
  blocks() const noexcept
  {
    usize n = 0;
    for ( const T *b = blocks_; b; b = link_load(b) ) ++n;
    return n;
  }

  [[nodiscard]] usize
  bytes_held() const noexcept
  {
    usize total = 0;
    usize bs = MinBytes;
    for ( usize i = 0, k = blocks(); i < k; ++i ) {
      total += bs;
      if ( bs < MaxBytes ) bs <<= 1;
    }
    return total;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// dual_arena
template<usize LeafB, usize LeafAl, usize IntB, usize IntAl, class Alloc = allocator_serial<>> class dual_arena
{
  node_arena<LeafB, LeafAl, Alloc> leaves_;
  node_arena<IntB, IntAl, Alloc> inner_;

public:
  dual_arena() = default;
  dual_arena(const dual_arena &) = delete;
  dual_arena &operator=(const dual_arena &) = delete;
  dual_arena(dual_arena &&) noexcept = default;
  dual_arena &operator=(dual_arena &&) noexcept = default;

  [[nodiscard]] node_idx
  allocate_leaf()
  {
    const node_idx s = leaves_.allocate();
    return tag_leaf(s);
  }

  [[nodiscard]] node_idx
  allocate_internal()
  {
    return inner_.allocate();
  }

  void
  deallocate(node_idx i) noexcept
  {
    if ( is_leaf_idx(i) )
      leaves_.deallocate(slot_of(i));
    else
      inner_.deallocate(i);
  }

  [[gnu::always_inline]] byte *
  raw(node_idx i) noexcept
  {
    return is_leaf_idx(i) ? leaves_.raw(slot_of(i)) : inner_.raw(i);
  }

  [[gnu::always_inline]] const byte *
  raw(node_idx i) const noexcept
  {
    return is_leaf_idx(i) ? leaves_.raw(slot_of(i)) : inner_.raw(i);
  }

  // kind is known statically at almost every call site; these skip the tag test entirely
  [[gnu::always_inline]] byte *
  raw_leaf(node_idx i) noexcept
  {
    return leaves_.raw(slot_of(i));
  }

  [[gnu::always_inline]] const byte *
  raw_leaf(node_idx i) const noexcept
  {
    return leaves_.raw(slot_of(i));
  }

  [[gnu::always_inline]] byte *
  raw_internal(node_idx i) noexcept
  {
    return inner_.raw(i);
  }

  [[gnu::always_inline]] const byte *
  raw_internal(node_idx i) const noexcept
  {
    return inner_.raw(i);
  }

  void
  reset() noexcept
  {
    leaves_.reset();
    inner_.reset();
  }

  void
  swap(dual_arena &o) noexcept
  {
    leaves_.swap(o.leaves_);
    inner_.swap(o.inner_);
  }

  void
  reserve_leaves(usize n)
  {
    leaves_.reserve(n);
  }

  void
  reserve_internal(usize n)
  {
    inner_.reserve(n);
  }

  [[nodiscard]] usize
  slots_used() const noexcept
  {
    return leaves_.slots_used() + inner_.slots_used();
  }

  [[nodiscard]] usize
  slots_live() const noexcept
  {
    return leaves_.slots_live() + inner_.slots_live();
  }

  [[nodiscard]] usize
  slots_reserved() const noexcept
  {
    return leaves_.slots_reserved() + inner_.slots_reserved();
  }

  [[nodiscard]] usize
  bytes_reserved() const noexcept
  {
    return leaves_.slots_reserved() * LeafB + inner_.slots_reserved() * IntB;
  }
};

};      // namespace __tree_store
};      // namespace micron
