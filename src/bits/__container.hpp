//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../type_traits.hpp"

#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../memory/cmemory.hpp"
#include "../memory/placement_new.hpp"
#include "../simd/strings.hpp"

namespace micron
{

namespace __impl
{
inline constexpr usize
grow(usize cap) noexcept
{
  return cap == 0 ? 8 : cap * 2;
}
};      // namespace __impl

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shared container bounds/state predicates
// NOTE: this must remain an empty base
template<typename Derived, typename T, bool Sf> class __container_checks
{
protected:
  [[gnu::always_inline]] inline const Derived *
  __self(void) const noexcept
  {
    return static_cast<const Derived *>(this);
  }

  // enforce this as noinline, saves I-cache and binary size
  template<typename E>
  [[gnu::noinline, gnu::cold, noreturn]] static void
  __fail(const char *msg)
  {
    exc<E>(msg);
  }

  [[gnu::always_inline]] inline bool
  __empty_check(void) const
  {
    return __self()->__c_len() == 0 || __self()->__c_data() == nullptr;
  }

  [[gnu::always_inline]] inline bool
  __index_check(usize n) const
  {
    return n >= __self()->__c_len();
  }

  [[gnu::always_inline]] inline bool
  __capacity_check(usize n) const
  {
    return n >= __self()->__c_cap();
  }

  // alias of __index_check
  [[gnu::always_inline]] inline bool
  __get_check(usize n) const
  {
    return n >= __self()->__c_len();
  }

  [[gnu::always_inline]] inline bool
  __iterator_check(const T *it) const
  {
    return it < __self()->__c_data() || it > __self()->__c_data() + __self()->__c_len();
  }

  [[gnu::always_inline]] inline bool
  __range_check(usize from, usize to) const
  {
    return from >= to || to > __self()->__c_len();
  }

  // deliberately over capacity, not length
  [[gnu::always_inline]] inline bool
  __cap_range_check(usize from, usize to) const
  {
    return from >= to || to > __self()->__c_cap();
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shared heap vector core
//
// NOTE: this must remain an empty base
template<typename Derived, typename T> class __vector_core
{
protected:
  [[gnu::always_inline]] inline Derived *
  __vself(void) noexcept
  {
    return static_cast<Derived *>(this);
  }

  [[gnu::always_inline]] inline const Derived *
  __vself(void) const noexcept
  {
    return static_cast<const Derived *>(this);
  }

  // must be noinline
  [[gnu::noinline, gnu::cold]] void
  __grow_one(void)
  {
    Derived *s = __vself();
    if ( s->capacity == static_cast<usize>(-1) ) exc<except::length_error>("container: capacity overflow");
    s->__core_reserve(s->recommended_capacity(s->capacity, s->capacity + 1));
  }

  [[gnu::noinline, gnu::cold]] void
  __grow_for(usize extra)
  {
    Derived *s = __vself();
    if ( extra > static_cast<usize>(-1) - s->length ) exc<except::length_error>("container: size overflow");
    const usize minimum = s->length + extra;
    s->__core_reserve(s->recommended_capacity(s->capacity, minimum));
  }

  template<typename U>
  [[gnu::always_inline]] inline void
  __emplace_at(usize i, U &&v)
  {
    Derived *s = __vself();
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> )
      new (addr(s->memory[i])) T(micron::forward<U>(v));
    else
      s->memory[i] = micron::forward<U>(v);
  }

public:
  [[gnu::always_inline]] inline void
  push_back(const T &v)
  {
    Derived *s = __vself();
    if ( s->length < s->capacity ) [[likely]] {
      __emplace_at(s->length++, v);
      return;
    }
    __grow_one();
    __emplace_at(s->length++, v);
  }

  [[gnu::always_inline]] inline void
  push_back(T &&v)
  {
    Derived *s = __vself();
    if ( s->length < s->capacity ) [[likely]] {
      __emplace_at(s->length++, micron::move(v));
      return;
    }
    __grow_one();
    __emplace_at(s->length++, micron::move(v));
  }

  template<typename... Args>
  [[gnu::always_inline]] inline void
  emplace_back(Args &&...v)
  {
    Derived *s = __vself();
    if ( s->length < s->capacity ) [[likely]] {
      new (addr(s->memory[s->length++])) T(micron::forward<Args>(v)...);
      return;
    }
    __grow_one();
    new (addr(s->memory[s->length++])) T(micron::forward<Args>(v)...);
  }
};

namespace __impl_container
{

template<typename T>
inline void
shallow_copy(T *__restrict dest, const T *__restrict src, usize cnt) noexcept
{
  micron::memcpy(reinterpret_cast<byte *>(dest), reinterpret_cast<const byte *>(src),
                 cnt * (sizeof(T)));      // always is page aligned, 256 is
                                          // fine, just realign back to bytes
};

template<typename T>
inline void
shallow_copy(T *__restrict dest, T *__restrict src, usize cnt) noexcept
{
  micron::memcpy(reinterpret_cast<byte *>(dest), reinterpret_cast<byte *>(src),
                 cnt * (sizeof(T)));      // always is page aligned, 256 is
                                          // fine, just realign back to bytes
};

// deep copy routine, nec. if obj. has const/dest (can be ignored but WILL
// cause segfaulting if underlying doesn't account for double deletes)
template<typename T>
inline void
deep_copy(T *__restrict dest, T *__restrict src, usize cnt)
{
  // guard against copying into uninit memory
  if constexpr ( noexcept(T(micron::declval<const T &>())) ) {
    for ( usize i = 0; i < cnt; ++i ) new (addr(dest[i])) T(src[i]);
  } else {
    usize i = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      for ( ; i < cnt; ++i ) new (addr(dest[i])) T(src[i]);
    } catch ( ... ) {
      for ( usize j = 0; j < i; ++j ) dest[j].~T();
      throw;
    }
#else
    for ( ; i < cnt; ++i ) new (addr(dest[i])) T(src[i]);
#endif
  }
};

template<typename T>
inline void
deep_copy(T *__restrict dest, const T *__restrict src, usize cnt)
{
  if constexpr ( noexcept(T(micron::declval<const T &>())) ) {
    for ( usize i = 0; i < cnt; ++i ) new (addr(dest[i])) T(src[i]);
  } else {
    usize i = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      for ( ; i < cnt; ++i ) new (addr(dest[i])) T(src[i]);
    } catch ( ... ) {
      for ( usize j = 0; j < i; ++j ) dest[j].~T();
      throw;
    }
#else
    for ( ; i < cnt; ++i ) new (addr(dest[i])) T(src[i]);
#endif
  }
};

template<typename T>
inline void
deep_copy_assign(T *__restrict dest, T *__restrict src, usize cnt)
{
  // guard against copying into uninit memory
  for ( usize i = 0; i < cnt; ++i ) dest[i] = src[i];
};

template<typename T>
inline void
deep_copy_assign(T *__restrict dest, const T *__restrict src, usize cnt)
{
  for ( usize i = 0; i < cnt; ++i ) dest[i] = src[i];
};

template<typename T>
inline void
shallow_move(T *__restrict dest, T *__restrict src, usize cnt)
{
  micron::memcpy(dest, src, cnt);
  // micron::byteset(reinterpret_cast<byte *>(src), 0x0, cnt * sizeof(T));
};

template<typename T>
inline void
deep_move(T *__restrict dest, T *__restrict src, usize cnt)
{
  if constexpr ( noexcept(T(micron::move(micron::declval<T &>()))) ) {
    for ( usize i = 0; i < cnt; ++i ) {
      new (addr(dest[i])) T(micron::move(src[i]));
      src[i].~T();
      // WARNING: P1144
    }
  } else {
    // WARNING: if move ctor throws at i,/ already-constructed dest[0..i-1] are destroyed; rethrow exception
    usize i = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      for ( ; i < cnt; ++i ) {
        new (addr(dest[i])) T(micron::move(src[i]));
        src[i].~T();
      }
    } catch ( ... ) {
      for ( usize j = 0; j < i; ++j ) dest[j].~T();
      throw;
    }
#else
    for ( ; i < cnt; ++i ) {
      new (addr(dest[i])) T(micron::move(src[i]));
      src[i].~T();
    }
#endif
  }
};

template<typename T>
inline void
deep_move_assign(T *__restrict dest, T *__restrict src, usize cnt)
{
  for ( usize i = 0; i < cnt; ++i ) dest[i] = micron::move(src[i]);
};

template<usize N, typename T>
inline void
deep_copy(T *__restrict dest, T *__restrict src)
{
  if constexpr ( noexcept(T(micron::declval<const T &>())) ) {
    for ( usize i = 0; i < N; ++i ) new (addr(dest[i])) T(src[i]);
  } else {
    usize i = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      for ( ; i < N; ++i ) new (addr(dest[i])) T(src[i]);
    } catch ( ... ) {
      for ( usize j = 0; j < i; ++j ) dest[j].~T();
      throw;
    }
#else
    for ( ; i < N; ++i ) new (addr(dest[i])) T(src[i]);
#endif
  }
};

template<usize N, typename T>
inline void
deep_copy(T *__restrict dest, const T *__restrict src)
{
  if constexpr ( noexcept(T(micron::declval<const T &>())) ) {
    for ( usize i = 0; i < N; ++i ) new (addr(dest[i])) T(src[i]);
  } else {
    usize i = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      for ( ; i < N; ++i ) new (addr(dest[i])) T(src[i]);
    } catch ( ... ) {
      for ( usize j = 0; j < i; ++j ) dest[j].~T();
      throw;
    }
#else
    for ( ; i < N; ++i ) new (addr(dest[i])) T(src[i]);
#endif
  }
};

template<usize N, typename T>
inline void
deep_copy_assign(T *__restrict dest, T *__restrict src)
{
  for ( usize i = 0; i < N; ++i ) dest[i] = src[i];
};

template<usize N, typename T>
inline void
deep_copy_assign(T *__restrict dest, const T *__restrict src)
{
  for ( usize i = 0; i < N; ++i ) dest[i] = src[i];
};

template<usize N, typename T>
inline void
shallow_move(T *__restrict dest, T *__restrict src)
{
  micron::cmemcpy<N, T, T>(dest, src);
  // micron::cbyteset<N * sizeof(T)>(reinterpret_cast<byte *>(src), 0x0);
};

template<usize N, typename T>
inline void
deep_move(T *__restrict dest, T *__restrict src)
{
  if constexpr ( noexcept(T(micron::move(micron::declval<T &>()))) ) {
    for ( usize i = 0; i < N; ++i ) {
      new (addr(dest[i])) T(micron::move(src[i]));
      src[i].~T();
      // WARNING: P1144
    }
  } else {
    usize i = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      for ( ; i < N; ++i ) {
        new (addr(dest[i])) T(micron::move(src[i]));
        src[i].~T();
      }
    } catch ( ... ) {
      for ( usize j = 0; j < i; ++j ) dest[j].~T();
      throw;
    }
#else
    for ( ; i < N; ++i ) {
      new (addr(dest[i])) T(micron::move(src[i]));
      src[i].~T();
    }
#endif
  }
};

// move-CONSTRUCT into raw storage WITHOUT ending the source's lifetime; NEEDED
template<usize N, typename T>
inline void
deep_move_init(T *__restrict dest, T *__restrict src)
{
  if constexpr ( noexcept(T(micron::move(micron::declval<T &>()))) ) {
    for ( usize i = 0; i < N; ++i ) new (addr(dest[i])) T(micron::move(src[i]));
  } else {
    usize i = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      for ( ; i < N; ++i ) new (addr(dest[i])) T(micron::move(src[i]));
    } catch ( ... ) {
      for ( usize j = 0; j < i; ++j ) dest[j].~T();
      throw;
    }
#else
    for ( ; i < N; ++i ) new (addr(dest[i])) T(micron::move(src[i]));
#endif
  }
};

template<usize N, typename T>
inline void
deep_move_assign(T *__restrict dest, T *__restrict src)
{
  for ( usize i = 0; i < N; ++i ) dest[i] = micron::move(src[i]);
};

template<usize N, typename T>
inline void
shallow_copy(T *__restrict dest, const T *__restrict src) noexcept
{
  micron::cmemcpy<N * sizeof(T)>(reinterpret_cast<byte *>(dest),
                                 reinterpret_cast<const byte *>(src));      // always is page aligned, 256 is
                                                                            // fine, just realign back to bytes
};

template<usize N, typename T>
inline void
shallow_copy(T *__restrict dest, T *__restrict src) noexcept
{
  micron::cmemcpy<N * sizeof(T)>(reinterpret_cast<byte *>(dest),
                                 reinterpret_cast<const byte *>(src));      // always is page aligned, 256 is
                                                                            // fine, just realign back to bytes
};

template<typename T>
inline void
copy(T *__restrict dest, T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy(dest, src, cnt);
  } else {
    shallow_copy(dest, src, cnt);
  }
}

template<typename T>
inline void
copy(T *__restrict dest, const T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy(dest, src, cnt);
  } else {
    shallow_copy(dest, src, cnt);
  }
}

template<typename T>
inline void
copy_assign(T *__restrict dest, T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy_assign(dest, src, cnt);
  } else {
    shallow_copy(dest, src, cnt);
  }
}

template<typename T>
inline void
copy_assign(T *__restrict dest, const T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy_assign(dest, src, cnt);
  } else {
    shallow_copy(dest, src, cnt);
  }
}

template<typename T>
inline void
move_assign(T *__restrict dest, T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_move_assignable_v<micron::remove_cv_t<T>> ) {
    deep_move_assign(dest, src, cnt);
  } else {
    shallow_move(dest, src, cnt);
  }
}

template<typename T>
inline void
move(T *__restrict dest, T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_move(dest, src, cnt);
  } else {
    shallow_move(dest, src, cnt);
  }
}

template<usize N, typename T>
inline void
copy(T *__restrict dest, T *__restrict src)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy<N, T>(dest, src);
  } else {
    shallow_copy<N, T>(dest, src);
  }
}

template<usize N, typename T>
inline void
copy(T *__restrict dest, const T *__restrict src)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy<N, T>(dest, src);
  } else {
    shallow_copy<N, T>(dest, src);
  }
}

template<usize N, typename T>
inline void
copy_assign(T *__restrict dest, T *__restrict src)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy_assign<N, T>(dest, src);
  } else {
    shallow_copy<N, T>(dest, src);
  }
}

template<usize N, typename T>
inline void
copy_assign(T *__restrict dest, const T *__restrict src)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy_assign<N, T>(dest, src);
  } else {
    shallow_copy<N, T>(dest, src);
  }
}

template<usize N, typename T>
inline void
move_assign(T *__restrict dest, T *__restrict src)
{
  if constexpr ( !micron::is_trivially_move_assignable_v<micron::remove_cv_t<T>> ) {
    deep_move_assign<N, T>(dest, src);
  } else {
    shallow_move<N, T>(dest, src);
  }
}

template<usize N, typename T>
inline void
move(T *__restrict dest, T *__restrict src)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_move<N, T>(dest, src);
  } else {
    shallow_move<N, T>(dest, src);
  }
}

// non-destroying move-construct; NEEDED
template<usize N, typename T>
inline void
move_init(T *__restrict dest, T *__restrict src)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_move_init<N, T>(dest, src);
  } else {
    shallow_move<N, T>(dest, src);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// linear scan
template<typename T>
[[gnu::always_inline]] inline usize
find_index(const T *p, usize len, const T &v) noexcept
{
  using U = micron::remove_cv_t<T>;
  if constexpr ( (micron::is_integral_v<U> or micron::is_enum_v<U> or micron::is_pointer_v<U>)
                 and (sizeof(U) == 1 or sizeof(U) == 2 or sizeof(U) == 4 or sizeof(U) == 8) ) {
    return micron::simd::find_first_elem(p, len, v);
  } else {
    for ( usize i = 0; i < len; ++i )
      if ( p[i] == v ) return i;
    return len;
  }
}

template<typename T>
inline void
open_gap(T *base, usize len, usize p, usize cnt)
{
  if ( cnt == 0 ) return;
  if constexpr ( micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    micron::memmove(base + p + cnt, base + p, len - p);
  } else {
    const usize n = len - p;
    for ( usize i = n; i-- > 0; ) {
      if ( p + cnt + i >= len )
        new (addr(base[p + cnt + i])) T(micron::move(base[p + i]));
      else
        base[p + cnt + i] = micron::move(base[p + i]);
    }
    const usize raw = (cnt < n) ? cnt : n;
    for ( usize i = 0; i < raw; ++i ) base[p + i].~T();
  }
}

// NOTE: assumes T's move-ctor does not throw during the tail slide
template<typename T, typename Fn>
inline void
fill_gap(T *base, usize length, usize p, usize cnt, Fn &&fn)
{
  if ( cnt == 0 ) return;
#if !defined(__micron_freestanding) || defined(__micron_eh)
  usize k = 0;
  try {
    for ( ; k < cnt; ++k ) fn(p + k);
  } catch ( ... ) {
    for ( usize j = 0; j < k; ++j ) base[p + j].~T();
    const usize tail = length - p;
    for ( usize j = 0; j < tail; ++j ) {
      new (addr(base[p + j])) T(micron::move(base[p + cnt + j]));
      base[p + cnt + j].~T();
    }
    throw;
  }
#else
  // surpress may be unused
  (void)base;
  (void)length;
  for ( usize k = 0; k < cnt; ++k ) fn(p + k);
#endif
}

template<typename T>
inline void
fill_gap_copy(T *base, usize length, usize p, usize cnt, const T &val)
{
  if constexpr ( micron::is_trivially_copyable_v<micron::remove_cv_t<T>> and micron::is_trivially_constructible_v<T, const T &> ) {
    if ( cnt == 0 ) return;
    micron::typeset(base + p, val, cnt);
    return;
  } else {
    fill_gap(base, length, p, cnt, [&](usize i) { new (addr(base[i])) T(val); });
  }
}

template<typename T>
inline void
close_gap(T *base, usize len, usize p, usize cnt)
{
  if ( cnt == 0 ) return;
  if constexpr ( micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    micron::memmove(base + p, base + p + cnt, len - p - cnt);
  } else {
    const usize tail = len - p - cnt;
    for ( usize i = 0; i < tail; ++i ) base[p + i] = micron::move(base[p + cnt + i]);
    for ( usize i = len - cnt; i < len; ++i ) base[i].~T();
    micron::byteset(reinterpret_cast<byte *>(base + (len - cnt)), 0x0, cnt * sizeof(T));
  }
}

template<usize N, typename T>
inline void
destroy(T *src)
{
  if constexpr ( !micron::is_trivially_destructible_v<micron::remove_cv_t<T>> ) {
    if constexpr ( N % 4 == 0 ) {
      for ( usize i = 0; i < N; i += 4 ) {
        src[i].~T();
        src[i + 1].~T();
        src[i + 2].~T();
        src[i + 3].~T();
      }
    } else {
      for ( usize i = 0; i < N; ++i ) src[i].~T();
    }
    // WARNING:after calling destructors, manually zero it out, wasteful but necessary for edge case correctness
    // in case you're wondering why, there seem to be edge cases when applying aggressive optimization options where the compiler opts NOT
    // to init certain memory (if allocing large ptr lists) causing the memory to be left in a garbled state. for OUR classes ~T() WILL
    // destroy the objects BUT wont if they are in a garbled state (nothing to destroy). meaning destruction was technically valid, even
    // though nothing was destroyed. ergo best to manually zero the memory afterwards
    micron::cbyteset<N * sizeof(T)>(src, 0x0);
  } else {
    micron::cbyteset<N * sizeof(T)>(src, 0x0);
  }
}

template<typename T>
inline void
destroy(T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_destructible_v<micron::remove_cv_t<T>> ) {
    for ( usize i = 0; i < cnt; ++i ) src[i].~T();
    // WARNING: after calling destructors, manually zero it out, wasteful but necessary for edge case correctness
    micron::byteset(src, 0x0, cnt * sizeof(T));
  } else {
    micron::byteset(src, 0x0, cnt * sizeof(T));
  }
  // NOTE: the zeroing here is part of destroy()'s semantics, ie micron::array::clear() calls destroy and then skips reconstructing a
  // trivially-constructible T; if you want the fast and cheap variant call destroy_fast()
}

template<usize N, typename T>
inline void
destroy_fast(T *src)
{
  if constexpr ( !micron::is_trivially_destructible_v<micron::remove_cv_t<T>> ) {
    if constexpr ( N % 4 == 0 ) {
      for ( usize i = 0; i < N; i += 4 ) {
        src[i].~T();
        src[i + 1].~T();
        src[i + 2].~T();
        src[i + 3].~T();
      }
    } else {
      for ( usize i = 0; i < N; ++i ) src[i].~T();
    }
  } else {
  }
}

template<typename T>
inline void
destroy_fast(T *src, usize cnt)
{
  if constexpr ( !micron::is_trivially_destructible_v<micron::remove_cv_t<T>> ) {
    for ( usize i = 0; i < cnt; ++i ) src[i].~T();
  } else {
  }
}

template<usize N, typename T>
void
zero(T *src)
{
  micron::cbyteset<N * sizeof(T)>(src, 0x0);
}

template<usize N, typename T>
void
set(T *__restrict src, const T &val)
{
  if constexpr ( !micron::is_trivially_assignable_v<T &, const T &> ) {
    if constexpr ( N % 4 == 0 ) {
      for ( usize i = 0; i < N; i += 4 ) {
        src[i] = val;
        src[i + 1] = val;
        src[i + 2] = val;
        src[i + 3] = val;
      }
    } else {
      for ( usize i = 0; i < N; ++i ) src[i] = val;
    }
  } else {
    micron::ctypeset<N>(src, val);
  }
}

template<typename T>
void
set(T *__restrict src, const T &val, usize cnt)
{
  // WARNING: micron::typeset is __attribute__((nonnull(1))); passing nullptr, 0 to it is UB
  if ( cnt == 0 ) return;
  if constexpr ( !micron::is_trivially_assignable_v<T &, const T &> ) {

    for ( usize i = 0; i < cnt; ++i ) src[i] = val;
  } else {
    micron::typeset(src, val, cnt);
  }
}

template<usize N, typename T>
void
construct(T *__restrict src, const T &val)
{
  if constexpr ( !micron::is_trivially_constructible_v<T, const T &> ) {
    if constexpr ( noexcept(T(micron::declval<const T &>())) ) {
      if constexpr ( N % 4 == 0 ) {
        for ( usize i = 0; i < N; i += 4 ) {
          new (addr(src[i])) T(val);
          new (addr(src[i + 1])) T(val);
          new (addr(src[i + 2])) T(val);
          new (addr(src[i + 3])) T(val);
        }
      } else {
        for ( usize i = 0; i < N; ++i ) new (addr(src[i])) T(val);
      }
    } else {
      // throwing element ctor: unwind the already-built prefix on throw
      usize i = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
      try {
        for ( ; i < N; ++i ) new (addr(src[i])) T(val);
      } catch ( ... ) {
        for ( usize j = 0; j < i; ++j ) src[j].~T();
        throw;
      }
#else
      for ( ; i < N; ++i ) new (addr(src[i])) T(val);
#endif
    }
  } else {
    micron::ctypeset<N>(src, val);
  }
}

template<typename T>
void
construct(T *__restrict src, const T &val, usize cnt)
{
  if constexpr ( !micron::is_trivially_constructible_v<T, const T &> ) {
    if constexpr ( noexcept(T(micron::declval<const T &>())) ) {
      for ( usize i = 0; i < cnt; ++i ) new (addr(src[i])) T(val);
    } else {
      // throwing element ctor: unwind the already-built prefix on throw
      usize i = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
      try {
        for ( ; i < cnt; ++i ) new (addr(src[i])) T(val);
      } catch ( ... ) {
        for ( usize j = 0; j < i; ++j ) src[j].~T();
        throw;
      }
#else
      for ( ; i < cnt; ++i ) new (addr(src[i])) T(val);
#endif
    }
  } else {
    micron::typeset(src, val, cnt);
  }
}

};      // namespace __impl_container
};      // namespace micron
