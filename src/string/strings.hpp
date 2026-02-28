//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "format.hpp"
#include "string.hpp"

namespace micron
{

using str8 = hstring<schar>;
using ustr8 = hstring<unicode8>;
using wstr = hstring<wide>;
using ustr32 = hstring<unicode32>;
using string = hstring<char>;
template <usize N> using bstr = sstring<N, byte>;
template <usize N> using sstr = sstring<N, schar>;
template <usize N> using wsstring = sstring<N, wide>;
template <usize N> using utfsstring = sstring<N, unicode8>;
template <usize N> using utf16sstring = sstring<N, unicode16>;
template <usize N> using utf32sstring = sstring<N, unicode32>;

template <typename U, typename V>
constexpr bool
lexi_compare(U a, U b, V c, V d)
{
  for ( ; a != b && c != d; ++a, ++c ) {
    if ( *a < *c )
      return true;
    if ( *c < *a )
      return false;
  }
  return (a == b) && (c != d);
}

inline string
operator+(string &&str, string &&data)
{
  string out;
  out += str;
  out += data;
  return out;
};

template <usize N, usize M>
inline string
operator+(sstr<N> &&data, sstr<M> &&str)
{
  string out;
  out += data;
  out += str;
  return out;
};

inline string
operator+(const string &str, const string &data)
{
  string out;
  out += str;
  out += data;
  return out;
};

inline string
operator+(string &str, string &data)
{
  string out;
  out += str;
  out += data;
  return out;
};

template <usize N, usize M>
inline string
operator+(sstr<N> &data, sstr<M> &str)
{
  string out;
  out += data;
  out += str;
  return out;
};

inline string
operator+(string &str, const char *data)
{
  string out;
  out += str;
  out += data;
  return out;
};

inline string
operator+(string &&str, const char *data)
{
  string out;
  out += str;
  out += data;
  return out;
};

inline string
operator+(const char *data, string &str)
{
  string out;
  out += data;
  out += str;
  return out;
};

inline string
operator+(const char *data, string &&str)
{
  string out;
  out += data;
  out += str;
  return out;
};

template <usize M>
inline string
operator+(string &str, const char (&data)[M])
{
  string out;
  out += str;
  out += data;
  return out;
};

/*template <usize M>
inline string
operator+(string &&str, const char (&data)[M])
{
  string out;
  out += str;
  out += data;
  return out;
};
*/
template <usize M>
inline string
operator+(const char (&data)[M], string &str)
{
  string out;
  out += data;
  out += str;
  return out;
};

template <usize M>
inline string
operator+(const char (&data)[M], string &&str)
{
  string out;
  out += data;
  out += str;
  return out;
};

template <usize N>
inline sstr<N>
operator+(sstr<N> &str, const char *data)
{
  sstr<N> out;
  out += str;
  out += data;
  return out;
};

template <usize N, usize M>
inline sstr<N>
operator+(const char *data, sstr<N> &str)
{
  sstr<N> out;
  out += data;
  out += str;
  return out;
};

template <usize N>
inline sstr<N>
operator+(sstr<N> &&str, const char *data)
{
  sstr<N> out;
  out += str;
  out += data;
  return out;
};

template <usize N, usize M>
inline sstr<N>
operator+(const char *data, sstr<N> &&str)
{
  sstr<N> out;
  out += data;
  out += str;
  return out;
};

template <usize N, usize M>
inline sstr<N>
operator+(sstr<N> &str, const char (&data)[M])
{
  sstr<N> out;
  out += str;
  out.append_null(data);
  return out;
};

template <usize N, usize M>
inline sstr<N>
operator+(const char (&data)[M], sstr<N> &str)
{
  sstr<N> out;
  out.append_null(data);
  out += str;
  return out;
};

/*template <usize N, usize M>
inline sstr<N>
operator+(sstr<N> &&str, const char (&data)[M])
{
  sstr<N> out;
  out += str;
  out += data;
  return out;
};*/
template <usize N, usize M>
inline sstr<N>
operator+(const char (&data)[M], sstr<N> &&str)
{
  sstr<N> out;
  out += data;
  out += str;
  return out;
};

/*template <typename O, typename T>
O &
operator<<(O &os, hstring<T> &o)
{
  os << o.c_str();
  return os;
}*/

template <typename O, typename T>
O &
operator<<(O &os, const hstring<T> &&o)
{
  os << o.c_str();
  return os;
}

template <typename O, typename T>
O &
operator<<(O &os, hstring<T> &&o)
{
  os << o.c_str();
  return os;
}

template <typename O, usize N, typename T>
O &
operator<<(O &os, const sstring<N, T> &o)
{
  os << o.c_str();
  return os;
}

template <typename O, usize N, typename T>
O &
operator<<(O &os, const sstring<N, T> &&o)
{
  os << o.c_str();
  return os;
}

template <typename O, usize N, typename T>
O &
operator<<(O &os, sstring<N, T> &o)
{
  os << o.c_str();
  return os;
}

template <typename O, usize N, typename T>
O &
operator<<(O &os, sstring<N, T> &&o)
{
  os << o.c_str();
  return os;
}

template <typename T>
bool
operator<(const hstring<T> &lhs, const hstring<T> &rhs)
{
  return lexi_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T>
bool
operator>(const hstring<T> &lhs, const hstring<T> &rhs)
{
  return lexi_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}
};     // namespace micron
