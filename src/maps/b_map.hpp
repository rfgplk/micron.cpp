//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// btree_map: heap-allocated, resizable, fractal-tree-flavoured hashed-routing B-tree
//
// WARNING: IN DEV UNSAFE

#include "../bits/__arch.hpp"
#include "../concepts.hpp"
#include "../except.hpp"
#include "../hash/hash.hpp"
#include "../memory/actions.hpp"
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
inline constexpr node_idx kEmpty = 0xFFFFFFFFu;
inline constexpr u8 kFreeSentinel = 0xFFu;      // nkeys=0xFF marks a free slot

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

__attribute__((always_inline)) static inline void
bmemmove(void *dst, const void *src, usize nbytes) noexcept
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

__attribute__((always_inline)) static inline usize
hash_scan_gt(const hash64_t *p, usize n, hash64_t h) noexcept
{
  usize i = 0;
  while ( i < n && p[i] <= h ) ++i;
  return i;
}

};      // namespace __btree_impl

template<typename K, is_movable_object V, class Alloc = allocator_serial<>, usize FanoutOverride = 0>
  requires micron::is_default_constructible_v<K> and micron::is_default_constructible_v<V>
class btree_map
{
public:
  using node_idx = __btree_impl::node_idx;

private:
  static constexpr node_idx kEmpty = __btree_impl::kEmpty;
  static constexpr u8 kFreeSentinel = __btree_impl::kFreeSentinel;

  static constexpr usize __kv_bytes = sizeof(K) + sizeof(V);
  static constexpr usize __auto_leaf_fanout = __btree_impl::pick_leaf_fanout<__kv_bytes>();
  static constexpr usize __auto_int_fanout = __btree_impl::pick_internal_fanout<__kv_bytes>();
  static constexpr usize __leaf_fanout = (FanoutOverride > 0) ? FanoutOverride : __auto_leaf_fanout;
  static constexpr usize __int_fanout = (FanoutOverride > 0) ? FanoutOverride : __auto_int_fanout;
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
      for ( usize i = 0; i < __int_fanout; ++i ) children[i] = kEmpty;
    }

    ~internal_node()
    {
      for ( u8 i = 0; i < buf_used; ++i ) {
        buffer[i].key_ptr()->~K();
        buffer[i].value_ptr()->~V();
      }
    }
  };

  struct alignas(__node_align) leaf_node {
    u8 nkeys;
    u8 leaf_flag;
    u8 _hdr[2];
    node_idx next_leaf;
    node_idx prev_leaf;
    u8 _hdr2[4];
    hash64_t hashes[__leaf_fanout];
    alignas(K) raw_byte keys_raw[sizeof(K) * __leaf_fanout];
    alignas(V) raw_byte values_raw[sizeof(V) * __leaf_fanout];

    leaf_node() : nkeys(0), leaf_flag(1), next_leaf(kEmpty), prev_leaf(kEmpty)
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
  node_idx free_head_;
  node_idx leaf_list_head_;

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
    if ( free_head_ != kEmpty ) {
      node_idx i = free_head_;
      free_slot_hdr *h = reinterpret_cast<free_slot_hdr *>(slot_raw(i));
      free_head_ = h->next_free;
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
    h->sentinel = kFreeSentinel;
    h->_pad[0] = 0;
    h->_pad[1] = 0;
    h->_pad[2] = 0;
    h->next_free = free_head_;
    free_head_ = i;
  }

  void
  alloc_buckets(usize n)
  {
    buckets_ = new bucket_head[n];
    for ( usize i = 0; i < n; ++i ) {
      buckets_[i].root = kEmpty;
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
    L.prev_leaf = kEmpty;
    L.next_leaf = leaf_list_head_;
    if ( leaf_list_head_ != kEmpty ) lnode(leaf_list_head_).prev_leaf = li;
    leaf_list_head_ = li;
  }

  void
  leaf_list_unlink(node_idx li) noexcept
  {
    leaf_node &L = lnode(li);
    if ( L.prev_leaf != kEmpty )
      lnode(L.prev_leaf).next_leaf = L.next_leaf;
    else
      leaf_list_head_ = L.next_leaf;
    if ( L.next_leaf != kEmpty ) lnode(L.next_leaf).prev_leaf = L.prev_leaf;
    L.next_leaf = kEmpty;
    L.prev_leaf = kEmpty;
  }

  void
  leaf_list_insert_after(node_idx after_leaf, node_idx new_leaf) noexcept
  {
    leaf_node &A = lnode(after_leaf);
    leaf_node &N = lnode(new_leaf);
    N.prev_leaf = after_leaf;
    N.next_leaf = A.next_leaf;
    if ( A.next_leaf != kEmpty ) lnode(A.next_leaf).prev_leaf = new_leaf;
    A.next_leaf = new_leaf;
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
    if ( b.root == kEmpty ) return nullptr;
    node_idx cur = b.root;
    while ( !slot_is_leaf(cur) ) {
      internal_node &N = inode(cur);
      __builtin_prefetch(N.children, 0, 1);
      V *bp = nullptr;
      int r = buffer_probe(N, h, k, &bp);
      if ( r == 1 ) return bp;
      if ( r == 2 ) return nullptr;
      usize ci = __btree_impl::hash_scan_gt(N.pivots, N.nkeys, h);
      cur = N.children[ci];
      if ( cur == kEmpty ) return nullptr;
    }
    leaf_node &L = lnode(cur);
    usize p = leaf_find_slot(L, h, k);
    if ( p == __leaf_fanout ) return nullptr;
    return addr(L.values()[p]);
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
      return addr(L.values()[p]);
    }
    usize ip = leaf_insert_pos(L, h);

    if ( ip < L.nkeys ) {
      const usize count = L.nkeys - ip;
      __btree_impl::bmemmove(&L.hashes[ip + 1], &L.hashes[ip], count * sizeof(hash64_t));
      __btree_impl::bmemmove(L.keys() + (ip + 1), L.keys() + ip, count * sizeof(K));
      __btree_impl::bmemmove(L.values() + (ip + 1), L.values() + ip, count * sizeof(V));
    }
    L.hashes[ip] = h;
    new (L.keys() + ip) K(k);
    new (L.values() + ip) V(micron::move(v));
    ++L.nkeys;
    *replaced = false;
    return addr(L.values()[ip]);
  }

  V *
  leaf_insert_no_split(node_idx li, hash64_t h, const K &k, const V &v, bool *replaced)
  {
    V vc = v;
    return leaf_insert_no_split(li, h, k, micron::move(vc), replaced);
  }

  micron::pair<node_idx, hash64_t>
  split_leaf(node_idx li)
  {
    node_idx ri = alloc_leaf();
    leaf_node &L = lnode(li);
    leaf_node &R = lnode(ri);
    constexpr usize mid = __leaf_fanout / 2;
    R.nkeys = static_cast<u8>(L.nkeys - mid);

    __btree_impl::bmemmove(&R.hashes[0], &L.hashes[mid], R.nkeys * sizeof(hash64_t));
    __btree_impl::bmemmove(R.keys(), L.keys() + mid, R.nkeys * sizeof(K));
    __btree_impl::bmemmove(R.values(), L.values() + mid, R.nkeys * sizeof(V));
    L.nkeys = static_cast<u8>(mid);
    leaf_list_insert_after(li, ri);
    return { ri, R.hashes[0] };
  }

  micron::pair<node_idx, hash64_t>
  split_internal(node_idx ii)
  {

    drain_internal_buffer(ii);
    node_idx ri = alloc_internal();
    internal_node &L = inode(ii);
    internal_node &R = inode(ri);
    constexpr usize mid = __int_fanout / 2;
    hash64_t promoted = L.pivots[mid - 1];

    R.nkeys = static_cast<u8>(L.nkeys - mid);
    for ( usize j = 0; j < R.nkeys; ++j ) R.pivots[j] = L.pivots[mid + j];
    for ( usize j = 0; j <= R.nkeys; ++j ) R.children[j] = L.children[mid + j];
    L.nkeys = static_cast<u8>(mid - 1);
    return { ri, promoted };
  }

  void
  internal_insert_pivot(internal_node &N, usize ip, hash64_t h, node_idx right_child)
  {
    for ( usize j = N.nkeys; j > ip; --j ) N.pivots[j] = N.pivots[j - 1];
    for ( usize j = static_cast<usize>(N.nkeys) + 1; j > ip + 1; --j ) N.children[j] = N.children[j - 1];
    N.pivots[ip] = h;
    N.children[ip + 1] = right_child;
    ++N.nkeys;
  }

  V *
  descend_apply(node_idx ni, hash64_t h, const K &k, V &&v, op_tag tag, bool *out_inserted, hash64_t *out_split_pivot,
                node_idx *out_split_right)
  {
    *out_split_right = kEmpty;
    if ( slot_is_leaf(ni) ) {
      leaf_node &L = lnode(ni);
      if ( tag == OP_ERASE ) {
        usize p = leaf_find_slot(L, h, k);
        if ( p == __leaf_fanout ) {
          *out_inserted = false;
          return nullptr;
        }

        L.keys()[p].~K();
        L.values()[p].~V();
        if ( p + 1 < L.nkeys ) {
          const usize tail = L.nkeys - p - 1;
          __btree_impl::bmemmove(&L.hashes[p], &L.hashes[p + 1], tail * sizeof(hash64_t));
          __btree_impl::bmemmove(L.keys() + p, L.keys() + (p + 1), tail * sizeof(K));
          __btree_impl::bmemmove(L.values() + p, L.values() + (p + 1), tail * sizeof(V));
        }
        --L.nkeys;
        *out_inserted = true;
        return nullptr;
      }

      bool replaced = false;
      if ( L.nkeys < __leaf_fanout ) {
        V *vp = leaf_insert_no_split(ni, h, k, micron::move(v), &replaced);
        *out_inserted = !replaced;
        return vp;
      }

      usize ep = leaf_find_slot(L, h, k);
      if ( ep != __leaf_fanout ) {
        L.values()[ep] = micron::move(v);
        *out_inserted = false;
        return addr(L.values()[ep]);
      }

      auto sp = split_leaf(ni);
      node_idx right = sp.a;
      hash64_t med = sp.b;
      node_idx target = (h < med) ? ni : right;
      V *vp = leaf_insert_no_split(target, h, k, micron::move(v), &replaced);
      *out_inserted = !replaced;
      *out_split_pivot = med;
      *out_split_right = right;
      return vp;
    }

    internal_node &N = inode(ni);

    for ( u8 i = 0; i < N.buf_used; ++i ) {
      if ( N.buffer[i].hash == h && *N.buffer[i].key_ptr() == k ) {
        *N.buffer[i].value_ptr() = micron::move(v);
        N.buffer[i].tag = tag;
        *out_inserted = true;
        return N.buffer[i].value_ptr();
      }
    }
    if ( N.buf_used < __buf_size ) {
      pending_op &op = N.buffer[N.buf_used];
      op.hash = h;
      new (op.key_ptr()) K(k);
      new (op.value_ptr()) V(micron::move(v));
      op.tag = tag;
      ++N.buf_used;
      *out_inserted = true;
      return (tag == OP_ERASE) ? nullptr : op.value_ptr();
    }

    drain_internal_buffer(ni);

    internal_node &N2 = inode(ni);
    usize ci = __btree_impl::hash_scan_gt(N2.pivots, N2.nkeys, h);
    hash64_t child_split_pivot = 0;
    node_idx child_split_right = kEmpty;
    V *vp = descend_apply(N2.children[ci], h, k, micron::move(v), tag, out_inserted, &child_split_pivot, &child_split_right);
    if ( child_split_right != kEmpty ) {
      internal_node &N3 = inode(ni);
      if ( N3.nkeys < __int_fanout - 1 ) {
        internal_insert_pivot(N3, ci, child_split_pivot, child_split_right);
      } else {

        internal_insert_pivot(N3, ci, child_split_pivot, child_split_right);

        auto sp = split_internal(ni);
        *out_split_pivot = sp.b;
        *out_split_right = sp.a;
      }
    }
    return vp;
  }

  V *
  descend_apply_drained(node_idx ni, pending_op &op, bool *out_inserted, hash64_t *out_split_pivot, node_idx *out_split_right)
  {
    return descend_apply(ni, op.hash, *op.key_ptr(), micron::move(*op.value_ptr()), op.tag, out_inserted, out_split_pivot, out_split_right);
  }

  void
  drain_internal_buffer(node_idx ii)
  {

    u8 cnt;
    {
      internal_node &N0 = inode(ii);
      cnt = N0.buf_used;
      if ( cnt == 0 ) return;
    }
    pending_op local[__buf_size];
    {
      internal_node &N0 = inode(ii);

      __btree_impl::bmemmove(local, N0.buffer, cnt * sizeof(pending_op));
      N0.buf_used = 0;
    }
    for ( u8 i = 0; i < cnt; ++i ) {
      bool dummy_inserted = false;
      hash64_t split_pivot = 0;
      node_idx split_right = kEmpty;
      hash64_t op_hash;
      {
        internal_node &N = inode(ii);
        op_hash = local[i].hash;
        usize ci = __btree_impl::hash_scan_gt(N.pivots, N.nkeys, op_hash);
        node_idx child = N.children[ci];
        descend_apply_drained(child, local[i], &dummy_inserted, &split_pivot, &split_right);
      }
      if ( split_right != kEmpty ) {
        internal_node &N2 = inode(ii);
        usize ci = __btree_impl::hash_scan_gt(N2.pivots, N2.nkeys, op_hash);
        if ( N2.nkeys < __int_fanout - 1 ) internal_insert_pivot(N2, ci, split_pivot, split_right);
      }
    }

    for ( u8 i = 0; i < cnt; ++i ) {
      local[i].key_ptr()->~K();
      local[i].value_ptr()->~V();
    }
  }

  void
  ensure_root_capacity(bucket_head &b, hash64_t h)
  {
    if ( b.root == kEmpty ) return;
    if ( slot_is_leaf(b.root) ) {
      leaf_node &L = lnode(b.root);
      if ( L.nkeys < __leaf_fanout ) return;

      auto sp = split_leaf(b.root);
      node_idx new_root = alloc_internal();
      internal_node &NR = inode(new_root);
      NR.pivots[0] = sp.b;
      NR.children[0] = b.root;
      NR.children[1] = sp.a;
      NR.nkeys = 1;
      b.root = new_root;
      (void)h;
      return;
    }
    internal_node &R = inode(b.root);
    if ( R.nkeys < __int_fanout - 1 ) return;

    auto sp = split_internal(b.root);
    node_idx new_root = alloc_internal();
    internal_node &NR = inode(new_root);
    NR.pivots[0] = sp.b;
    NR.children[0] = b.root;
    NR.children[1] = sp.a;
    NR.nkeys = 1;
    b.root = new_root;
    (void)h;
  }

  V *
  insert_impl(hash64_t h, const K &k, V &&v, op_tag tag, bool *out_inserted)
  {
    maybe_rehash();
    bucket_head &b = buckets_[h & mask_];
    if ( b.root == kEmpty ) {
      if ( tag == OP_ERASE ) {
        *out_inserted = false;
        return nullptr;
      }
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
      return addr(L.values()[0]);
    }
    ensure_root_capacity(b, h);
    bucket_head &b3 = buckets_[h & mask_];
    hash64_t split_pivot = 0;
    node_idx split_right = kEmpty;
    bool did_op = false;
    V *vp = descend_apply(b3.root, h, k, micron::move(v), tag, &did_op, &split_pivot, &split_right);
    if ( split_right != kEmpty ) {

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
      if ( tag == OP_INSERT ) {

        ++b5.size;
        ++total_size_;
      } else if ( tag == OP_ERASE ) {
        if ( b5.size > 0 ) --b5.size;
        if ( total_size_ > 0 ) --total_size_;
      }
    }
    *out_inserted = did_op;
    return vp;
  }

  void
  maybe_rehash()
  {
    if ( total_size_ * __load_denom < n_buckets_ * __leaf_fanout * __load_num ) return;
    rehash(n_buckets_ * 2);
  }

  void
  rehash(usize new_n_buckets)
  {

    bucket_head *new_buckets = new bucket_head[new_n_buckets];
    for ( usize i = 0; i < new_n_buckets; ++i ) {
      new_buckets[i].root = kEmpty;
      new_buckets[i].size = 0;
    }

    for ( usize bi = 0; bi < n_buckets_; ++bi ) {
      if ( buckets_[bi].root != kEmpty ) drain_recursive(buckets_[bi].root);
    }
    for ( usize bi = 0; bi < n_buckets_; ++bi ) {
      if ( buckets_[bi].root != kEmpty ) free_internals_recursive(buckets_[bi].root);
      buckets_[bi].root = kEmpty;
    }

    node_idx old_head = leaf_list_head_;
    leaf_list_head_ = kEmpty;

    delete[] buckets_;
    buckets_ = new_buckets;
    n_buckets_ = new_n_buckets;
    mask_ = new_n_buckets - 1;
    usize old_total = total_size_;
    total_size_ = 0;

    node_idx cur = old_head;
    while ( cur != kEmpty ) {
      hash64_t hashes_copy[__leaf_fanout];
      alignas(K) raw_byte keys_raw_copy[sizeof(K) * __leaf_fanout];
      alignas(V) raw_byte values_raw_copy[sizeof(V) * __leaf_fanout];
      K *keys_copy = reinterpret_cast<K *>(keys_raw_copy);
      V *values_copy = reinterpret_cast<V *>(values_raw_copy);
      u8 n;
      node_idx next;
      {
        leaf_node &L = lnode(cur);
        next = L.next_leaf;
        n = L.nkeys;

        __btree_impl::bmemmove(hashes_copy, L.hashes, n * sizeof(hash64_t));
        __btree_impl::bmemmove(keys_copy, L.keys(), n * sizeof(K));
        __btree_impl::bmemmove(values_copy, L.values(), n * sizeof(V));
        L.next_leaf = kEmpty;
        L.prev_leaf = kEmpty;
        L.nkeys = 0;
      }
      free_slot(cur);
      for ( u8 j = 0; j < n; ++j ) {
        bool dummy = false;
        insert_impl(hashes_copy[j], keys_copy[j], micron::move(values_copy[j]), OP_INSERT, &dummy);
      }

      for ( u8 j = 0; j < n; ++j ) {
        keys_copy[j].~K();
        values_copy[j].~V();
      }
      cur = next;
    }
    (void)old_total;
  }

  void
  drain_recursive(node_idx ni)
  {
    if ( slot_is_leaf(ni) ) return;
    drain_internal_buffer(ni);
    internal_node &N = inode(ni);
    u8 nc = static_cast<u8>(N.nkeys + 1);
    for ( u8 i = 0; i < nc; ++i )
      if ( N.children[i] != kEmpty ) drain_recursive(N.children[i]);
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
      if ( kids[i] != kEmpty ) free_internals_recursive(kids[i]);
    free_slot(ni);
  }

  void
  free_tree(node_idx ni) noexcept
  {
    if ( ni == kEmpty ) return;
    if ( slot_is_leaf(ni) ) {
      leaf_list_unlink(ni);
      free_slot(ni);
      return;
    }
    internal_node &N = inode(ni);
    u8 nc = static_cast<u8>(N.nkeys + 1);
    node_idx kids[__int_fanout];
    for ( u8 i = 0; i < nc; ++i ) kids[i] = N.children[i];
    for ( u8 i = 0; i < nc; ++i )
      if ( kids[i] != kEmpty ) free_tree(kids[i]);
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
      while ( leaf_ != kEmpty && slot_ >= m_->lnode(leaf_).nkeys ) {
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
      while ( leaf_ != kEmpty && slot_ >= m_->lnode(leaf_).nkeys ) {
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
      : __slab(default_n_slots()), buckets_(nullptr), n_buckets_(0), mask_(0), total_size_(0), free_head_(kEmpty), leaf_list_head_(kEmpty)
  {
    n_buckets_ = default_n_buckets();
    mask_ = n_buckets_ - 1;
    alloc_buckets(n_buckets_);
  }

  explicit btree_map(usize n)
      : __slab(default_n_slots()), buckets_(nullptr), n_buckets_(0), mask_(0), total_size_(0), free_head_(kEmpty), leaf_list_head_(kEmpty)
  {
    n_buckets_ = round_pow2(n);
    mask_ = n_buckets_ - 1;
    alloc_buckets(n_buckets_);
  }

  btree_map(const btree_map &) = delete;

  btree_map(btree_map &&o) noexcept
      : __slab(micron::move(o.__slab)), buckets_(o.buckets_), n_buckets_(o.n_buckets_), mask_(o.mask_), total_size_(o.total_size_),
        free_head_(o.free_head_), leaf_list_head_(o.leaf_list_head_)
  {
    o.buckets_ = nullptr;
    o.n_buckets_ = 0;
    o.mask_ = 0;
    o.total_size_ = 0;
    o.free_head_ = kEmpty;
    o.leaf_list_head_ = kEmpty;
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
    __slab = micron::move(o.__slab);
    buckets_ = o.buckets_;
    n_buckets_ = o.n_buckets_;
    mask_ = o.mask_;
    total_size_ = o.total_size_;
    free_head_ = o.free_head_;
    leaf_list_head_ = o.leaf_list_head_;
    o.buckets_ = nullptr;
    o.n_buckets_ = 0;
    o.mask_ = 0;
    o.total_size_ = 0;
    o.free_head_ = kEmpty;
    o.leaf_list_head_ = kEmpty;
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
      if ( buckets_[i].root != kEmpty ) {
        free_tree(buckets_[i].root);
        buckets_[i].root = kEmpty;
        buckets_[i].size = 0;
      }
    }
    leaf_list_head_ = kEmpty;
    total_size_ = 0;
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
      node_idx t = free_head_;
      free_head_ = o.free_head_;
      o.free_head_ = t;
    }
    {
      node_idx t = leaf_list_head_;
      leaf_list_head_ = o.leaf_list_head_;
      o.leaf_list_head_ = t;
    }
  }

  V *
  insert(const K &key, V &&value)
  {
    hash64_t h = hash<hash64_t>(key);
    bool inserted = false;
    return insert_impl(h, key, micron::move(value), OP_INSERT, &inserted);
  }

  V *
  insert(K &&key, V &&value)
  {
    K kc = micron::move(key);
    hash64_t h = hash<hash64_t>(kc);
    bool inserted = false;
    return insert_impl(h, kc, micron::move(value), OP_INSERT, &inserted);
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
    return insert_impl(h, key, micron::move(value), OP_INSERT, &inserted);
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
    hash64_t h = hash<hash64_t>(key);
    bool did = false;
    V tmp{};
    insert_impl(h, key, micron::move(tmp), OP_ERASE, &did);
    return did;
  }

  bool
  erase_hash(hash64_t h, const K &key)
  {
    bool did = false;
    V tmp{};
    insert_impl(h, key, micron::move(tmp), OP_ERASE, &did);
    return did;
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
  min()
  {
    node_idx l = leaf_list_head_;
    while ( l != kEmpty && lnode(l).nkeys == 0 ) l = lnode(l).next_leaf;
    if ( l == kEmpty ) return nullptr;
    return addr(lnode(l).values()[0]);
  }

  const V *
  min() const
  {
    return const_cast<btree_map *>(this)->min();
  }

  V *
  max()
  {
    node_idx l = leaf_list_head_;
    if ( l == kEmpty ) return nullptr;
    node_idx best = kEmpty;
    while ( l != kEmpty ) {
      if ( lnode(l).nkeys > 0 ) best = l;
      l = lnode(l).next_leaf;
    }
    if ( best == kEmpty ) return nullptr;
    leaf_node &L = lnode(best);
    return addr(L.values()[L.nkeys - 1]);
  }

  const V *
  max() const
  {
    return const_cast<btree_map *>(this)->max();
  }

  V *
  lower_bound(const K &key)
  {
    hash64_t h = hash<hash64_t>(key);
    node_idx l = leaf_list_head_;
    while ( l != kEmpty ) {
      leaf_node &L = lnode(l);
      for ( u8 i = 0; i < L.nkeys; ++i ) {
        if ( L.hashes[i] >= h ) return addr(L.values()[i]);
      }
      l = L.next_leaf;
    }
    return nullptr;
  }

  const V *
  lower_bound(const K &key) const
  {
    return const_cast<btree_map *>(this)->lower_bound(key);
  }

  V *
  upper_bound(const K &key)
  {
    hash64_t h = hash<hash64_t>(key);
    node_idx l = leaf_list_head_;
    while ( l != kEmpty ) {
      leaf_node &L = lnode(l);
      for ( u8 i = 0; i < L.nkeys; ++i ) {
        if ( L.hashes[i] > h ) return addr(L.values()[i]);
      }
      l = L.next_leaf;
    }
    return nullptr;
  }

  const V *
  upper_bound(const K &key) const
  {
    return const_cast<btree_map *>(this)->upper_bound(key);
  }

  V *
  predecessor(const K &key)
  {
    hash64_t h = hash<hash64_t>(key);
    node_idx l = leaf_list_head_;
    V *best = nullptr;
    while ( l != kEmpty ) {
      leaf_node &L = lnode(l);
      for ( u8 i = 0; i < L.nkeys; ++i ) {
        if ( L.hashes[i] < h ) best = addr(L.values()[i]);
      }
      l = L.next_leaf;
    }
    return best;
  }

  const V *
  predecessor(const K &key) const
  {
    return const_cast<btree_map *>(this)->predecessor(key);
  }

  V *
  successor(const K &key)
  {
    return upper_bound(key);
  }

  const V *
  successor(const K &key) const
  {
    return upper_bound(key);
  }

  void
  flush_all() noexcept
  {
    if ( !buckets_ ) return;
    for ( usize bi = 0; bi < n_buckets_; ++bi ) {
      if ( buckets_[bi].root != kEmpty ) drain_recursive(buckets_[bi].root);
    }
  }

  iterator
  begin() noexcept
  {
    flush_all();
    node_idx l = leaf_list_head_;
    while ( l != kEmpty && lnode(l).nkeys == 0 ) l = lnode(l).next_leaf;
    return iterator(this, l, 0);
  }

  iterator
  end() noexcept
  {
    return iterator(this, kEmpty, 0);
  }

  const_iterator
  begin() const noexcept
  {
    const_cast<btree_map *>(this)->flush_all();
    node_idx l = leaf_list_head_;
    while ( l != kEmpty && lnode(l).nkeys == 0 ) l = lnode(l).next_leaf;
    return const_iterator(this, l, 0);
  }

  const_iterator
  end() const noexcept
  {
    return const_iterator(this, kEmpty, 0);
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

  int
  height() const noexcept
  {
    int hmax = 0;
    for ( usize i = 0; i < n_buckets_; ++i ) {
      if ( buckets_[i].root == kEmpty ) continue;
      int h = 1;
      node_idx c = buckets_[i].root;
      while ( !slot_is_leaf(c) ) {
        ++h;
        c = inode(c).children[0];
        if ( c == kEmpty ) break;
      }
      if ( h > hmax ) hmax = h;
    }
    return hmax;
  }
};

template<typename K, typename V> using bmap = btree_map<K, V>;

};      // namespace micron
