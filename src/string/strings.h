#pragma once

#include "format.h"
#include "string.h"

namespace micron
{

using str8 = hstring<schar>;
using ustr8 = hstring<unicode8>;
using wstr = hstring<wide>;
using ustr32 = hstring<unicode32>;
using string = hstring<schar>;
template <size_t N> using bstr = sstring<N, byte>;
template <size_t N> using sstr = sstring<N, schar>;
template <size_t N> using wsstring = sstring<N, wide>;
template <size_t N> using utfsstring = sstring<N, unicode8>;
template <size_t N> using utf16sstring = sstring<N, unicode16>;
template <size_t N> using utf32sstring = sstring<N, unicode32>;

inline string
operator+(string &&str, string &&data)
{
  string out;
  out += str;
  out += data;
  return out;
};

template <size_t N, size_t M>
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

template <size_t N, size_t M>
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

template <size_t M>
inline string
operator+(string &str, const char (&data)[M])
{
  string out;
  out += str;
  out += data;
  return out;
};
/*template <size_t M>
inline string
operator+(string &&str, const char (&data)[M])
{
  string out;
  out += str;
  out += data;
  return out;
};
*/
template <size_t M>
inline string
operator+(const char (&data)[M], string &str)
{
  string out;
  out += data;
  out += str;
  return out;
};
template <size_t M>
inline string
operator+(const char (&data)[M], string &&str)
{
  string out;
  out += data;
  out += str;
  return out;
};

template <size_t N>
inline sstr<N>
operator+(sstr<N> &str, const char *data)
{
  sstr<N> out;
  out += str;
  out += data;
  return out;
};
template <size_t N, size_t M>
inline sstr<N>
operator+(const char *data, sstr<N> &str)
{
  sstr<N> out;
  out += data;
  out += str;
  return out;
};
template <size_t N>
inline sstr<N>
operator+(sstr<N> &&str, const char *data)
{
  sstr<N> out;
  out += str;
  out += data;
  return out;
};
template <size_t N, size_t M>
inline sstr<N>
operator+(const char *data, sstr<N> &&str)
{
  sstr<N> out;
  out += data;
  out += str;
  return out;
};

template <size_t N, size_t M>
inline sstr<N>
operator+(sstr<N> &str, const char (&data)[M])
{
  sstr<N> out;
  out += str;
  out += data;
  return out;
};
template <size_t N, size_t M>
inline sstr<N>
operator+(const char (&data)[M], sstr<N> &str)
{
  sstr<N> out;
  out += data;
  out += str;
  return out;
};
/*template <size_t N, size_t M>
inline sstr<N>
operator+(sstr<N> &&str, const char (&data)[M])
{
  sstr<N> out;
  out += str;
  out += data;
  return out;
};*/
template <size_t N, size_t M>
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
template <typename O, size_t N, typename T>
O &
operator<<(O &os, const sstring<N, T> &o)
{
  os << o.c_str();
  return os;
}

template <typename O, size_t N, typename T>
O &
operator<<(O &os, const sstring<N, T> &&o)
{
  os << o.c_str();
  return os;
}
template <typename O, size_t N, typename T>
O &
operator<<(O &os, sstring<N, T> &o)
{
  os << o.c_str();
  return os;
}

template <typename O, size_t N, typename T>
O &
operator<<(O &os, sstring<N, T> &&o)
{
  os << o.c_str();
  return os;
}
};
