//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../type_traits.hpp"

#include "../memory/actions.hpp"
#include "../memory/cmemory.hpp"

namespace micron
{
namespace __impl_container
{

template <typename T>
inline void
shallow_copy(T *__restrict dest, const T *__restrict src, usize cnt) noexcept
{
  micron::memcpy(reinterpret_cast<byte *>(dest), reinterpret_cast<const byte *>(src),
                 cnt * (sizeof(T)));     // always is page aligned, 256 is
                                         // fine, just realign back to bytes
};

template <typename T>
inline void
shallow_copy(T *__restrict dest, T *__restrict src, usize cnt) noexcept
{
  micron::memcpy(reinterpret_cast<byte *>(dest), reinterpret_cast<byte *>(src),
                 cnt * (sizeof(T)));     // always is page aligned, 256 is
                                         // fine, just realign back to bytes
};

// deep copy routine, nec. if obj. has const/dest (can be ignored but WILL
// cause segfaulting if underlying doesn't account for double deletes)
template <typename T>
inline void
deep_copy(T *__restrict dest, T *__restrict src, usize cnt)
{
  // guard against copying into uninit memory
  for ( usize i = 0; i < cnt; ++i ) new (addr(dest[i])) T(src[i]);
};

template <typename T>
inline void
deep_copy(T *__restrict dest, const T *__restrict src, usize cnt)
{
  for ( usize i = 0; i < cnt; ++i ) new (addr(dest[i])) T(src[i]);
};

template <typename T>
inline void
deep_copy_assign(T *__restrict dest, T *__restrict src, usize cnt)
{
  // guard against copying into uninit memory
  for ( usize i = 0; i < cnt; ++i ) dest[i] = src[i];
};

template <typename T>
inline void
deep_copy_assign(T *__restrict dest, const T *__restrict src, usize cnt)
{
  for ( usize i = 0; i < cnt; ++i ) dest[i] = src[i];
};

template <typename T>
inline void
shallow_move(T *__restrict dest, T *__restrict src, usize cnt)
{
  micron::memcpy(dest, src, cnt);
  // micron::byteset(reinterpret_cast<byte *>(src), 0x0, cnt * sizeof(T));
};

template <typename T>
inline void
deep_move(T *__restrict dest, T *__restrict src, usize cnt)
{
  for ( usize i = 0; i < cnt; ++i ) {
    new (addr(dest[i])) T(micron::move(src[i]));
    src[i].~T();
    // WARNING: P1144
  }
};

template <typename T>
inline void
deep_move_assign(T *__restrict dest, T *__restrict src, usize cnt)
{
  for ( usize i = 0; i < cnt; ++i ) dest[i] = micron::move(src[i]);
};

template <usize N, typename T>
inline void
deep_copy(T *__restrict dest, T *__restrict src)
{
  for ( usize i = 0; i < N; ++i ) new (addr(dest[i])) T(src[i]);
};

template <usize N, typename T>
inline void
deep_copy(T *__restrict dest, const T *__restrict src)
{
  for ( usize i = 0; i < N; ++i ) new (addr(dest[i])) T(src[i]);
};

template <usize N, typename T>
inline void
deep_copy_assign(T *__restrict dest, T *__restrict src)
{
  for ( usize i = 0; i < N; ++i ) dest[i] = src[i];
};

template <usize N, typename T>
inline void
deep_copy_assign(T *__restrict dest, const T *__restrict src)
{
  for ( usize i = 0; i < N; ++i ) dest[i] = src[i];
};

template <usize N, typename T>
inline void
shallow_move(T *__restrict dest, T *__restrict src)
{
  micron::cmemcpy<N, T, T>(dest, src);
  // micron::cbyteset<N * sizeof(T)>(reinterpret_cast<byte *>(src), 0x0);
};

template <usize N, typename T>
inline void
deep_move(T *__restrict dest, T *__restrict src)
{
  for ( usize i = 0; i < N; ++i ) {
    new (addr(dest[i])) T(micron::move(src[i]));
    src[i].~T();
    // WARNING: P1144
  }
};

template <usize N, typename T>
inline void
deep_move_assign(T *__restrict dest, T *__restrict src)
{
  for ( usize i = 0; i < N; ++i ) dest[i] = micron::move(src[i]);
};

template <usize N, typename T>
inline void
shallow_copy(T *__restrict dest, const T *__restrict src) noexcept
{
  micron::cmemcpy<N * sizeof(T)>(reinterpret_cast<byte *>(dest),
                                 reinterpret_cast<const byte *>(src));     // always is page aligned, 256 is
                                                                           // fine, just realign back to bytes
};

template <usize N, typename T>
inline void
shallow_copy(T *__restrict dest, T *__restrict src) noexcept
{
  micron::cmemcpy<N * sizeof(T)>(reinterpret_cast<byte *>(dest),
                                 reinterpret_cast<const byte *>(src));     // always is page aligned, 256 is
                                                                           // fine, just realign back to bytes
};

template <typename T>
inline void
copy(T *__restrict dest, T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy(dest, src, cnt);
  } else {
    shallow_copy(dest, src, cnt);
  }
}

template <typename T>
inline void
copy(T *__restrict dest, const T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy(dest, src, cnt);
  } else {
    shallow_copy(dest, src, cnt);
  }
}

template <typename T>
inline void
copy_assign(T *__restrict dest, T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy_assign(dest, src, cnt);
  } else {
    shallow_copy(dest, src, cnt);
  }
}

template <typename T>
inline void
copy_assign(T *__restrict dest, const T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy_assign(dest, src, cnt);
  } else {
    shallow_copy(dest, src, cnt);
  }
}

template <typename T>
inline void
move_assign(T *__restrict dest, T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_move_assignable_v<micron::remove_cv_t<T>> ) {
    deep_move_assign(dest, src, cnt);
  } else {
    shallow_move(dest, src, cnt);
  }
}

template <typename T>
inline void
move(T *__restrict dest, T *__restrict src, usize cnt)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_move(dest, src, cnt);
  } else {
    shallow_move(dest, src, cnt);
  }
}

template <usize N, typename T>
inline void
copy(T *__restrict dest, T *__restrict src)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy<N, T>(dest, src);
  } else {
    shallow_copy<N, T>(dest, src);
  }
}

template <usize N, typename T>
inline void
copy(T *__restrict dest, const T *__restrict src)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy<N, T>(dest, src);
  } else {
    shallow_copy<N, T>(dest, src);
  }
}

template <usize N, typename T>
inline void
copy_assign(T *__restrict dest, T *__restrict src)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy_assign<N, T>(dest, src);
  } else {
    shallow_copy<N, T>(dest, src);
  }
}

template <usize N, typename T>
inline void
copy_assign(T *__restrict dest, const T *__restrict src)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy_assign<N, T>(dest, src);
  } else {
    shallow_copy<N, T>(dest, src);
  }
}

template <usize N, typename T>
inline void
move_assign(T *__restrict dest, T *__restrict src)
{
  if constexpr ( !micron::is_trivially_move_assignable_v<micron::remove_cv_t<T>> ) {
    deep_move_assign<N, T>(dest, src);
  } else {
    shallow_move<N, T>(dest, src);
  }
}

template <usize N, typename T>
inline void
move(T *__restrict dest, T *__restrict src)
{
  if constexpr ( !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_move<N, T>(dest, src);
  } else {
    shallow_move<N, T>(dest, src);
  }
}

template <usize N, typename T>
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

template <typename T>
inline void
destroy(T *src, usize cnt)
{
  if constexpr ( !micron::is_trivially_destructible_v<micron::remove_cv_t<T>> ) {
    for ( usize i = 0; i < cnt; ++i ) src[i].~T();
    // WARNING: after calling destructors, manually zero it out, wasteful but necessary for edge case correctness
    micron::byteset(src, 0x0, cnt * sizeof(T));
  } else {
    micron::byteset(src, 0x0, cnt * sizeof(T));
  }
}

template <usize N, typename T>
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

template <typename T>
inline void
destroy_fast(T *src, usize cnt)
{
  if constexpr ( !micron::is_trivially_destructible_v<micron::remove_cv_t<T>> ) {
    for ( usize i = 0; i < cnt; ++i ) src[i].~T();
  } else {
  }
}

template <usize N, typename T>
void
zero(T *src)
{
  micron::cbyteset<N * sizeof(T)>(src, 0x0);
}

template <usize N, typename T>
void
set(T *__restrict src, const T &val)
{
  if constexpr ( !micron::is_trivially_assignable_v<T, T> ) {
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

template <typename T>
void
set(T *__restrict src, const T &val, usize cnt)
{
  if constexpr ( !micron::is_trivially_assignable_v<T, T> ) {

    for ( usize i = 0; i < cnt; ++i ) src[i] = val;
  } else {
    micron::typeset(src, val, cnt);
  }
}

template <usize N, typename T>
void
construct(T *__restrict src, const T &val)
{
  if constexpr ( !micron::is_trivially_constructible_v<T, const T &> ) {
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
    micron::ctypeset<N>(src, val);
  }
}

template <typename T>
void
construct(T *__restrict src, const T &val, usize cnt)
{
  if constexpr ( !micron::is_trivially_constructible_v<T, const T &> ) {
    for ( usize i = 0; i < cnt; ++i ) new (addr(src[i])) T(val);
  } else {
    micron::typeset(src, val, cnt);
  }
}

};     // namespace __impl_container
};     // namespace micron
