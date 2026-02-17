//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"

#include "../memory/memory.hpp"
#include "../types.hpp"
#include "impl.hpp"
#include "io.hpp"

#include "__std.hpp"

namespace micron
{
namespace io
{

template <typename T>
concept is_container = requires(T t) {
  { t.cbegin() } -> micron::same_as<typename T::const_iterator>;
  { t.cend() } -> micron::same_as<typename T::const_iterator>;
  { t.begin() } -> micron::same_as<typename T::iterator>;
  { t.end() } -> micron::same_as<typename T::iterator>;
} && !requires(T t) {
  { t.c_str() } -> micron::same_as<const char *>;
};

// PRINTK BLOCK OF FUNCS
template <int outstream = stdout_fileno, typename T, size_t M>
  requires(micron::is_same_v<T, char> or micron::is_same_v<T, schar> or micron::is_same_v<T, wide> or micron::is_same_v<T, unicode8>
           or micron::is_same_v<T, unicode32> or micron::is_same_v<T, const char> or micron::is_same_v<T, const schar>
           or micron::is_same_v<T, const wide> or micron::is_same_v<T, const unicode8> or micron::is_same_v<T, const unicode32>)
inline void
printk(T (&c)[M])
{
  if constexpr ( outstream == stdout_fileno )
    io::fput(c, io::stdout);
  else if constexpr ( outstream == stderr_fileno )
    io::fput(c, io::stderr);
}

template <int outstream = stdout_fileno, typename T>
  requires(micron::is_same_v<T, char *> or micron::is_same_v<T, schar *> or micron::is_same_v<T, wide *> or micron::is_same_v<T, unicode8 *>
           or micron::is_same_v<T, unicode32 *> or micron::is_same_v<T, const char *> or micron::is_same_v<T, const schar *>
           or micron::is_same_v<T, const wide *> or micron::is_same_v<T, const unicode8 *> or micron::is_same_v<T, const unicode32 *>)
inline void
printk(const T &c)
{
  if constexpr ( outstream == stdout_fileno )
    io::fput(c, io::stdout);
  else if constexpr ( outstream == stderr_fileno )
    io::fput(c, io::stderr);
}

template <int outstream = stdout_fileno>
inline void
printk(char c)
{
  if constexpr ( outstream == stdout_fileno )
    io::fput(c, io::stdout);
  else if constexpr ( outstream == stderr_fileno )
    io::fput(c, io::stderr);
}

template <int outstream = stdout_fileno>
inline void
sprintk(const char *c, size_t len)
{
  if constexpr ( outstream == stdout_fileno )
    io::fput(c, len, io::stdout);
  else if constexpr ( outstream == stderr_fileno )
    io::fput(c, len, io::stderr);
}     // raw print

// this is necessary because strings provide all three methods, will cause
// ambiguity
template <typename T>
concept has_cstr = requires(T t) {
  { t.c_str() } -> micron::same_as<const char *>;
};

// must be a class that provides .c_str()
template <has_cstr T, int outstream = stdout_fileno>
  requires micron::is_class_v<T>
void
printk(const T &str)
{
  if constexpr ( outstream == stdout_fileno )
    io::fput(str.c_str(), io::stdout);
  else if constexpr ( outstream == stderr_fileno )
    io::fput(str.c_str(), io::stderr);
}

// if type is a pointer, or, pointerlike
template <typename T, int outstream = stdout_fileno>
  requires(micron::is_pointer_v<T> and !micron::is_same_v<T, char *> and !micron::is_same_v<T, schar *> and !micron::is_same_v<T, wide *>
           and !micron::is_same_v<T, unicode8 *> and !micron::is_same_v<T, unicode32 *> and !micron::is_same_v<T, const char *>
           and !micron::is_same_v<T, const schar *> and !micron::is_same_v<T, const wide *> and !micron::is_same_v<T, const unicode8 *>
           and !micron::is_same_v<T, const unicode32 *>)
void
printk(const T &x)
{
  char print_buffer[32];     // long enough, overkill
  const char *ptr = format::ptr_to_char(x, &print_buffer[0]);
  if constexpr ( outstream == stdout_fileno )
    io::fput(ptr, io::stdout);
  else if constexpr ( outstream == stderr_fileno )
    io::fput(ptr, io::stderr);
  return;
}

// if type is arithmetic or not
template <typename T, int outstream = stdout_fileno>
  requires micron::is_arithmetic_v<T>
void
printk(const T &x)
{
  char print_buffer[128];     // long enough, overkill
  if constexpr ( micron::same_as<T, bool> ) {
    const char *ptr = format::bool_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, i8> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, u8> ) {
    const char *ptr = format::uint_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, i16> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, u16> ) {
    const char *ptr = format::uint_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, int> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, uint> ) {
    const char *ptr = format::uint_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, long> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, unsigned long> ) {
    const char *ptr = format::uint64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, char> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, unsigned char> ) {
    const char *ptr = format::uint_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, short> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, unsigned short> ) {
    const char *ptr = format::uint_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, long long> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, unsigned long long> ) {
    const char *ptr = format::uint64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, i32> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, u32> or micron::same_as<T, unsigned> ) {
    const char *ptr = format::uint_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, i64> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, u64> ) {
    const char *ptr = format::uint64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  // just in case it's different from u64
  if constexpr ( micron::same_as<T, max_t> or micron::same_as<T, ssize_t> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, umax_t> or micron::same_as<T, size_t> ) {
    const char *ptr = format::uint64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, flong> ) {
    const char *ptr = format::flong_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, f128> ) {
    const char *ptr = format::f128_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, f64> ) {
    const char *ptr = format::f64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }

  if constexpr ( micron::same_as<T, double> ) {
    const char *ptr = format::f128_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, float> ) {
    const char *ptr = format::f32_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, f32> ) {
    const char *ptr = format::f32_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno )
      io::fput(ptr, io::stdout);
    else if constexpr ( outstream == stderr_fileno )
      io::fput(ptr, io::stderr);
    return;
  }
  if constexpr ( micron::same_as<T, char> ) {
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(print_buffer, 1, io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(print_buffer, 1, io::stderr);
    }
    return;
  }
  const char *err_str = "You tried to output an unknown/unformattable "
                        "type. Try using bin()";
  io::fput(err_str, io::stderr);
  return;
}

// spec case, single byte
template <typename T, int outstream = stdout_fileno>
  requires micron::is_same_v<T, byte> && (sizeof(T) == 1)
void
printk(T &x)
{
  byte c[2];
  c[0] = x;
  c[1] = 0x0;
  if constexpr ( outstream == stdout_fileno )
    io::fput(reinterpret_cast<const char *>(c), io::stdout);
  else if constexpr ( outstream == stderr_fileno )
    io::fput(reinterpret_cast<const char *>(c), io::stderr);
}

// non null trm string
template <typename T, int outstream = stdout_fileno>
  requires micron::is_same_v<T, byte *>
void
printk(T ptr, size_t len)
{
  byte *null_trm = new byte[len + 1];
  micron::memcpy(null_trm, ptr, len);
  null_trm[len] = 0x0;
  if constexpr ( outstream == stdout_fileno )
    io::fput(reinterpret_cast<const char *>(null_trm), io::stdout);
  else if constexpr ( outstream == stderr_fileno )
    io::fput(reinterpret_cast<const char *>(null_trm), io::stderr);
  delete[] null_trm;
}

// PRINTKN BLOCK OF FUNCS, APPENDS NEWLINE
template <int outstream = stdout_fileno, typename T, size_t M>
  requires(micron::is_same_v<T, char> or micron::is_same_v<T, schar> or micron::is_same_v<T, wide> or micron::is_same_v<T, unicode8>
           or micron::is_same_v<T, unicode32> or micron::is_same_v<T, const char> or micron::is_same_v<T, const schar>
           or micron::is_same_v<T, const wide> or micron::is_same_v<T, const unicode8> or micron::is_same_v<T, const unicode32>)
void
printkn(T (&c)[M])
{
  if constexpr ( outstream == stdout_fileno ) {
    io::fput(c, io::stdout);
    io::fput("\n", io::stdout);
  } else if constexpr ( outstream == stderr_fileno ) {
    io::fput(c, io::stderr);
    io::fput("\n", io::stderr);
  }
}

template <int outstream = stdout_fileno, typename T>
  requires(micron::is_same_v<T, char *> or micron::is_same_v<T, schar *> or micron::is_same_v<T, wide *> or micron::is_same_v<T, unicode8 *>
           or micron::is_same_v<T, unicode32 *> or micron::is_same_v<T, const char *> or micron::is_same_v<T, const schar *>
           or micron::is_same_v<T, const wide *> or micron::is_same_v<T, const unicode8 *> or micron::is_same_v<T, const unicode32 *>)
void
printkn(const T &c)
{
  if constexpr ( outstream == stdout_fileno ) {
    io::fput(c, io::stdout);
    io::fput("\n", io::stdout);
  } else if constexpr ( outstream == stderr_fileno ) {
    io::fput(c, io::stderr);
    io::fput("\n", io::stderr);
  }
}

template <typename T, int outstream = stdout_fileno, size_t M>
void
printk(const char (&c)[M])
{
  if constexpr ( outstream == stdout_fileno ) {
    io::fput(c, M, io::stdout);
    // io::fput("\n", io::stdout);
  } else if constexpr ( outstream == stderr_fileno ) {
    io::fput(c, M, io::stderr);
    // io::fput("\n", io::stderr);
  }
}

template <typename T, int outstream = stdout_fileno, size_t M>
void
printkn(const char (&c)[M])
{
  if constexpr ( outstream == stdout_fileno ) {
    io::fput(c, M, io::stdout);
    io::fput("\n", io::stdout);
  } else if constexpr ( outstream == stderr_fileno ) {
    io::fput(c, M, io::stderr);
    io::fput("\n", io::stderr);
  }
}

template <int outstream = stdout_fileno>
void
sprintkn(const char *c, size_t len)
{
  if constexpr ( outstream == stdout_fileno ) {
    io::fput(c, len, io::stdout);
    io::fput("\n", io::stdout);
  } else if constexpr ( outstream == stderr_fileno ) {
    io::fput(c, len, io::stderr);
    io::fput("\n", io::stderr);
  }
}

// raw print
// must be a class that provides .c_str()
template <has_cstr T, int outstream = stdout_fileno>
  requires(micron::is_class_v<T>) && (micron::same_as<typename T::value_type, char8_t>)
void
printkn(const T &str)
{
  if constexpr ( outstream == stdout_fileno ) {
    io::fput(str.c_str(), io::stdout);
    io::fput("\n", io::stdout);
  } else if constexpr ( outstream == stderr_fileno ) {
    io::fput(str.c_str(), io::stderr);
    io::fput("\n", io::stderr);
  }
}

template <has_cstr T, int outstream = stdout_fileno>
  requires(micron::is_class_v<T>) && (micron::same_as<typename T::value_type, char>)
void
printkn(const T &str)
{
  if constexpr ( outstream == stdout_fileno ) {
    io::fput(str.c_str(), io::stdout);
    io::fput("\n", io::stdout);
  } else if constexpr ( outstream == stderr_fileno ) {
    io::fput(str.c_str(), io::stderr);
    io::fput("\n", io::stderr);
  }
}

template <has_cstr T, int outstream = stdout_fileno>
  requires(micron::is_class_v<T>) && (micron::same_as<typename T::value_type, wchar_t>)
void
printkn(const T &str)
{
  if constexpr ( outstream == stdout_fileno ) {
    wfput(str.w_str(), io::stdout);
    io::fput("\n", io::stdout);
  } else if constexpr ( outstream == stderr_fileno ) {
    wfput(str.w_str(), io::stderr);
    io::fput("\n", io::stderr);
  }
}

template <has_cstr T, int outstream = stdout_fileno>
  requires(micron::is_class_v<T>) && (micron::same_as<typename T::value_type, char32_t>)
void
printkn(const T &str)
{
  if constexpr ( outstream == stdout_fileno ) {
    io::unifput(str.uni_str(), io::stdout);
    io::fput("\n", io::stdout);
  } else if constexpr ( outstream == stderr_fileno ) {
    io::unifput(str.uni_str(), io::stderr);
    io::fput("\n", io::stderr);
  }
}

// if type is a pointer, or, pointerlike
template <typename T, int outstream = stdout_fileno>
  requires(micron::is_pointer_v<T> and !micron::is_same_v<T, char *> and !micron::is_same_v<T, schar *> and !micron::is_same_v<T, wide *>
           and !micron::is_same_v<T, unicode8 *> and !micron::is_same_v<T, unicode32 *> and !micron::is_same_v<T, const char *>
           and !micron::is_same_v<T, const schar *> and !micron::is_same_v<T, const wide *> and !micron::is_same_v<T, const unicode8 *>
           and !micron::is_same_v<T, const unicode32 *>)
void
printkn(const T &x)
{
  char print_buffer[32];     // long enough, overkill
  const char *ptr = format::ptr_to_char(x, &print_buffer[0]);
  if constexpr ( outstream == stdout_fileno ) {
    io::fput(ptr, io::stdout);
    io::fput("\n", io::stdout);
  } else if constexpr ( outstream == stderr_fileno ) {
    io::fput(ptr, io::stderr);
    io::fput("\n", io::stderr);
  }
  return;
}

// if type is arithmetic or not
template <typename T, int outstream = stdout_fileno>
  requires micron::is_arithmetic_v<T>
void
printkn(const T &x)
{
  char print_buffer[128];     // long enough, overkill
  if constexpr ( micron::same_as<T, bool> ) {
    const char *ptr = format::bool_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(ptr, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(ptr, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  if constexpr ( micron::same_as<T, i8> or micron::same_as<T, u8> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(ptr, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(ptr, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  if constexpr ( micron::same_as<T, i16> or micron::same_as<T, u16> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(ptr, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(ptr, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  if constexpr ( micron::same_as<T, i32> or micron::same_as<T, u32> or micron::same_as<T, unsigned> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(ptr, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(ptr, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  if constexpr ( micron::same_as<T, i64> or micron::same_as<T, u64> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(ptr, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(ptr, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  // just in case it's different from u64
  if constexpr ( micron::same_as<T, max_t> or micron::same_as<T, umax_t> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(ptr, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(ptr, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  if constexpr ( micron::same_as<T, flong> ) {
    const char *ptr = format::flong_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(ptr, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(ptr, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  if constexpr ( micron::same_as<T, f128> ) {
    const char *ptr = format::f128_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(ptr, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(ptr, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  if constexpr ( micron::same_as<T, f64> ) {
    const char *ptr = format::f64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(ptr, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(ptr, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  if constexpr ( micron::same_as<T, double> ) {
    const char *ptr = format::f128_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(ptr, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(ptr, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  if constexpr ( micron::same_as<T, float> ) {
    const char *ptr = format::f32_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(ptr, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(ptr, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  if constexpr ( micron::same_as<T, f32> ) {
    const char *ptr = format::f32_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(ptr, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(ptr, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  if constexpr ( micron::same_as<T, char> ) {
    if constexpr ( outstream == stdout_fileno ) {
      io::fput(print_buffer, 1, io::stdout);
      io::fput("\n", io::stdout);
    } else if constexpr ( outstream == stderr_fileno ) {
      io::fput(print_buffer, 1, io::stderr);
      io::fput("\n", io::stderr);
    }
    return;
  }
  const char *err_str = "You tried to output an unknown/unformattable "
                        "type. Try using bin()\n";
  io::fput(err_str, io::stderr);
  return;
}

// spec case, single byte
template <typename T, int outstream = stdout_fileno>
  requires micron::is_same_v<T, byte> && (sizeof(T) == 1)
void
printkn(T &x)
{
  byte c[3];
  c[0] = x;
  c[1] = '\n';
  c[2] = 0x0;
  if constexpr ( outstream == stdout_fileno )
    io::fput(reinterpret_cast<const char *>(c), io::stdout);
  else if constexpr ( outstream == stderr_fileno )
    io::fput(reinterpret_cast<const char *>(c), io::stderr);
}

// non null trm string
template <typename T, int outstream = stdout_fileno>
  requires micron::is_same_v<T, byte *>
void
printkn(T ptr, size_t len)
{
  byte *null_trm = new byte[len + 2];
  micron::memcpy(null_trm, ptr, len);
  null_trm[len - 1] = '\n';
  null_trm[len] = 0x0;
  if constexpr ( outstream == stdout_fileno )
    io::fput(reinterpret_cast<const char *>(null_trm), io::stdout);
  else if constexpr ( outstream == stderr_fileno )
    io::fput(reinterpret_cast<const char *>(null_trm), io::stderr);
  delete[] null_trm;
}

template <is_container T, int outstream = stdout_fileno>
void
printkn(T &&str)
{
  if constexpr ( outstream == stdout_fileno ) {
    auto start = str.cbegin();
    auto end = str.cend();
    printk<stdout_fileno>("{ ");
    for ( ; start != end; ++start ) {
      printk<T, io::stdout>(*start);
      if ( start + 1 != end )
        printk<stdout_fileno>(", ");
    }
    printk<stdout_fileno>(" }");
    printk<stdout_fileno>("\n");
  } else if constexpr ( outstream == stderr_fileno ) {
    auto start = str.cbegin();
    auto end = str.cend();
    printk<stderr_fileno>("{ ");
    for ( ; start != end; ++start ) {
      printk<T, stderr_fileno>(*start);
      if ( start + 1 != end )
        printk<stderr_fileno>(", ");
    }
    printk<stderr_fileno>(" }");
    printk<stderr_fileno>("\n");
  }
}

template <is_container T, int outstream = stdout_fileno>
void
printkn(const T &str)
{
  if constexpr ( outstream == stdout_fileno ) {
    auto start = str.cbegin();

    auto end = str.cend();
    printk<stdout_fileno>("{ ");
    for ( ; start != end; ++start ) {
      printk(*start);
      if ( start + 1 != end )
        printk<stdout_fileno>(", ");
    }
    printk<stdout_fileno>(" }");
    printk<stdout_fileno>("\n");
  } else if constexpr ( outstream == stderr_fileno ) {
    const auto *start = str.cbegin();
    const auto *end = str.cend();
    printk<stderr_fileno>("{ ");
    for ( ; start != end; ++start ) {
      printk(*start);
      if ( start + 1 != end )
        printk<stderr_fileno>(", ");
    }
    printk<stderr_fileno>(" }");
    printk<stderr_fileno>("\n");
  }
}

template <is_container T, int outstream = stdout_fileno>
void
printk(T &&str)
{
  if constexpr ( outstream == stdout_fileno ) {
    const auto *start = str.cbegin();
    const auto *end = str.cend();
    printk<stdout_fileno>("{ ");
    for ( ; start != end; ++start ) {
      printk(*start);
      if ( start + 1 != end )
        printk<stdout_fileno>(", ");
    }
    printk<stdout_fileno>(" }");
  } else if constexpr ( outstream == stderr_fileno ) {
    const auto *start = str.cbegin();
    const auto *end = str.cend();
    printk<stderr_fileno>("{ ");
    for ( ; start != end; ++start ) {
      printk(*start);
      if ( start + 1 != end )
        printk<stderr_fileno>(", ");
    }
    printk<stderr_fileno>(" }");
  }
}

template <is_container T, int outstream = stdout_fileno>
void
printk(const T &str)
{
  if constexpr ( outstream == stdout_fileno ) {
    const auto *start = str.cbegin();

    const auto *end = str.cend();
    printk<T, io::stdout>("{ ");
    for ( ; start != end; ++start ) {
      printk<T, io::stdout>(*start);
      if ( start + 1 != end )
        printk<T, io::stdout>(", ");
    }
    printk<T, io::stdout>(" }");
  } else if constexpr ( outstream == stderr_fileno ) {
    const auto *start = str.cbegin();
    const auto *end = str.cend();
    printk<T, io::stderr>("{ ");
    for ( ; start != end; ++start ) {
      printk<T, io::stderr>(*start);
      if ( start + 1 != end )
        printk<T, io::stderr>(", ");
    }
    printk<T, io::stderr>(" }");
  }
}

// dump binary data
template <is_container T>
void
bin(const T &data)
{
  char print_buffer[4096];     // long enough
  size_t cnt = 0;
  size_t sz = data.size();
  do {
    size_t _rd = (sz - 4096) < 0 ? sz : 4096;
    const char *ptr = format::bytes_to_hex(&data[cnt], _rd, &print_buffer[0]);
    sz = (sz - 4096) < 0 ? 0 : (sz - 4096);
    cnt += _rd;
  } while ( sz );
}

template <has_cstr T>
void
bin(const T &data)
{
  char print_buffer[4096];     // long enough
  size_t cnt = 0;
  size_t sz = data.size();
  do {
    // NOTE: this is _rd to prevent shadowing in posix/file
    size_t _rd = (sz - 4096) < 0 ? sz : 4096;
    const char *ptr = format::bytes_to_hex(const_cast<byte *>(reinterpret_cast<const byte *>(&data[cnt])), _rd, &print_buffer[0]);
    sz = (sz - 4096) < 0 ? 0 : (sz - 4096);
    cnt += _rd;
    io::printk(" 0x");     // formatting it nicely
    io::printk(print_buffer);
    // buffer_flush_stdout();
  } while ( sz );
}

// printing functions
// print = print / no new line
// printk = print / newline after each arg
// println = print / then append newline

void
print_buffer(char **buf [[maybe_unused]])
{
  // if ( buf )
  //   if ( *buf )
  //     set_buffering(buf);
}

inline void
flush(void)
{
  io::fflush(stdout);
  // buffer_flush_stdout();
}

template <typename... T>
void
print(T &&...str)
{
  (printk(str), ...);
  flush();
}

template <typename... T>
void
printn(T &&...str)
{
  (printkn(str), ...);
}

/*template <typename... T>
void
println(T &...str)
{
  (printk(str), ...);
  printk("\n");
}*/
template <typename... T>
inline void
print(const T &...str)
{
  (printk(str), ...);
  flush();
}

template <typename... T>
inline void
printn(const T &...str)
{
  (printkn(str), ...);
}

template <typename... T>
inline void
println(const T &...str)
{
  if constexpr ( sizeof...(T) > 1 ) {
    (printk(str), ...);
    printk("\n");
  } else {
    (printkn(str), ...);
  }
}

template <typename... T>
inline void
println(const T *...str)
{
  if constexpr ( sizeof...(T) > 1 ) {
    (printk(str), ...);
    printk("\n");
  } else {
    (printkn(str), ...);
  }
}

// error functions
// error = print / no new line
// errork = print / newline after each arg
// errorln = print / then append newline

template <typename... T>
void
error(T &...str)
{
  (printk<T..., stderr_fileno>(str), ...);
}

template <typename... T>
void
errorn(T &...str)
{
  (printkn<T..., stderr_fileno>(str), ...);
}

template <typename... T>
void
errorln(T &...str)
{
  (printk<T..., stderr_fileno>(str), ...);
  printk<T..., stderr_fileno>("\n");
}

template <typename... T>
inline void
error(const T &...str)
{
  (printk<T..., stderr_fileno>(str), ...);
}

template <typename... T>
inline void
errorn(const T &...str)
{
  (printkn<T..., stderr_fileno>(str), ...);
}

template <typename... T>
inline void
errorln(const T &...str)
{
  if constexpr ( sizeof...(T) > 1 ) {
    (printk<T..., stderr_fileno>(str), ...);
    printk<stderr_fileno>("\n");
  } else {
    (printkn<T..., stderr_fileno>(str), ...);
  }
}

}     // namespace io
};     // namespace micron
