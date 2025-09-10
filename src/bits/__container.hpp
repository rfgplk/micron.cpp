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
shallow_copy(T *dest, T *src, size_t cnt)
{
  micron::memcpy(reinterpret_cast<byte *>(dest), reinterpret_cast<byte *>(src),
                 cnt * (sizeof(T) / sizeof(byte)));     // always is page aligned, 256 is
                                                        // fine, just realign back to bytes
};
// deep copy routine, nec. if obj. has const/dest (can be ignored but WILL
// cause segfaulting if underlying doesn't account for double deletes)
template <typename T>
inline void
deep_copy(T *dest, T *src, size_t cnt)
{
  for ( size_t i = 0; i < cnt; i++ )
    dest[i] = src[i];
};

template <typename T>
inline void
shallow_move(T *dest, T *src, size_t cnt)
{
  micron::memcpy(reinterpret_cast<byte *>(dest), reinterpret_cast<byte *>(src), cnt);
  micron::memset(reinterpret_cast<byte *>(src), 0x0, cnt);
};
template <typename T>
inline void
deep_move(T *dest, T *src, size_t cnt)
{
  for ( size_t i = 0; i < cnt; i++ )
    dest[i] = micron::move(src[i]);
};

template <typename T>
inline void
copy(T *dest, T *src, size_t cnt)
{
  if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
    deep_copy(dest, src, cnt);
  } else {
    shallow_copy(dest, src, cnt);
  }
}
template <typename T>
inline void
move(T *&dest, T *&src, size_t cnt)
{
  if constexpr ( micron::is_class_v<T> or !micron::is_trivially_move_assignable_v<T> ) {
    deep_move(dest, src, cnt);
  } else {
    shallow_move(dest, src, cnt);
  }
}
};
};
