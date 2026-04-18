#pragma once

#include "../../types.hpp"

namespace micron
{
namespace posix
{

struct fd_t {
  i32 fd;

  ~fd_t() = default;
  constexpr fd_t(void) : fd{} {};
  constexpr fd_t(i32 x) : fd(x) {};
  constexpr fd_t(const fd_t &) = default;
  constexpr fd_t(fd_t &&) = default;
  constexpr fd_t &operator=(const fd_t &) = default;
  constexpr fd_t &operator=(fd_t &&) = default;

  constexpr fd_t &
  operator=(i32 x)
  {
    fd = x;
    return *this;
  }

  constexpr inline bool
  closed() const
  {
    return fd == -1;
  }

  constexpr inline bool
  open() const
  {
    return fd >= 0;
  }

  constexpr inline bool
  invalid() const
  {
    if ( fd < 0 ) return true;
    return false;
  }

  constexpr inline auto
  has_error() const -> u32
  {
    if ( fd < 0 ) return fd * -1;
    return 0;
  }

  constexpr inline void
  reset()
  {
    fd = -1;
  }

  constexpr explicit
  operator i32() const
  {
    return fd;
  }

  constexpr explicit
  operator bool() const
  {
    return fd >= 0;
  }

  constexpr bool
  operator==(const fd_t &o) const
  {
    return fd == o.fd;
  }

  constexpr bool
  operator!=(const fd_t &o) const
  {
    return fd != o.fd;
  }

  constexpr bool
  operator<(const fd_t &o) const
  {
    return fd < o.fd;
  }

  constexpr bool
  operator>(const fd_t &o) const
  {
    return fd > o.fd;
  }

  constexpr bool
  operator<=(const fd_t &o) const
  {
    return fd <= o.fd;
  }

  constexpr bool
  operator>=(const fd_t &o) const
  {
    return fd >= o.fd;
  }

  constexpr bool
  operator==(i32 x) const
  {
    return fd == x;
  }

  constexpr bool
  operator!=(i32 x) const
  {
    return fd != x;
  }

  constexpr bool
  operator<(i32 x) const
  {
    return fd < x;
  }

  constexpr bool
  operator>(i32 x) const
  {
    return fd > x;
  }

  constexpr bool
  operator<=(i32 x) const
  {
    return fd <= x;
  }

  constexpr bool
  operator>=(i32 x) const
  {
    return fd >= x;
  }

  constexpr fd_t
  operator+(i32 n) const
  {
    return fd_t(fd + n);
  }

  constexpr fd_t
  operator-(i32 n) const
  {
    return fd_t(fd - n);
  }

  constexpr fd_t &
  operator+=(i32 n)
  {
    fd += n;
    return *this;
  }

  constexpr fd_t &
  operator-=(i32 n)
  {
    fd -= n;
    return *this;
  }

  constexpr fd_t &
  operator++()
  {
    ++fd;
    return *this;
  }

  constexpr fd_t
  operator++(int)
  {
    fd_t t(*this);
    ++fd;
    return t;
  }

  constexpr fd_t &
  operator--()
  {
    --fd;
    return *this;
  }

  constexpr fd_t
  operator--(int)
  {
    fd_t t(*this);
    --fd;
    return t;
  }

  constexpr fd_t
  operator-() const
  {
    return fd_t(-fd);
  }
};

inline bool
operator==(i32 x, const fd_t &f)
{
  return x == f.fd;
}

inline bool
operator!=(i32 x, const fd_t &f)
{
  return x != f.fd;
}

inline bool
operator<(i32 x, const fd_t &f)
{
  return x < f.fd;
}

inline bool
operator>(i32 x, const fd_t &f)
{
  return x > f.fd;
}

inline bool
operator<=(i32 x, const fd_t &f)
{
  return x <= f.fd;
}

inline bool
operator>=(i32 x, const fd_t &f)
{
  return x >= f.fd;
}

inline constexpr fd_t invalid_fd{ -1 };

using dir_t = fd_t;

};     // namespace posix

};     // namespace micron
