//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// btree_map: heap-allocated, resizable, fractal-tree-flavoured hashed-routing B-tree

#include "../bits/__arch.hpp"
#include "../bits/__container.hpp"
#include "../concepts.hpp"
#include "../except.hpp"
#include "../hash/hash.hpp"
#include "../memory/actions.hpp"
#include "../memory/addr.hpp"
#include "../memory/allocation/resources.hpp"
#include "../memory/cmemory.hpp"
#include "../simd/aliases.hpp"
#include "../simd/types.hpp"
#include "../tags.hpp"
#include "../tuple.hpp"
#include "../types.hpp"

namespace micron
{

namespace __btree_impl
{

using node_idx = u32;
inline constexpr node_idx __k_empty = 0xFFFFFFFFu;
inline constexpr u8 __k_fsentinel = 0xFFu;      // nkeys=0xFF marks a free slot

// per-K,V fanout/buffer heuristics
template<usize KvBytes>
consteval usize
pick_leaf_fanout()
{
  if constexpr ( KvBytes <= 24 )
    return 32;
  else if constexpr ( KvBytes <= 64 )
    return 16;
  else
    return 8;
}

template<usize>
consteval usize
pick_internal_fanout()
{
  return 16;
}

template<usize Fanout>
consteval usize
pick_buf_size()
{
  if constexpr ( Fanout <= 8 )
    return 2;
  else if constexpr ( Fanout <= 16 )
    return 4;
  else
    return 8;
}

#if defined(__micron_x86_avx2)
inline constexpr usize __hash_window = 4;
#elif defined(__micron_x86_sse2)
inline constexpr usize __hash_window = 2;
#elif defined(__micron_arm_neon)
inline constexpr usize __hash_window = 2;
#else
inline constexpr usize __hash_window = 0;
#endif

inline constexpr usize __node_alignment = 64;

__attribute__((always_inline)) static inline u32
hash_scan_eq(const hash64_t *p, usize n, hash64_t h) noexcept
{
  u32 mask = 0;
#if defined(__micron_x86_avx2)
  const __m256i target = simd::avx::splat_i64(static_cast<long long>(h));
  usize i = 0;
  while ( i + 4 <= n ) {
    __m256i v = simd::avx::loadu_i256(reinterpret_cast<const __m256i_u *>(p + i));
    __m256i eq = simd::avx2::eq_i64(v, target);
    u32 b = static_cast<u32>(simd::avx2::movemask_i8(eq));
    u32 lane = 0;
    if ( b & (1u << 0) ) lane |= 1u << 0;
    if ( b & (1u << 8) ) lane |= 1u << 1;
    if ( b & (1u << 16) ) lane |= 1u << 2;
    if ( b & (1u << 24) ) lane |= 1u << 3;
    mask |= lane << i;
    i += 4;
  }
  while ( i < n ) {
    if ( p[i] == h ) mask |= 1u << i;
    ++i;
  }
#elif defined(__micron_x86_sse2)
  const __m128i target = simd::sse::splat_i64(static_cast<long long>(h));
  usize i = 0;
  while ( i + 2 <= n ) {
    __m128i v = simd::sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p + i));
    __m128i eq = simd::sse::eq_i64(v, target);
    u32 b = static_cast<u32>(static_cast<u16>(simd::sse::movemask_i8(eq)));
    u32 lane = 0;
    if ( b & (1u << 0) ) lane |= 1u << 0;
    if ( b & (1u << 8) ) lane |= 1u << 1;
    mask |= lane << i;
    i += 2;
  }
  while ( i < n ) {
    if ( p[i] == h ) mask |= 1u << i;
    ++i;
  }
#elif defined(__micron_arm_neon)

  for ( usize i = 0; i < n; ++i )
    if ( p[i] == h ) mask |= 1u << i;
#else
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == h ) mask |= 1u << i;
#endif
  return mask;
}

__attribute__((always_inline)) static inline usize
hash_scan_gt(const hash64_t *p, usize n, hash64_t h) noexcept
{
  usize i = 0;
  while ( i < n && p[i] <= h ) ++i;
  return i;
}

__attribute__((always_inline)) static inline usize
hash_bsearch_gt(const hash64_t *p, usize n, hash64_t h) noexcept
{
  usize lo = 0;
  usize hi = n;
  while ( lo < hi ) {
    const usize mid = lo + ((hi - lo) >> 1);
    if ( p[mid] <= h )
      lo = mid + 1;
    else
      hi = mid;
  }
  return lo;
}

template<usize N>
__attribute__((always_inline)) static inline usize
hash_route_gt(const hash64_t *p, usize n, hash64_t h) noexcept
{
  if constexpr ( N <= 8 )
    return hash_scan_gt(p, n, h);
  else
    return hash_bsearch_gt(p, n, h);
}

// NOTE: these are needed because it's impossible to adapt bits/__container for maps directly
//
// TODO: eventually move these out to bits/__maps or such

template<typename T>
inline constexpr bool is_byte_relocatable_v = micron::is_trivially_copyable_v<T> && micron::is_trivially_destructible_v<T>;

template<typename T>
__attribute__((always_inline)) static inline void
typed_shift_right(T *base, usize first, usize last) noexcept
{
  if ( first >= last ) return;
  if constexpr ( is_byte_relocatable_v<T> ) {
    const usize cnt = last - first;
    using __alias_byte = unsigned char __attribute__((may_alias));
    __alias_byte *d = reinterpret_cast<__alias_byte *>(base + first + 1);
    const __alias_byte *s = reinterpret_cast<const __alias_byte *>(base + first);
    const usize nbytes = cnt * sizeof(T);
    for ( usize i = nbytes; i > 0; --i ) d[i - 1] = s[i - 1];
  } else {
    for ( usize i = last; i > first; --i ) {
      new (micron::addr(base[i])) T(micron::move(base[i - 1]));
      base[i - 1].~T();
    }
  }
}

template<typename T>
__attribute__((always_inline)) static inline void
typed_erase_at(T *base, usize first, usize last) noexcept
{
  if ( first >= last ) return;
  if constexpr ( is_byte_relocatable_v<T> ) {
    const usize cnt = (first + 1 < last) ? (last - first - 1) : 0;
    if ( cnt ) {
      using __alias_byte = unsigned char __attribute__((may_alias));
      __alias_byte *d = reinterpret_cast<__alias_byte *>(base + first);
      const __alias_byte *s = reinterpret_cast<const __alias_byte *>(base + first + 1);
      const usize nbytes = cnt * sizeof(T);
      for ( usize i = 0; i < nbytes; ++i ) d[i] = s[i];
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
__attribute__((always_inline)) static inline void
typed_relocate_n(T *dest, T *src, usize cnt)
{
  if constexpr ( is_byte_relocatable_v<T> ) {
    using __alias_byte = unsigned char __attribute__((may_alias));
    __alias_byte *d = reinterpret_cast<__alias_byte *>(dest);
    const __alias_byte *s = reinterpret_cast<const __alias_byte *>(src);
    const usize nbytes = cnt * sizeof(T);
    for ( usize i = 0; i < nbytes; ++i ) d[i] = s[i];
  } else {
    for ( usize i = 0; i < cnt; ++i ) {
      new (micron::addr(dest[i])) T(micron::move(src[i]));
      src[i].~T();
    }
  }
}

__attribute__((always_inline)) static inline void
trivial_memmove(void *dst, const void *src, usize nbytes) noexcept
{
  using __alias_byte = unsigned char __attribute__((may_alias));
  __alias_byte *d = static_cast<__alias_byte *>(dst);
  const __alias_byte *s = static_cast<const __alias_byte *>(src);
  if ( d == s || nbytes == 0 ) return;
  if ( d < s ) {
    for ( usize i = 0; i < nbytes; ++i ) d[i] = s[i];
  } else {
    for ( usize i = nbytes; i > 0; --i ) d[i - 1] = s[i - 1];
  }
}

};      // namespace __btree_impl

// ON REFERENCE / ITERATOR INVALIDATION:
//  all nodes live in ONE contiguous node pool that is reallocated (moved) when it grows
//  this is a DELIBERATE performance tradeoff; a cache-friendly flat slab with no per-node allocation;
//  chosen over std::unordered_map-style reference stability
//  as a result ANY insert/emplace that grows the pool invalidates EVERY previously returned V*/V&/kv_ref AND every iterator at once
//  (references are __NOT__ stable across a grow/rehash, unlike std::unordered_map)
template<typename K, is_movable_object V, class Alloc = allocator_serial<>, usize FanoutOverride = 0> class btree_map
{
public:
  using node_idx = __btree_impl::node_idx;

private:
  static constexpr node_idx __k_empty = __btree_impl::__k_empty;
  static constexpr u8 __k_fsentinel = __btree_impl::__k_fsentinel;

  static constexpr usize __kv_bytes = sizeof(K) + sizeof(V);
  static constexpr usize __auto_leaf_fanout = __btree_impl::pick_leaf_fanout<__kv_bytes>();
  static constexpr usize __auto_int_fanout = __btree_impl::pick_internal_fanout<__kv_bytes>();
  static constexpr usize __leaf_fanout = (FanoutOverride > 0) ? FanoutOverride : __auto_leaf_fanout;
  static constexpr usize __int_fanout = (FanoutOverride > 0) ? FanoutOverride : __auto_int_fanout;
  static_assert(FanoutOverride == 0 || FanoutOverride >= 3, "btree_map: FanoutOverride must be 0 (auto) or >= 3");
  static constexpr usize __buf_size = __btree_impl::pick_buf_size<__int_fanout>();
  static constexpr usize __node_align = __btree_impl::__node_alignment;

  enum op_tag : u8 { OP_INSERT = 0, OP_ERASE = 1 };

  using raw_byte = unsigned char __attribute__((may_alias));

  struct pending_op {
    hash64_t hash;
    alignas(K) raw_byte key_raw[sizeof(K)];
    alignas(V) raw_byte value_raw[sizeof(V)];
    op_tag tag;
    u8 _pad[7];

    K *
    key_ptr() noexcept
    {
      return reinterpret_cast<K *>(key_raw);
    }

    V *
    value_ptr() noexcept
    {
      return reinterpret_cast<V *>(value_raw);
    }

    const K *
    key_ptr() const noexcept
    {
      return reinterpret_cast<const K *>(key_raw);
    }

    const V *
    value_ptr() const noexcept
    {
      return reinterpret_cast<const V *>(value_raw);
    }
  };

  static_assert(micron::is_trivially_destructible_v<pending_op>,
                "pending_op must be trivially destructible: drain_internal_buffer relies on raw storage "
                "with manually-managed K/V lifetimes inside a local array");

  struct alignas(__node_align) internal_node {
    u8 nkeys;
    u8 leaf_flag;
    u8 buf_used;
    u8 _hdr[5];
    hash64_t pivots[__int_fanout - 1];
    node_idx children[__int_fanout];
    pending_op buffer[__buf_size];

    internal_node() : nkeys(0), leaf_flag(0), buf_used(0)
    {
      for ( usize i = 0; i < __int_fanout; ++i ) children[i] = __k_empty;
    }

    ~internal_node()
    {
      for ( u8 i = 0; i < buf_used; ++i ) {
        buffer[i].key_ptr()->~K();
        if ( buffer[i].tag != OP_ERASE ) buffer[i].value_ptr()->~V();
      }
    }
  };

  struct alignas(__node_align) leaf_node {
    u8 nkeys;
    u8 leaf_flag;
    u8 _hdr[2];
    node_idx next_leaf;
    node_idx prev_leaf;
    node_idx overflow_next;
    hash64_t hashes[__leaf_fanout];
    alignas(K) raw_byte keys_raw[sizeof(K) * __leaf_fanout];
    alignas(V) raw_byte values_raw[sizeof(V) * __leaf_fanout];

    leaf_node() : nkeys(0), leaf_flag(1), next_leaf(__k_empty), prev_leaf(__k_empty), overflow_next(__k_empty)
    {
      for ( usize i = 0; i < __leaf_fanout; ++i ) hashes[i] = 0;
    }

    ~leaf_node()
    {
      K *kp = keys();
      V *vp = values();
      for ( u8 i = 0; i < nkeys; ++i ) {
        kp[i].~K();
        vp[i].~V();
      }
    }

    K *
    keys() noexcept
    {
      return static_cast<K *>(static_cast<void *>(&keys_raw[0]));
    }

    V *
    values() noexcept
    {
      return static_cast<V *>(static_cast<void *>(&values_raw[0]));
    }

    const K *
    keys() const noexcept
    {
      return static_cast<const K *>(static_cast<const void *>(&keys_raw[0]));
    }

    const V *
    values() const noexcept
    {
      return static_cast<const V *>(static_cast<const void *>(&values_raw[0]));
    }
  };

  struct free_slot_hdr {
    u8 sentinel;
    u8 _pad[3];
    node_idx next_free;
  };

  static constexpr usize __raw_max = sizeof(internal_node) > sizeof(leaf_node) ? sizeof(internal_node) : sizeof(leaf_node);
  static constexpr usize __slot_size = (__raw_max + (__node_align - 1)) & ~(__node_align - 1);

  struct alignas(__node_align) node_slot {
    byte raw[__slot_size];
  };

  using __mem = __immutable_memory_resource<node_slot, Alloc>;

  struct bucket_head {
    node_idx root;
    u32 size;
  };

  __mem __slab;
  bucket_head *buckets_;
  usize n_buckets_;
  usize mask_;
  usize total_size_;
  node_idx __fhead;
  node_idx leaf_list_head_;
  bool __rehashing;      // re-entrance guard

  static constexpr usize __min_buckets = 16;
  static constexpr usize __load_num = 7;
  static constexpr usize __load_denom = 8;

  __attribute__((always_inline)) byte *
  slot_raw(node_idx i) noexcept
  {
    return __slab.memory[i].raw;
  }

  __attribute__((always_inline)) const byte *
  slot_raw(node_idx i) const noexcept
  {
    return __slab.memory[i].raw;
  }

  __attribute__((always_inline)) bool
  slot_is_leaf(node_idx i) const noexcept
  {
    return slot_raw(i)[1] != 0;
  }

  __attribute__((always_inline)) internal_node &
  inode(node_idx i) noexcept
  {
    return *reinterpret_cast<internal_node *>(slot_raw(i));
  }

  __attribute__((always_inline)) const internal_node &
  inode(node_idx i) const noexcept
  {
    return *reinterpret_cast<const internal_node *>(slot_raw(i));
  }

  __attribute__((always_inline)) leaf_node &
  lnode(node_idx i) noexcept
  {
    return *reinterpret_cast<leaf_node *>(slot_raw(i));
  }

  __attribute__((always_inline)) const leaf_node &
  lnode(node_idx i) const noexcept
  {
    return *reinterpret_cast<const leaf_node *>(slot_raw(i));
  }

  static usize
  round_pow2(usize n) noexcept
  {
    if ( n <= __min_buckets ) return __min_buckets;
    usize p = 1;
    while ( p < n ) p <<= 1;
    return p;
  }

  static usize
  default_n_buckets() noexcept
  {
    usize auto_b = Alloc::auto_size() / sizeof(bucket_head);
    return round_pow2(auto_b < __min_buckets ? __min_buckets : auto_b);
  }

  static usize
  default_n_slots() noexcept
  {
    usize auto_s = Alloc::auto_size() / sizeof(node_slot);
    return round_pow2(auto_s < __min_buckets ? __min_buckets : auto_s);
  }

  void
  grow_slab()
  {

    usize old_cap = __slab.capacity;
    __slab.expand(old_cap == 0 ? __min_buckets : old_cap);
  }

  node_idx
  alloc_slot()
  {
    if ( __fhead != __k_empty ) {
      node_idx i = __fhead;
      free_slot_hdr *h = reinterpret_cast<free_slot_hdr *>(slot_raw(i));
      __fhead = h->next_free;
      return i;
    }
    if ( __slab.length >= __slab.capacity ) grow_slab();
    return static_cast<node_idx>(__slab.length++);
  }

  node_idx
  alloc_internal()
  {
    node_idx i = alloc_slot();
    new (slot_raw(i)) internal_node();
    return i;
  }

  node_idx
  alloc_leaf()
  {
    node_idx i = alloc_slot();
    new (slot_raw(i)) leaf_node();
    return i;
  }

  void
  free_slot(node_idx i) noexcept
  {
    if ( slot_is_leaf(i) ) {
      lnode(i).~leaf_node();
    } else {
      inode(i).~internal_node();
    }
    free_slot_hdr *h = reinterpret_cast<free_slot_hdr *>(slot_raw(i));
    h->sentinel = __k_fsentinel;
    h->_pad[0] = 0;
    h->_pad[1] = 0;
    h->_pad[2] = 0;
    h->next_free = __fhead;
    __fhead = i;
  }

  void
  alloc_buckets(usize n)
  {
    buckets_ = new bucket_head[n];
    for ( usize i = 0; i < n; ++i ) {
      buckets_[i].root = __k_empty;
      buckets_[i].size = 0;
    }
  }

  void
  free_buckets() noexcept
  {
    delete[] buckets_;
    buckets_ = nullptr;
  }

  void
  leaf_list_push_front(node_idx li) noexcept
  {
    leaf_node &L = lnode(li);
    L.prev_leaf = __k_empty;
    L.next_leaf = leaf_list_head_;
    if ( leaf_list_head_ != __k_empty ) lnode(leaf_list_head_).prev_leaf = li;
    leaf_list_head_ = li;
  }

  void
  leaf_list_unlink(node_idx li) noexcept
  {
    leaf_node &L = lnode(li);
    if ( L.prev_leaf != __k_empty )
      lnode(L.prev_leaf).next_leaf = L.next_leaf;
    else
      leaf_list_head_ = L.next_leaf;
    if ( L.next_leaf != __k_empty ) lnode(L.next_leaf).prev_leaf = L.prev_leaf;
    L.next_leaf = __k_empty;
    L.prev_leaf = __k_empty;
  }

  void
  leaf_list_insert_after(node_idx after_leaf, node_idx new_leaf) noexcept
  {
    leaf_node &A = lnode(after_leaf);
    leaf_node &N = lnode(new_leaf);
    N.prev_leaf = after_leaf;
    N.next_leaf = A.next_leaf;
    if ( A.next_leaf != __k_empty ) lnode(A.next_leaf).prev_leaf = new_leaf;
    A.next_leaf = new_leaf;
  }

  __attribute__((always_inline)) bool
  leaf_is_single_class(const leaf_node &L) const noexcept
  {
    return L.nkeys > 0 && L.hashes[0] == L.hashes[L.nkeys - 1];
  }

  __attribute__((always_inline)) node_idx
  leaf_chain_tail(node_idx primary) const noexcept
  {
    node_idx cur = primary;
    while ( lnode(cur).overflow_next != __k_empty ) cur = lnode(cur).overflow_next;
    return cur;
  }

  void
  free_leaf_and_chain(node_idx primary) noexcept
  {
    node_idx cur = primary;
    while ( cur != __k_empty ) {
      const node_idx next_ov = lnode(cur).overflow_next;
      leaf_list_unlink(cur);
      free_slot(cur);
      cur = next_ov;
    }
  }

  usize
  leaf_find_slot(const leaf_node &L, hash64_t h, const K &k) const noexcept
  {
    u32 m = __btree_impl::hash_scan_eq(L.hashes, L.nkeys, h);
    while ( m ) {
      u32 bit = static_cast<u32>(__builtin_ctz(m));
      if ( L.keys()[bit] == k ) return bit;
      m &= m - 1;
    }
    return __leaf_fanout;
  }

  usize
  leaf_insert_pos(const leaf_node &L, hash64_t h) const noexcept
  {
    usize i = 0;
    while ( i < L.nkeys && L.hashes[i] <= h ) ++i;
    return i;
  }

  int
  buffer_probe(const internal_node &N, hash64_t h, const K &k, V **out_value_ptr) const noexcept
  {
    u32 fields[__buf_size];
    u32 cnt = 0;
    for ( u8 i = 0; i < N.buf_used; ++i )
      if ( N.buffer[i].hash == h && *N.buffer[i].key_ptr() == k ) fields[cnt++] = i;
    if ( cnt == 0 ) return 0;

    u32 winner = fields[cnt - 1];
    const pending_op &op = N.buffer[winner];
    if ( op.tag == OP_ERASE ) return 2;
    *out_value_ptr = const_cast<V *>(op.value_ptr());
    return 1;
  }

  V *
  find_impl(hash64_t h, const K &k) noexcept
  {
    if ( !buckets_ || n_buckets_ == 0 ) return nullptr;
    bucket_head &b = buckets_[h & mask_];
    if ( b.root == __k_empty ) return nullptr;
    node_idx cur = b.root;
    while ( !slot_is_leaf(cur) ) {
      internal_node &N = inode(cur);
      __builtin_prefetch(N.children, 0, 1);
      V *bp = nullptr;
      int r = buffer_probe(N, h, k, &bp);
      if ( r == 1 ) return bp;
      if ( r == 2 ) return nullptr;
      usize ci = __btree_impl::hash_route_gt<__int_fanout>(N.pivots, N.nkeys, h);
      cur = N.children[ci];
      if ( cur == __k_empty ) return nullptr;
    }
    while ( cur != __k_empty ) {
      leaf_node &L = lnode(cur);
      const usize p = leaf_find_slot(L, h, k);
      if ( p != __leaf_fanout ) return micron::addr(L.values()[p]);
      cur = L.overflow_next;
    }
    return nullptr;
  }

  const V *
  find_impl(hash64_t h, const K &k) const noexcept
  {
    return const_cast<btree_map *>(this)->find_impl(h, k);
  }

  V *
  leaf_insert_no_split(node_idx li, hash64_t h, const K &k, V &&v, bool *replaced)
  {
    leaf_node &L = lnode(li);
    usize p = leaf_find_slot(L, h, k);
    if ( p != __leaf_fanout ) {
      L.values()[p] = micron::move(v);
      *replaced = true;
      return micron::addr(L.values()[p]);
    }
    const usize ip = leaf_insert_pos(L, h);
    const usize n_before = static_cast<usize>(L.nkeys);

    if ( ip < n_before ) {
      const usize count = n_before - ip;
      __btree_impl::trivial_memmove(micron::addr(L.hashes[ip + 1]), micron::addr(L.hashes[ip]), count * sizeof(hash64_t));
      __btree_impl::typed_shift_right(L.keys(), ip, n_before);
      __btree_impl::typed_shift_right(L.values(), ip, n_before);
    }
    L.hashes[ip] = h;
    new (L.keys() + ip) K(k);
    new (L.values() + ip) V(micron::move(v));
    ++L.nkeys;
    *replaced = false;
    return micron::addr(L.values()[ip]);
  }

  V *
  leaf_insert_no_split(node_idx li, hash64_t h, const K &k, const V &v, bool *replaced)
  {
    V vc = v;
    return leaf_insert_no_split(li, h, k, micron::move(vc), replaced);
  }

  micron::pair<node_idx, hash64_t>
  split_leaf_mixed(node_idx li)
  {
    usize mid;
    {
      const leaf_node &Lpre = lnode(li);
      const usize n = static_cast<usize>(Lpre.nkeys);
      mid = __leaf_fanout / 2;
      while ( mid < n && Lpre.hashes[mid - 1] == Lpre.hashes[mid] ) ++mid;
      if ( mid >= n ) {
        mid = __leaf_fanout / 2;
        while ( mid > 0 && Lpre.hashes[mid - 1] == Lpre.hashes[mid] ) --mid;
      }
    }
    node_idx ri = alloc_leaf();
    leaf_node &L = lnode(li);      // re-fetch after potential slab grow
    leaf_node &R = lnode(ri);
    R.nkeys = static_cast<u8>(L.nkeys - mid);
    __btree_impl::trivial_memmove(micron::addr(R.hashes[0]), micron::addr(L.hashes[mid]), R.nkeys * sizeof(hash64_t));
    __btree_impl::typed_relocate_n(R.keys(), L.keys() + mid, R.nkeys);
    __btree_impl::typed_relocate_n(R.values(), L.values() + mid, R.nkeys);
    L.nkeys = static_cast<u8>(mid);
    leaf_list_insert_after(li, ri);
    return { ri, R.hashes[0] };
  }

  micron::pair<node_idx, hash64_t>
  asymmetric_split_leaf(node_idx ni, hash64_t h, const K &k, V &&v, V **out_value, bool *out_inserted)
  {
    const hash64_t leaf_hash = lnode(ni).hashes[0];
    node_idx ri = alloc_leaf();

    if ( h > leaf_hash ) {
      leaf_node &R = lnode(ri);
      R.hashes[0] = h;
      new (R.keys()) K(k);
      new (R.values()) V(micron::move(v));
      R.nkeys = 1;
      const node_idx tail = leaf_chain_tail(ni);
      leaf_list_insert_after(tail, ri);
      *out_value = micron::addr(R.values()[0]);
      *out_inserted = true;
      return { ri, h };
    }
    leaf_node &L = lnode(ni);
    leaf_node &R = lnode(ri);
    const u8 n = L.nkeys;
    __btree_impl::trivial_memmove(R.hashes, L.hashes, static_cast<usize>(n) * sizeof(hash64_t));
    __btree_impl::typed_relocate_n(R.keys(), L.keys(), static_cast<usize>(n));
    __btree_impl::typed_relocate_n(R.values(), L.values(), static_cast<usize>(n));
    R.nkeys = n;
    R.overflow_next = L.overflow_next;
    L.overflow_next = __k_empty;
    L.nkeys = 0;
    L.hashes[0] = h;
    new (L.keys()) K(k);
    new (L.values()) V(micron::move(v));
    L.nkeys = 1;
    leaf_list_insert_after(ni, ri);
    *out_value = micron::addr(L.values()[0]);
    *out_inserted = true;
    return { ri, leaf_hash };
  }

  V *
  chain_insert_same_hash(node_idx primary, hash64_t h, const K &k, V &&v, bool *replaced)
  {
    node_idx cur = primary;
    node_idx tail = primary;
    node_idx with_space = (lnode(primary).nkeys < __leaf_fanout) ? primary : __k_empty;
    while ( cur != __k_empty ) {
      leaf_node &C = lnode(cur);
      const usize p = leaf_find_slot(C, h, k);
      if ( p != __leaf_fanout ) {
        C.values()[p] = micron::move(v);
        *replaced = true;
        return micron::addr(C.values()[p]);
      }
      if ( with_space == __k_empty && C.nkeys < __leaf_fanout ) with_space = cur;
      tail = cur;
      cur = C.overflow_next;
    }
    if ( with_space != __k_empty ) {
      return leaf_insert_no_split(with_space, h, k, micron::move(v), replaced);
    }
    node_idx new_li = alloc_leaf();
    leaf_node &Lnew = lnode(new_li);
    Lnew.hashes[0] = h;
    new (Lnew.keys()) K(k);
    new (Lnew.values()) V(micron::move(v));
    Lnew.nkeys = 1;
    lnode(tail).overflow_next = new_li;
    leaf_list_insert_after(tail, new_li);
    *replaced = false;
    return micron::addr(Lnew.values()[0]);
  }

  static inline void
  buffer_construct_key(pending_op &op, const K &k)
  {
    new (op.key_ptr()) K(k);
  }

  static inline void
  buffer_destroy_key(pending_op &op) noexcept
  {
    op.key_ptr()->~K();
  }

  static inline void
  buffer_construct_value(pending_op &op, V &&v)
  {
    new (op.value_ptr()) V(micron::move(v));
  }

  static inline void
  buffer_destroy_value(pending_op &op) noexcept
  {
    op.value_ptr()->~V();
  }

  static inline void
  buffer_relocate(pending_op &dst, pending_op &src)
  {
    dst.hash = src.hash;
    if constexpr ( micron::is_trivially_copyable_v<K> ) {
      using __alias_byte = unsigned char __attribute__((may_alias));
      __alias_byte *d = reinterpret_cast<__alias_byte *>(dst.key_raw);
      const __alias_byte *s = reinterpret_cast<const __alias_byte *>(src.key_raw);
      for ( usize i = 0; i < sizeof(K); ++i ) d[i] = s[i];
    } else {
      new (dst.key_ptr()) K(micron::move(*src.key_ptr()));
      src.key_ptr()->~K();
    }
    if ( src.tag != OP_ERASE ) {
      if constexpr ( micron::is_trivially_copyable_v<V> ) {
        using __alias_byte = unsigned char __attribute__((may_alias));
        __alias_byte *d = reinterpret_cast<__alias_byte *>(dst.value_raw);
        const __alias_byte *s = reinterpret_cast<const __alias_byte *>(src.value_raw);
        for ( usize i = 0; i < sizeof(V); ++i ) d[i] = s[i];
      } else {
        new (dst.value_ptr()) V(micron::move(*src.value_ptr()));
        src.value_ptr()->~V();
      }
    }
    dst.tag = src.tag;
  }

  void
  internal_insert_pivot(internal_node &N, usize ip, hash64_t new_pivot, node_idx right_child) noexcept
  {
    for ( usize j = N.nkeys; j > ip; --j ) N.pivots[j] = N.pivots[j - 1];
    for ( usize j = static_cast<usize>(N.nkeys) + 1; j > ip + 1; --j ) N.children[j] = N.children[j - 1];
    N.pivots[ip] = new_pivot;
    N.children[ip + 1] = right_child;
    ++N.nkeys;
  }

  void
  internal_insert_pivot_or_split(node_idx ii, usize ip, hash64_t new_pivot, node_idx right_child, hash64_t *out_split_pivot,
                                 node_idx *out_split_right)
  {
    *out_split_right = __k_empty;
    {
      internal_node &N = inode(ii);
      if ( N.nkeys < __int_fanout - 1 ) {
        internal_insert_pivot(N, ip, new_pivot, right_child);
        return;
      }
    }
    hash64_t exp_pivots[__int_fanout];
    node_idx exp_children[__int_fanout + 1];
    usize nb;
    {
      const internal_node &N = inode(ii);
      nb = static_cast<usize>(N.nkeys);
      for ( usize j = 0; j < nb; ++j ) exp_pivots[j] = N.pivots[j];
      for ( usize j = 0; j <= nb; ++j ) exp_children[j] = N.children[j];
    }
    for ( usize j = nb; j > ip; --j ) exp_pivots[j] = exp_pivots[j - 1];
    for ( usize j = nb + 1; j > ip + 1; --j ) exp_children[j] = exp_children[j - 1];
    exp_pivots[ip] = new_pivot;
    exp_children[ip + 1] = right_child;
    const usize total = nb + 1;

    node_idx ri = alloc_internal();
    const usize lmid = total / 2;
    const hash64_t promoted = exp_pivots[lmid];
    {
      internal_node &L = inode(ii);
      L.nkeys = static_cast<u8>(lmid);
      for ( usize j = 0; j < lmid; ++j ) L.pivots[j] = exp_pivots[j];
      for ( usize j = 0; j <= lmid; ++j ) L.children[j] = exp_children[j];
    }
    {
      internal_node &R = inode(ri);
      R.nkeys = static_cast<u8>(total - lmid - 1);
      for ( usize j = 0; j < R.nkeys; ++j ) R.pivots[j] = exp_pivots[lmid + 1 + j];
      for ( usize j = 0; j <= R.nkeys; ++j ) R.children[j] = exp_children[lmid + 1 + j];
    }
    *out_split_pivot = promoted;
    *out_split_right = ri;
  }

  V *
  descend_apply(node_idx ni, hash64_t h, const K &k, V *v_for_insert, op_tag tag, bool *out_inserted, hash64_t *out_split_pivot,
                node_idx *out_split_right)
  {
    *out_split_right = __k_empty;
    if ( slot_is_leaf(ni) ) {
      if ( tag == OP_ERASE ) {
        node_idx cur = ni;
        while ( cur != __k_empty ) {
          leaf_node &C = lnode(cur);
          const usize p = leaf_find_slot(C, h, k);
          if ( p != __leaf_fanout ) {
            const usize n_before = static_cast<usize>(C.nkeys);
            if ( p + 1 < n_before ) {
              const usize tail = n_before - p - 1;
              __btree_impl::trivial_memmove(micron::addr(C.hashes[p]), micron::addr(C.hashes[p + 1]), tail * sizeof(hash64_t));
            }
            __btree_impl::typed_erase_at(C.keys(), p, n_before);
            __btree_impl::typed_erase_at(C.values(), p, n_before);
            --C.nkeys;
            *out_inserted = true;
            return nullptr;
          }
          cur = C.overflow_next;
        }
        *out_inserted = false;
        return nullptr;
      }

      {
        node_idx cur = ni;
        while ( cur != __k_empty ) {
          leaf_node &C = lnode(cur);
          const usize p = leaf_find_slot(C, h, k);
          if ( p != __leaf_fanout ) {
            C.values()[p] = micron::move(*v_for_insert);
            *out_inserted = false;
            return micron::addr(C.values()[p]);
          }
          cur = C.overflow_next;
        }
      }

      bool replaced = false;
      leaf_node &L = lnode(ni);
      if ( L.nkeys < __leaf_fanout ) {
        V *vp = leaf_insert_no_split(ni, h, k, micron::move(*v_for_insert), &replaced);
        *out_inserted = !replaced;
        return vp;
      }
      if ( leaf_is_single_class(L) ) {
        const hash64_t leaf_hash = L.hashes[0];
        if ( h == leaf_hash ) {
          V *vp = chain_insert_same_hash(ni, h, k, micron::move(*v_for_insert), &replaced);
          *out_inserted = !replaced;
          return vp;
        }
        V *vp = nullptr;
        auto sp = asymmetric_split_leaf(ni, h, k, micron::move(*v_for_insert), &vp, out_inserted);
        *out_split_right = sp.a;
        *out_split_pivot = sp.b;
        return vp;
      }

      auto sp = split_leaf_mixed(ni);
      const node_idx right = sp.a;
      const hash64_t med = sp.b;
      const node_idx target = (h < med) ? ni : right;
      V *vp = leaf_insert_no_split(target, h, k, micron::move(*v_for_insert), &replaced);
      *out_inserted = !replaced;
      *out_split_pivot = med;
      *out_split_right = right;
      return vp;
    }

    {
      internal_node &N = inode(ni);
      for ( u8 i = 0; i < N.buf_used; ++i ) {
        if ( N.buffer[i].hash == h && *N.buffer[i].key_ptr() == k ) {
          const op_tag old_tag = static_cast<op_tag>(N.buffer[i].tag);
          if ( tag == OP_INSERT ) {
            if ( old_tag == OP_INSERT ) {
              *N.buffer[i].value_ptr() = micron::move(*v_for_insert);
            } else {
              new (N.buffer[i].value_ptr()) V(micron::move(*v_for_insert));
            }
            N.buffer[i].tag = OP_INSERT;
            *out_inserted = (old_tag == OP_ERASE);
            return N.buffer[i].value_ptr();
          } else {
            if ( old_tag == OP_INSERT ) {
              N.buffer[i].value_ptr()->~V();
            }
            N.buffer[i].tag = OP_ERASE;
            *out_inserted = (old_tag == OP_INSERT);
            return nullptr;
          }
        }
      }
      if ( N.buf_used < __buf_size ) {
        pending_op &op = N.buffer[N.buf_used];
        op.hash = h;
        new (op.key_ptr()) K(k);
        if ( tag == OP_INSERT ) new (op.value_ptr()) V(micron::move(*v_for_insert));
        op.tag = tag;
        ++N.buf_used;
        *out_inserted = true;
        return (tag == OP_INSERT) ? op.value_ptr() : nullptr;
      }
    }

    hash64_t drain_pivot = 0;
    node_idx drain_right = __k_empty;
    drain_internal_buffer(ni, &drain_pivot, &drain_right);

    if ( drain_right != __k_empty ) {
      *out_split_pivot = drain_pivot;
      *out_split_right = drain_right;
      const node_idx target = (h >= drain_pivot) ? drain_right : ni;
      usize ci;
      node_idx child;
      {
        const internal_node &T = inode(target);
        ci = __btree_impl::hash_route_gt<__int_fanout>(T.pivots, T.nkeys, h);
        child = T.children[ci];
      }
      bool dummy_inserted = false;
      hash64_t csp = 0;
      node_idx csr = __k_empty;
      V *vp = descend_apply(child, h, k, v_for_insert, tag, &dummy_inserted, &csp, &csr);
      *out_inserted = dummy_inserted;
      if ( csr != __k_empty ) {
        internal_node &T2 = inode(target);
        internal_insert_pivot(T2, ci, csp, csr);
      }
      return vp;
    }

    usize ci;
    node_idx child;
    {
      const internal_node &N2 = inode(ni);
      ci = __btree_impl::hash_route_gt<__int_fanout>(N2.pivots, N2.nkeys, h);
      child = N2.children[ci];
    }
    bool dummy_inserted = false;
    hash64_t csp = 0;
    node_idx csr = __k_empty;
    V *vp = descend_apply(child, h, k, v_for_insert, tag, &dummy_inserted, &csp, &csr);
    *out_inserted = dummy_inserted;
    if ( csr != __k_empty ) {
      internal_insert_pivot_or_split(ni, ci, csp, csr, out_split_pivot, out_split_right);
    }
    return vp;
  }

  V *
  descend_apply_drained(node_idx ni, pending_op &op, bool *out_inserted, hash64_t *out_split_pivot, node_idx *out_split_right)
  {
    if ( op.tag == OP_INSERT ) {
      return descend_apply(ni, op.hash, *op.key_ptr(), op.value_ptr(), OP_INSERT, out_inserted, out_split_pivot, out_split_right);
    }
    return descend_apply(ni, op.hash, *op.key_ptr(), nullptr, OP_ERASE, out_inserted, out_split_pivot, out_split_right);
  }

  void
  drain_internal_buffer(node_idx ii, hash64_t *out_split_pivot, node_idx *out_split_right)
  {
    *out_split_right = __k_empty;
    u8 cnt;
    {
      internal_node &N0 = inode(ii);
      cnt = N0.buf_used;
      if ( cnt == 0 ) return;
    }

    pending_op local[__buf_size];
    {
      internal_node &N0 = inode(ii);
      for ( u8 i = 0; i < cnt; ++i ) buffer_relocate(local[i], N0.buffer[i]);
      N0.buf_used = 0;
    }

    constexpr usize exp_cap = static_cast<usize>(__int_fanout - 1) + __buf_size;
    hash64_t exp_pivots[exp_cap];
    node_idx exp_children[exp_cap + 1];
    usize exp_n;
    {
      const internal_node &N0 = inode(ii);
      exp_n = static_cast<usize>(N0.nkeys);
      for ( usize j = 0; j < exp_n; ++j ) exp_pivots[j] = N0.pivots[j];
      for ( usize j = 0; j <= exp_n; ++j ) exp_children[j] = N0.children[j];
    }

#ifndef __micron_freestanding
    try {
#endif
      for ( u8 i = 0; i < cnt; ++i ) {
        const hash64_t op_hash = local[i].hash;
        const usize ci = __btree_impl::hash_route_gt<__int_fanout>(exp_pivots, exp_n, op_hash);
        const node_idx child = exp_children[ci];
        bool dummy_inserted = false;
        hash64_t csp = 0;
        node_idx csr = __k_empty;
        descend_apply_drained(child, local[i], &dummy_inserted, &csp, &csr);
        if ( local[i].tag == OP_INSERT ) {
          if ( !dummy_inserted && total_size_ > 0 ) --total_size_;
        } else {
          if ( !dummy_inserted ) ++total_size_;
        }
        if ( csr != __k_empty ) {
          for ( usize j = exp_n; j > ci; --j ) exp_pivots[j] = exp_pivots[j - 1];
          for ( usize j = exp_n + 1; j > ci + 1; --j ) exp_children[j] = exp_children[j - 1];
          exp_pivots[ci] = csp;
          exp_children[ci + 1] = csr;
          ++exp_n;
        }
      }
#ifndef __micron_freestanding
    } catch ( ... ) {
      // a throwing K/V move or an allocator failure mid-drain must not leak the
      // relocated ops still living in local[] (the normal destroy below is skipped)
      for ( u8 i = 0; i < cnt; ++i ) {
        buffer_destroy_key(local[i]);
        if ( local[i].tag != OP_ERASE ) buffer_destroy_value(local[i]);
      }
      throw;
    }
#endif

    for ( u8 i = 0; i < cnt; ++i ) {
      buffer_destroy_key(local[i]);
      if ( local[i].tag != OP_ERASE ) buffer_destroy_value(local[i]);
    }

    if ( exp_n <= static_cast<usize>(__int_fanout - 1) ) {
      internal_node &N = inode(ii);
      N.nkeys = static_cast<u8>(exp_n);
      for ( usize j = 0; j < exp_n; ++j ) N.pivots[j] = exp_pivots[j];
      for ( usize j = 0; j <= exp_n; ++j ) N.children[j] = exp_children[j];
      return;
    }
    node_idx ri = alloc_internal();
    const usize lmid = exp_n / 2;
    const hash64_t promoted = exp_pivots[lmid];
    {
      internal_node &L = inode(ii);
      L.nkeys = static_cast<u8>(lmid);
      for ( usize j = 0; j < lmid; ++j ) L.pivots[j] = exp_pivots[j];
      for ( usize j = 0; j <= lmid; ++j ) L.children[j] = exp_children[j];
    }
    {
      internal_node &R = inode(ri);
      R.nkeys = static_cast<u8>(exp_n - lmid - 1);
      for ( usize j = 0; j < R.nkeys; ++j ) R.pivots[j] = exp_pivots[lmid + 1 + j];
      for ( usize j = 0; j <= R.nkeys; ++j ) R.children[j] = exp_children[lmid + 1 + j];
    }
    *out_split_pivot = promoted;
    *out_split_right = ri;
  }

  micron::pair<node_idx, hash64_t>
  split_internal(node_idx ii)
  {
    hash64_t drain_pivot = 0;
    node_idx drain_right = __k_empty;
    drain_internal_buffer(ii, &drain_pivot, &drain_right);
    if ( drain_right != __k_empty ) return { drain_right, drain_pivot };

    internal_node &N = inode(ii);
    if ( N.nkeys < 2 ) return { __k_empty, 0 };
    node_idx ri = alloc_internal();
    internal_node &L = inode(ii);
    internal_node &R = inode(ri);
    const usize n = static_cast<usize>(L.nkeys);
    const usize mid = n / 2;
    const hash64_t promoted = L.pivots[mid];
    L.nkeys = static_cast<u8>(mid);
    R.nkeys = static_cast<u8>(n - mid - 1);
    for ( usize j = 0; j < R.nkeys; ++j ) R.pivots[j] = L.pivots[mid + 1 + j];
    for ( usize j = 0; j <= R.nkeys; ++j ) R.children[j] = L.children[mid + 1 + j];
    return { ri, promoted };
  }

  void
  ensure_root_capacity(bucket_head &b, hash64_t h)
  {
    (void)h;
    if ( b.root == __k_empty ) return;
    if ( slot_is_leaf(b.root) ) {
      leaf_node &L = lnode(b.root);
      if ( L.nkeys < __leaf_fanout ) return;
      if ( leaf_is_single_class(L) ) return;

      auto sp = split_leaf_mixed(b.root);
      node_idx new_root = alloc_internal();
      internal_node &NR = inode(new_root);
      NR.pivots[0] = sp.b;
      NR.children[0] = b.root;
      NR.children[1] = sp.a;
      NR.nkeys = 1;
      b.root = new_root;
      return;
    }
    internal_node &R = inode(b.root);
    if ( R.nkeys < __int_fanout - 1 ) return;

    auto sp = split_internal(b.root);
    if ( sp.a == __k_empty ) return;
    node_idx new_root = alloc_internal();
    internal_node &NR = inode(new_root);
    NR.pivots[0] = sp.b;
    NR.children[0] = b.root;
    NR.children[1] = sp.a;
    NR.nkeys = 1;
    b.root = new_root;
  }

  V *
  insert_impl(hash64_t h, const K &k, V &&v, bool *out_inserted)
  {
    maybe_rehash();
    bucket_head &b = buckets_[h & mask_];
    if ( b.root == __k_empty ) {
      node_idx li = alloc_leaf();
      bucket_head &b2 = buckets_[h & mask_];
      leaf_node &L = lnode(li);
      L.hashes[0] = h;
      new (L.keys()) K(k);
      new (L.values()) V(micron::move(v));
      L.nkeys = 1;
      b2.root = li;
      ++b2.size;
      ++total_size_;
      leaf_list_push_front(li);
      *out_inserted = true;
      return micron::addr(L.values()[0]);
    }
    ensure_root_capacity(b, h);
    bucket_head &b3 = buckets_[h & mask_];
    hash64_t split_pivot = 0;
    node_idx split_right = __k_empty;
    bool did_op = false;
    V *vp = descend_apply(b3.root, h, k, micron::addr(v), OP_INSERT, &did_op, &split_pivot, &split_right);
    if ( split_right != __k_empty ) {
      bucket_head &b4 = buckets_[h & mask_];
      node_idx new_root = alloc_internal();
      internal_node &NR = inode(new_root);
      NR.pivots[0] = split_pivot;
      NR.children[0] = b4.root;
      NR.children[1] = split_right;
      NR.nkeys = 1;
      b4.root = new_root;
    }
    bucket_head &b5 = buckets_[h & mask_];
    if ( did_op ) {
      ++b5.size;
      ++total_size_;
    }
    *out_inserted = did_op;
    return vp;
  }

  bool
  erase_impl(hash64_t h, const K &k)
  {
    if ( !buckets_ || n_buckets_ == 0 ) return false;
    bucket_head &b = buckets_[h & mask_];
    if ( b.root == __k_empty ) return false;
    ensure_root_capacity(b, h);
    bucket_head &b3 = buckets_[h & mask_];
    hash64_t split_pivot = 0;
    node_idx split_right = __k_empty;
    bool did_op = false;
    descend_apply(b3.root, h, k, nullptr, OP_ERASE, &did_op, &split_pivot, &split_right);
    if ( split_right != __k_empty ) {
      bucket_head &b4 = buckets_[h & mask_];
      node_idx new_root = alloc_internal();
      internal_node &NR = inode(new_root);
      NR.pivots[0] = split_pivot;
      NR.children[0] = b4.root;
      NR.children[1] = split_right;
      NR.nkeys = 1;
      b4.root = new_root;
    }
    if ( did_op ) {
      bucket_head &b5 = buckets_[h & mask_];
      if ( b5.size > 0 ) --b5.size;
      if ( total_size_ > 0 ) --total_size_;
    }
    return did_op;
  }

  void
  maybe_rehash()
  {
    if ( __rehashing ) return;
    if ( total_size_ * __load_denom < n_buckets_ * __leaf_fanout * __load_num ) return;
    rehash(n_buckets_ * 2);
  }

  void
  rehash(usize new_n_buckets)
  {
    __rehashing = true;
    bucket_head *new_buckets = new bucket_head[new_n_buckets];
    for ( usize i = 0; i < new_n_buckets; ++i ) {
      new_buckets[i].root = __k_empty;
      new_buckets[i].size = 0;
    }

    for ( usize bi = 0; bi < n_buckets_; ++bi ) {
      if ( buckets_[bi].root != __k_empty ) drain_recursive(buckets_[bi].root);
    }
    for ( usize bi = 0; bi < n_buckets_; ++bi ) {
      if ( buckets_[bi].root != __k_empty ) free_internals_recursive(buckets_[bi].root);
      buckets_[bi].root = __k_empty;
    }

    node_idx old_head = leaf_list_head_;
    leaf_list_head_ = __k_empty;

    delete[] buckets_;
    buckets_ = new_buckets;
    n_buckets_ = new_n_buckets;
    mask_ = new_n_buckets - 1;
    total_size_ = 0;

    node_idx cur = old_head;
    while ( cur != __k_empty ) {
      hash64_t hashes_copy[__leaf_fanout];
      alignas(K) raw_byte keys_raw_copy[sizeof(K) * __leaf_fanout];
      alignas(V) raw_byte values_raw_copy[sizeof(V) * __leaf_fanout];
      K *keys_copy = reinterpret_cast<K *>(static_cast<void *>(&keys_raw_copy[0]));
      V *values_copy = reinterpret_cast<V *>(static_cast<void *>(&values_raw_copy[0]));
      u8 n;
      node_idx next;
      {
        leaf_node &L = lnode(cur);
        next = L.next_leaf;
        n = L.nkeys;

        for ( u8 j = 0; j < n; ++j ) hashes_copy[j] = L.hashes[j];
        __btree_impl::typed_relocate_n(keys_copy, L.keys(), n);
        __btree_impl::typed_relocate_n(values_copy, L.values(), n);
        L.next_leaf = __k_empty;
        L.prev_leaf = __k_empty;
        L.nkeys = 0;
      }
      free_slot(cur);
      for ( u8 j = 0; j < n; ++j ) {
        bool dummy = false;
        insert_impl(hashes_copy[j], keys_copy[j], micron::move(values_copy[j]), &dummy);
      }

      for ( u8 j = 0; j < n; ++j ) {
        keys_copy[j].~K();
        values_copy[j].~V();
      }
      cur = next;
    }
    __rehashing = false;
  }

  void
  drain_recursive(node_idx ni)
  {
    if ( slot_is_leaf(ni) ) return;
    hash64_t dp = 0;
    node_idx dr = __k_empty;
    drain_internal_buffer(ni, &dp, &dr);
    {
      internal_node &N = inode(ni);
      u8 nc = static_cast<u8>(N.nkeys + 1);
      for ( u8 i = 0; i < nc; ++i )
        if ( N.children[i] != __k_empty ) drain_recursive(N.children[i]);
    }
    if ( dr != __k_empty ) {
      drain_recursive(dr);
      free_internals_recursive(dr);
    }
  }

  void
  free_internals_recursive(node_idx ni)
  {
    if ( slot_is_leaf(ni) ) return;
    internal_node &N = inode(ni);
    u8 nc = static_cast<u8>(N.nkeys + 1);
    node_idx kids[__int_fanout];
    for ( u8 i = 0; i < nc; ++i ) kids[i] = N.children[i];
    for ( u8 i = 0; i < nc; ++i )
      if ( kids[i] != __k_empty ) free_internals_recursive(kids[i]);
    free_slot(ni);
  }

  void
  free_tree(node_idx ni) noexcept
  {
    if ( ni == __k_empty ) return;
    if ( slot_is_leaf(ni) ) {
      free_leaf_and_chain(ni);
      return;
    }
    internal_node &N = inode(ni);
    u8 nc = static_cast<u8>(N.nkeys + 1);
    node_idx kids[__int_fanout];
    for ( u8 i = 0; i < nc; ++i ) kids[i] = N.children[i];
    for ( u8 i = 0; i < nc; ++i )
      if ( kids[i] != __k_empty ) free_tree(kids[i]);
    free_slot(ni);
  }

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using size_type = usize;
  using key_type = K;
  using mapped_type = V;
  using value_type = V;

  struct kv_ref {
    const K &key;
    V &value;
  };

  struct const_kv_ref {
    const K &key;
    const V &value;
  };

  class iterator
  {
    btree_map *m_;
    node_idx leaf_;
    u8 slot_;

  public:
    iterator(btree_map *m, node_idx leaf, u8 slot) : m_(m), leaf_(leaf), slot_(slot) { }

    kv_ref
    operator*() const
    {
      leaf_node &L = m_->lnode(leaf_);
      return { L.keys()[slot_], L.values()[slot_] };
    }

    iterator &
    operator++()
    {
      ++slot_;
      while ( leaf_ != __k_empty && slot_ >= m_->lnode(leaf_).nkeys ) {
        leaf_ = m_->lnode(leaf_).next_leaf;
        slot_ = 0;
      }
      return *this;
    }

    bool
    operator==(const iterator &o) const
    {
      return leaf_ == o.leaf_ && slot_ == o.slot_;
    }

    bool
    operator!=(const iterator &o) const
    {
      return !(*this == o);
    }
  };

  class const_iterator
  {
    const btree_map *m_;
    node_idx leaf_;
    u8 slot_;

  public:
    const_iterator(const btree_map *m, node_idx leaf, u8 slot) : m_(m), leaf_(leaf), slot_(slot) { }

    const_kv_ref
    operator*() const
    {
      const leaf_node &L = m_->lnode(leaf_);
      return { L.keys()[slot_], L.values()[slot_] };
    }

    const_iterator &
    operator++()
    {
      ++slot_;
      while ( leaf_ != __k_empty && slot_ >= m_->lnode(leaf_).nkeys ) {
        leaf_ = m_->lnode(leaf_).next_leaf;
        slot_ = 0;
      }
      return *this;
    }

    bool
    operator==(const const_iterator &o) const
    {
      return leaf_ == o.leaf_ && slot_ == o.slot_;
    }

    bool
    operator!=(const const_iterator &o) const
    {
      return !(*this == o);
    }
  };

  ~btree_map()
  {
    if ( buckets_ ) {
      clear();
      free_buckets();
    }
  }

  btree_map()
      : __slab(default_n_slots()), buckets_(nullptr), n_buckets_(0), mask_(0), total_size_(0), __fhead(__k_empty),
        leaf_list_head_(__k_empty), __rehashing(false)
  {
    n_buckets_ = default_n_buckets();
    mask_ = n_buckets_ - 1;
    alloc_buckets(n_buckets_);
  }

  explicit btree_map(usize n)
      : __slab(default_n_slots()), buckets_(nullptr), n_buckets_(0), mask_(0), total_size_(0), __fhead(__k_empty),
        leaf_list_head_(__k_empty), __rehashing(false)
  {
    n_buckets_ = round_pow2(n);
    mask_ = n_buckets_ - 1;
    alloc_buckets(n_buckets_);
  }

  btree_map(const btree_map &) = delete;

  btree_map(btree_map &&o) noexcept
      : __slab(micron::move(o.__slab)), buckets_(o.buckets_), n_buckets_(o.n_buckets_), mask_(o.mask_), total_size_(o.total_size_),
        __fhead(o.__fhead), leaf_list_head_(o.leaf_list_head_), __rehashing(o.__rehashing)
  {
    o.buckets_ = nullptr;
    o.n_buckets_ = 0;
    o.mask_ = 0;
    o.total_size_ = 0;
    o.__fhead = __k_empty;
    o.leaf_list_head_ = __k_empty;
    o.__rehashing = false;
  }

  btree_map &operator=(const btree_map &) = delete;

  btree_map &
  operator=(btree_map &&o) noexcept
  {
    if ( this == &o ) return *this;
    if ( buckets_ ) {
      clear();
      free_buckets();
    }
    __slab.free();
    __slab = micron::move(o.__slab);
    buckets_ = o.buckets_;
    n_buckets_ = o.n_buckets_;
    mask_ = o.mask_;
    total_size_ = o.total_size_;
    __fhead = o.__fhead;
    leaf_list_head_ = o.leaf_list_head_;
    __rehashing = o.__rehashing;
    o.buckets_ = nullptr;
    o.n_buckets_ = 0;
    o.mask_ = 0;
    o.total_size_ = 0;
    o.__fhead = __k_empty;
    o.leaf_list_head_ = __k_empty;
    o.__rehashing = false;
    return *this;
  }

  usize
  size() const noexcept
  {
    return total_size_;
  }

  bool
  empty() const noexcept
  {
    return total_size_ == 0;
  }

  usize
  max_size() const noexcept
  {
    return n_buckets_ * __leaf_fanout;
  }

  usize
  bucket_count() const noexcept
  {
    return n_buckets_;
  }

  float
  load_factor() const noexcept
  {
    return n_buckets_ > 0 ? static_cast<float>(total_size_) / static_cast<float>(n_buckets_ * __leaf_fanout) : 0.0f;
  }

  static constexpr usize
  leaf_fanout() noexcept
  {
    return __leaf_fanout;
  }

  static constexpr usize
  internal_fanout() noexcept
  {
    return __int_fanout;
  }

  static constexpr usize
  buffer_size() noexcept
  {
    return __buf_size;
  }

  void
  clear() noexcept
  {
    if ( !buckets_ ) return;
    for ( usize i = 0; i < n_buckets_; ++i ) {
      if ( buckets_[i].root != __k_empty ) {
        free_tree(buckets_[i].root);
        buckets_[i].root = __k_empty;
        buckets_[i].size = 0;
      }
    }
    leaf_list_head_ = __k_empty;
    total_size_ = 0;
    __slab.length = 0;
    __fhead = __k_empty;
  }

  void
  swap(btree_map &o) noexcept
  {
    {
      __mem tmp = micron::move(__slab);
      __slab = micron::move(o.__slab);
      o.__slab = micron::move(tmp);
    }
    {
      bucket_head *t = buckets_;
      buckets_ = o.buckets_;
      o.buckets_ = t;
    }
    {
      usize t = n_buckets_;
      n_buckets_ = o.n_buckets_;
      o.n_buckets_ = t;
    }
    {
      usize t = mask_;
      mask_ = o.mask_;
      o.mask_ = t;
    }
    {
      usize t = total_size_;
      total_size_ = o.total_size_;
      o.total_size_ = t;
    }
    {
      node_idx t = __fhead;
      __fhead = o.__fhead;
      o.__fhead = t;
    }
    {
      node_idx t = leaf_list_head_;
      leaf_list_head_ = o.leaf_list_head_;
      o.leaf_list_head_ = t;
    }
    {
      bool t = __rehashing;
      __rehashing = o.__rehashing;
      o.__rehashing = t;
    }
  }

  V *
  insert(const K &key, V &&value)
  {
    hash64_t h = hash<hash64_t>(key);
    bool inserted = false;
    return insert_impl(h, key, micron::move(value), &inserted);
  }

  V *
  insert(K &&key, V &&value)
  {
    K kc = micron::move(key);
    hash64_t h = hash<hash64_t>(kc);
    bool inserted = false;
    return insert_impl(h, kc, micron::move(value), &inserted);
  }

  V *
  insert(const K &key, const V &value)
  {
    V vc = value;
    return insert(key, micron::move(vc));
  }

  V *
  insert_hash(hash64_t h, const K &key, V &&value)
  {
    bool inserted = false;
    return insert_impl(h, key, micron::move(value), &inserted);
  }

  template<typename... Args>
  V *
  emplace(const K &key, Args &&...args)
  {
    return insert(key, V(micron::forward<Args>(args)...));
  }

  bool
  erase(const K &key)
  {
    return erase_impl(hash<hash64_t>(key), key);
  }

  bool
  erase_hash(hash64_t h, const K &key)
  {
    return erase_impl(h, key);
  }

  V *
  find(const K &key)
  {
    return find_impl(hash<hash64_t>(key), key);
  }

  const V *
  find(const K &key) const
  {
    return find_impl(hash<hash64_t>(key), key);
  }

  V *
  find_hash(hash64_t h, const K &key)
  {
    return find_impl(h, key);
  }

  const V *
  find_hash(hash64_t h, const K &key) const
  {
    return find_impl(h, key);
  }

  V &
  at(const K &key)
  {
    V *v = find(key);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("btree_map::at: key not found");
    return *v;
  }

  const V &
  at(const K &key) const
  {
    const V *v = find(key);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("btree_map::at: key not found");
    return *v;
  }

  V &
  operator[](const K &key)
    requires micron::is_default_constructible_v<V>
  {
    V *v = find(key);
    if ( v ) return *v;
    return *insert(key, V{});
  }

  bool
  contains(const K &key) const
  {
    return find(key) != nullptr;
  }

  usize
  count(const K &key) const
  {
    return contains(key) ? 1 : 0;
  }

  usize
  exists(const K &key) const
  {
    return count(key);
  }

  V *
  hash_min()
  {
    V *best = nullptr;
    hash64_t best_h = 0;
    node_idx l = leaf_list_head_;
    while ( l != __k_empty ) {
      leaf_node &L = lnode(l);
      for ( u8 i = 0; i < L.nkeys; ++i ) {
        if ( best == nullptr || L.hashes[i] < best_h ) {
          best = micron::addr(L.values()[i]);
          best_h = L.hashes[i];
        }
      }
      l = L.next_leaf;
    }
    return best;
  }

  const V *
  hash_min() const
  {
    return const_cast<btree_map *>(this)->hash_min();
  }

  V *
  hash_max()
  {
    V *best = nullptr;
    hash64_t best_h = 0;
    node_idx l = leaf_list_head_;
    while ( l != __k_empty ) {
      leaf_node &L = lnode(l);
      for ( u8 i = 0; i < L.nkeys; ++i ) {
        if ( best == nullptr || L.hashes[i] > best_h ) {
          best = micron::addr(L.values()[i]);
          best_h = L.hashes[i];
        }
      }
      l = L.next_leaf;
    }
    return best;
  }

  const V *
  hash_max() const
  {
    return const_cast<btree_map *>(this)->hash_max();
  }

  V *
  hash_lower_bound(const K &key)
  {
    const hash64_t h = hash<hash64_t>(key);
    V *best = nullptr;
    hash64_t best_h = 0;
    node_idx l = leaf_list_head_;
    while ( l != __k_empty ) {
      leaf_node &L = lnode(l);
      for ( u8 i = 0; i < L.nkeys; ++i ) {
        if ( L.hashes[i] >= h && (best == nullptr || L.hashes[i] < best_h) ) {
          best = micron::addr(L.values()[i]);
          best_h = L.hashes[i];
        }
      }
      l = L.next_leaf;
    }
    return best;
  }

  const V *
  hash_lower_bound(const K &key) const
  {
    return const_cast<btree_map *>(this)->hash_lower_bound(key);
  }

  V *
  hash_upper_bound(const K &key)
  {
    const hash64_t h = hash<hash64_t>(key);
    V *best = nullptr;
    hash64_t best_h = 0;
    node_idx l = leaf_list_head_;
    while ( l != __k_empty ) {
      leaf_node &L = lnode(l);
      for ( u8 i = 0; i < L.nkeys; ++i ) {
        if ( L.hashes[i] > h && (best == nullptr || L.hashes[i] < best_h) ) {
          best = micron::addr(L.values()[i]);
          best_h = L.hashes[i];
        }
      }
      l = L.next_leaf;
    }
    return best;
  }

  const V *
  hash_upper_bound(const K &key) const
  {
    return const_cast<btree_map *>(this)->hash_upper_bound(key);
  }

  V *
  hash_predecessor(const K &key)
  {
    const hash64_t h = hash<hash64_t>(key);
    V *best = nullptr;
    hash64_t best_h = 0;
    node_idx l = leaf_list_head_;
    while ( l != __k_empty ) {
      leaf_node &L = lnode(l);
      for ( u8 i = 0; i < L.nkeys; ++i ) {
        if ( L.hashes[i] < h && (best == nullptr || L.hashes[i] > best_h) ) {
          best = micron::addr(L.values()[i]);
          best_h = L.hashes[i];
        }
      }
      l = L.next_leaf;
    }
    return best;
  }

  const V *
  hash_predecessor(const K &key) const
  {
    return const_cast<btree_map *>(this)->hash_predecessor(key);
  }

  V *
  hash_successor(const K &key)
  {
    return hash_upper_bound(key);
  }

  const V *
  hash_successor(const K &key) const
  {
    return hash_upper_bound(key);
  }

  void
  flush_all() noexcept
  {
    if ( !buckets_ ) return;
    for ( usize bi = 0; bi < n_buckets_; ++bi ) {
      if ( buckets_[bi].root != __k_empty ) drain_recursive(buckets_[bi].root);
    }
  }

  iterator
  begin() noexcept
  {
    flush_all();
    node_idx l = leaf_list_head_;
    while ( l != __k_empty && lnode(l).nkeys == 0 ) l = lnode(l).next_leaf;
    return iterator(this, l, 0);
  }

  iterator
  end() noexcept
  {
    return iterator(this, __k_empty, 0);
  }

  const_iterator
  begin() const noexcept
  {
    const_cast<btree_map *>(this)->flush_all();
    node_idx l = leaf_list_head_;
    while ( l != __k_empty && lnode(l).nkeys == 0 ) l = lnode(l).next_leaf;
    return const_iterator(this, l, 0);
  }

  const_iterator
  end() const noexcept
  {
    return const_iterator(this, __k_empty, 0);
  }

  const_iterator
  cbegin() const noexcept
  {
    return begin();
  }

  const_iterator
  cend() const noexcept
  {
    return end();
  }

  template<typename Fn>
    requires micron::is_invocable_v<Fn, const K &, V &>
  void
  for_each(Fn &&fn)
  {
    for ( auto it = begin(); it != end(); ++it ) {
      auto kv = *it;
      fn(kv.key, const_cast<V &>(kv.value));
    }
  }

  template<typename Fn>
    requires micron::is_invocable_v<Fn, const K &, const V &>
  void
  for_each(Fn &&fn) const
  {
    for ( auto it = begin(); it != end(); ++it ) {
      auto kv = *it;
      fn(kv.key, kv.value);
    }
  }

  int
  height() const noexcept
  {
    int hmax = 0;
    for ( usize i = 0; i < n_buckets_; ++i ) {
      if ( buckets_[i].root == __k_empty ) continue;
      int h = 1;
      node_idx c = buckets_[i].root;
      while ( !slot_is_leaf(c) ) {
        ++h;
        c = inode(c).children[0];
        if ( c == __k_empty ) break;
      }
      if ( h > hmax ) hmax = h;
    }
    return hmax;
  }
};

template<typename K, typename V> using bmap = btree_map<K, V>;

};      // namespace micron
