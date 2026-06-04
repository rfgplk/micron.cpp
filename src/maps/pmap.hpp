//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/intrin.hpp"
#include "../bits.hpp"
#include "../hash/hash.hpp"
#include "../types.hpp"

#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/addr.hpp"
#include "../memory/new.hpp"
#include "../tuple.hpp"

namespace micron
{

// pmap
//
// persistent (immutable) hash array mapped trie (HAMT) with 32-way branching
// every mutation returns a fresh pmap that path-copies the modified spine and
// shares the rest with the source
//
// reclamation is driven by atomic intrusive refcounts on each shared node;
// the count saturates at __immortal_refs and the node becomes permanently live
//
// COLLISION CHAINS: keys whose 64-bit hashes are equal (or that stay equal past
// __max_depth re-mixes) share a leaf's overflow chain, which has no length bound
template<typename K, typename V>
  requires micron::is_copy_constructible_v<K> and micron::is_copy_constructible_v<V> and micron::is_move_constructible_v<K>
           and micron::is_move_constructible_v<V>
class pmap
{
  static constexpr usize __log2B = 5;
  static constexpr usize __B = 1u << __log2B;      // 32-way branching
  static constexpr u32 __mask32 = static_cast<u32>(__B - 1u);
  static constexpr usize __natural_depth = 12;           // 12 * 5 = 60 natural hash bits
  static constexpr usize __max_depth = 24;               // one re-mix grants 60 more bits
  static constexpr u32 __split_threshold = 8;            // grow leaf entries up to this, then split
  static constexpr u32 __immortal_refs = ~u32{ 0 };      // saturating immortal sentinel

  //   bit 0 = 1 -> leaf
  //   bit 0 = 0 -> internal (or 0 null)
  static constexpr uintptr_t __tag_leaf_bit = 1;
  static constexpr uintptr_t __tag_mask = 1;

  struct __internal;
  struct __leaf;
  struct __chain;

  static inline __attribute__((always_inline)) bool
  __is_leaf(uintptr_t p) noexcept
  {
    return (p & __tag_mask) == __tag_leaf_bit;
  }

  static inline __attribute__((always_inline)) __leaf *
  __as_leaf(uintptr_t p) noexcept
  {
    return reinterpret_cast<__leaf *>(p & ~__tag_mask);
  }

  static inline __attribute__((always_inline)) __internal *
  __as_internal(uintptr_t p) noexcept
  {
    return reinterpret_cast<__internal *>(p);
  }

  static inline __attribute__((always_inline)) uintptr_t
  __tag_leaf_ptr(__leaf *l) noexcept
  {
    return reinterpret_cast<uintptr_t>(l) | __tag_leaf_bit;
  }

  static inline __attribute__((always_inline)) uintptr_t
  __tag_internal_ptr(__internal *in) noexcept
  {
    return reinterpret_cast<uintptr_t>(in);
  }

  // internal node shape
  struct __internal {
    mutable u32 refs;
    u32 bitmap;
  };

  // chain links
  struct __chain {
    hash64_t h;
    __chain *next;
    K key;
    V value;

    __chain(hash64_t hh, const K &k, const V &v, __chain *nx = nullptr) : h(hh), next(nx), key(k), value(v) { }

    __chain(hash64_t hh, K &&k, V &&v, __chain *nx = nullptr) : h(hh), next(nx), key(micron::move(k)), value(micron::move(v)) { }
  };

  // leaf: refcount + first entry fused inline + zero-or-more chained overflow

  struct __leaf {
    mutable u32 refs;
    u32 overflow_len;
    hash64_t h;
    __chain *overflow;
    K key;
    V value;

    __leaf(hash64_t hh, const K &k, const V &v) : refs(1), overflow_len(0), h(hh), overflow(nullptr), key(k), value(v) { }

    __leaf(hash64_t hh, K &&k, V &&v) : refs(1), overflow_len(0), h(hh), overflow(nullptr), key(micron::move(k)), value(micron::move(v)) { }
  };

  static inline __attribute__((always_inline)) uintptr_t *
  __children_of(__internal *in) noexcept
  {
    return reinterpret_cast<uintptr_t *>(reinterpret_cast<byte *>(in) + sizeof(__internal));
  }

  static inline __attribute__((always_inline)) const uintptr_t *
  __children_of(const __internal *in) noexcept
  {
    return reinterpret_cast<const uintptr_t *>(reinterpret_cast<const byte *>(in) + sizeof(__internal));
  }

  static inline __attribute__((always_inline)) bool
  __has(u32 bitmap, u32 logical_idx) noexcept
  {
    return (bitmap >> logical_idx) & 1u;
  }

  static inline __attribute__((always_inline)) usize
  __phys_idx(u32 bitmap, u32 logical_idx) noexcept
  {
    return static_cast<usize>(micron::popcount(bitmap & ((1u << logical_idx) - 1u)));
  }

  static __internal *
  __alloc_internal(u32 bitmap)
  {
    usize n = static_cast<usize>(micron::popcount(bitmap));
    void *raw = ::operator new(sizeof(__internal) + n * sizeof(uintptr_t));
    auto *in = static_cast<__internal *>(raw);
    in->refs = 1;
    in->bitmap = bitmap;
    uintptr_t *cs = __children_of(in);
    for ( usize i = 0; i < n; ++i ) cs[i] = 0;
    return in;
  }

  static void
  __destroy_chain(__chain *c) noexcept
  {
    while ( c ) {
      __chain *nx = c->next;
      delete c;
      c = nx;
    }
  }

  static __chain *
  __copy_chain(const __chain *src)
  {
    __chain *head = nullptr;
    __chain **tail = &head;
#ifndef __micron_freestanding
    try {
#endif
      for ( const __chain *c = src; c; c = c->next ) {
        *tail = new __chain(c->h, c->key, c->value);
        tail = &(*tail)->next;
      }
#ifndef __micron_freestanding
    } catch ( ... ) {
      __destroy_chain(head);      // a throwing K/V copy must not leak the partial chain
      throw;
    }
#endif
    return head;
  }

  // once a refcount reaches __immortal_refs the node is treated as permanently live
  static inline __attribute__((always_inline)) void
  __retain_refs(u32 *p) noexcept
  {
    u32 cur = atom::load(p, __ATOMIC_RELAXED);
    for ( ;; ) {
      if ( cur == __immortal_refs ) [[unlikely]]
        return;
      if ( atom::compare_exchange(p, &cur, cur + 1u, true, __ATOMIC_RELAXED, __ATOMIC_RELAXED) ) return;
    }
  }

  static inline __attribute__((always_inline)) bool
  __dec_refs(u32 *p) noexcept
  {
    u32 cur = atom::load(p, __ATOMIC_RELAXED);
    for ( ;; ) {
      if ( cur == __immortal_refs ) [[unlikely]]
        return false;
      if ( atom::compare_exchange(p, &cur, cur - 1u, true, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED) ) return cur - 1u == 0u;
    }
  }

  static inline __attribute__((always_inline)) void
  __retain(uintptr_t p) noexcept
  {
    if ( !p ) return;
    if ( __is_leaf(p) )
      __retain_refs(&__as_leaf(p)->refs);
    else
      __retain_refs(&__as_internal(p)->refs);
  }

  static void
  __release(uintptr_t p) noexcept
  {
    if ( !p ) return;
    if ( __is_leaf(p) ) {
      __leaf *l = __as_leaf(p);
      if ( !__dec_refs(&l->refs) ) return;
      __destroy_chain(l->overflow);
      delete l;
      return;
    }
    __internal *in = __as_internal(p);
    if ( !__dec_refs(&in->refs) ) return;
    usize n = static_cast<usize>(micron::popcount(in->bitmap));
    uintptr_t *cs = __children_of(in);
    for ( usize i = 0; i < n; ++i ) __release(cs[i]);
    ::operator delete(in);
  }

  struct __node_guard {
    uintptr_t p;

    explicit __node_guard(uintptr_t q) noexcept : p(q) { }

    ~__node_guard()
    {
      if ( p ) __release(p);
    }

    void
    dismiss() noexcept
    {
      p = 0;
    }

    __node_guard(const __node_guard &) = delete;
    __node_guard &operator=(const __node_guard &) = delete;
  };

  static inline __attribute__((always_inline)) hash64_t
  __mix_hash(hash64_t h) noexcept
  {
    h ^= h >> 30;
    h *= 0xbf58476d1ce4e5b9ULL;
    h ^= h >> 27;
    h *= 0x94d049bb133111ebULL;
    h ^= h >> 31;
    return h;
  }

  static inline __attribute__((always_inline)) u32
  __idx_at(hash64_t h, usize depth) noexcept
  {
    if ( depth < __natural_depth ) [[likely]]
      return static_cast<u32>(h >> (depth * __log2B)) & __mask32;
    return static_cast<u32>(__mix_hash(h) >> ((depth - __natural_depth) * __log2B)) & __mask32;
  }

  static __internal *
  __internal_replace(const __internal *src, u32 logical_idx, uintptr_t new_child)
  {
    u32 bm = src->bitmap;
    usize n = static_cast<usize>(micron::popcount(bm));
    __internal *out = __alloc_internal(bm);
    const uintptr_t *src_cs = __children_of(src);
    uintptr_t *dst_cs = __children_of(out);
    usize phys = __phys_idx(bm, logical_idx);
    for ( usize i = 0; i < n; ++i ) {
      if ( i == phys ) {
        dst_cs[i] = new_child;
      } else {
        dst_cs[i] = src_cs[i];
        __retain(dst_cs[i]);
      }
    }
    return out;
  }

  static __internal *
  __internal_insert(const __internal *src, u32 logical_idx, uintptr_t new_child)
  {
    u32 nbm = src->bitmap | (1u << logical_idx);
    __internal *out = __alloc_internal(nbm);
    usize n_old = static_cast<usize>(micron::popcount(src->bitmap));
    const uintptr_t *src_cs = __children_of(src);
    uintptr_t *dst_cs = __children_of(out);
    usize phys = __phys_idx(nbm, logical_idx);
    for ( usize i = 0; i < phys; ++i ) {
      dst_cs[i] = src_cs[i];
      __retain(dst_cs[i]);
    }
    dst_cs[phys] = new_child;
    for ( usize i = phys; i < n_old; ++i ) {
      dst_cs[i + 1] = src_cs[i];
      __retain(dst_cs[i + 1]);
    }
    return out;
  }

  static __internal *
  __internal_remove(const __internal *src, u32 logical_idx)
  {
    u32 nbm = src->bitmap & ~(1u << logical_idx);
    if ( nbm == 0 ) return nullptr;
    __internal *out = __alloc_internal(nbm);
    usize n_old = static_cast<usize>(micron::popcount(src->bitmap));
    const uintptr_t *src_cs = __children_of(src);
    uintptr_t *dst_cs = __children_of(out);
    usize phys = __phys_idx(src->bitmap, logical_idx);
    for ( usize i = 0; i < phys; ++i ) {
      dst_cs[i] = src_cs[i];
      __retain(dst_cs[i]);
    }
    for ( usize i = phys + 1; i < n_old; ++i ) {
      dst_cs[i - 1] = src_cs[i];
      __retain(dst_cs[i - 1]);
    }
    return out;
  }

  static bool
  __chain_has(const __chain *c, hash64_t h, const K &k) noexcept
  {
    for ( ; c; c = c->next ) {
      if ( c->h == h && c->key == k ) return true;
    }
    return false;
  }

  static const V *
  __chain_find(const __chain *c, hash64_t h, const K &k) noexcept
  {
    for ( ; c; c = c->next ) {
      if ( c->h == h && c->key == k ) return micron::addressof(c->value);
    }
    return nullptr;
  }

  static __chain *
  __chain_copy_with_replace(const __chain *src, hash64_t h, const K &k, V v)
  {
    __chain *head = nullptr;
    __chain **tail = &head;
    bool replaced = false;
#ifndef __micron_freestanding
    try {
#endif
      for ( const __chain *c = src; c; c = c->next ) {
        if ( !replaced && c->h == h && c->key == k ) {
          *tail = new __chain(h, K(k), micron::move(v));
          replaced = true;
        } else {
          *tail = new __chain(c->h, c->key, c->value);
        }
        tail = &(*tail)->next;
      }
#ifndef __micron_freestanding
    } catch ( ... ) {
      __destroy_chain(head);
      throw;
    }
#endif
    return head;
  }

  static __chain *
  __chain_copy_with_erase(const __chain *src, hash64_t h, const K &k)
  {
    __chain *head = nullptr;
    __chain **tail = &head;
    bool dropped = false;
#ifndef __micron_freestanding
    try {
#endif
      for ( const __chain *c = src; c; c = c->next ) {
        if ( !dropped && c->h == h && c->key == k ) {
          dropped = true;
          continue;
        }
        *tail = new __chain(c->h, c->key, c->value);
        tail = &(*tail)->next;
      }
#ifndef __micron_freestanding
    } catch ( ... ) {
      __destroy_chain(head);
      throw;
    }
#endif
    return head;
  }

  static bool
  __leaf_would_diverge(const __leaf *l, hash64_t new_h, usize depth) noexcept
  {
    u32 ref_idx = __idx_at(l->h, depth);
    if ( __idx_at(new_h, depth) != ref_idx ) return true;
    for ( const __chain *c = l->overflow; c; c = c->next ) {
      if ( __idx_at(c->h, depth) != ref_idx ) return true;
    }
    return false;
  }

  static void
  __child_absorb(uintptr_t *slot, hash64_t h, const K &k, const V &v)
  {
    if ( *slot == 0 ) {
      *slot = __tag_leaf_ptr(new __leaf(h, k, v));
    } else {
      __leaf *l = __as_leaf(*slot);
      l->overflow = new __chain(h, k, v, l->overflow);
      ++l->overflow_len;
    }
  }

  static __internal *
  __split_leaf_with_extra(const __leaf *l, hash64_t new_h, const K &new_k, V new_v, usize depth)
  {
    u32 bm = 0;
    bm |= 1u << __idx_at(l->h, depth);
    bm |= 1u << __idx_at(new_h, depth);
    for ( const __chain *c = l->overflow; c; c = c->next ) bm |= 1u << __idx_at(c->h, depth);

    __internal *in = __alloc_internal(bm);
    uintptr_t *cs = __children_of(in);

    auto place = [&](hash64_t h, const K &k, const V &v) {
      u32 idx = __idx_at(h, depth);
      __child_absorb(&cs[__phys_idx(bm, idx)], h, k, v);
    };

#ifndef __micron_freestanding
    try {
#endif
      place(l->h, l->key, l->value);
      for ( const __chain *c = l->overflow; c; c = c->next ) place(c->h, c->key, c->value);
      __child_absorb(&cs[__phys_idx(bm, __idx_at(new_h, depth))], new_h, new_k, micron::move(new_v));
#ifndef __micron_freestanding
    } catch ( ... ) {
      __release(__tag_internal_ptr(in));
      throw;
    }
#endif

    return in;
  }

  static uintptr_t
  __insert_into(uintptr_t n, hash64_t h, const K &k, V v, usize depth, bool &existed)
  {
    if ( n == 0 ) {
      existed = false;
      return __tag_leaf_ptr(new __leaf(h, K(k), micron::move(v)));
    }
    if ( __is_leaf(n) ) {
      const __leaf *l = __as_leaf(n);

      if ( l->h == h && l->key == k ) {
        existed = true;
        __leaf *nl = new __leaf(h, K(k), micron::move(v));
        __node_guard g{ __tag_leaf_ptr(nl) };
        nl->overflow_len = l->overflow_len;
        nl->overflow = __copy_chain(l->overflow);      // if it throws, g frees nl (overflow still null)
        g.dismiss();
        return __tag_leaf_ptr(nl);
      }

      if ( __chain_has(l->overflow, h, k) ) {
        existed = true;
        __leaf *nl = new __leaf(l->h, l->key, l->value);
        __node_guard g{ __tag_leaf_ptr(nl) };
        nl->overflow_len = l->overflow_len;
        nl->overflow = __chain_copy_with_replace(l->overflow, h, K(k), micron::move(v));
        g.dismiss();
        return __tag_leaf_ptr(nl);
      }

      existed = false;
      u32 total_after = 1u + l->overflow_len + 1u;
      if ( total_after > __split_threshold && depth < __max_depth && __leaf_would_diverge(l, h, depth) ) {
        return __tag_internal_ptr(__split_leaf_with_extra(l, h, k, micron::move(v), depth));
      }

      __leaf *nl = new __leaf(l->h, l->key, l->value);
      __node_guard g{ __tag_leaf_ptr(nl) };
      nl->overflow_len = l->overflow_len + 1u;
      __chain *rest = __copy_chain(l->overflow);
      nl->overflow = rest;      // nl now owns rest; if the prepend below throws, g frees nl + rest
      nl->overflow = new __chain(h, K(k), micron::move(v), rest);
      g.dismiss();
      return __tag_leaf_ptr(nl);
    }

    const __internal *in = __as_internal(n);
    u32 idx = __idx_at(h, depth);
    if ( !__has(in->bitmap, idx) ) {
      existed = false;
      __leaf *nl = new __leaf(h, K(k), micron::move(v));
      __node_guard g{ __tag_leaf_ptr(nl) };      // freed if __internal_insert's alloc throws (OOM)
      uintptr_t res = __tag_internal_ptr(__internal_insert(in, idx, __tag_leaf_ptr(nl)));
      g.dismiss();
      return res;
    }
    usize phys = __phys_idx(in->bitmap, idx);
    uintptr_t old_child = __children_of(in)[phys];
    uintptr_t new_child = __insert_into(old_child, h, k, micron::move(v), depth + 1u, existed);
    __node_guard g{ new_child };      // freed if __internal_replace's alloc throws (OOM)
    uintptr_t res = __tag_internal_ptr(__internal_replace(in, idx, new_child));
    g.dismiss();
    return res;
  }

  static const V *
  __find_in(uintptr_t n, hash64_t h, const K &k, usize depth) noexcept
  {
    while ( n != 0 ) {
      if ( __is_leaf(n) ) {
        const __leaf *l = __as_leaf(n);
        if ( l->h == h && l->key == k ) return micron::addressof(l->value);
        return __chain_find(l->overflow, h, k);
      }
      const __internal *in = __as_internal(n);
      u32 idx = __idx_at(h, depth);
      if ( !__has(in->bitmap, idx) ) return nullptr;
      n = __children_of(in)[__phys_idx(in->bitmap, idx)];
      ++depth;
    }
    return nullptr;
  }

  static uintptr_t
  __erase_from(uintptr_t n, hash64_t h, const K &k, usize depth, bool &erased)
  {
    if ( n == 0 ) {
      erased = false;
      return 0;
    }
    if ( __is_leaf(n) ) {
      const __leaf *l = __as_leaf(n);

      if ( l->h == h && l->key == k ) {
        erased = true;
        if ( l->overflow == nullptr ) return 0;

        __chain *first = l->overflow;
        __leaf *nl = new __leaf(first->h, first->key, first->value);
        __node_guard g{ __tag_leaf_ptr(nl) };
        nl->overflow_len = l->overflow_len - 1u;
        nl->overflow = __copy_chain(first->next);
        g.dismiss();
        return __tag_leaf_ptr(nl);
      }

      if ( __chain_has(l->overflow, h, k) ) {
        erased = true;
        __leaf *nl = new __leaf(l->h, l->key, l->value);
        __node_guard g{ __tag_leaf_ptr(nl) };
        nl->overflow_len = l->overflow_len - 1u;
        nl->overflow = __chain_copy_with_erase(l->overflow, h, k);
        g.dismiss();
        return __tag_leaf_ptr(nl);
      }

      erased = false;
      __retain(n);
      return n;
    }

    const __internal *in = __as_internal(n);
    u32 idx = __idx_at(h, depth);
    if ( !__has(in->bitmap, idx) ) {
      erased = false;
      __retain(n);
      return n;
    }
    usize phys = __phys_idx(in->bitmap, idx);
    uintptr_t old_child = __children_of(in)[phys];
    uintptr_t new_child = __erase_from(old_child, h, k, depth + 1u, erased);
    if ( !erased ) {

      __release(new_child);
      __retain(n);
      return n;
    }
    if ( new_child == 0 ) {
      __internal *nn = __internal_remove(in, idx);
      if ( nn == nullptr ) return 0;
      return __tag_internal_ptr(nn);
    }
    __node_guard g{ new_child };      // freed if __internal_replace's alloc throws (OOM)
    uintptr_t res = __tag_internal_ptr(__internal_replace(in, idx, new_child));
    g.dismiss();
    return res;
  }

  template<typename Fn>
  static void
  __walk(uintptr_t n, Fn &&fn)
  {
    if ( n == 0 ) return;
    if ( __is_leaf(n) ) {
      const __leaf *l = __as_leaf(n);
      fn(l->key, l->value);
      for ( const __chain *c = l->overflow; c; c = c->next ) fn(c->key, c->value);
      return;
    }
    const __internal *in = __as_internal(n);
    usize cnt = static_cast<usize>(micron::popcount(in->bitmap));
    const uintptr_t *cs = __children_of(in);
    for ( usize i = 0; i < cnt; ++i ) __walk(cs[i], micron::forward<Fn>(fn));
  }

  uintptr_t __root = 0;
  usize __length = 0;

  pmap(uintptr_t r, usize l) : __root(r), __length(l) { }

public:
  using category_type = map_tag;
  using mutability_type = immutable_tag;
  using memory_type = heap_tag;
  using size_type = usize;
  using key_type = K;
  using mapped_type = V;

  pmap() = default;

  ~pmap() { __release(__root); }

  pmap(const pmap &o) : __root(o.__root), __length(o.__length) { __retain(__root); }

  pmap(pmap &&o) noexcept : __root(o.__root), __length(o.__length)
  {
    o.__root = 0;
    o.__length = 0;
  }

  pmap &
  operator=(const pmap &o)
  {
    if ( this == &o ) return *this;
    __retain(o.__root);
    __release(__root);
    __root = o.__root;
    __length = o.__length;
    return *this;
  }

  pmap &
  operator=(pmap &&o) noexcept
  {
    if ( this == &o ) return *this;
    __release(__root);
    __root = o.__root;
    __length = o.__length;
    o.__root = 0;
    o.__length = 0;
    return *this;
  }

  usize
  size() const noexcept
  {
    return __length;
  }

  bool
  empty() const noexcept
  {
    return __length == 0;
  }

  pmap
  insert(const K &k, const V &v) const
  {
    bool existed = false;
    hash64_t h = hash<hash64_t>(k);
    uintptr_t nr = __insert_into(__root, h, k, V(v), 0, existed);
    return pmap(nr, existed ? __length : __length + 1);
  }

  pmap
  insert(const K &k, V &&v) const
  {
    bool existed = false;
    hash64_t h = hash<hash64_t>(k);
    uintptr_t nr = __insert_into(__root, h, k, micron::move(v), 0, existed);
    return pmap(nr, existed ? __length : __length + 1);
  }

  pmap
  set(const K &k, const V &v) const
  {
    return insert(k, v);
  }

  template<typename... Args>
  pmap
  emplace(const K &k, Args &&...args) const
  {
    return insert(k, V(micron::forward<Args>(args)...));
  }

  pmap
  erase(const K &k) const
  {
    bool erased = false;
    hash64_t h = hash<hash64_t>(k);
    uintptr_t nr = __erase_from(__root, h, k, 0, erased);
    return pmap(nr, erased ? __length - 1 : __length);
  }

  pmap
  clear() const
  {
    return pmap();
  }

  const V *
  find(const K &k) const noexcept
  {
    return __find_in(__root, hash<hash64_t>(k), k, 0);
  }

  bool
  contains(const K &k) const noexcept
  {
    return find(k) != nullptr;
  }

  const V &
  at(const K &k) const
  {
    const V *v = find(k);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("pmap::at(): key not found");
    return *v;
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    __walk(__root, micron::forward<Fn>(fn));
  }

  const void *
  identity() const noexcept
  {
    return reinterpret_cast<const void *>(__root);
  }
};

};      // namespace micron
