//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <concepts>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <locale.h>

#include "../memory/memory.hpp"
#include "../types.hpp"
#include "impl.hpp"
#include "io.hpp"

namespace micron
{
namespace io
{

template <typename T>
concept is_container = requires(T t) {
  { t.cbegin() } -> std::same_as<typename T::const_iterator>;
  { t.cend() } -> std::same_as<typename T::const_iterator>;
  { t.begin() } -> std::same_as<typename T::iterator>;
  { t.end() } -> std::same_as<typename T::iterator>;
} && !requires(T t) {
  { t.c_str() } -> std::same_as<const char *>;
};

// PRINTK BLOCK OF FUNCS
template <int outstream = STDOUT_FILENO>
inline void
printk(const char *c)
{
  if constexpr ( outstream == STDOUT_FILENO )
    fput(c, stdout);
  else if constexpr ( outstream == STDERR_FILENO )
    fput(c, stderr);
}
template <int outstream = STDOUT_FILENO>
inline void
sprintk(const char *c, size_t len)
{
  if constexpr ( outstream == STDOUT_FILENO )
    fput(c, len, stdout);
  else if constexpr ( outstream == STDERR_FILENO )
    fput(c, len, stderr);
}     // raw print

// this is necessary because strings provide all three methods, will cause
// ambiguity
template <typename T>
concept has_cstr = requires(T t) {
  { t.c_str() } -> std::same_as<const char *>;
};
// must be a class that provides .c_str()
template <has_cstr T, int outstream = STDOUT_FILENO>
  requires std::is_class_v<T>
void
printk(const T &str)
{
  if constexpr ( outstream == STDOUT_FILENO )
    fput(str.c_str(), stdout);
  else if constexpr ( outstream == STDERR_FILENO )
    fput(str.c_str(), stderr);
}

// if type is a pointer, or, pointerlike
template <typename T, int outstream = STDOUT_FILENO>
  requires std::is_pointer_v<T>
void
printk(const T &x)
{
  char print_buffer[32];     // long enough, overkill
  const char *ptr = format::ptr_to_char(x, &print_buffer[0]);
  if constexpr ( outstream == STDOUT_FILENO )
    fput(ptr, stdout);
  else if constexpr ( outstream == STDERR_FILENO )
    fput(ptr, stderr);
  return;
}

// if type is arithmetic or not
template <typename T, int outstream = STDOUT_FILENO>
  requires std::is_arithmetic_v<T>
void
printk(const T &x)
{
  char print_buffer[128];     // long enough, overkill
  if constexpr ( std::same_as<T, bool> ) {
    const char *ptr = format::bool_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, i8> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, u8> ) {
    const char *ptr = format::uint_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, i16> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, u16> ) {
    const char *ptr = format::uint_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, int> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, uint> ) {
    const char *ptr = format::uint_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, long> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, unsigned long> ) {
    const char *ptr = format::uint64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, char> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, unsigned char> ) {
    const char *ptr = format::uint_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, short> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, unsigned short> ) {
    const char *ptr = format::uint_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, long long> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, unsigned long long> ) {
    const char *ptr = format::uint64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, i32> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, u32> or std::same_as<T, unsigned> ) {
    const char *ptr = format::uint_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, i64> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, u64> ) {
    const char *ptr = format::uint64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  // just in case it's different from u64
  if constexpr ( std::same_as<T, max_t> or std::same_as<T, ssize_t> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, umax_t> or std::same_as<T, size_t> ) {
    const char *ptr = format::uint64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, flong> ) {
    const char *ptr = format::flong_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, f128> ) {
    const char *ptr = format::f128_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, f64> ) {
    const char *ptr = format::f64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }

  if constexpr ( std::same_as<T, double> ) {
    const char *ptr = format::f128_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, float> ) {
    const char *ptr = format::f32_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  if constexpr ( std::same_as<T, f32> ) {
    const char *ptr = format::f32_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO )
      fput(ptr, stdout);
    else if constexpr ( outstream == STDERR_FILENO )
      fput(ptr, stderr);
    return;
  }
  const char *err_str = "You tried to output an unknown/unformattable "
                        "type. Try using bin()";
  fput(err_str, stderr);
  return;
}

// spec case, single byte
template <typename T, int outstream = STDOUT_FILENO>
  requires std::is_same_v<T, byte> && (sizeof(T) == 1)
void printk(T &x)
{
  byte c[2];
  c[0] = x;
  c[1] = 0x0;
  if constexpr ( outstream == STDOUT_FILENO )
    fput(reinterpret_cast<const char *>(c), stdout);
  else if constexpr ( outstream == STDERR_FILENO )
    fput(reinterpret_cast<const char *>(c), stderr);
}
// non null trm string
template <typename T, int outstream = STDOUT_FILENO>
  requires std::is_same_v<T, byte *>
void
printk(T ptr, size_t len)
{
  byte *null_trm = new byte[len + 1];
  micron::memcpy(null_trm, ptr, len);
  null_trm[len] = 0x0;
  if constexpr ( outstream == STDOUT_FILENO )
    fput(reinterpret_cast<const char *>(null_trm), stdout);
  else if constexpr ( outstream == STDERR_FILENO )
    fput(reinterpret_cast<const char *>(null_trm), stderr);
  delete[] null_trm;
}

// PRINTKN BLOCK OF FUNCS, APPENDS NEWLINE
template <int outstream = STDOUT_FILENO>
inline void
printkn(const char *c)
{
  if constexpr ( outstream == STDOUT_FILENO ) {
    fput(c, stdout);
    fput("\n", stdout);
  } else if constexpr ( outstream == STDERR_FILENO ) {
    fput(c, stderr);
    fput("\n", stderr);
  }
}

template <typename T, int outstream = STDOUT_FILENO, size_t M>
inline void
printk(const char (&c)[M])
{
  if constexpr ( outstream == STDOUT_FILENO ) {
    fput(c, M, stdout);
    fput("\n", stdout);
  } else if constexpr ( outstream == STDERR_FILENO ) {
    fput(c, M, stderr);
    fput("\n", stderr);
  }
}
template <typename T, int outstream = STDOUT_FILENO, size_t M>
inline void
printkn(const char (&c)[M])
{
  if constexpr ( outstream == STDOUT_FILENO ) {
    fput(c, M, stdout);
    fput("\n", stdout);
  } else if constexpr ( outstream == STDERR_FILENO ) {
    fput(c, M, stderr);
    fput("\n", stderr);
  }
}
template <int outstream = STDOUT_FILENO>
inline void
sprintkn(const char *c, size_t len)
{
  if constexpr ( outstream == STDOUT_FILENO ) {
    fput(c, len, stdout);
    fput("\n", stdout);
  } else if constexpr ( outstream == STDERR_FILENO ) {
    fput(c, len, stderr);
    fput("\n", stderr);
  }
}
// raw print
// must be a class that provides .c_str()
template <has_cstr T, int outstream = STDOUT_FILENO>
  requires (std::is_class_v<T>) && (std::same_as<typename T::value_type, char8_t>)
void
printkn(const T &str)
{
  if constexpr ( outstream == STDOUT_FILENO ) {
    fput(str.c_str(), stdout);
    fput("\n", stdout);
  } else if constexpr ( outstream == STDERR_FILENO ) {
    fput(str.c_str(), stderr);
    fput("\n", stderr);
  }
}

template <has_cstr T, int outstream = STDOUT_FILENO>
  requires (std::is_class_v<T>) && (std::same_as<typename T::value_type, char>)
void
printkn(const T &str)
{
  if constexpr ( outstream == STDOUT_FILENO ) {
    fput(str.c_str(), stdout);
    fput("\n", stdout);
  } else if constexpr ( outstream == STDERR_FILENO ) {
    fput(str.c_str(), stderr);
    fput("\n", stderr);
  }
}

template <has_cstr T, int outstream = STDOUT_FILENO>
  requires (std::is_class_v<T>) && (std::same_as<typename T::value_type, wchar_t>)
void
printkn(const T &str)
{
  if constexpr ( outstream == STDOUT_FILENO ) {
    wfput(str.w_str(), stdout);
    fput("\n", stdout);
  } else if constexpr ( outstream == STDERR_FILENO ) {
    wfput(str.w_str(), stderr);
    fput("\n", stderr);
  }
}

template <has_cstr T, int outstream = STDOUT_FILENO>
  requires (std::is_class_v<T>) && (std::same_as<typename T::value_type, char32_t>)
void
printkn(const T &str)
{
  if constexpr ( outstream == STDOUT_FILENO ) {
    unifput(str.uni_str(), stdout);
    fput("\n", stdout);
  } else if constexpr ( outstream == STDERR_FILENO ) {
    unifput(str.uni_str(), stderr);
    fput("\n", stderr);
  }
}
// if type is a pointer, or, pointerlike
template <typename T, int outstream = STDOUT_FILENO>
  requires std::is_pointer_v<T>
void
printkn(const T &x)
{
  char print_buffer[32];     // long enough, overkill
  const char *ptr = format::ptr_to_char(x, &print_buffer[0]);
  if constexpr ( outstream == STDOUT_FILENO ) {
    fput(ptr, stdout);
    fput("\n", stdout);
  } else if constexpr ( outstream == STDERR_FILENO ) {
    fput(ptr, stderr);
    fput("\n", stderr);
  }
  return;
}

// if type is arithmetic or not
template <typename T, int outstream = STDOUT_FILENO>
  requires std::is_arithmetic_v<T>
void
printkn(const T &x)
{
  char print_buffer[128];     // long enough, overkill
  if constexpr ( std::same_as<T, bool> ) {
    const char *ptr = format::bool_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO ) {
      fput(ptr, stdout);
      fput("\n", stdout);
    } else if constexpr ( outstream == STDERR_FILENO ) {
      fput(ptr, stderr);
      fput("\n", stderr);
    }
    return;
  }
  if constexpr ( std::same_as<T, i8> or std::same_as<T, u8> ) {
    const char *ptr = format::int_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO ) {
      fput(ptr, stdout);
      fput("\n", stdout);
    } else if constexpr ( outstream == STDERR_FILENO ) {
      fput(ptr, stderr);
      fput("\n", stderr);
    }
    return;
  }
  if constexpr ( std::same_as<T, i16> or std::same_as<T, u16> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO ) {
      fput(ptr, stdout);
      fput("\n", stdout);
    } else if constexpr ( outstream == STDERR_FILENO ) {
      fput(ptr, stderr);
      fput("\n", stderr);
    }
    return;
  }
  if constexpr ( std::same_as<T, i32> or std::same_as<T, u32> or std::same_as<T, unsigned> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO ) {
      fput(ptr, stdout);
      fput("\n", stdout);
    } else if constexpr ( outstream == STDERR_FILENO ) {
      fput(ptr, stderr);
      fput("\n", stderr);
    }
    return;
  }
  if constexpr ( std::same_as<T, i64> or std::same_as<T, u64> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO ) {
      fput(ptr, stdout);
      fput("\n", stdout);
    } else if constexpr ( outstream == STDERR_FILENO ) {
      fput(ptr, stderr);
      fput("\n", stderr);
    }
    return;
  }
  // just in case it's different from u64
  if constexpr ( std::same_as<T, max_t> or std::same_as<T, umax_t> ) {
    const char *ptr = format::int64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO ) {
      fput(ptr, stdout);
      fput("\n", stdout);
    } else if constexpr ( outstream == STDERR_FILENO ) {
      fput(ptr, stderr);
      fput("\n", stderr);
    }
    return;
  }
  if constexpr ( std::same_as<T, flong> ) {
    const char *ptr = format::flong_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO ) {
      fput(ptr, stdout);
      fput("\n", stdout);
    } else if constexpr ( outstream == STDERR_FILENO ) {
      fput(ptr, stderr);
      fput("\n", stderr);
    }
    return;
  }
  if constexpr ( std::same_as<T, f128> ) {
    const char *ptr = format::f128_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO ) {
      fput(ptr, stdout);
      fput("\n", stdout);
    } else if constexpr ( outstream == STDERR_FILENO ) {
      fput(ptr, stderr);
      fput("\n", stderr);
    }
    return;
  }
  if constexpr ( std::same_as<T, f64> ) {
    const char *ptr = format::f64_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO ) {
      fput(ptr, stdout);
      fput("\n", stdout);
    } else if constexpr ( outstream == STDERR_FILENO ) {
      fput(ptr, stderr);
      fput("\n", stderr);
    }
    return;
  }
  if constexpr ( std::same_as<T, double> ) {
    const char *ptr = format::f128_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO ) {
      fput(ptr, stdout);
      fput("\n", stdout);
    } else if constexpr ( outstream == STDERR_FILENO ) {
      fput(ptr, stderr);
      fput("\n", stderr);
    }
    return;
  }
  if constexpr ( std::same_as<T, float> ) {
    const char *ptr = format::f32_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO ) {
      fput(ptr, stdout);
      fput("\n", stdout);
    } else if constexpr ( outstream == STDERR_FILENO ) {
      fput(ptr, stderr);
      fput("\n", stderr);
    }
    return;
  }
  if constexpr ( std::same_as<T, f32> ) {
    const char *ptr = format::f32_to_char(x, &print_buffer[0]);
    if constexpr ( outstream == STDOUT_FILENO ) {
      fput(ptr, stdout);
      fput("\n", stdout);
    } else if constexpr ( outstream == STDERR_FILENO ) {
      fput(ptr, stderr);
      fput("\n", stderr);
    }
    return;
  }
  const char *err_str = "You tried to output an unknown/unformattable "
                        "type. Try using bin()\n";
  fput(err_str, stderr);
  return;
}

// spec case, single byte
template <typename T, int outstream = STDOUT_FILENO>
  requires std::is_same_v<T, byte> && (sizeof(T) == 1)
void printkn(T &x)
{
  byte c[3];
  c[0] = x;
  c[1] = '\n';
  c[2] = 0x0;
  if constexpr ( outstream == STDOUT_FILENO )
    fput(reinterpret_cast<const char *>(c), stdout);
  else if constexpr ( outstream == STDERR_FILENO )
    fput(reinterpret_cast<const char *>(c), stderr);
}
// non null trm string
template <typename T, int outstream = STDOUT_FILENO>
  requires std::is_same_v<T, byte *>
void
printkn(T ptr, size_t len)
{
  byte *null_trm = new byte[len + 2];
  micron::memcpy(null_trm, ptr, len);
  null_trm[len - 1] = '\n';
  null_trm[len] = 0x0;
  if constexpr ( outstream == STDOUT_FILENO )
    fput(reinterpret_cast<const char *>(null_trm), stdout);
  else if constexpr ( outstream == STDERR_FILENO )
    fput(reinterpret_cast<const char *>(null_trm), stderr);
  delete[] null_trm;
}
template <is_container T, int outstream = STDOUT_FILENO>
void
printkn(T &&str)
{
  if constexpr ( outstream == STDOUT_FILENO ) {
    auto start = str.cbegin();
    auto end = str.cend();
    printk<STDOUT_FILENO>("{ ");
    for ( ; start != end; ++start ) {
      printk<T, stdout>(*start);
      if ( start + 1 != end )
        printk<STDOUT_FILENO>(", ");
    }
    printk<STDOUT_FILENO>(" }");
    printk<STDOUT_FILENO>("\n");
  } else if constexpr ( outstream == STDERR_FILENO ) {
    auto start = str.cbegin();
    auto end = str.cend();
    printk<STDERR_FILENO>("{ ");
    for ( ; start != end; ++start ) {
      printk<T, STDERR_FILENO>(*start);
      if ( start + 1 != end )
        printk<STDERR_FILENO>(", ");
    }
    printk<STDERR_FILENO>(" }");
    printk<STDERR_FILENO>("\n");
  }
}
template <is_container T, int outstream = STDOUT_FILENO>
void
printkn(const T &str)
{
  if constexpr ( outstream == STDOUT_FILENO ) {
    auto start = str.cbegin();

    auto end = str.cend();
    printk<STDOUT_FILENO>("{ ");
    for ( ; start != end; ++start ) {
      printk(*start);
      if ( start + 1 != end )
        printk<STDOUT_FILENO>(", ");
    }
    printk<STDOUT_FILENO>(" }");
    printk<STDOUT_FILENO>("\n");
  } else if constexpr ( outstream == STDERR_FILENO ) {
    const auto *start = str.cbegin();
    const auto *end = str.cend();
    printk<STDERR_FILENO>("{ ");
    for ( ; start != end; ++start ) {
      printk(*start);
      if ( start + 1 != end )
        printk<STDERR_FILENO>(", ");
    }
    printk<STDERR_FILENO>(" }");
    printk<STDERR_FILENO>("\n");
  }
}
template <is_container T, int outstream = STDOUT_FILENO>
void
printk(T &&str)
{
  if constexpr ( outstream == STDOUT_FILENO ) {
    const auto *start = str.cbegin();
    const auto *end = str.cend();
    printk<STDOUT_FILENO>("{ ");
    for ( ; start != end; ++start ) {
      printk(*start);
      if ( start + 1 != end )
        printk<STDOUT_FILENO>(", ");
    }
    printk<STDOUT_FILENO>(" }");
  } else if constexpr ( outstream == STDERR_FILENO ) {
    const auto *start = str.cbegin();
    const auto *end = str.cend();
    printk<STDERR_FILENO>("{ ");
    for ( ; start != end; ++start ) {
      printk(*start);
      if ( start + 1 != end )
        printk<STDERR_FILENO>(", ");
    }
    printk<STDERR_FILENO>(" }");
  }
}
template <is_container T, int outstream = STDOUT_FILENO>
void
printk(const T &str)
{
  if constexpr ( outstream == STDOUT_FILENO ) {
    const auto *start = str.cbegin();

    const auto *end = str.cend();
    printk<T, stdout>("{ ");
    for ( ; start != end; ++start ) {
      printk<T, stdout>(*start);
      if ( start + 1 != end )
        printk<T, stdout>(", ");
    }
    printk<T, stdout>(" }");
  } else if constexpr ( outstream == STDERR_FILENO ) {
    const auto *start = str.cbegin();
    const auto *end = str.cend();
    printk<T, stderr>("{ ");
    for ( ; start != end; ++start ) {
      printk<T, stderr>(*start);
      if ( start + 1 != end )
        printk<T, stderr>(", ");
    }
    printk<T, stderr>(" }");
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
    const char *ptr
        = format::bytes_to_hex(const_cast<byte *>(reinterpret_cast<const byte *>(&data[cnt])), _rd, &print_buffer[0]);
    sz = (sz - 4096) < 0 ? 0 : (sz - 4096);
    cnt += _rd;
    io::printk(" 0x");     // formatting it nicely
    io::printk(print_buffer);
    buffer_flush_stdout();
  } while ( sz );
}

// printing functions
// print = print / no new line
// printk = print / newline after each arg
// println = print / then append newline

void
print_buffer(char **buf)
{
  if ( buf )
    if ( *buf )
      set_buffering(buf);
}
inline void
flush(void)
{
  buffer_flush_stdout();
}

template <typename... T>
void
print(T &...str)
{
  (printk(str), ...);
  flush();
}
template <typename... T>
void
printn(T &...str)
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

// error functions
// error = print / no new line
// errork = print / newline after each arg
// errorln = print / then append newline

template <typename... T>
void
error(T &...str)
{
  (printk<T..., STDERR_FILENO>(str), ...);
}
template <typename... T>
void
errorn(T &...str)
{
  (printkn<T..., STDERR_FILENO>(str), ...);
}
template <typename... T>
void
errorln(T &...str)
{
  (printk<T..., STDERR_FILENO>(str), ...);
  printk<T..., STDERR_FILENO>("\n");
}
template <typename... T>
inline void
error(const T &...str)
{
  (printk<T..., STDERR_FILENO>(str), ...);
}
template <typename... T>
inline void
errorn(const T &...str)
{
  (printkn<T..., STDERR_FILENO>(str), ...);
}
template <typename... T>
inline void
errorln(const T &...str)
{
  if constexpr ( sizeof...(T) > 1 ) {
    (printk<T..., STDERR_FILENO>(str), ...);
    printk<STDERR_FILENO>("\n");
  } else {
    (printkn<T..., STDERR_FILENO>(str), ...);
  }
}

}     // namespace io
};     // namespace micron
