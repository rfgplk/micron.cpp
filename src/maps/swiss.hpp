#pragma once
#include "../bits.hpp"
#include "../bits/__arch.hpp"
#include "../hash/hash.hpp"
#include "../simd/aliases.hpp"
#include "../simd/types.hpp"
#include "../types.hpp"

#include "../memory/actions.hpp"
#include "../memory/cmemory.hpp"
#include "../tuple.hpp"

#include "hopscotch.hpp"

namespace micron
{
constexpr u8 __empty = 0b11111111;         // 0xFF - empty slot
constexpr u8 __deleted = 0b11111110;       // 0xFE - tombstone
constexpr u8 __sentinel = 0b10000000;      // 0x80 - high bit set for occupied

struct __mask {
  i32 bits;

  explicit __mask(i32 b) : bits(b) { }

  bool
  any() const
  {
    return bits != 0;
  }

  int
  lowest() const
  {
    return countr_zero(static_cast<u32>(bits));
  }

  void
  clear_lowest()
  {
    bits &= bits - 1;
  }

  explicit
  operator bool() const
  {
    return any();
  }
};

// NH = probe-window cap (number of slots scanned from a key's home)
template<typename K, typename V, usize N, usize NH = N>
  requires(N >= 16 and (N % 16) == 0 and NH <= N)
class stack_swiss_map
{
  static constexpr u8
  __h2(hash64_t h)
  {
    // 7-bit tag for the control byte
    u8 v = static_cast<u8>((h >> 40) & 0x7Fu);
    u8 c = static_cast<u8>(v | __sentinel);
    // NEVER alias the reserved control bytes __deleted (0xFE) / __empty (0xFF)
    return c >= __deleted ? static_cast<u8>(c - 2u) : c;
  }

  static constexpr u8
  __hash(const K &k)
  {
    return __h2(hash<hash64_t>(k));
  }

  static constexpr usize
  __h1(hash64_t h)
  {
    return static_cast<usize>(h);
  }

  static constexpr usize
  __b_index(const K &k)
  {
    return __h1(hash<hash64_t>(k)) % N;
  }

  static constexpr usize
  __b_index_key(hash64_t k)
  {
    return __h1(k) % N;
  }

  __mask
  __match(u8 hash_val, usize ind) const
  {
    // NOTE: must be unaligned load, no guarantee ind will always be aligned
#if defined(__micron_arch_x86_any)
    simd::i128 match = simd::sse::splat_i8(static_cast<char>(hash_val));
    simd::i128 meta = simd::sse::loadu_i128(reinterpret_cast<const __m128i_u *>(&__control_bytes[ind]));
    int mask = simd::sse::movemask_i8(simd::sse::eq_i8(match, meta));
    return __mask(mask);
#elif defined(__micron_arm_neon)
    uint8x16_t match = simd::neon::splat_u8(hash_val);
    uint8x16_t meta = simd::neon::load_u8(&__control_bytes[ind]);
    return __mask(static_cast<i32>(simd::neon::movemask_u8(simd::neon::eq(meta, match))));
#else
    i32 mask = 0;
    for ( usize i = 0; i < 16; ++i )
      if ( __control_bytes[ind + i] == hash_val ) mask |= (1 << i);
    return __mask(mask);
#endif
  }

  __mask
  __match_empty(usize ind) const
  {
#if defined(__micron_arch_x86_any)
    simd::i128 empty = simd::sse::splat_i8(static_cast<char>(__empty));
    simd::i128 meta = simd::sse::loadu_i128(reinterpret_cast<const __m128i_u *>(&__control_bytes[ind]));
    int mask = simd::sse::movemask_i8(simd::sse::eq_i8(empty, meta));
    return __mask(mask);
#elif defined(__micron_arm_neon)
    uint8x16_t empty = simd::neon::splat_u8(__empty);
    uint8x16_t meta = simd::neon::load_u8(&__control_bytes[ind]);
    return __mask(static_cast<i32>(simd::neon::movemask_u8(simd::neon::eq(empty, meta))));
#else
    i32 mask = 0;
    for ( usize i = 0; i < 16; ++i )
      if ( __control_bytes[ind + i] == __empty ) mask |= (1 << i);
    return __mask(mask);
#endif
  }

  __mask
  __match_empty_or_deleted(usize ind) const
  {
#if defined(__micron_arch_x86_any)
    simd::i128 meta = simd::sse::loadu_i128(reinterpret_cast<const __m128i_u *>(&__control_bytes[ind]));
    simd::i128 empty = simd::sse::splat_i8(static_cast<char>(__empty));
    simd::i128 deleted = simd::sse::splat_i8(static_cast<char>(__deleted));
    int mask = simd::sse::movemask_i8(simd::sse::or_i128(simd::sse::eq_i8(meta, empty), simd::sse::eq_i8(meta, deleted)));
    return __mask(mask);
#elif defined(__micron_arm_neon)
    uint8x16_t meta = simd::neon::load_u8(&__control_bytes[ind]);
    uint8x16_t empty = simd::neon::splat_u8(__empty);
    uint8x16_t deleted = simd::neon::splat_u8(__deleted);
    uint8x16_t combined = simd::neon::or_(simd::neon::eq(meta, empty), simd::neon::eq(meta, deleted));
    return __mask(static_cast<i32>(simd::neon::movemask_u8(combined)));
#else
    i32 mask = 0;
    for ( usize i = 0; i < 16; ++i ) {
      u8 c = __control_bytes[ind + i];
      if ( c == __empty || c == __deleted ) mask |= (1 << i);
    }
    return __mask(mask);
#endif
  }

  template<typename KK, typename VV>
  micron::pair<bool, V *>
  __insert_impl(KK &&key, VV &&value)
  {
    V *existing = find(key);
    if ( existing ) {
      return { false, existing };
    }

    if ( __size >= N ) {
      return { false, nullptr };
    }

    u8 h2 = __hash(key);
    usize start = __b_index(key);

    for ( usize i = 0; i < NH; i += 16 ) {
      usize group_start = (start + i) % N;

      if ( group_start + 16 <= N ) {
        __mask m = __match_empty_or_deleted(group_start);
        if ( m.any() ) {
          usize probe = group_start + m.lowest();
          __control_bytes[probe] = h2;
          __entries[probe] = __swiss_entry{ micron::forward<KK>(key), micron::forward<VV>(value) };
          ++__size;
          return { true, micron::addressof(__entries[probe].value) };
        }
      } else {
        for ( usize j = 0; j < 16 && i + j < NH; ++j ) {
          usize probe = (start + i + j) % N;
          if ( __control_bytes[probe] == __empty || __control_bytes[probe] == __deleted ) {
            __control_bytes[probe] = h2;
            __entries[probe] = __swiss_entry{ micron::forward<KK>(key), micron::forward<VV>(value) };
            ++__size;
            return { true, micron::addressof(__entries[probe].value) };
          }
        }
      }
    }

    return { false, nullptr };
  }

  template<typename VV>
    requires(micron::same_as<K, u64>)
  micron::pair<bool, V *>
  __load_impl(hash64_t key, VV &&value)
  {
    V *existing = exists(key);
    if ( existing ) {
      return { false, existing };
    }

    if ( __size >= N ) {
      return { false, nullptr };
    }

    u8 h2 = __h2(key);
    usize start = __b_index_key(key);

    for ( usize i = 0; i < NH; i += 16 ) {
      usize group_start = (start + i) % N;

      if ( group_start + 16 <= N ) {
        __mask m = __match_empty_or_deleted(group_start);
        if ( m.any() ) {
          usize probe = group_start + m.lowest();
          __control_bytes[probe] = h2;
          // NOTE: treat the hash as the key itself, obviously will only be valid for u64 keys
          __entries[probe] = __swiss_entry{ key, micron::forward<VV>(value) };
          ++__size;
          return { true, micron::addressof(__entries[probe].value) };
        }
      } else {
        for ( usize j = 0; j < 16 && i + j < NH; ++j ) {
          usize probe = (start + i + j) % N;
          if ( __control_bytes[probe] == __empty || __control_bytes[probe] == __deleted ) {
            __control_bytes[probe] = h2;
            __entries[probe] = __swiss_entry{ key, micron::forward<VV>(value) };
            ++__size;
            return { true, micron::addressof(__entries[probe].value) };
          }
        }
      }
    }

    return { false, nullptr };
  }

public:
  using category_type = map_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  using key_type = K;
  using mapped_type = V;
  using size_type = usize;

  struct __swiss_entry {
    K key;
    V value;

    __swiss_entry() : key{}, value{} { }

    __swiss_entry(const K &k, const V &v) : key(k), value(v) { }

    __swiss_entry(K &&k, V &&v) : key(micron::move(k)), value(micron::move(v)) { }
  };

  alignas(16) u8 __control_bytes[N];
  __swiss_entry __entries[N];
  usize __size = 0;

  ~stack_swiss_map() = default;

  stack_swiss_map() : __size(0)
  {
    for ( usize i = 0; i < N; ++i ) {
      __control_bytes[i] = __empty;
    }
  }

  stack_swiss_map(const stack_swiss_map &other) : __size(other.__size)
  {
    for ( usize i = 0; i < N; ++i ) {
      __control_bytes[i] = other.__control_bytes[i];
      if ( __control_bytes[i] != __empty && __control_bytes[i] != __deleted ) {
        __entries[i] = other.__entries[i];
      }
    }
  }

  stack_swiss_map(stack_swiss_map &&other) noexcept : __size(other.__size)
  {
    for ( usize i = 0; i < N; ++i ) {
      __control_bytes[i] = other.__control_bytes[i];
      if ( __control_bytes[i] != __empty && __control_bytes[i] != __deleted ) {
        __entries[i] = micron::move(other.__entries[i]);
      }
    }
    other.__size = 0;
    for ( usize i = 0; i < N; ++i ) {
      other.__control_bytes[i] = __empty;
    }
  }

  stack_swiss_map &
  operator=(const stack_swiss_map &other)
  {
    if ( this != &other ) {
      __size = other.__size;
      for ( usize i = 0; i < N; ++i ) {
        __control_bytes[i] = other.__control_bytes[i];
        if ( __control_bytes[i] != __empty && __control_bytes[i] != __deleted ) {
          __entries[i] = other.__entries[i];
        }
      }
    }
    return *this;
  }

  stack_swiss_map &
  operator=(stack_swiss_map &&other) noexcept
  {
    if ( this != &other ) {
      __size = other.__size;
      for ( usize i = 0; i < N; ++i ) {
        __control_bytes[i] = other.__control_bytes[i];
        if ( __control_bytes[i] != __empty && __control_bytes[i] != __deleted ) {
          __entries[i] = micron::move(other.__entries[i]);
        }
      }
      other.__size = 0;
      for ( usize i = 0; i < N; ++i ) {
        other.__control_bytes[i] = __empty;
      }
    }
    return *this;
  }

  usize
  size() const noexcept
  {
    return __size;
  }

  bool
  empty() const noexcept
  {
    return __size == 0;
  }

  constexpr usize
  max_size() const noexcept
  {
    return N;
  }

  constexpr usize
  capacity() const noexcept
  {
    return N;
  }

  void
  clear() noexcept
  {
    // __control_bytes is a plain u8[N]; the optimized byte-wise memset avoids
    // the strict-aliasing UB of writing through a u64* (illegal under -Ofast)
    // and is internally SIMD-accelerated
    micron::memset(&__control_bytes[0], __empty, N);
    __size = 0;
  }

  // prehashed
  // NOTE: external hash MUST match internal hash
  template<typename X = void>
    requires(micron::same_as<K, u64>)
  micron::pair<bool, V *>
  load(hash64_t key, const V &value)
  {
    return __load_impl(key, value);
  }

  template<typename X = void>
    requires(micron::same_as<K, u64>)
  micron::pair<bool, V *>
  load(hash64_t key, V &&value)
  {
    return __load_impl(key, micron::move(value));
  }

  template<typename X = void>
    requires(micron::same_as<K, u64>)
  V &
  load_find(hash64_t key)
  {
    auto result = load(key, V{});
    if ( !result.b ) {
      exc<except::library_error>("micron stack_swiss_map::operator[](): map is full");
    }
    return *result.b;
  }

  micron::pair<bool, V *>
  insert(const K &key, const V &value)
  {
    return __insert_impl(key, value);
  }

  micron::pair<bool, V *>
  insert(K &&key, V &&value)
  {
    return __insert_impl(micron::move(key), micron::move(value));
  }

  micron::pair<bool, V *>
  insert(const micron::pair<K, V> &kv)
  {
    return insert(kv.a, kv.b);
  }

  template<typename KK, typename VV>
  micron::pair<bool, V *>
  insert_or_assign(KK &&key, VV &&value)
  {
    u8 h2 = __hash(key);
    usize start = __b_index(key);

    // first pass: check for existing key and overwrite
    for ( usize i = 0; i < NH; i += 16 ) {
      usize group_start = (start + i) % N;

      if ( group_start + 16 <= N ) {
        __mask m = __match(h2, group_start);
        while ( m.any() ) {
          usize probe = group_start + m.lowest();
          if ( __entries[probe].key == key ) {
            __entries[probe].value = micron::forward<VV>(value);
            return { false, micron::addressof(__entries[probe].value) };
          }
          m.clear_lowest();
        }
        if ( __match_empty(group_start).any() ) break;
      } else {
        for ( usize j = 0; j < 16 && i + j < NH; ++j ) {
          usize probe = (start + i + j) % N;
          if ( __control_bytes[probe] == h2 && __entries[probe].key == key ) {
            __entries[probe].value = micron::forward<VV>(value);
            return { false, micron::addressof(__entries[probe].value) };
          }
          if ( __control_bytes[probe] == __empty ) goto insert;
        }
      }
    }

  insert:
    if ( __size >= N ) return { false, nullptr };

    // second pass: find first empty or deleted slot
    for ( usize i = 0; i < NH; i += 16 ) {
      usize group_start = (start + i) % N;

      if ( group_start + 16 <= N ) {
        __mask m = __match_empty_or_deleted(group_start);
        if ( m.any() ) {
          usize probe = group_start + m.lowest();
          __control_bytes[probe] = h2;
          __entries[probe] = __swiss_entry{ micron::forward<KK>(key), micron::forward<VV>(value) };
          ++__size;
          return { true, micron::addressof(__entries[probe].value) };
        }
      } else {
        for ( usize j = 0; j < 16 && i + j < NH; ++j ) {
          usize probe = (start + i + j) % N;
          if ( __control_bytes[probe] == __empty || __control_bytes[probe] == __deleted ) {
            __control_bytes[probe] = h2;
            __entries[probe] = __swiss_entry{ micron::forward<KK>(key), micron::forward<VV>(value) };
            ++__size;
            return { true, micron::addressof(__entries[probe].value) };
          }
        }
      }
    }

    return { false, nullptr };
  }

  template<typename... Args>
  micron::pair<bool, V *>
  emplace(const K &key, Args &&...args)
  {
    V *existing = find(key);
    if ( existing ) {
      return { false, existing };
    }

    if ( __size >= N ) {
      return { false, nullptr };
    }

    u8 h2 = __hash(key);
    usize start = __b_index(key);

    for ( usize i = 0; i < NH; i += 16 ) {
      usize group_start = (start + i) % N;

      if ( group_start + 16 <= N ) {
        __mask m = __match_empty_or_deleted(group_start);
        if ( m.any() ) {
          usize probe = group_start + m.lowest();
          __control_bytes[probe] = h2;
          __entries[probe].key = key;
          new (micron::addr(__entries[probe].value)) V(micron::forward<Args>(args)...);
          ++__size;
          return { true, micron::addressof(__entries[probe].value) };
        }
      } else {
        for ( usize j = 0; j < 16 && i + j < NH; ++j ) {
          usize probe = (start + i + j) % N;
          if ( __control_bytes[probe] == __empty || __control_bytes[probe] == __deleted ) {
            __control_bytes[probe] = h2;
            __entries[probe].key = key;
            new (micron::addr(__entries[probe].value)) V(micron::forward<Args>(args)...);
            ++__size;
            return { true, micron::addressof(__entries[probe].value) };
          }
        }
      }
    }

    return { false, nullptr };
  }

  bool
  erase(const K &key)
  {
    u8 h2 = __hash(key);
    usize start = __b_index(key);

    for ( usize i = 0; i < NH; i += 16 ) {
      usize group_start = (start + i) % N;

      if ( group_start + 16 <= N ) {
        __mask m = __match(h2, group_start);
        while ( m.any() ) {
          usize probe = group_start + m.lowest();
          if ( __entries[probe].key == key ) {
            __control_bytes[probe] = __deleted;
            --__size;
            return true;
          }
          m.clear_lowest();
        }
        if ( __match_empty(group_start).any() ) return false;      // first empty -> key absent (see find)
      } else {
        for ( usize j = 0; j < 16 && i + j < NH; ++j ) {
          usize probe = (start + i + j) % N;
          if ( __control_bytes[probe] == h2 && __entries[probe].key == key ) {
            __control_bytes[probe] = __deleted;
            --__size;
            return true;
          }
          if ( __control_bytes[probe] == __empty ) return false;
        }
      }
    }

    return false;
  }

  V *
  find(const K &key)
  {
    u8 h2 = __hash(key);
    usize start = __b_index(key);

    for ( usize i = 0; i < NH; i += 16 ) {
      usize group_start = (start + i) % N;

      if ( group_start + 16 <= N ) {
        __mask m = __match(h2, group_start);
        while ( m.any() ) {
          usize probe = group_start + m.lowest();
          if ( __entries[probe].key == key ) {
            return micron::addressof(__entries[probe].value);
          }
          m.clear_lowest();
        }
        // a key is always placed at/ before the first never-used EMPTY slot in its
        // probe sequence, so an empty group means the key is absent -> stop here
        if ( __match_empty(group_start).any() ) return nullptr;
      } else {
        for ( usize j = 0; j < 16 && i + j < NH; ++j ) {
          usize probe = (start + i + j) % N;
          if ( __control_bytes[probe] == h2 && __entries[probe].key == key ) {
            return micron::addressof(__entries[probe].value);
          }
          if ( __control_bytes[probe] == __empty ) return nullptr;
        }
      }
    }

    return nullptr;
  }

  const V *
  find(const K &key) const
  {
    return const_cast<stack_swiss_map *>(this)->find(key);
  }

  template<typename X = void>
    requires(micron::same_as<K, u64>)
  V *
  exists(const hash64_t key)
  {
    u8 h2 = __h2(key);
    usize start = __b_index_key(key);

    for ( usize i = 0; i < NH; i += 16 ) {
      usize group_start = (start + i) % N;

      if ( group_start + 16 <= N ) {
        __mask m = __match(h2, group_start);
        while ( m.any() ) {
          usize probe = group_start + m.lowest();
          if ( __entries[probe].key == key ) {
            return micron::addressof(__entries[probe].value);
          }
          m.clear_lowest();
        }
        // a key is always placed at/ before the first never-used EMPTY slot in its
        // probe sequence, so an empty group means the key is absent -> stop here
        if ( __match_empty(group_start).any() ) return nullptr;
      } else {
        for ( usize j = 0; j < 16 && i + j < NH; ++j ) {
          usize probe = (start + i + j) % N;
          if ( __control_bytes[probe] == h2 && __entries[probe].key == key ) {
            return micron::addressof(__entries[probe].value);
          }
          if ( __control_bytes[probe] == __empty ) return nullptr;
        }
      }
    }

    return nullptr;
  }

  template<typename X = void>
    requires(micron::same_as<K, u64>)
  const V *
  exists(const hash64_t &key) const
  {
    return const_cast<stack_swiss_map *>(this)->exists(key);
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

  // impossible to disambiguate these, prefer load_find
  V &
  operator[](const K &key)
  {
    auto result = insert(key, V{});
    if ( !result.b ) {
      exc<except::library_error>("micron stack_swiss_map::operator[](): map is full");
    }
    return *result.b;
  }

  V &
  at(const K &key)
  {
    V *v = find(key);
    if ( !v ) {
      exc<except::library_error>("micron stack_swiss_map::at(): key not found");
    }
    return *v;
  }

  const V &
  at(const K &key) const
  {
    const V *v = find(key);
    if ( !v ) {
      exc<except::library_error>("micron stack_swiss_map::at(): key not found");
    }
    return *v;
  }

  class iterator
  {
  private:
    stack_swiss_map *map_;
    usize index_;

    void
    advance()
    {
      while ( index_ < N && (map_->__control_bytes[index_] == __empty || map_->__control_bytes[index_] == __deleted) ) {
        ++index_;
      }
    }

  public:
    using value_type = micron::pair<const K &, V &>;
    using difference_type = ptrdiff_t;
    using pointer = micron::pair<const K *, V *>;
    using reference = micron::pair<const K &, V &>;

    iterator(stack_swiss_map *m, usize idx) : map_(m), index_(idx) { advance(); }

    reference
    operator*()
    {
      return { map_->__entries[index_].key, map_->__entries[index_].value };
    }

    pointer
    operator->()
    {
      return { micron::addressof(map_->__entries[index_].key), micron::addressof(map_->__entries[index_].value) };
    }

    iterator &
    operator++()
    {
      ++index_;
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
    operator==(const iterator &other) const
    {
      return map_ == other.map_ && index_ == other.index_;
    }

    bool
    operator!=(const iterator &other) const
    {
      return !(*this == other);
    }
  };

  class const_iterator
  {
  private:
    const stack_swiss_map *map_;
    usize index_;

    void
    advance()
    {
      while ( index_ < N && (map_->__control_bytes[index_] == __empty || map_->__control_bytes[index_] == __deleted) ) {
        ++index_;
      }
    }

  public:
    using value_type = micron::pair<const K &, const V &>;
    using difference_type = ptrdiff_t;
    using pointer = micron::pair<const K *, const V *>;
    using reference = micron::pair<const K &, const V &>;

    const_iterator(const stack_swiss_map *m, usize idx) : map_(m), index_(idx) { advance(); }

    reference
    operator*() const
    {
      return { map_->__entries[index_].key, map_->__entries[index_].value };
    }

    pointer
    operator->() const
    {
      return { micron::addressof(map_->__entries[index_].key), micron::addressof(map_->__entries[index_].value) };
    }

    const_iterator &
    operator++()
    {
      ++index_;
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
    operator==(const const_iterator &other) const
    {
      return map_ == other.map_ && index_ == other.index_;
    }

    bool
    operator!=(const const_iterator &other) const
    {
      return !(*this == other);
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
    return iterator(this, N);
  }

  const_iterator
  begin() const
  {
    return const_iterator(this, 0);
  }

  const_iterator
  end() const
  {
    return const_iterator(this, N);
  }

  const_iterator
  cbegin() const
  {
    return const_iterator(this, 0);
  }

  const_iterator
  cend() const
  {
    return const_iterator(this, N);
  }

  template<typename Fn>
    requires micron::is_invocable_v<Fn, const K &, V &>
  __attribute__((always_inline)) void
  for_each(Fn &&fn)
  {
    for ( usize i = 0; i < N; ++i )
      if ( __control_bytes[i] != __empty && __control_bytes[i] != __deleted ) fn(__entries[i].key, __entries[i].value);
  }

  template<typename Fn>
    requires micron::is_invocable_v<Fn, const K &, const V &>
  __attribute__((always_inline)) void
  for_each(Fn &&fn) const
  {
    for ( usize i = 0; i < N; ++i )
      if ( __control_bytes[i] != __empty && __control_bytes[i] != __deleted ) fn(__entries[i].key, __entries[i].value);
  }
};

template<typename K, typename V, usize N, usize NH = 16> using swiss = stack_swiss_map<K, V, N, NH>;
}      // namespace micron
