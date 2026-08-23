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
#include "../numerics.hpp"
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

  template<typename T>
  static T *
  __alloc_array(usize count)
  {
    if ( count > static_cast<usize>(-1) / sizeof(T) ) exc<except::library_error>("rb_map: allocation overflow");
    if constexpr ( alignof(T) <= 32 )
      return static_cast<T *>(::operator new(sizeof(T) * count));
    else
      return static_cast<T *>(::operator new(sizeof(T) * count, static_cast<std::align_val_t>(alignof(T))));
  }

  template<typename T>
  static void
  __free_array(T *ptr) noexcept
  {
    if ( !ptr ) return;
    if constexpr ( alignof(T) <= 32 )
      ::operator delete(ptr);
    else
      ::operator delete(ptr, static_cast<std::align_val_t>(alignof(T)));
  }

  struct __raw_bins_tag { };

  rb_map(__raw_bins_tag, usize count) { __alloc_bins(count); }

  static usize
  __round_pow2(usize n) noexcept
  {
    if ( n <= __min_bins ) return __min_bins;
    usize p = 1;
    while ( p < n ) {
      usize next = p << 1u;
      if ( next <= p ) return p;
      p = next;
    }
    return p;
  }

  static constexpr usize
  __load_limit(usize bins) noexcept
  {
    return (bins / __load_denom) * __load_num + ((bins % __load_denom) * __load_num) / __load_denom;
  }

  void
  __alloc_bins(usize count)
  {
    if ( count > static_cast<usize>(numeric_limits<i32>::max()) ) exc<except::library_error>("rb_map: bin index capacity overflow");
    __bins = __alloc_array<__bin_t>(count);
    for ( usize i = 0; i < count; ++i ) {
      __bins[i].list_head = -1;
      __bins[i].tree = nullptr;
      __bins[i].chain_len = 0;
    }
    __n_bins = count;
    __bin_mask = count - 1u;
  }

  __attribute__((always_inline)) usize
  __bin_of(hash64_t h) const noexcept
  {
    return static_cast<usize>(h) & __bin_mask;
  }

  void
  __grow_soa()
  {
    if ( __cap_soa > static_cast<usize>(numeric_limits<i32>::max()) / 2u )
      exc<except::library_error>("rb_map: SoA index capacity overflow");
    usize new_cap = __cap_soa == 0 ? 16 : __cap_soa * 2;
    hash64_t *nh = nullptr;
    i32 *nn = nullptr;
    i32 *nb = nullptr;
    K *nk = nullptr;
    V *nv = nullptr;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      nh = __alloc_array<hash64_t>(new_cap);
      nn = __alloc_array<i32>(new_cap);
      nb = __alloc_array<i32>(new_cap);
      nk = __alloc_array<K>(new_cap);
      nv = __alloc_array<V>(new_cap);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      __free_array(nv);
      __free_array(nk);
      __free_array(nb);
      __free_array(nn);
      __free_array(nh);
      throw;
    }
#endif
    usize moved = 0;
    if ( __n_soa > 0 ) {
#if !defined(__micron_freestanding) || defined(__micron_eh)
      try {
        for ( ; moved < __n_soa; ++moved ) {
          if constexpr ( micron::is_nothrow_move_constructible_v<K> && micron::is_nothrow_move_constructible_v<V> )
            new (nk + moved) K(micron::move(__keys[moved]));
          else if constexpr ( micron::is_copy_constructible_v<K> )
            new (nk + moved) K(__keys[moved]);
          else
            new (nk + moved) K(micron::move(__keys[moved]));
          try {
            if constexpr ( micron::is_nothrow_move_constructible_v<K> && micron::is_nothrow_move_constructible_v<V> )
              new (nv + moved) V(micron::move(__values[moved]));
            else if constexpr ( micron::is_copy_constructible_v<K> )
              new (nv + moved) V(__values[moved]);
            else
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
        __free_array(nv);
        __free_array(nk);
        __free_array(nb);
        __free_array(nn);
        __free_array(nh);
        throw;
      }
#else
      for ( ; moved < __n_soa; ++moved ) {
        new (nk + moved) K(micron::move(__keys[moved]));
        new (nv + moved) V(micron::move(__values[moved]));
      }
#endif
      micron::memcpy(reinterpret_cast<byte *>(nh), reinterpret_cast<byte *>(__hashes), __n_soa * sizeof(hash64_t));
      micron::memcpy(reinterpret_cast<byte *>(nn), reinterpret_cast<byte *>(__next), __n_soa * sizeof(i32));
      micron::memcpy(reinterpret_cast<byte *>(nb), reinterpret_cast<byte *>(__home_bin), __n_soa * sizeof(i32));
      for ( usize i = 0; i < __n_soa; ++i ) {
        __keys[i].~K();
        __values[i].~V();
      }
    }
    __free_array(__hashes);
    __free_array(__keys);
    __free_array(__values);
    __free_array(__next);
    __free_array(__home_bin);
    __hashes = nh;
    __keys = nk;
    __values = nv;
    __next = nn;
    __home_bin = nb;
    __cap_soa = new_cap;
  }

  void
  __soa_remove(i32 idx)
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
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
    while ( cur != -1 ) {
      i32 nxt = __next[cur];
      if constexpr ( micron::is_copy_constructible_v<K> )
        t->insert(__tree_entry(__hashes[cur], __keys[cur], __values[cur]));
      else
        t->insert(__tree_entry(__hashes[cur], micron::move(__keys[cur]), micron::move(__values[cur])));
      cur = nxt;
    }
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      delete t;
      throw;
    }
#endif

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
    if ( __total < __load_limit(__n_bins) ) return false;
    if ( __n_bins > static_cast<usize>(numeric_limits<i32>::max()) / 2u )
      exc<except::library_error>("rb_map: bin index capacity overflow");
    __rehash(__n_bins * 2);
    return true;
  }

  void
  __rehash(usize new_n_bins)
  {
    if ( new_n_bins < __min_bins ) new_n_bins = __min_bins;
    new_n_bins = __round_pow2(new_n_bins);

    if constexpr ( micron::is_copy_constructible_v<K> ) {
      rb_map next(__raw_bins_tag{}, new_n_bins);
      for ( usize i = 0; i < __n_soa; ++i ) next.__insert_absent(__hashes[i], __keys[i], __values[i]);
      for ( usize i = 0; i < __n_bins; ++i ) {
        const __tree_t *tree = __bins[i].tree;
        if ( !tree ) continue;
        tree->for_each([&next](const __tree_entry &entry) { next.__insert_absent(entry.hash, entry.key, entry.value); });
      }
      swap(next);
      return;
    }

    usize total = __total;
    auto *snap = total ? __alloc_array<__tree_entry>(total) : nullptr;
    usize w = 0;

#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      for ( usize i = 0; i < __n_soa; ++i ) {
        new (snap + w) __tree_entry(__hashes[i], micron::move(__keys[i]), micron::move(__values[i]));
        ++w;
      }
    } catch ( ... ) {
      for ( usize j = 0; j < w; ++j ) snap[j].~__tree_entry();
      __free_array(snap);
      throw;
    }
#else
    for ( usize i = 0; i < __n_soa; ++i ) {
      new (snap + w) __tree_entry(__hashes[i], micron::move(__keys[i]), micron::move(__values[i]));
      ++w;
    }
#endif
    for ( usize i = 0; i < __n_soa; ++i ) {
      __keys[i].~K();
      __values[i].~V();
    }
    __n_soa = 0;

    if ( __bins ) {
#if !defined(__micron_freestanding) || defined(__micron_eh)
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
        __free_array(snap);
        for ( usize i = 0; i < __n_bins; ++i )
          if ( __bins[i].tree ) delete __bins[i].tree;
        __free_array(__bins);
        __bins = nullptr;
        __n_bins = 0;
        __bin_mask = 0;
        __total = 0;
        throw;
      }
#else
      for ( usize i = 0; i < __n_bins; ++i ) {
        __tree_t *t = __bins[i].tree;
        if ( !t ) continue;
        while ( !t->empty() ) {
          new (snap + w) __tree_entry(t->extract_min());
          ++w;
        }
      }
#endif
      for ( usize i = 0; i < __n_bins; ++i )
        if ( __bins[i].tree ) delete __bins[i].tree;
      __free_array(__bins);
      __bins = nullptr;
    }

    __bins = __alloc_array<__bin_t>(new_n_bins);
    for ( usize i = 0; i < new_n_bins; ++i ) {
      __bins[i].list_head = -1;
      __bins[i].tree = nullptr;
      __bins[i].chain_len = 0;
    }
    __n_bins = new_n_bins;
    __bin_mask = new_n_bins - 1u;
    __total = 0;

    usize consumed = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
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
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      for ( usize j = consumed; j < w; ++j ) snap[j].~__tree_entry();
      __free_array(snap);
      throw;
    }
#endif
    __free_array(snap);
  }

  void
  __free_all()
  {
    if ( __bins ) {
      for ( usize i = 0; i < __n_bins; ++i )
        if ( __bins[i].tree ) delete __bins[i].tree;
      __free_array(__bins);
      __bins = nullptr;
    }
    if ( __n_soa > 0 ) {
      for ( usize i = 0; i < __n_soa; ++i ) {
        __keys[i].~K();
        __values[i].~V();
      }
    }
    __free_array(__hashes);
    __free_array(__keys);
    __free_array(__values);
    __free_array(__next);
    __free_array(__home_bin);
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

  V *
  __find_hash(hash64_t h, const K &key)
  {
    if ( !__bins ) return nullptr;
    __bin_t &bin = __bins[__bin_of(h)];
    if ( bin.tree ) {
      __tree_entry *entry = bin.tree->find_by(
          [h, &key](const __tree_entry &data) { return h != data.hash ? h < data.hash : key < data.key; },
          [h, &key](const __tree_entry &data) { return data.hash != h ? data.hash < h : data.key < key; });
      return entry ? micron::addressof(entry->value) : nullptr;
    }
    for ( i32 i = bin.list_head; i != -1; i = __next[i] )
      if ( __hashes[i] == h && __keys[i] == key ) return micron::addressof(__values[i]);
    return nullptr;
  }

  const V *
  __find_hash(hash64_t h, const K &key) const
  {
    return const_cast<rb_map *>(this)->__find_hash(h, key);
  }

  template<class KK, class VV>
  micron::pair<bool, V *>
  __insert_absent(hash64_t h, KK &&key, VV &&value)
  {
    if ( __total == numeric_limits<usize>::max() ) exc<except::library_error>("rb_map: size overflow");
    if ( __total + 1u > __load_limit(__n_bins) ) {
      if ( __n_bins > static_cast<usize>(numeric_limits<i32>::max()) / 2u )
        exc<except::library_error>("rb_map: bin index capacity overflow");
      __rehash(__n_bins * 2u);
    }
    usize bin_index = __bin_of(h);
    __bin_t &bin = __bins[bin_index];

    if ( !bin.tree && bin.chain_len + 1u >= __treeify_threshold && __n_bins >= __min_treeify_cap ) __treeify(bin_index);
    if ( bin.tree ) {
      __tree_entry &inserted =
          bin.tree->insert(__tree_entry(h, K(micron::forward<KK>(key)), V(micron::forward<VV>(value))));
      ++__total;
      return { true, micron::addressof(inserted.value) };
    }

    if ( __n_soa == __cap_soa ) __grow_soa();
    i32 index = static_cast<i32>(__n_soa);
    new (__keys + index) K(micron::forward<KK>(key));
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      new (__values + index) V(micron::forward<VV>(value));
    } catch ( ... ) {
      __keys[index].~K();
      throw;
    }
#else
    new (__values + index) V(micron::forward<VV>(value));
#endif
    __hashes[index] = h;
    __next[index] = bin.list_head;
    __home_bin[index] = static_cast<i32>(bin_index);
    bin.list_head = index;
    ++bin.chain_len;
    ++__n_soa;
    ++__total;
    return { true, micron::addressof(__values[index]) };
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
    __alloc_bins(__min_bins);
  }

  explicit rb_map(usize cap) { __alloc_bins(__round_pow2(cap)); }

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
    __alloc_bins(__min_bins);
  }

  bool
  treeified(usize bin) const noexcept
  {
    return bin < __n_bins && __bins[bin].tree != nullptr;
  }

  V *
  find_hash(hash64_t h, const K &key)
  {
    return __find_hash(h, key);
  }

  const V *
  find_hash(hash64_t h, const K &key) const
  {
    return __find_hash(h, key);
  }

  V *
  find(const K &key)
  {
    return __find_hash(hash<hash64_t>(key), key);
  }

  const V *
  find(const K &key) const
  {
    return __find_hash(hash<hash64_t>(key), key);
  }

  bool
  contains(const K &k) const
  {
    return find(k) != nullptr;
  }

  usize
  count(const K &k) const
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
    if ( V *existing = __find_hash(h, k) ) {
      *existing = micron::forward<VV>(v);
      return { false, existing };
    }
    return __insert_absent(h, micron::forward<KK>(k), micron::forward<VV>(v));
  }

  template<class KK, class VV>
    requires(micron::is_same_v<micron::remove_cvref_t<KK>, K> && micron::is_same_v<micron::remove_cvref_t<VV>, V>)
  micron::pair<bool, V *>
  insert(KK &&k, VV &&v)
  {
    hash64_t h = hash<hash64_t>(k);
    if ( V *existing = __find_hash(h, k) ) return { false, existing };
    return __insert_absent(h, micron::forward<KK>(k), micron::forward<VV>(v));
  }

  template<class KK, class VV>
    requires(micron::is_same_v<micron::remove_cvref_t<KK>, K> && micron::is_same_v<micron::remove_cvref_t<VV>, V>)
  micron::pair<bool, V *>
  insert_hash(hash64_t h, KK &&k, VV &&v)
  {
    if ( V *existing = __find_hash(h, k) ) return { false, existing };
    return __insert_absent(h, micron::forward<KK>(k), micron::forward<VV>(v));
  }

  template<class KK, typename... Args>
    requires micron::is_same_v<micron::remove_cvref_t<KK>, K>
  micron::pair<bool, V *>
  emplace(KK &&k, Args &&...args)
  {
    hash64_t h = hash<hash64_t>(k);
    if ( V *existing = __find_hash(h, k) ) return { false, existing };
    return __insert_absent(h, micron::forward<KK>(k), V(micron::forward<Args>(args)...));
  }

  V &
  operator[](const K &k)
  {
    hash64_t h = hash<hash64_t>(k);
    V *v = __find_hash(h, k);
    if ( v ) return *v;
    auto r = __insert_absent(h, k, V{});
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

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // two-phase cursor
  class const_iterator
  {
    const rb_map *__m = nullptr;
    usize __soa_i = 0;
    usize __bin_i = 0;
    typename __tree_t::const_iterator __ti{};

    void
    __enter_bin(usize b)
    {
      __bin_i = b;
      while ( __bin_i < __m->__n_bins ) {
        const __tree_t *t = __m->__bins[__bin_i].tree;
        if ( t ) {
          __ti = t->begin();
          if ( __ti != t->end() ) return;
        }
        ++__bin_i;
      }
      __ti = typename __tree_t::const_iterator{};
    }

  public:
    using value_type = micron::pair<const K &, const V &>;
    using reference = micron::pair<const K &, const V &>;
    using difference_type = ssize_t;

    constexpr const_iterator() = default;

    const_iterator(const rb_map *__mm, bool __end) : __m(__mm), __soa_i(__end ? __mm->__n_soa : 0), __bin_i(__end ? __mm->__n_bins : 0)
    {
      if ( __end ) return;
      if ( __soa_i >= __m->__n_soa ) __enter_bin(0);
    }

    reference
    operator*() const
    {
      if ( __soa_i < __m->__n_soa ) return reference{ __m->__keys[__soa_i], __m->__values[__soa_i] };
      return reference{ __ti->key, __ti->value };
    }

    const_iterator &
    operator++()
    {
      if ( __soa_i < __m->__n_soa ) {
        ++__soa_i;
        if ( __soa_i >= __m->__n_soa ) __enter_bin(0);
        return *this;
      }
      ++__ti;
      const __tree_t *t = __m->__bins[__bin_i].tree;
      if ( __ti == t->end() ) __enter_bin(__bin_i + 1);
      return *this;
    }

    const_iterator
    operator++(int)
    {
      const_iterator __t = *this;
      ++*this;
      return __t;
    }

    bool
    operator==(const const_iterator &__o) const noexcept
    {
      return __soa_i == __o.__soa_i && __bin_i == __o.__bin_i && __ti == __o.__ti;
    }

    bool
    operator!=(const const_iterator &__o) const noexcept
    {
      return !(*this == __o);
    }
  };

  using iterator = const_iterator;

  const_iterator
  begin() const
  {
    return const_iterator{ this, false };
  }

  const_iterator
  end() const
  {
    return const_iterator{ this, true };
  }

  const_iterator
  cbegin() const
  {
    return begin();
  }

  const_iterator
  cend() const
  {
    return end();
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
