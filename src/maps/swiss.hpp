#pragma once
#include "../bits.hpp"
#include "../simd/types.hpp"
#include "../types.hpp"
#include "hash/hash.hpp"

#include "../memory/actions.hpp"
#include "../tuple.hpp"

namespace micron
{
constexpr u8 __empty = 0b11111111;        // 0xFF - empty slot
constexpr u8 __deleted = 0b11111110;      // 0xFE - tombstone
constexpr u8 __sentinel = 0b10000000;     // 0x80 - high bit set for occupied

struct __mask {
  i32 bits;

  explicit __mask(i32 b) : bits(b) {}

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

template <typename K, typename V, size_t N, size_t NH = 16>
  requires(N >= 16 and (N % 16) == 0 and NH <= N)
class stack_swiss_map
{
  static constexpr u8
  __h2(hash64_t h)
  {
    return static_cast<u8>((h >> 57) | __sentinel);
  }

  static constexpr u8
  __hash(const K &k)
  {
    return __h2(hash<hash64_t>(k));
  }

  static constexpr size_t
  __h1(hash64_t h)
  {
    return static_cast<size_t>(h);
  }

  static constexpr size_t
  __b_index(const K &k)
  {
    return __h1(hash<hash64_t>(k)) % N;
  }

  __mask
  __match(u8 hash_val, size_t ind) const
  {
    simd::i128 match = _mm_set1_epi8(static_cast<i8>(hash_val));
    simd::i128 meta = _mm_load_si128(reinterpret_cast<const simd::i128 *>(&__control_bytes[ind]));
    int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(match, meta));
    return __mask(mask);
  }

  __mask
  __match_empty(size_t ind) const
  {
    simd::i128 empty = _mm_set1_epi8(static_cast<i8>(__empty));
    simd::i128 meta = _mm_load_si128(reinterpret_cast<const simd::i128 *>(&__control_bytes[ind]));
    int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(empty, meta));
    return __mask(mask);
  }

  __mask
  __match_empty_or_deleted(size_t ind) const
  {
    simd::i128 meta = _mm_load_si128(reinterpret_cast<const simd::i128 *>(&__control_bytes[ind]));
    simd::i128 sentinel = _mm_set1_epi8(static_cast<i8>(__sentinel));
    int mask = _mm_movemask_epi8(_mm_cmpgt_epi8(meta, sentinel) | _mm_cmpeq_epi8(meta, _mm_set1_epi8(static_cast<i8>(__deleted))));
    return __mask(mask);
  }

  template <typename KK, typename VV>
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
    size_t start = __b_index(key);

    for ( size_t i = 0; i < NH; i += 16 ) {
      size_t group_start = (start + i) % N;

      if ( group_start + 16 <= N ) {
        __mask m = __match_empty_or_deleted(group_start);
        if ( m.any() ) {
          size_t probe = group_start + m.lowest();
          __control_bytes[probe] = h2;
          __entries[probe] = __swiss_entry{ micron::forward<KK>(key), micron::forward<VV>(value) };
          ++__size;
          return { true, &__entries[probe].value };
        }
      } else {
        for ( size_t j = 0; j < 16 && i + j < NH; ++j ) {
          size_t probe = (start + i + j) % N;
          if ( __control_bytes[probe] == __empty || __control_bytes[probe] == __deleted ) {
            __control_bytes[probe] = h2;
            __entries[probe] = __swiss_entry{ micron::forward<KK>(key), micron::forward<VV>(value) };
            ++__size;
            return { true, &__entries[probe].value };
          }
        }
      }
    }

    return { false, nullptr };
  }

public:
  struct __swiss_entry {
    K key;
    V value;

    __swiss_entry() : key{}, value{} {}
    __swiss_entry(const K &k, const V &v) : key(k), value(v) {}
    __swiss_entry(K &&k, V &&v) : key(micron::move(k)), value(micron::move(v)) {}
  };

  alignas(16) u8 __control_bytes[N];
  __swiss_entry __entries[N];
  size_t __size = 0;

  ~stack_swiss_map() = default;
  stack_swiss_map() : __size(0)
  {
    for ( size_t i = 0; i < N; ++i ) {
      __control_bytes[i] = __empty;
    }
  }

  stack_swiss_map(const stack_swiss_map &other) : __size(other.__size)
  {
    for ( size_t i = 0; i < N; ++i ) {
      __control_bytes[i] = other.__control_bytes[i];
      if ( __control_bytes[i] != __empty && __control_bytes[i] != __deleted ) {
        __entries[i] = other.__entries[i];
      }
    }
  }

  stack_swiss_map(stack_swiss_map &&other) noexcept : __size(other.__size)
  {
    for ( size_t i = 0; i < N; ++i ) {
      __control_bytes[i] = other.__control_bytes[i];
      if ( __control_bytes[i] != __empty && __control_bytes[i] != __deleted ) {
        __entries[i] = micron::move(other.__entries[i]);
      }
    }
    other.__size = 0;
    for ( size_t i = 0; i < N; ++i ) {
      other.__control_bytes[i] = __empty;
    }
  }

  stack_swiss_map &
  operator=(const stack_swiss_map &other)
  {
    if ( this != &other ) {
      __size = other.__size;
      for ( size_t i = 0; i < N; ++i ) {
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
      for ( size_t i = 0; i < N; ++i ) {
        __control_bytes[i] = other.__control_bytes[i];
        if ( __control_bytes[i] != __empty && __control_bytes[i] != __deleted ) {
          __entries[i] = micron::move(other.__entries[i]);
        }
      }
      other.__size = 0;
      for ( size_t i = 0; i < N; ++i ) {
        other.__control_bytes[i] = __empty;
      }
    }
    return *this;
  }

  size_t
  size() const noexcept
  {
    return __size;
  }
  bool
  empty() const noexcept
  {
    return __size == 0;
  }
  constexpr size_t
  max_size() const noexcept
  {
    return N;
  }
  constexpr size_t
  capacity() const noexcept
  {
    return N;
  }

  void
  clear() noexcept
  {
    for ( size_t i = 0; i < N; ++i ) {
      __control_bytes[i] = __empty;
    }
    __size = 0;
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
    return insert(kv.first, kv.second);
  }

  template <typename... Args>
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
    size_t start = __b_index(key);

    for ( size_t i = 0; i < NH; i += 16 ) {
      size_t group_start = (start + i) % N;

      if ( group_start + 16 <= N ) {
        __mask m = __match_empty_or_deleted(group_start);
        if ( m.any() ) {
          size_t probe = group_start + m.lowest();
          __control_bytes[probe] = h2;
          __entries[probe].key = key;
          new (&__entries[probe].value) V(micron::forward<Args>(args)...);
          ++__size;
          return { true, &__entries[probe].value };
        }
      } else {
        for ( size_t j = 0; j < 16 && i + j < NH; ++j ) {
          size_t probe = (start + i + j) % N;
          if ( __control_bytes[probe] == __empty || __control_bytes[probe] == __deleted ) {
            __control_bytes[probe] = h2;
            __entries[probe].key = key;
            new (&__entries[probe].value) V(micron::forward<Args>(args)...);
            ++__size;
            return { true, &__entries[probe].value };
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
    size_t start = __b_index(key);

    for ( size_t i = 0; i < NH; i += 16 ) {
      size_t group_start = (start + i) % N;

      if ( group_start + 16 <= N ) {
        __mask m = __match(h2, group_start);
        while ( m.any() ) {
          size_t probe = group_start + m.lowest();
          if ( __entries[probe].key == key ) {
            __control_bytes[probe] = __deleted;
            --__size;
            return true;
          }
          m.clear_lowest();
        }
      } else {
        for ( size_t j = 0; j < 16 && i + j < NH; ++j ) {
          size_t probe = (start + i + j) % N;
          if ( __control_bytes[probe] == h2 && __entries[probe].key == key ) {
            __control_bytes[probe] = __deleted;
            --__size;
            return true;
          }
        }
      }
    }

    return false;
  }

  V *
  find(const K &key)
  {
    u8 h2 = __hash(key);
    size_t start = __b_index(key);

    for ( size_t i = 0; i < NH; i += 16 ) {
      size_t group_start = (start + i) % N;

      if ( group_start + 16 <= N ) {
        __mask m = __match(h2, group_start);
        while ( m.any() ) {
          size_t probe = group_start + m.lowest();
          if ( __entries[probe].key == key ) {
            return &__entries[probe].value;
          }
          m.clear_lowest();
        }
      } else {
        for ( size_t j = 0; j < 16 && i + j < NH; ++j ) {
          size_t probe = (start + i + j) % N;
          if ( __control_bytes[probe] == h2 && __entries[probe].key == key ) {
            return &__entries[probe].value;
          }
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

  bool
  contains(const K &key) const
  {
    return find(key) != nullptr;
  }

  size_t
  count(const K &key) const
  {
    return contains(key) ? 1 : 0;
  }

  V &
  operator[](const K &key)
  {
    V *v = find(key);
    if ( v ) {
      return *v;
    }

    auto result = insert(key, V{});
    if ( !result.first ) {
      exc<except::library_error>("micron stack_swiss_map::operator[](): map is full");
    }
    return *result.second;
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
    size_t index_;

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

    iterator(stack_swiss_map *m, size_t idx) : map_(m), index_(idx) { advance(); }

    reference
    operator*()
    {
      return { map_->__entries[index_].key, map_->__entries[index_].value };
    }

    pointer
    operator->()
    {
      return { &map_->__entries[index_].key, &map_->__entries[index_].value };
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
    size_t index_;

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

    const_iterator(const stack_swiss_map *m, size_t idx) : map_(m), index_(idx) { advance(); }

    reference
    operator*() const
    {
      return { map_->__entries[index_].key, map_->__entries[index_].value };
    }

    pointer
    operator->() const
    {
      return { &map_->__entries[index_].key, &map_->__entries[index_].value };
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
};

}     // namespace micron
