//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../bits.hpp"
#include "../hash/hash.hpp"
#include "../types.hpp"

#include "../allocator.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/addr.hpp"
#include "../memory/new.hpp"
#include "../tuple.hpp"

#include "../trees/rb.hpp"

// NOTE: stable, yet experimental

namespace micron
{

// rb_map
//
// hash map with two-tier collision handling:
//  a) light bins use a singly-linked chain through a dense SoA storage
//  b) heavy bins are *treeified* into a per-bin red-black tree once the chain
//     length exceeds __treeify_threshold
//
// the SoA layout means _hashes[] is contiguous and has no key/value padding so it stays cache-resident for repeated lookups

template<typename K, typename V, class Alloc = micron::allocator_serial<>>
  requires micron::is_copy_constructible_v<V> and micron::is_move_constructible_v<V>
class rb_map
{
  // configurable
  static constexpr usize __treeify_threshold = 8;
  static constexpr usize __untreeify_threshold = 6;
  static constexpr usize __min_treeify_cap = 64;
  static constexpr usize __min_bins = 16;
  static constexpr usize __load_num = 3;
  static constexpr usize __load_denom = 4;

  struct __tree_entry {
    hash64_t hash;
    K key;
    V value;

    __tree_entry() : hash(0), key{}, value{} { }

    __tree_entry(hash64_t h, const K &k, const V &v) : hash(h), key(k), value(v) { }

    __tree_entry(hash64_t h, K &&k, V &&v) : hash(h), key(micron::move(k)), value(micron::move(v)) { }

    __tree_entry(const __tree_entry &) = default;
    __tree_entry(__tree_entry &&) = default;
    __tree_entry &operator=(const __tree_entry &) = default;
    __tree_entry &operator=(__tree_entry &&) = default;

    bool
    operator<(const __tree_entry &o) const
    {
      if ( hash != o.hash ) return hash < o.hash;
      return key < o.key;
    }
  };

  using __tree_t = rb_tree<__tree_entry>;

  struct __bin_t {
    i32 list_head;       // -1 = empty when tree == nullptr; ignored otherwise
    __tree_t *tree;      // nullptr when bin is in list mode
    u32 chain_len;       // length of list_head chain
  };

  // SoA dense storage; valid indices [0, __n_soa)
  hash64_t *__hashes = nullptr;
  K *__keys = nullptr;
  V *__values = nullptr;
  i32 *__next = nullptr;
  i32 *__home_bin = nullptr;
  usize __n_soa = 0;
  usize __cap_soa = 0;

  __bin_t *__bins = nullptr;
  usize __n_bins = 0;
  usize __bin_mask = 0;

  usize __total = 0;

  static usize
  __round_pow2(usize n) noexcept
  {
    if ( n <= __min_bins ) return __min_bins;
    usize p = 1;
    while ( p < n ) p <<= 1;
    return p;
  }

  __attribute__((always_inline)) usize
  __bin_of(hash64_t h) const noexcept
  {
    return static_cast<usize>(h) & __bin_mask;
  }

  void
  __grow_soa()
  {
    usize new_cap = __cap_soa == 0 ? 16 : __cap_soa * 2;
    auto *nh = static_cast<hash64_t *>(::operator new(sizeof(hash64_t) * new_cap));
    auto *nn = static_cast<i32 *>(::operator new(sizeof(i32) * new_cap));
    auto *nb = static_cast<i32 *>(::operator new(sizeof(i32) * new_cap));
    auto *nk = static_cast<K *>(::operator new(sizeof(K) * new_cap));
    auto *nv = static_cast<V *>(::operator new(sizeof(V) * new_cap));
    usize moved = 0;
    if ( __n_soa > 0 ) {
      try {
        for ( ; moved < __n_soa; ++moved ) {
          new (nk + moved) K(micron::move(__keys[moved]));
          try {
            new (nv + moved) V(micron::move(__values[moved]));
          } catch ( ... ) {
            nk[moved].~K();
            throw;
          }
        }
      } catch ( ... ) {
        for ( usize i = 0; i < moved; ++i ) {
          nv[i].~V();
          nk[i].~K();
        }
        ::operator delete(nv);
        ::operator delete(nk);
        ::operator delete(nb);
        ::operator delete(nn);
        ::operator delete(nh);
        throw;
      }
      micron::memcpy(reinterpret_cast<byte *>(nh), reinterpret_cast<byte *>(__hashes), __n_soa * sizeof(hash64_t));
      micron::memcpy(reinterpret_cast<byte *>(nn), reinterpret_cast<byte *>(__next), __n_soa * sizeof(i32));
      micron::memcpy(reinterpret_cast<byte *>(nb), reinterpret_cast<byte *>(__home_bin), __n_soa * sizeof(i32));
      for ( usize i = 0; i < __n_soa; ++i ) {
        __keys[i].~K();
        __values[i].~V();
      }
    }
    if ( __hashes ) ::operator delete(__hashes);
    if ( __keys ) ::operator delete(__keys);
    if ( __values ) ::operator delete(__values);
    if ( __next ) ::operator delete(__next);
    if ( __home_bin ) ::operator delete(__home_bin);
    __hashes = nh;
    __keys = nk;
    __values = nv;
    __next = nn;
    __home_bin = nb;
    __cap_soa = new_cap;
  }

  void
  __soa_remove(i32 idx) noexcept
  {
    i32 last = static_cast<i32>(__n_soa) - 1;
    if ( idx != last ) {

      i32 last_home = __home_bin[last];
      __bin_t &lb = __bins[last_home];
      if ( !lb.tree ) {

        if ( lb.list_head == last ) {
          lb.list_head = idx;
        } else {
          i32 prev = lb.list_head;
          while ( prev != -1 && __next[prev] != last ) prev = __next[prev];
          if ( prev != -1 ) __next[prev] = idx;
        }
      }
      __hashes[idx] = __hashes[last];
      __next[idx] = __next[last];
      __home_bin[idx] = __home_bin[last];
      __keys[idx] = micron::move(__keys[last]);
      __values[idx] = micron::move(__values[last]);
    }
    __keys[__n_soa - 1].~K();
    __values[__n_soa - 1].~V();
    --__n_soa;
  }

  void
  __treeify(usize bin_idx)
  {
    __bin_t &b = __bins[bin_idx];
    if ( b.tree ) return;
    auto *t = new __tree_t();

    i32 cur = b.list_head;
    while ( cur != -1 ) {
      i32 nxt = __next[cur];
      t->insert(__tree_entry(__hashes[cur], micron::move(__keys[cur]), micron::move(__values[cur])));
      cur = nxt;
    }

    b.list_head = -1;
    b.tree = t;
    b.chain_len = 0;

    i32 i = 0;
    while ( i < static_cast<i32>(__n_soa) ) {
      if ( __home_bin[i] == static_cast<i32>(bin_idx) ) {
        __soa_remove(i);
      } else {
        ++i;
      }
    }
  }

  void
  __untreeify(usize bin_idx)
  {
    __bin_t &b = __bins[bin_idx];
    if ( !b.tree ) return;
    __tree_t *t = b.tree;

    while ( __n_soa + t->size() > __cap_soa ) __grow_soa();

    b.tree = nullptr;
    b.list_head = -1;
    b.chain_len = 0;
    while ( !t->empty() ) {
      __tree_entry e = t->extract_min();
      i32 ix = static_cast<i32>(__n_soa);
      new (__keys + ix) K(micron::move(e.key));
      new (__values + ix) V(micron::move(e.value));
      __hashes[ix] = e.hash;
      __next[ix] = b.list_head;
      __home_bin[ix] = static_cast<i32>(bin_idx);
      b.list_head = ix;
      ++b.chain_len;
      ++__n_soa;
    }
    delete t;
  }

  bool
  __resize_if_needed()
  {
    if ( __total < __n_bins * __load_num / __load_denom ) return false;
    __rehash(__n_bins * 2);
    return true;
  }

  void
  __rehash(usize new_n_bins)
  {
    if ( new_n_bins < __min_bins ) new_n_bins = __min_bins;
    new_n_bins = __round_pow2(new_n_bins);

    usize total = __total;
    auto *snap = total ? static_cast<__tree_entry *>(::operator new(sizeof(__tree_entry) * total)) : nullptr;
    usize w = 0;

    try {
      for ( usize i = 0; i < __n_soa; ++i ) {
        new (snap + w) __tree_entry(__hashes[i], micron::move(__keys[i]), micron::move(__values[i]));
        ++w;
      }
    } catch ( ... ) {
      for ( usize j = 0; j < w; ++j ) snap[j].~__tree_entry();
      if ( snap ) ::operator delete(snap);
      throw;
    }
    for ( usize i = 0; i < __n_soa; ++i ) {
      __keys[i].~K();
      __values[i].~V();
    }
    __n_soa = 0;

    if ( __bins ) {
      try {
        for ( usize i = 0; i < __n_bins; ++i ) {
          __tree_t *t = __bins[i].tree;
          if ( !t ) continue;
          while ( !t->empty() ) {
            new (snap + w) __tree_entry(t->extract_min());
            ++w;
          }
        }
      } catch ( ... ) {
        for ( usize j = 0; j < w; ++j ) snap[j].~__tree_entry();
        if ( snap ) ::operator delete(snap);
        for ( usize i = 0; i < __n_bins; ++i )
          if ( __bins[i].tree ) delete __bins[i].tree;
        ::operator delete(__bins);
        __bins = nullptr;
        __n_bins = 0;
        __bin_mask = 0;
        __total = 0;
        throw;
      }
      for ( usize i = 0; i < __n_bins; ++i )
        if ( __bins[i].tree ) delete __bins[i].tree;
      ::operator delete(__bins);
      __bins = nullptr;
    }

    __bins = static_cast<__bin_t *>(::operator new(sizeof(__bin_t) * new_n_bins));
    for ( usize i = 0; i < new_n_bins; ++i ) {
      __bins[i].list_head = -1;
      __bins[i].tree = nullptr;
      __bins[i].chain_len = 0;
    }
    __n_bins = new_n_bins;
    __bin_mask = new_n_bins - 1u;
    __total = 0;

    usize consumed = 0;
    try {
      for ( ; consumed < w; ++consumed ) {
        __tree_entry &e = snap[consumed];
        usize bi = __bin_of(e.hash);
        __bin_t &b = __bins[bi];
        if ( b.tree ) {
          b.tree->insert(micron::move(e));
          ++__total;
        } else {
          if ( __n_soa == __cap_soa ) __grow_soa();
          i32 ix = static_cast<i32>(__n_soa);
          new (__keys + ix) K(micron::move(e.key));
          new (__values + ix) V(micron::move(e.value));
          __hashes[ix] = e.hash;
          __next[ix] = b.list_head;
          __home_bin[ix] = static_cast<i32>(bi);
          b.list_head = ix;
          ++b.chain_len;
          ++__n_soa;
          ++__total;
          if ( b.chain_len >= __treeify_threshold && __n_bins >= __min_treeify_cap ) {
            __treeify(bi);
          }
        }
        e.~__tree_entry();
      }
    } catch ( ... ) {
      for ( usize j = consumed; j < w; ++j ) snap[j].~__tree_entry();
      if ( snap ) ::operator delete(snap);
      throw;
    }
    if ( snap ) ::operator delete(snap);
  }

  void
  __free_all()
  {
    if ( __bins ) {
      for ( usize i = 0; i < __n_bins; ++i )
        if ( __bins[i].tree ) delete __bins[i].tree;
      ::operator delete(__bins);
      __bins = nullptr;
    }
    if ( __n_soa > 0 ) {
      for ( usize i = 0; i < __n_soa; ++i ) {
        __keys[i].~K();
        __values[i].~V();
      }
    }
    if ( __hashes ) ::operator delete(__hashes);
    if ( __keys ) ::operator delete(__keys);
    if ( __values ) ::operator delete(__values);
    if ( __next ) ::operator delete(__next);
    if ( __home_bin ) ::operator delete(__home_bin);
    __hashes = nullptr;
    __keys = nullptr;
    __values = nullptr;
    __next = nullptr;
    __home_bin = nullptr;
    __n_soa = 0;
    __cap_soa = 0;
    __n_bins = 0;
    __bin_mask = 0;
    __total = 0;
  }

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  using size_type = usize;
  using key_type = K;
  using mapped_type = V;

  ~rb_map() { __free_all(); }

  rb_map()
  {
    __n_bins = __min_bins;
    __bin_mask = __n_bins - 1u;
    __bins = static_cast<__bin_t *>(::operator new(sizeof(__bin_t) * __n_bins));
    for ( usize i = 0; i < __n_bins; ++i ) {
      __bins[i].list_head = -1;
      __bins[i].tree = nullptr;
      __bins[i].chain_len = 0;
    }
  }

  explicit rb_map(usize cap) : rb_map()
  {
    if ( cap > __min_bins ) __rehash(cap);
  }

  rb_map(const rb_map &) = delete;
  rb_map &operator=(const rb_map &) = delete;

  rb_map(rb_map &&o) noexcept
      : __hashes(o.__hashes), __keys(o.__keys), __values(o.__values), __next(o.__next), __home_bin(o.__home_bin), __n_soa(o.__n_soa),
        __cap_soa(o.__cap_soa), __bins(o.__bins), __n_bins(o.__n_bins), __bin_mask(o.__bin_mask), __total(o.__total)
  {
    o.__hashes = nullptr;
    o.__keys = nullptr;
    o.__values = nullptr;
    o.__next = nullptr;
    o.__home_bin = nullptr;
    o.__bins = nullptr;
    o.__n_soa = o.__cap_soa = o.__n_bins = o.__bin_mask = o.__total = 0;
  }

  rb_map &
  operator=(rb_map &&o) noexcept
  {
    if ( this == &o ) return *this;
    __free_all();
    __hashes = o.__hashes;
    __keys = o.__keys;
    __values = o.__values;
    __next = o.__next;
    __home_bin = o.__home_bin;
    __n_soa = o.__n_soa;
    __cap_soa = o.__cap_soa;
    __bins = o.__bins;
    __n_bins = o.__n_bins;
    __bin_mask = o.__bin_mask;
    __total = o.__total;
    o.__hashes = nullptr;
    o.__keys = nullptr;
    o.__values = nullptr;
    o.__next = nullptr;
    o.__home_bin = nullptr;
    o.__bins = nullptr;
    o.__n_soa = o.__cap_soa = o.__n_bins = o.__bin_mask = o.__total = 0;
    return *this;
  }

  usize
  size() const noexcept
  {
    return __total;
  }

  usize
  bin_count() const noexcept
  {
    return __n_bins;
  }

  bool
  empty() const noexcept
  {
    return __total == 0;
  }

  float
  load_factor() const noexcept
  {
    return __n_bins > 0u ? static_cast<float>(__total) / static_cast<float>(__n_bins) : 0.0f;
  }

  void
  clear()
  {
    __free_all();
    __n_bins = __min_bins;
    __bin_mask = __n_bins - 1u;
    __bins = static_cast<__bin_t *>(::operator new(sizeof(__bin_t) * __n_bins));
    for ( usize i = 0; i < __n_bins; ++i ) {
      __bins[i].list_head = -1;
      __bins[i].tree = nullptr;
      __bins[i].chain_len = 0;
    }
  }

  bool
  treeified(usize bin) const noexcept
  {
    return bin < __n_bins && __bins[bin].tree != nullptr;
  }

  V *
  find(const K &k) noexcept
  {
    hash64_t h = hash<hash64_t>(k);
    __bin_t &b = __bins[__bin_of(h)];
    if ( b.tree ) {
      // heterogeneous lookup by (hash,key)
      __tree_entry *e = b.tree->find_by([h, &k](const __tree_entry &d) { return h != d.hash ? h < d.hash : k < d.key; },
                                        [h, &k](const __tree_entry &d) { return d.hash != h ? d.hash < h : d.key < k; });
      return e ? micron::addressof(e->value) : nullptr;
    }
    for ( i32 i = b.list_head; i != -1; i = __next[i] ) {
      if ( __hashes[i] == h && __keys[i] == k ) return micron::addressof(__values[i]);
    }
    return nullptr;
  }

  const V *
  find(const K &k) const noexcept
  {
    return const_cast<rb_map *>(this)->find(k);
  }

  bool
  contains(const K &k) const noexcept
  {
    return find(k) != nullptr;
  }

  usize
  count(const K &k) const noexcept
  {
    return find(k) ? 1u : 0u;
  }

  V &
  at(const K &k)
  {
    V *v = find(k);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("rb_map::at(): key not found");
    return *v;
  }

  const V &
  at(const K &k) const
  {
    const V *v = find(k);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("rb_map::at(): key not found");
    return *v;
  }

  template<class KK, class VV>
    requires(micron::is_same_v<micron::remove_cvref_t<KK>, K> && micron::is_same_v<micron::remove_cvref_t<VV>, V>)
  micron::pair<bool, V *>
  insert_or_assign(KK &&k, VV &&v)
  {
    hash64_t h = hash<hash64_t>(k);

    {
      __bin_t &b = __bins[__bin_of(h)];
      if ( b.tree ) {
        __tree_entry *e = b.tree->find_by([h, &k](const __tree_entry &d) { return h != d.hash ? h < d.hash : k < d.key; },
                                          [h, &k](const __tree_entry &d) { return d.hash != h ? d.hash < h : d.key < k; });
        if ( e ) {
          e->value = micron::forward<VV>(v);
          return { false, micron::addressof(e->value) };
        }
      } else {
        for ( i32 i = b.list_head; i != -1; i = __next[i] ) {
          if ( __hashes[i] == h && __keys[i] == k ) {
            __values[i] = micron::forward<VV>(v);
            return { false, micron::addressof(__values[i]) };
          }
        }
      }
    }

    if ( __total + 1u > __n_bins * __load_num / __load_denom ) __rehash(__n_bins * 2);
    usize bi = __bin_of(h);
    __bin_t &b = __bins[bi];

    if ( !b.tree && b.chain_len + 1u >= __treeify_threshold && __n_bins >= __min_treeify_cap ) {
      __treeify(bi);
    }
    if ( b.tree ) {
      __tree_entry &inserted = b.tree->insert(__tree_entry(h, K(micron::forward<KK>(k)), V(micron::forward<VV>(v))));
      ++__total;
      return { true, micron::addressof(inserted.value) };
    }

    if ( __n_soa == __cap_soa ) __grow_soa();
    i32 ix = static_cast<i32>(__n_soa);
    new (__keys + ix) K(micron::forward<KK>(k));
    try {
      new (__values + ix) V(micron::forward<VV>(v));
    } catch ( ... ) {
      __keys[ix].~K();
      throw;
    }
    __hashes[ix] = h;
    __next[ix] = b.list_head;
    __home_bin[ix] = static_cast<i32>(bi);
    b.list_head = ix;
    ++b.chain_len;
    ++__n_soa;
    ++__total;
    return { true, micron::addressof(__values[ix]) };
  }

  template<class KK, class VV>
    requires(micron::is_same_v<micron::remove_cvref_t<KK>, K> && micron::is_same_v<micron::remove_cvref_t<VV>, V>)
  micron::pair<bool, V *>
  insert(KK &&k, VV &&v)
  {

    V *ex = find(k);
    if ( ex ) return { false, ex };
    return insert_or_assign(micron::forward<KK>(k), micron::forward<VV>(v));
  }

  template<class KK, typename... Args>
    requires micron::is_same_v<micron::remove_cvref_t<KK>, K>
  micron::pair<bool, V *>
  emplace(KK &&k, Args &&...args)
  {
    V *ex = find(k);
    if ( ex ) return { false, ex };
    return insert_or_assign(micron::forward<KK>(k), V(micron::forward<Args>(args)...));
  }

  V &
  operator[](const K &k)
  {
    V *v = find(k);
    if ( v ) return *v;
    auto r = insert_or_assign(k, V{});
    return *r.b;
  }

  bool
  erase(const K &k)
  {
    hash64_t h = hash<hash64_t>(k);
    usize bi = __bin_of(h);
    __bin_t &b = __bins[bi];
    if ( b.tree ) {
      bool ok = b.tree->erase_by([h, &k](const __tree_entry &d) { return h != d.hash ? h < d.hash : k < d.key; },
                                 [h, &k](const __tree_entry &d) { return d.hash != h ? d.hash < h : d.key < k; });
      if ( ok ) {
        --__total;
        if ( b.tree->size() < __untreeify_threshold ) __untreeify(bi);
      }
      return ok;
    }
    i32 prev = -1;
    for ( i32 i = b.list_head; i != -1; i = __next[i] ) {
      if ( __hashes[i] == h && __keys[i] == k ) {
        if ( prev == -1 )
          b.list_head = __next[i];
        else
          __next[prev] = __next[i];
        --b.chain_len;
        --__total;
        __soa_remove(i);
        return true;
      }
      prev = i;
    }
    return false;
  }

  usize
  max_size() const noexcept
  {
    return __n_bins;
  }

  void
  swap(rb_map &o) noexcept
  {
    micron::swap(__hashes, o.__hashes);
    micron::swap(__keys, o.__keys);
    micron::swap(__values, o.__values);
    micron::swap(__next, o.__next);
    micron::swap(__home_bin, o.__home_bin);
    micron::swap(__n_soa, o.__n_soa);
    micron::swap(__cap_soa, o.__cap_soa);
    micron::swap(__bins, o.__bins);
    micron::swap(__n_bins, o.__n_bins);
    micron::swap(__bin_mask, o.__bin_mask);
    micron::swap(__total, o.__total);
  }

  template<typename Fn>
  void
  for_each(Fn &&fn)
  {
    for ( usize i = 0; i < __n_soa; ++i ) fn(__keys[i], __values[i]);
    for ( usize i = 0; i < __n_bins; ++i ) {
      __tree_t *t = __bins[i].tree;
      if ( !t ) continue;
      t->for_each([&fn](__tree_entry &e) { fn(e.key, e.value); });
    }
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    for ( usize i = 0; i < __n_soa; ++i ) fn(__keys[i], __values[i]);
    for ( usize i = 0; i < __n_bins; ++i ) {
      const __tree_t *t = __bins[i].tree;
      if ( !t ) continue;
      t->for_each([&fn](const __tree_entry &e) { fn(e.key, e.value); });
    }
  }
};

template<typename K, typename V, class Alloc = micron::allocator_serial<>> using rmap = rb_map<K, V, Alloc>;

};      // namespace micron
