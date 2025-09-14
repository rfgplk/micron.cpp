//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../except.hpp"
#include "../math/generic.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "string.hpp"

#include "../slice.hpp"

namespace micron
{
constexpr double __string_epsilon = 1e-12;

/*
template <typename T>
inline micron::hstring<T>
to_string(const T *str)
{
  return micron::hstring<T>(str);
};
*/
template <is_string T>
inline auto
c_str(const T &str) -> const char*
{
  return reinterpret_cast<const char*>(&str[0]);
}

namespace format
{

template <typename T>
inline bool
isupper(typename T::iterator t)
{
  if constexpr ( micron::is_same_v<typename T::value_type, char> ) {
    // ascii
    if ( *t <= 0x5A && *t >= 0x41 )
      return true;
    else
      return false;
  }
  return false;
  // TODO: implement utf8
}

template <is_string T>
inline bool
islower(typename T::iterator t)
{
  if constexpr ( micron::is_same_v<typename T::value_type, char> ) {
    // ascii
    if ( *t <= 0x7A && *t >= 0x61 )
      return true;
    else
      return false;
  }
  return false;
  // TODO: implement utf8
}

template <typename T>
inline void
to_upper(typename T::iterator t)
{
  if constexpr ( micron::is_same_v<typename T::value_type, char> ) {
    // ascii
    if ( *t <= 0x7A && *t >= 0x61 )
      *t -= 32;
  }
  // TODO: implement utf8
}

template <typename T>
inline void
to_lower(typename T::iterator t)
{
  if constexpr ( micron::is_same_v<typename T::value_type, char> ) {
    // ascii
    if ( *t <= 0x5A && *t >= 0x41 )
      *t += 32;
  }
  // TODO: implement utf8
}
template <typename T>
  requires micron::is_fundamental_v<T>
inline bool
isupper(const T t)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    // ascii
    if ( t <= 0x5A && t >= 0x41 )
      return true;
    else
      return false;
  }
  return false;
  // TODO: implement utf8
}

template <typename T>
  requires micron::is_fundamental_v<T>
inline bool
islower(const T t)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    // ascii
    if ( t <= 0x7A && t >= 0x61 )
      return true;
    else
      return false;
  }
  return false;
  // TODO: implement utf8
}

template <typename T>
inline T
to_upper(const T t)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    // ascii
    if ( t <= 0x5A && t >= 0x41 )
      return (t - 32);
  }
  return false;
  // TODO: implement utf8
}

template <typename T>
inline bool
to_lower(const T t)
{
  if constexpr ( micron::is_same_v<T, char> ) {
    // ascii
    if ( t <= 0x7A && t >= 0x61 )
      return (t + 32);
  }
  return false;
  // TODO: implement utf8
}
template <is_string T>
T &
casefold(T &str)
{
  auto itr = str.begin();
  while ( itr != str.end() ) {
    if ( isupper<T>(itr) )
      to_lower<T>(itr);
    ++itr;
  }
  return str;
}

template <is_string T>
T &
upper(T &str)
{
  auto itr = str.begin();
  while ( itr != str.end() ) {
    if ( islower<T>(itr) )
      to_upper<T>(itr);
    ++itr;
  }
  return str;
}
template <char Tk = ' ', is_string T>
T &
strip(T &data)
{
  // TODO: OPTIMIZE
  // pythonic strip all leading and trailing tokens, ws by default
  bool flag = true;
  do {
    if ( *data.begin() == Tk )
      data.erase(0);
    else
      flag = false;
  } while ( flag );
  flag = true;
  do {
    if ( *(data.end() - 1) == Tk )
      data.erase(data.end() - 1);
    else
      flag = false;
  } while ( flag );
  return data;
}
template <is_string T>
bool
ends_with(const T &data, const char *fnd)
{
  auto sz = micron::strlen(fnd);
  if ( strcmp(data.end() - sz, &fnd[0]) == 0 )
    return true;
  else
    return false;
}
template <is_string T>
bool
ends_with(const T &data, const T &fnd)
{
  // auto buf = data.substr((data.size()) - fnd.size(), fnd.size());
  if ( strcmp(data.end() - fnd.size() - 1, fnd.begin()) == 0 )
    return true;
  else
    return false;
}

template <is_string T>
bool
starts_with(const T &data, const char *fnd)
{
  auto sz = micron::strlen(fnd);
  auto buf = data.substr(0, sz);
  if ( strcmp(data.begin(), &fnd[0]) == 0 )
    return true;
  else
    return false;
}
template <is_string T>
bool
starts_with(const T &data, const T &fnd)
{
  auto buf = data.substr(0, fnd.size());
  if ( strcmp(data.begin(), fnd.begin()) == 0 )
    return true;
  else
    return false;
}

template <is_string T>
T
concat(const char *lhs, const char *rhs)
{
  T str;
  str += lhs;
  str += rhs;
  return str;
}

hstring<schar>
concat(const char *lhs, const char *rhs)
{
  hstring str;
  str += lhs;
  str += rhs;
  return str;
}

template <size_t N, typename T>
sstring<N, T> &
concat(sstring<N, T> &lhs, const char *rhs)
{
  auto sz = micron::strlen(rhs);
  if ( sz + lhs.size() > lhs.max_size() )
    throw micron::except::library_error("concat range error.");
  auto *p = lhs.begin();
  micron::bytemove(p + sz, p, lhs.size());
  micron::memcpy(p, rhs, sz);
  lhs.set_size(lhs.size() + sz);
  return lhs;
}

template <size_t N, typename T>
sstring<N, T> &
concat(const char *lhs, sstring<N, T> &rhs)
{
  auto sz = micron::strlen(lhs);
  if ( sz + rhs.size() > rhs.max_size() )
    throw micron::except::library_error("concat range error.");
  auto *p = rhs.begin();
  micron::bytemove(p + sz, p, rhs.size());
  micron::memcpy(p, lhs, sz);
  rhs.set_size(rhs.size() + sz);
  return rhs;
}

template <is_string... T>
hstring<schar>
concat(const T &...strs)
{
  hstring<schar> str;
  ((str += strs), ...);
  return str;
}

template <is_string T>
T
split(const T &data, size_t at = 0)
{
  if ( at > data.size() )
    throw micron::except::library_error("micron::split() out of bounds.");
  if ( !at )
    return data.substr(data.size() / 2, data.size() - (data.size() / 2));
  return data.substr(at, data.size() - at);
}

template <is_string T>
T
split(const T &data, typename T::const_iterator itr)
{
  if ( itr >= data.end() or itr < data.begin() )
    throw micron::except::library_error("micron::split() out of bounds.");
  return data.substr(data.begin(), itr);
}

template <is_string T>
auto
contains(const T &data, typename T::const_iterator from, const char fnd) -> bool
{
  if ( data.empty() )
    return false;
  if ( from < data.begin() or from >= data.end() )
    return false;
  for ( auto itr = from; itr != data.end(); ++itr ) {
    if ( *itr == fnd ) {
      return true;
    }
  }
  return false;
}

template <is_string T>
auto
contains(const T &data, typename T::const_iterator from, const char *fnd) -> bool
{
  size_t sz = micron::strlen(fnd);
  if ( sz > data.size() )
    return false;
  if ( data.empty() or sz == 0 )
    return false;
  if ( from < data.begin() or from >= data.end() )
    return false;
  for ( auto itr = from; itr != data.end(); ++itr ) {
    u64 j = 0;
    while ( (itr + j) != data.end() && j <= sz && *(itr + j) == fnd[j] ) {
      ++j;
    }
    if ( j == sz ) {
      return true;
    }
  }
  return false;
}

template <is_string T>
auto
contains(const T &data, const char fnd) -> bool
{
  if ( data.empty() )
    return false;
  for ( auto itr = data.begin(); itr != data.end(); ++itr ) {
    if ( *itr == fnd ) {
      return true;
    }
  }
  return false;
}

template <is_string T>
auto
contains(const T &data, const char *fnd) -> bool
{
  size_t sz = micron::strlen(fnd);
  if ( sz > data.size() )
    return false;
  if ( data.empty() or sz == 0 )
    return false;
  for ( auto itr = data.begin(); itr != data.end(); ++itr ) {
    u64 j = 0;
    while ( (itr + j) != data.end() && j <= sz && *(itr + j) == fnd[j] ) {
      ++j;
    }
    if ( j == sz ) {
      return true;
    }
  }
  return false;
}

template <is_string T>
bool
contains(const T &data, const T &fnd)
{
  auto itr = fnd.begin();
  for ( auto i : fnd ) {
    i32 b = 0;
    auto itr_data = data.begin();
    for ( auto j : data ) {
      if ( i == j )     // hit
      {
        b = 1;
        i32 a = 1;
        for ( size_t k = 0; k < (size_t)(itr - fnd.begin()); k++ ) {
          if ( *(itr + k) != *(itr_data + k) ) {
            a = 0;
            break;
          }
        }
        if ( a )
          return true;
      }
      ++itr_data;
    }
    if ( !b )
      break;
    ++itr;
  }
  return false;
}

template <is_string T>
auto
find(const T &data, const char fnd) -> typename T::iterator
{
  if ( data.empty() )
    return nullptr;
  for ( auto itr = data.begin(); itr != data.end(); ++itr ) {
    if ( *(itr) == fnd ) {
      return itr;
    }
  }
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find(const T &data, const char *fnd) -> typename T::iterator
{
  size_t sz = micron::strlen(fnd);
  if ( sz > data.size() )
    return nullptr;
  if ( data.empty() or sz == 0 )
    return nullptr;
  for ( auto itr = data.begin(); itr != data.end(); ++itr ) {
    u64 j = 0;
    while ( (itr + j) != data.end() && j <= sz && *(itr + j) == fnd[j] ) {
      ++j;
    }
    if ( j == sz ) {
      return itr;
    }
  }
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find(const T &data, typename T::iterator from, const char fnd) -> typename T::iterator
{
  if ( from < data.begin() or from >= data.end() )
    return nullptr;
  if ( !from )
    return nullptr;
  if ( data.empty() )
    return nullptr;
  for ( auto itr = from; itr != data.end(); ++itr ) {
    if ( *(itr) == fnd ) {
      return itr;
    }
  }
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find(const T &data, typename T::iterator from, const T &fnd) -> typename T::iterator
{
  if ( from < data.begin() or from >= data.end() )
    return nullptr;
  if ( !from )
    return nullptr;
  size_t sz = fnd.size();
  if ( sz > data.size() )
    return nullptr;
  if ( data.empty() or sz == 0 )
    return nullptr;
  for ( auto itr = from; itr != data.end(); ++itr ) {
    u64 j = 0;
    while ( (itr + j) != data.end() && j <= sz && *(itr + j) == fnd[j] ) {
      ++j;
    }
    if ( j == sz ) {
      return itr;
    }
  }
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find(const T &data, typename T::iterator from, const char *fnd) -> typename T::iterator
{
  if ( from < data.begin() or from >= data.end() )
    return nullptr;
  if ( !from )
    return nullptr;
  size_t sz = micron::strlen(fnd);
  if ( sz > data.size() )
    return nullptr;
  if ( data.empty() or sz == 0 )
    return nullptr;
  for ( auto itr = from; itr != data.end(); ++itr ) {
    u64 j = 0;
    while ( (itr + j) != data.end() && j <= sz && *(itr + j) == fnd[j] ) {
      ++j;
    }
    if ( j == sz ) {
      return itr;
    }
  }
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find_reverse(const T &data, typename T::iterator from, const T &fnd) -> typename T::iterator
{
  if ( from < data.begin() or from >= data.end() )
    return nullptr;
  if ( !from )
    return nullptr;
  size_t sz = fnd.size();
  if ( sz > data.size() )
    return nullptr;
  if ( data.empty() or sz == 0 )
    return nullptr;
  for ( auto itr = from; itr != data.begin(); --itr ) {
    u64 j = 0;
    while ( (itr + j) != data.begin() && j <= sz && *(itr + j) == fnd[j] ) {
      ++j;
    }
    if ( j == sz ) {
      return itr;
    }
  }
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find_reverse(const T &data, typename T::iterator from, const char *fnd) -> typename T::iterator
{
  if ( from < data.begin() or from >= data.end() )
    return nullptr;
  if ( !from )
    return nullptr;
  size_t sz = micron::strlen(fnd);
  if ( sz > data.size() )
    return nullptr;
  if ( data.empty() or sz == 0 )
    return nullptr;
  for ( auto itr = from; itr != data.begin(); --itr ) {
    u64 j = 0;
    while ( (itr + j) != data.begin() && j <= sz && *(itr + j) == fnd[j] ) {
      ++j;
    }
    if ( j == sz ) {
      return itr;
    }
  }
  return (typename T::iterator) nullptr;
}

template <is_string T>
auto
find(const T &data, const T &fnd) -> typename T::iterator
{
  if ( fnd.size() > data.size() )
    return nullptr;
  if ( data.empty() or fnd.empty() )
    return nullptr;
  for ( auto itr = data.begin(); itr != data.end(); ++itr ) {
    u64 j = 0;
    while ( (itr + j) != data.end() && j <= fnd.size() && *(itr + j) == fnd[j] ) {
      ++j;
    }
    if ( j == fnd.size() ) {
      return itr;
    }
  }
  return nullptr;
}

template <is_string T>
T &
replace(T &str, const char *lhs, const char *rhs)
{
  auto sz = str.size();
  auto szl = micron::strlen(lhs);
  auto szr = micron::strlen(rhs);
  if ( (szr - szl) + sz > str.max_size() )
    throw micron::except::library_error("concat range error.");
  typename T::iterator itr = find(str, lhs);
  micron::memcpy(itr, rhs, szr);
  micron::bytemove(itr + szr, itr + szl, sz - ((szr - szl) > 0 ? (szr - szl) : 0));
  str.set_size(str.size() - ((szr - szl) > 0 ? (szr - szl) : 0));
  return str;
}

template <is_string T>
T &
replace_all(T &str, const char *lhs, const char *rhs)
{
  auto sz = str.size();
  auto szl = micron::strlen(lhs);
  auto szr = micron::strlen(rhs);
  if ( (szr - szl) + sz > str.max_size() )
    throw micron::except::library_error("concat range error.");
  typename T::iterator itr = find(str, lhs);
  while ( itr != nullptr ) {
    sz = str.size();
    micron::memcpy(itr, rhs, szr);
    micron::bytemove(itr + szr, itr + szl, sz - ((szr - szl) > 0 ? (szr - szl) : 0));
    str.set_size(str.size() - math::abs(szl - szr));
    itr = find(str, lhs);
  }
  return str;
}

template <typename T>
f32
to_float(const T &o)
{
  f32 res = 0.0f;
  f32 div = 1.0f;
  bool fl_dec = false;

  i64 c = 0;
  const auto *buf = o.cdata();
  while ( *(buf + c) ) {
    if ( *(buf + c) == ' ' && res < __string_epsilon ) {
      ++c;
      continue;
    }
    if ( *(buf + c) == ',' ) {
      fl_dec = true;
    } else if ( fl_dec ) {
      div *= 10.0f;
      res += (*(buf + c) - '0') / div;
    } else {
      res = res * 10.0f + (*(buf + c) - '0');
    }
    ++c;
  }
  return res;
}
template <typename T>
f64
to_double(const T &o)
{
  f64 res = 0.0f;
  f64 div = 1.0f;
  bool fl_dec = false;

  i64 c = 0;
  const auto *buf = o.cdata();
  while ( *(buf + c) ) {
    if ( *(buf + c) == ' ' && res < __string_epsilon ) {
      ++c;
      continue;
    }
    if ( *(buf + c) == ',' ) {
      fl_dec = true;
    } else if ( fl_dec ) {
      div *= 10.0f;
      res += (*(buf + c) - '0') / div;
    } else {
      res = res * 10.0f + (*(buf + c) - '0');
    }
    ++c;
  }
  return res;
}

template <typename T>
i32
to_integer(const T &o)
{
  i32 t = 0;
  i64 c = 0;
  const auto *buf = o.cdata();
  while ( *(buf + c) ) {
    if ( *(buf + c) == ' ' && t == 0 ) {
      ++c;
      continue;
    }
    t = t * 10 + (*(buf + c) - '0');
    ++c;
  }
  return t;
}

template <typename R, is_string T>
R *
to_pointer_addr(typename T::iterator start, typename T::iterator end, u32 base = 16)
{
  u64 result = 0;
  int i = 0;
  while ( start[i] == ' ' || start[i] == '\t' )
    i++;

  if ( base == 16 && start[i] == '0' && (start[i + 1] == 'x' || start[i + 1] == 'X') ) {
    i += 2;     // Skip "0x" or "0X" for hexadecimal
  }

  while ( start[i] != '\0' ) {
    u32 digit;
    if ( start[i] >= '0' && start[i] <= '9' ) {
      digit = start[i] - '0';
    } else if ( base > 10 && start[i] >= 'A' && start[i] <= 'F' ) {
      digit = start[i] - 'A' + 10;
    } else if ( base > 10 && start[i] >= 'a' && start[i] <= 'f' ) {
      digit = start[i] - 'a' + 10;
    } else if ( start + i == end ) {
      break;
    } else {
      break;     // Invalid character for the given base
    }

    if ( digit >= base )
      break;     // Out of range digit

    result = result * base + digit;
    i++;
  }
  return reinterpret_cast<R *>(reinterpret_cast<u64 *>(result));
}
template <typename T>
i64
to_long(const T &o)
{
  i64 t = 0;
  i64 c = 0;
  const auto *buf = o.cdata();
  while ( *(buf + c) ) {
    if ( *(buf + c) == ' ' && t == 0 ) {
      ++c;
      continue;
    }
    t = t * 10 + (*(buf + c) - '0');
    ++c;
  }
  return t;
}
};     // namespace format
};     // namespace micron

// void string_to_literal(){};
// i32 string_to_integer(){};
// i64 string_to_long(){};
// byte string_to_bytes(){};
// f32 string_to_float(){};
// f64 string_to_double(){};
// template <typename F = char>
// string<F> to_string(){};
