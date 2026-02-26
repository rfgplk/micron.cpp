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
shallow_copy(T *__restrict dest, const T *__restrict src, size_t cnt) noexcept
{
  micron::memcpy(reinterpret_cast<byte *>(dest), reinterpret_cast<const byte *>(src),
                 cnt * (sizeof(T) / sizeof(byte)));     // always is page aligned, 256 is
                                                        // fine, just realign back to bytes
};

template <typename T>
inline void
shallow_copy(T *__restrict dest, T *__restrict src, size_t cnt) noexcept
{
  micron::memcpy(reinterpret_cast<byte *>(dest), reinterpret_cast<byte *>(src),
                 cnt * (sizeof(T) / sizeof(byte)));     // always is page aligned, 256 is
                                                        // fine, just realign back to bytes
};

// deep copy routine, nec. if obj. has const/dest (can be ignored but WILL
// cause segfaulting if underlying doesn't account for double deletes)
template <typename T>
inline void
deep_copy(T *__restrict dest, T *__restrict src, size_t cnt)
{
  for ( size_t i = 0; i < cnt; i++ )
    dest[i] = src[i];
};

template <typename T>
inline void
deep_copy(T *__restrict dest, const T *__restrict src, size_t cnt)
{
  for ( size_t i = 0; i < cnt; i++ )
    dest[i] = src[i];
};

template <typename T>
inline void
shallow_move(T *__restrict dest, T *__restrict src, size_t cnt)
{
  micron::memcpy(dest, src, cnt);
  micron::byteset(reinterpret_cast<byte *>(src), 0x0, cnt * sizeof(T));
};

template <typename T>
inline void
deep_move(T *__restrict dest, T *__restrict src, size_t cnt)
{
  for ( size_t i = 0; i < cnt; i++ )
    dest[i] = micron::move(src[i]);
};

template <size_t N, typename T>
inline void
deep_copy(T *__restrict dest, T *__restrict src)
{
  for ( size_t i = 0; i < N; i++ )
    dest[i] = src[i];
};

template <size_t N, typename T>
inline void
deep_copy(T *__restrict dest, const T *__restrict src)
{
  for ( size_t i = 0; i < N; i++ )
    dest[i] = src[i];
};

template <size_t N, typename T>
inline void
shallow_move(T *__restrict dest, T *__restrict src)
{
  micron::cmemcpy<N>(dest, src);
  micron::cbyteset<N * sizeof(T)>(reinterpret_cast<byte *>(src), 0x0);
};

template <size_t N, typename T>
inline void
deep_move(T *__restrict dest, T *__restrict src)
{
  for ( size_t i = 0; i < N; i++ )
    dest[i] = micron::move(src[i]);
};

template <size_t N, typename T>
inline void
shallow_copy(T *__restrict dest, const T *__restrict src) noexcept
{
  micron::cmemcpy<N * sizeof(T)>(reinterpret_cast<byte *>(dest),
                                 reinterpret_cast<const byte *>(src));     // always is page aligned, 256 is
                                                                           // fine, just realign back to bytes
};

template <size_t N, typename T>
inline void
shallow_copy(T *__restrict dest, T *__restrict src) noexcept
{
  micron::cmemcpy<N * sizeof(T)>(reinterpret_cast<byte *>(dest),
                                 reinterpret_cast<const byte *>(src));     // always is page aligned, 256 is
                                                                           // fine, just realign back to bytes
};

template <typename T>
inline void
copy(T *__restrict dest, T *__restrict src, size_t cnt)
{
  if constexpr ( micron::is_class_v<micron::remove_cv_t<T>> or !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy(dest, src, cnt);
  } else {
    shallow_copy(dest, src, cnt);
  }
}

template <typename T>
inline void
copy(T *__restrict dest, const T *__restrict src, size_t cnt)
{
  if constexpr ( micron::is_class_v<micron::remove_cv_t<T>> or !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy(dest, src, cnt);
  } else {
    shallow_copy(dest, src, cnt);
  }
}

template <typename T>
inline void
move(T *__restrict dest, T *__restrict src, size_t cnt)
{
  if constexpr ( micron::is_class_v<micron::remove_cv_t<T>> or !micron::is_trivially_move_assignable_v<micron::remove_cv_t<T>> ) {
    deep_move(dest, src, cnt);
  } else {
    shallow_move(dest, src, cnt);
  }
}

template <size_t N, typename T>
inline void
copy(T *__restrict dest, T *__restrict src)
{
  if constexpr ( micron::is_class_v<micron::remove_cv_t<T>> or !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy<N, T>(dest, src);
  } else {
    shallow_copy<N, T>(dest, src);
  }
}

template <size_t N, typename T>
inline void
copy(T *__restrict dest, const T *__restrict src)
{
  if constexpr ( micron::is_class_v<micron::remove_cv_t<T>> or !micron::is_trivially_copyable_v<micron::remove_cv_t<T>> ) {
    deep_copy<N, T>(dest, src);
  } else {
    shallow_copy<N, T>(dest, src);
  }
}

template <size_t N, typename T>
inline void
move(T *__restrict dest, T *__restrict src)
{
  if constexpr ( micron::is_class_v<micron::remove_cv_t<T>> or !micron::is_trivially_move_assignable_v<micron::remove_cv_t<T>> ) {
    deep_move<N, T>(dest, src);
  } else {
    shallow_move<N, T>(dest, src);
  }
}

template <size_t N, typename T>
inline void
destroy(T *src)
{
  if constexpr ( micron::is_class_v<micron::remove_cv_t<T>> or !micron::is_trivially_destructible_v<micron::remove_cv_t<T>> ) {
    if constexpr ( N % 4 == 0 ) {
      for ( size_t i = 0; i < N; i += 4 ) {
        src[i].~T();
        src[i + 1].~T();
        src[i + 2].~T();
        src[i + 3].~T();
      }
    } else {
      for ( size_t i = 0; i < N; i++ )
        src[i].~T();
    }
  } else {
    micron::cbyteset<N * sizeof(T)>(src, 0x0);
  }
}

template <typename T>
inline void
destroy(T *src, size_t cnt)
{
  if constexpr ( micron::is_class_v<micron::remove_cv_t<T>> or !micron::is_trivially_destructible_v<micron::remove_cv_t<T>> ) {
    for ( size_t i = 0; i < cnt; ++i )
      src[i].~T();
  } else {
    micron::byteset(src, 0x0, cnt * sizeof(T));
  }
}

template <size_t N, typename T>
void
zero(T *src)
{
  micron::cbyteset<N * sizeof(T)>(src, 0x0);
}

template <size_t N, typename T>
void
set(T *__restrict src, const T &val)
{
  if constexpr ( micron::is_class_v<micron::remove_cv_t<T>> or !micron::is_trivially_assignable_v<T, T> ) {
    if constexpr ( N % 4 == 0 ) {
      for ( size_t i = 0; i < N; i += 4 ) {
        src[i] = val;
        src[i + 1] = val;
        src[i + 2] = val;
        src[i + 3] = val;
      }
    } else {
      for ( size_t i = 0; i < N; i++ )
        src[i] = val;
    }
  } else {
    micron::ctypeset<N>(src, val);
  }
}

template <typename T>
void
set(T *__restrict src, const T &val, size_t cnt)
{
  if constexpr ( micron::is_class_v<micron::remove_cv_t<T>> or !micron::is_trivially_assignable_v<T, T> ) {

    for ( size_t i = 0; i < cnt; i++ )
      src[i] = val;
  } else {
    micron::typeset(src, val, cnt);
  }
}

};     // namespace __impl_container
};     // namespace micron
