//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../bits.hpp"
#include "../bits/__arch.hpp"
#include "../hash/hash.hpp"
#include "../simd/aliases.hpp"
#include "../simd/types.hpp"
#include "../types.hpp"

#include "../allocator.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/allocation/resources.hpp"
#include "../memory/new.hpp"
#include "../tuple.hpp"

#include "swiss.hpp"

namespace micron
{

// growable heap-backed swiss table, other same as swiss_map
template<typename K, typename V, class Alloc = micron::allocator_serial<>>
  requires micron::is_copy_constructible_v<V> and micron::is_move_constructible_v<V>
class heap_swiss_map
{
  struct __hs_entry {
    K key;
    V value;

    __hs_entry() : key{}, value{} { }

    __hs_entry(const K &k, const V &v) : key(k), value(v) { }

    __hs_entry(K &&k, V &&v) : key(micron::move(k)), value(micron::move(v)) { }

    __hs_entry(const __hs_entry &) = default;
    __hs_entry(__hs_entry &&) = default;
    __hs_entry &operator=(const __hs_entry &) = default;
    __hs_entry &operator=(__hs_entry &&) = default;
  };

  u8 *__ctrl = nullptr;
  __hs_entry *__entries = nullptr;
  usize __n_slots = 0;
  usize __cap_mask = 0;
  usize __size = 0;
  usize __growth_left = 0;      // slots remaining before resize

  static constexpr usize __min_cap = 16;
  static constexpr usize __group = 16;
  static constexpr usize __load_num = 7;
  static constexpr usize __load_denom = 8;

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

  static constexpr u8
  __h2(hash64_t h) noexcept
  {
    u8 v = static_cast<u8>((h >> 40) & 0x7Fu);
    u8 c = static_cast<u8>(v | __sentinel);
    return c >= __deleted ? static_cast<u8>(c - 2u) : c;
  }

  static constexpr usize
  __h1(hash64_t h) noexcept
  {
    return static_cast<usize>(h);
  }

  ::micron::__mask
  __match_h2(u8 hash_val, usize ind) const noexcept
  {
#if defined(__micron_arch_x86_any)
    simd::i128 m = simd::sse::splat_i8(static_cast<char>(hash_val));
    simd::i128 c = simd::sse::loadu_i128(reinterpret_cast<const __m128i_u *>(&__ctrl[ind]));
    return ::micron::__mask(simd::sse::movemask_i8(simd::sse::eq_i8(m, c)));
#elif defined(__micron_arm_neon)
    uint8x16_t m = simd::neon::splat_u8(hash_val);
    uint8x16_t c = simd::neon::load_u8(&__ctrl[ind]);
    return ::micron::__mask(static_cast<i32>(simd::neon::movemask_u8(simd::neon::eq(c, m))));
#else
    i32 r = 0;
    for ( usize i = 0; i < 16; ++i )
      if ( __ctrl[ind + i] == hash_val ) r |= (1 << i);
    return ::micron::__mask(r);
#endif
  }

  ::micron::__mask
  __match_empty_slots(usize ind) const noexcept
  {
#if defined(__micron_arch_x86_any)
    simd::i128 e = simd::sse::splat_i8(static_cast<char>(__empty));
    simd::i128 c = simd::sse::loadu_i128(reinterpret_cast<const __m128i_u *>(&__ctrl[ind]));
    return ::micron::__mask(simd::sse::movemask_i8(simd::sse::eq_i8(e, c)));
#elif defined(__micron_arm_neon)
    uint8x16_t e = simd::neon::splat_u8(__empty);
    uint8x16_t c = simd::neon::load_u8(&__ctrl[ind]);
    return ::micron::__mask(static_cast<i32>(simd::neon::movemask_u8(simd::neon::eq(c, e))));
#else
    i32 r = 0;
    for ( usize i = 0; i < 16; ++i )
      if ( __ctrl[ind + i] == __empty ) r |= (1 << i);
    return ::micron::__mask(r);
#endif
  }

  ::micron::__mask
  __match_empty_or_del(usize ind) const noexcept
  {
#if defined(__micron_arch_x86_any)
    simd::i128 c = simd::sse::loadu_i128(reinterpret_cast<const __m128i_u *>(&__ctrl[ind]));
    simd::i128 e = simd::sse::splat_i8(static_cast<char>(__empty));
    simd::i128 d = simd::sse::splat_i8(static_cast<char>(__deleted));
    return ::micron::__mask(simd::sse::movemask_i8(simd::sse::or_i128(simd::sse::eq_i8(c, e), simd::sse::eq_i8(c, d))));
#elif defined(__micron_arm_neon)
    uint8x16_t c = simd::neon::load_u8(&__ctrl[ind]);
    uint8x16_t e = simd::neon::splat_u8(__empty);
    uint8x16_t d = simd::neon::splat_u8(__deleted);
    return ::micron::__mask(static_cast<i32>(simd::neon::movemask_u8(simd::neon::or_(simd::neon::eq(c, e), simd::neon::eq(c, d)))));
#else
    i32 r = 0;
    for ( usize i = 0; i < 16; ++i ) {
      u8 cc = __ctrl[ind + i];
      if ( cc == __empty || cc == __deleted ) r |= (1 << i);
    }
    return ::micron::__mask(r);
#endif
  }

  void
  __alloc_storage(usize n_slots)
  {
    u8 *ctrl = new u8[n_slots + __group];
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      chunk<byte> blk = Alloc::create(n_slots * sizeof(__hs_entry));
      micron::memset(ctrl, __empty, n_slots + __group);
      __ctrl = ctrl;
      __entries = reinterpret_cast<__hs_entry *>(blk.ptr);
      __n_slots = n_slots;
      __cap_mask = n_slots - 1u;
      __growth_left = n_slots * __load_num / __load_denom;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      delete[] ctrl;
      throw;
    }
#endif
  }

  void
  __destroy_occupied() noexcept
  {
    if ( !__entries || !__ctrl ) return;
    if constexpr ( !micron::is_trivially_destructible_v<__hs_entry> ) {
      for ( usize i = 0; i < __n_slots; ++i ) {
        u8 c = __ctrl[i];
        if ( c != __empty && c != __deleted ) __entries[i].~__hs_entry();
      }
    }
  }

  void
  __free_storage() noexcept
  {
    if ( __ctrl ) {
      delete[] __ctrl;
      __ctrl = nullptr;
    }
    if ( __entries ) {
      chunk<byte> ch{ reinterpret_cast<byte *>(__entries), __n_slots * sizeof(__hs_entry) };
      Alloc::destroy(ch);
      __entries = nullptr;
    }
  }

  void
  __mirror_ctrl(usize probe, u8 v) noexcept
  {
    __ctrl[probe] = v;
    if ( probe < __group ) __ctrl[__n_slots + probe] = v;
  }

  template<typename KK, typename VV>
  V *
  __probe_insert(KK &&key, VV &&value, u8 h2, usize start)
  {
    usize i = 0;
    while ( i < __n_slots ) {
      usize g = (start + i) & __cap_mask;
      ::micron::__mask em = __match_empty_or_del(g);
      if ( em.any() ) {
        usize probe = (g + em.lowest()) & __cap_mask;
        new (micron::addr(__entries[probe])) __hs_entry{ micron::forward<KK>(key), micron::forward<VV>(value) };
        __mirror_ctrl(probe, h2);
        ++__size;
        --__growth_left;
        return micron::addr(__entries[probe].value);
      }
      i += __group;
    }
    return nullptr;
  }

  V *
  __probe_find(const K &key, u8 h2, usize start) const noexcept
  {
    if ( !__ctrl ) return nullptr;
    usize i = 0;
    while ( i < __n_slots ) {
      usize g = (start + i) & __cap_mask;
      ::micron::__mask m = __match_h2(h2, g);
      while ( m.any() ) {
        usize probe = (g + m.lowest()) & __cap_mask;
        if ( __entries[probe].key == key ) return const_cast<V *>(micron::addr(__entries[probe].value));
        m.clear_lowest();
      }
      if ( __match_empty_slots(g).any() ) return nullptr;
      i += __group;
    }
    return nullptr;
  }

  void
  __rehash(usize new_n_slots)
  {
    u8 *old_ctrl = __ctrl;
    __hs_entry *old_entries = __entries;
    usize old_n = __n_slots;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    // rollback-only: read solely by the catch below, so they must not be declared without it
    usize old_mask = __cap_mask;
    usize old_size = __size;
    usize old_growth = __growth_left;
    try {
#endif
      __ctrl = nullptr;
      __entries = nullptr;
      __n_slots = 0;
      __cap_mask = 0;
      __size = 0;
      __growth_left = 0;
      __alloc_storage(new_n_slots);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      __ctrl = old_ctrl;
      __entries = old_entries;
      __n_slots = old_n;
      __cap_mask = old_mask;
      __size = old_size;
      __growth_left = old_growth;
      throw;
    }
#endif
    if ( old_entries && old_ctrl ) {
      for ( usize i = 0; i < old_n; ++i ) {
        u8 c = old_ctrl[i];
        if ( c != __empty && c != __deleted ) {
          hash64_t kh = hash<hash64_t>(old_entries[i].key);
          __probe_insert(micron::move(old_entries[i].key), micron::move(old_entries[i].value), __h2(kh), __h1(kh) & __cap_mask);
          if constexpr ( !micron::is_trivially_destructible_v<__hs_entry> ) old_entries[i].~__hs_entry();
        }
      }
    }
    if ( old_ctrl ) delete[] old_ctrl;
    if ( old_entries ) {
      chunk<byte> ch{ reinterpret_cast<byte *>(old_entries), old_n * sizeof(__hs_entry) };
      Alloc::destroy(ch);
    }
  }

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef usize size_type;
  typedef K key_type;
  typedef V mapped_type;
  typedef __hs_entry value_type;
  typedef __hs_entry &reference;
  typedef const __hs_entry &const_reference;
  typedef __hs_entry *pointer;
  typedef const __hs_entry *const_pointer;

  ~heap_swiss_map()
  {
    __destroy_occupied();
    __free_storage();
  }

  heap_swiss_map() { __alloc_storage(__min_cap); }

  explicit heap_swiss_map(usize n) { __alloc_storage(round_pow2(n)); }

  heap_swiss_map(const heap_swiss_map &o)
  {
    __alloc_storage(o.__n_slots);
    for ( usize i = 0; i < o.__n_slots; ++i ) {
      u8 c = o.__ctrl[i];
      if ( c != __empty && c != __deleted ) {
        new (micron::addr(__entries[i])) __hs_entry(o.__entries[i]);
        __mirror_ctrl(i, c);
      }
    }
    __size = o.__size;
    __growth_left = o.__growth_left;
  }

  heap_swiss_map(heap_swiss_map &&o) noexcept
      : __ctrl(o.__ctrl), __entries(o.__entries), __n_slots(o.__n_slots), __cap_mask(o.__cap_mask), __size(o.__size),
        __growth_left(o.__growth_left)
  {
    o.__ctrl = nullptr;
    o.__entries = nullptr;
    o.__n_slots = 0;
    o.__cap_mask = 0;
    o.__size = 0;
    o.__growth_left = 0;
  }

  heap_swiss_map &
  operator=(const heap_swiss_map &o)
  {
    if ( this == &o ) return *this;
    __destroy_occupied();
    __free_storage();
    __alloc_storage(o.__n_slots);
    for ( usize i = 0; i < o.__n_slots; ++i ) {
      u8 c = o.__ctrl[i];
      if ( c != __empty && c != __deleted ) {
        new (micron::addr(__entries[i])) __hs_entry(o.__entries[i]);
        __mirror_ctrl(i, c);
      }
    }
    __size = o.__size;
    __growth_left = o.__growth_left;
    return *this;
  }

  heap_swiss_map &
  operator=(heap_swiss_map &&o) noexcept
  {
    if ( this == &o ) return *this;
    __destroy_occupied();
    __free_storage();
    __ctrl = o.__ctrl;
    __entries = o.__entries;
    __n_slots = o.__n_slots;
    __cap_mask = o.__cap_mask;
    __size = o.__size;
    __growth_left = o.__growth_left;
    o.__ctrl = nullptr;
    o.__entries = nullptr;
    o.__n_slots = 0;
    o.__cap_mask = 0;
    o.__size = 0;
    o.__growth_left = 0;
    return *this;
  }

  usize
  size() const noexcept
  {
    return __size;
  }

  usize
  max_size() const noexcept
  {
    return __n_slots;
  }

  usize
  capacity() const noexcept
  {
    return __n_slots;
  }

  bool
  empty() const noexcept
  {
    return __size == 0;
  }

  float
  load_factor() const noexcept
  {
    return __n_slots > 0u ? static_cast<float>(__size) / static_cast<float>(__n_slots) : 0.0f;
  }

  void
  clear() noexcept
  {
    __destroy_occupied();
    if ( __ctrl ) micron::memset(__ctrl, __empty, __n_slots + __group);
    __size = 0;
    __growth_left = __n_slots * __load_num / __load_denom;
  }

  void
  reserve(usize n)
  {
    usize need = round_pow2(n);
    if ( need > __n_slots ) __rehash(need);
  }

  template<typename KK, typename VV>
  micron::pair<bool, V *>
  insert_or_assign(KK &&key, VV &&value)
  {
    if ( __growth_left == 0 ) __rehash(__n_slots * 2);
    hash64_t kh = hash<hash64_t>(key);
    u8 h2 = __h2(kh);
    usize start = __h1(kh) & __cap_mask;
    V *ex = __probe_find(key, h2, start);
    if ( ex ) {
      *ex = micron::forward<VV>(value);
      return { false, ex };
    }
    return { true, __probe_insert(micron::forward<KK>(key), micron::forward<VV>(value), h2, start) };
  }

  micron::pair<bool, V *>
  insert(const K &key, const V &value)
  {
    if ( __growth_left == 0 ) __rehash(__n_slots * 2);
    hash64_t kh = hash<hash64_t>(key);
    u8 h2 = __h2(kh);
    usize start = __h1(kh) & __cap_mask;
    V *ex = __probe_find(key, h2, start);
    if ( ex ) return { false, ex };
    return { true, __probe_insert(key, value, h2, start) };
  }

  micron::pair<bool, V *>
  insert(K &&key, V &&value)
  {
    if ( __growth_left == 0 ) __rehash(__n_slots * 2);
    hash64_t kh = hash<hash64_t>(key);
    u8 h2 = __h2(kh);
    usize start = __h1(kh) & __cap_mask;
    V *ex = __probe_find(key, h2, start);
    if ( ex ) return { false, ex };
    return { true, __probe_insert(micron::move(key), micron::move(value), h2, start) };
  }

  micron::pair<bool, V *>
  insert(const micron::pair<K, V> &kv)
  {
    return insert(kv.a, kv.b);
  }

  template<typename... Args>
  micron::pair<bool, V *>
  emplace(const K &key, Args &&...args)
  {
    if ( __growth_left == 0 ) __rehash(__n_slots * 2);
    hash64_t kh = hash<hash64_t>(key);
    u8 h2 = __h2(kh);
    usize start = __h1(kh) & __cap_mask;
    V *ex = __probe_find(key, h2, start);
    if ( ex ) return { false, ex };
    return { true, __probe_insert(K(key), V(micron::forward<Args>(args)...), h2, start) };
  }

  V *
  find(const K &key) noexcept
  {
    hash64_t kh = hash<hash64_t>(key);
    return __probe_find(key, __h2(kh), __h1(kh) & __cap_mask);
  }

  const V *
  find(const K &key) const noexcept
  {
    hash64_t kh = hash<hash64_t>(key);
    return __probe_find(key, __h2(kh), __h1(kh) & __cap_mask);
  }

  bool
  contains(const K &key) const noexcept
  {
    return find(key) != nullptr;
  }

  usize
  count(const K &key) const noexcept
  {
    return find(key) ? 1u : 0u;
  }

  V &
  at(const K &key)
  {
    V *v = find(key);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("heap_swiss_map::at(): key not found");
    return *v;
  }

  const V &
  at(const K &key) const
  {
    const V *v = find(key);
    if ( !v ) [[unlikely]]
      exc<except::library_error>("heap_swiss_map::at(): key not found");
    return *v;
  }

  V &
  operator[](const K &key)
  {
    V *v = find(key);
    if ( v ) return *v;
    auto r = insert(key, V{});
    if ( !r.b ) [[unlikely]]
      exc<except::library_error>("heap_swiss_map::operator[](): insertion failed");
    return *r.b;
  }

  void
  swap(heap_swiss_map &o) noexcept
  {
    micron::swap(__ctrl, o.__ctrl);
    micron::swap(__entries, o.__entries);
    micron::swap(__n_slots, o.__n_slots);
    micron::swap(__cap_mask, o.__cap_mask);
    micron::swap(__size, o.__size);
    micron::swap(__growth_left, o.__growth_left);
  }

  template<typename Fn>
  void
  for_each(Fn &&fn)
  {
    if ( !__ctrl ) return;
    for ( usize i = 0; i < __n_slots; ++i ) {
      u8 c = __ctrl[i];
      if ( c != __empty && c != __deleted ) fn(__entries[i].key, __entries[i].value);
    }
  }

  template<typename Fn>
  void
  for_each(Fn &&fn) const
  {
    if ( !__ctrl ) return;
    for ( usize i = 0; i < __n_slots; ++i ) {
      u8 c = __ctrl[i];
      if ( c != __empty && c != __deleted ) fn(__entries[i].key, __entries[i].value);
    }
  }

  bool
  erase(const K &key) noexcept
  {
    if ( !__ctrl ) return false;
    hash64_t kh = hash<hash64_t>(key);
    u8 h2 = __h2(kh);
    usize start = __h1(kh) & __cap_mask;
    usize i = 0;
    while ( i < __n_slots ) {
      usize g = (start + i) & __cap_mask;
      ::micron::__mask m = __match_h2(h2, g);
      while ( m.any() ) {
        usize probe = (g + m.lowest()) & __cap_mask;
        if ( __entries[probe].key == key ) {
          if constexpr ( !micron::is_trivially_destructible_v<__hs_entry> ) __entries[probe].~__hs_entry();
          __mirror_ctrl(probe, __deleted);
          --__size;
          return true;
        }
        m.clear_lowest();
      }
      if ( __match_empty_slots(g).any() ) return false;
      i += __group;
    }
    return false;
  }

  class iterator
  {
    heap_swiss_map *__m;
    usize __i;

    void
    advance()
    {
      while ( __i < __m->__n_slots && (__m->__ctrl[__i] == __empty || __m->__ctrl[__i] == __deleted) ) ++__i;
    }

  public:
    using value_type = micron::pair<const K &, V &>;
    using difference_type = ptrdiff_t;
    using pointer = micron::pair<const K *, V *>;
    using reference = micron::pair<const K &, V &>;

    iterator(heap_swiss_map *m, usize i) : __m(m), __i(i) { advance(); }

    reference
    operator*()
    {
      return { __m->__entries[__i].key, __m->__entries[__i].value };
    }

    pointer
    operator->()
    {
      return { micron::addr(__m->__entries[__i].key), micron::addr(__m->__entries[__i].value) };
    }

    iterator &
    operator++()
    {
      ++__i;
      advance();
      return *this;
    }

    iterator
    operator++(int)
    {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool
    operator==(const iterator &o) const
    {
      return __m == o.__m && __i == o.__i;
    }

    bool
    operator!=(const iterator &o) const
    {
      return !(*this == o);
    }
  };

  class const_iterator
  {
    const heap_swiss_map *__m;
    usize __i;

    void
    advance()
    {
      while ( __i < __m->__n_slots && (__m->__ctrl[__i] == __empty || __m->__ctrl[__i] == __deleted) ) ++__i;
    }

  public:
    using value_type = micron::pair<const K &, const V &>;
    using difference_type = ptrdiff_t;
    using pointer = micron::pair<const K *, const V *>;
    using reference = micron::pair<const K &, const V &>;

    const_iterator(const heap_swiss_map *m, usize i) : __m(m), __i(i) { advance(); }

    reference
    operator*() const
    {
      return { __m->__entries[__i].key, __m->__entries[__i].value };
    }

    pointer
    operator->() const
    {
      return { micron::addr(__m->__entries[__i].key), micron::addr(__m->__entries[__i].value) };
    }

    const_iterator &
    operator++()
    {
      ++__i;
      advance();
      return *this;
    }

    const_iterator
    operator++(int)
    {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool
    operator==(const const_iterator &o) const
    {
      return __m == o.__m && __i == o.__i;
    }

    bool
    operator!=(const const_iterator &o) const
    {
      return !(*this == o);
    }
  };

  iterator
  begin()
  {
    return iterator(this, 0);
  }

  iterator
  end()
  {
    return iterator(this, __n_slots);
  }

  const_iterator
  begin() const
  {
    return const_iterator(this, 0);
  }

  const_iterator
  end() const
  {
    return const_iterator(this, __n_slots);
  }

  const_iterator
  cbegin() const
  {
    return const_iterator(this, 0);
  }

  const_iterator
  cend() const
  {
    return const_iterator(this, __n_slots);
  }
};

template<typename K, typename V, class Alloc = micron::allocator_serial<>> using hswiss = heap_swiss_map<K, V, Alloc>;

};      // namespace micron
