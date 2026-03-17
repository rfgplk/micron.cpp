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

#include "../tuple.hpp"

#include "__std.hpp"

#include "../string/conversions/bits.hpp"
#include "../string/conversions/floating_point.hpp"
#include "../string/format.hpp"

namespace micron
{
namespace io
{

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// concepts

template <typename T>
concept is_container = requires(T t) {
  { t.cbegin() } -> micron::same_as<typename T::const_iterator>;
  { t.cend() } -> micron::same_as<typename T::const_iterator>;
  { t.begin() } -> micron::same_as<typename T::iterator>;
  { t.end() } -> micron::same_as<typename T::iterator>;
} && !requires(T t) {
  { t.c_str() } -> micron::same_as<const char *>;
};

template <typename T>
concept has_cstr = requires(T t) {
  { t.c_str() } -> micron::same_as<const char *>;
};

// is_map: detects hopscotch_map, robin_map, stack_swiss_map, dictionary_t
template <typename T>
concept is_map = requires {
  typename micron::remove_cvref_t<T>::category_type;
  requires micron::is_same_v<typename micron::remove_cvref_t<T>::category_type, map_tag>;
} || requires(micron::remove_cvref_t<T> t) {
  { t.size() } -> micron::convertible_to<usize>;
  { t.capacity() } -> micron::convertible_to<usize>;
  { t.empty() } -> micron::convertible_to<bool>;
  { t.clear() };
  { t.begin() };
  { t.end() };
};

// is_tagged_map: has category_type = map_tag (hopscotch, robin)
template <typename T>
concept is_tagged_map = requires {
  typename micron::remove_cvref_t<T>::category_type;
  requires micron::is_same_v<typename micron::remove_cvref_t<T>::category_type, map_tag>;
};

// is_swiss_map: stack_swiss_map: iterator already skips empty/deleted,
template <typename T>
concept is_swiss_map = requires(micron::remove_cvref_t<T> t) {
  { t.capacity() } -> micron::convertible_to<usize>;
  { t.size() } -> micron::convertible_to<usize>;
  { t.begin() };
  { t.end() };
  { (*t.begin()).a };
  { (*t.begin()).b };
} && !is_tagged_map<T> && !has_cstr<T>;

template <typename T>
concept is_char_ptr
    = micron::is_same_v<T, char *> || micron::is_same_v<T, schar *> || micron::is_same_v<T, wide *> || micron::is_same_v<T, unicode8 *>
      || micron::is_same_v<T, unicode32 *> || micron::is_same_v<T, const char *> || micron::is_same_v<T, const schar *>
      || micron::is_same_v<T, const wide *> || micron::is_same_v<T, const unicode8 *> || micron::is_same_v<T, const unicode32 *>;

template <typename T>
concept is_char_elem
    = micron::is_same_v<T, char> || micron::is_same_v<T, schar> || micron::is_same_v<T, wide> || micron::is_same_v<T, unicode8>
      || micron::is_same_v<T, unicode32> || micron::is_same_v<T, const char> || micron::is_same_v<T, const schar>
      || micron::is_same_v<T, const wide> || micron::is_same_v<T, const unicode8> || micron::is_same_v<T, const unicode32>;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// stream emit helpers

namespace __impl
{

template <int outstream>
inline void
emit(const char *s)
{
  if constexpr ( outstream == stdout_fileno )
    io::fput(s, io::stdout);
  else
    io::fput(s, io::stderr);
}

template <int outstream>
inline void
emit(const char *s, usize len)
{
  if constexpr ( outstream == stdout_fileno )
    io::fput(s, len, io::stdout);
  else
    io::fput(s, len, io::stderr);
}

template <int outstream>
inline void
emit(char c)
{
  if constexpr ( outstream == stdout_fileno )
    io::fput(c, io::stdout);
  else
    io::fput(c, io::stderr);
}

template <int outstream>
inline void
emit_nl()
{
  emit<outstream>("\n");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// buffers

inline usize
arith_to_buf(char *buf, usize buf_sz, i64 val)
{
  return micron::format::__impl::fmt_int_to_buf(buf, buf_sz, val, 10, false);
}

inline usize
arith_to_buf(char *buf, usize buf_sz, u64 val)
{
  return micron::format::__impl::fmt_uint_to_buf(buf, buf_sz, val, 10, false);
}

inline usize
arith_to_buf(char *buf, [[maybe_unused]]usize buf_sz, f32 val)
{
  return micron::__impl::__ryu::__f32::f2s_buffered(val, buf);
}

inline usize
arith_to_buf(char *buf, [[maybe_unused]]usize buf_sz, f64 val)
{
  return micron::__impl::__ryu::d2s_buffered(val, buf);
}

inline usize
arith_to_buf(char *buf, [[maybe_unused]]usize buf_sz, long double val)
{
  return micron::__impl::__ryu::d2s_buffered(static_cast<f64>(val), buf);
}

inline usize
arith_to_buf(char *buf, usize buf_sz, bool val)
{
  return micron::format::__impl::bool_to_buf(buf, buf_sz, val);
}

inline usize
ptr_to_buf(char *buf, usize buf_sz, const void *ptr)
{
  return micron::format::__impl::ptr_to_buf(buf, buf_sz, ptr);
}

};     // namespace __impl

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// forward declarations (needed for recursive container/map/pair printing)

template <typename T, int outstream>
  requires micron::is_arithmetic_v<T>
void printk(const T &x);

template <has_cstr T, int outstream>
  requires micron::is_class_v<T>
void printk(const T &str);

template <int outstream, typename T, usize M>
  requires is_char_elem<T>
inline void printk(T (&c)[M]);

template <int outstream> inline void printk(char c);

template <int outstream, typename T>
  requires is_char_ptr<T>
inline void printk(const T &c);

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: char array literal

template <int outstream = stdout_fileno, typename T, usize M>
  requires is_char_elem<T>
inline void
printk(T (&c)[M])
{
  __impl::emit<outstream>(reinterpret_cast<const char *>(c));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: char pointer

template <int outstream = stdout_fileno, typename T>
  requires is_char_ptr<T>
inline void
printk(const T &c)
{
  __impl::emit<outstream>(reinterpret_cast<const char *>(c));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: single char

template <int outstream = stdout_fileno>
inline void
printk(char c)
{
  __impl::emit<outstream>(c);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sprintk: raw print with explicit length

template <int outstream = stdout_fileno>
inline void
sprintk(const char *c, usize len)
{
  __impl::emit<outstream>(c, len);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: class with .c_str() (strings)

template <has_cstr T, int outstream = stdout_fileno>
  requires micron::is_class_v<T>
void
printk(const T &str)
{
  __impl::emit<outstream>(str.c_str());
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: non-char pointer (prints address)

template <typename T, int outstream = stdout_fileno>
  requires(micron::is_pointer_v<T> && !is_char_ptr<T>)
void
printk(const T &x)
{
  char buf[24];
  usize n = __impl::ptr_to_buf(buf, 24, static_cast<const void *>(x));
  buf[n] = '\0';
  __impl::emit<outstream>(buf);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printks
//
// for all types:
//   bool
//   char, signed char, unsigned char
//   wchar_t, char8_t, char16_t, char32_t
//   short, unsigned short
//   int, unsigned int
//   long, unsigned long
//   long long, unsigned long long
//   i8, u8, i16, u16, i32, u32, i64, u64
//   max_t, umax_t, usize
//   float, double, long double
//   f32, f64, flong, f128

template <typename T, int outstream = stdout_fileno>
  requires micron::is_arithmetic_v<T>
void
printk(const T &x)
{
  char buf[128];
  usize n = 0;

  if constexpr ( micron::is_same_v<T, bool> ) {
    n = __impl::arith_to_buf(buf, 128, x);
  } else if constexpr ( micron::is_same_v<T, f32> || micron::is_same_v<T, float> ) {
    n = __impl::arith_to_buf(buf, 128, static_cast<f32>(x));
  } else if constexpr ( micron::is_same_v<T, f64> || micron::is_same_v<T, double> ) {
    n = __impl::arith_to_buf(buf, 128, static_cast<f64>(x));
  } else if constexpr ( micron::is_same_v<T, long double> || micron::is_same_v<T, flong> ) {
    n = __impl::arith_to_buf(buf, 128, static_cast<long double>(x));
  } else if constexpr ( micron::is_signed_v<T> ) {
    n = __impl::arith_to_buf(buf, 128, static_cast<i64>(x));
  } else {
    n = __impl::arith_to_buf(buf, 128, static_cast<u64>(x));
  }

  buf[n] = '\0';
  __impl::emit<outstream>(buf);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: volatile arithmetic
// strip volatile qualifier, read once, delegate

template <typename T, int outstream = stdout_fileno>
  requires micron::is_arithmetic_v<T>
void
printk(const volatile T &x)
{
  T copy = const_cast<const T &>(x);
  printk<T, outstream>(copy);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: volatile pointer

template <typename T, int outstream = stdout_fileno>
  requires(micron::is_pointer_v<T> && !is_char_ptr<T>)
void
printk(const volatile T &x)
{
  T copy = const_cast<const T &>(x);
  printk<T, outstream>(copy);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: single byte

template <typename T, int outstream = stdout_fileno>
  requires(micron::is_same_v<T, byte> && sizeof(T) == 1)
void
printk(T &x)
{
  char c[2] = { static_cast<char>(x), '\0' };
  __impl::emit<outstream>(c);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: non-null-terminated byte buffer

template <typename T, int outstream = stdout_fileno>
  requires micron::is_same_v<T, byte *>
void
printk(T ptr, usize len)
{
  char stk[256];
  char *buf = (len < 255) ? stk : new char[len + 1];
  micron::memcpy(buf, ptr, len);
  buf[len] = '\0';
  __impl::emit<outstream>(buf);
  if ( buf != stk )
    delete[] buf;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: container (non-string, non-map iterable)

template <is_container T, int outstream = stdout_fileno>
  requires(!is_tagged_map<T> && !is_swiss_map<T>)
void
printk(const T &ctr)
{
  __impl::emit<outstream>("{ ");
  bool first = true;
  for ( auto itr = ctr.cbegin(); itr != ctr.cend(); ++itr ) {
    if ( !first )
      __impl::emit<outstream>(", ");
    // NOTE: old approach of decltype was wonky for cv-qualified types (ie printing const cont& et al)
    // this works BUT the container must define a value_type (STL does so does micron)
    printk<typename T::value_type, outstream>(*itr);
    first = false;
  }
  __impl::emit<outstream>(" }");
}

template <is_container T, int outstream = stdout_fileno>
  requires(!is_tagged_map<T> && !is_swiss_map<T>)
void
printk(T &&ctr)
{
  printk<T, outstream>(static_cast<const T &>(ctr));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: tagged map (hopscotch_map, robin_map, dictionary_t)
// iterates all slots, skips empty (key == 0), prints as { hash: value, ... }

template <is_tagged_map T, int outstream = stdout_fileno>
void
printk(const T &m)
{
  __impl::emit<outstream>("{ ");
  bool first = true;
  for ( auto itr = m.begin(); itr != m.end(); ++itr ) {
    if ( !itr->key )
      continue;
    if ( !first )
      __impl::emit<outstream>(", ");
    // print hash key as decimal
    char kbuf[24];
    usize kn = __impl::arith_to_buf(kbuf, 24, static_cast<u64>(itr->key));
    kbuf[kn] = '\0';
    __impl::emit<outstream>(kbuf);
    __impl::emit<outstream>(": ");
    printk<decltype(itr->value), outstream>(itr->value);
    first = false;
  }
  __impl::emit<outstream>(" }");
}

template <is_tagged_map T, int outstream = stdout_fileno>
void
printk(T &&m)
{
  printk<T, outstream>(static_cast<const T &>(m));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: swiss map (stack_swiss_map)
// iterator already skips empty/deleted; dereferences to pair with .a and .b

template <is_swiss_map T, int outstream = stdout_fileno>
void
printk(const T &m)
{
  __impl::emit<outstream>("{ ");
  bool first = true;
  for ( auto itr = m.begin(); itr != m.end(); ++itr ) {
    if ( !first )
      __impl::emit<outstream>(", ");
    auto entry = *itr;
    printk<decltype(entry.a), outstream>(entry.a);
    __impl::emit<outstream>(": ");
    printk<decltype(entry.b), outstream>(entry.b);
    first = false;
  }
  __impl::emit<outstream>(" }");
}

template <is_swiss_map T, int outstream = stdout_fileno>
void
printk(T &&m)
{
  printk<T, outstream>(static_cast<const T &>(m));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printk: pair

template <typename A, typename B, int outstream = stdout_fileno>
void
printk(const pair<A, B> &p)
{
  __impl::emit<outstream>("[");
  printk<A, outstream>(p.a);
  __impl::emit<outstream>(", ");
  printk<B, outstream>(p.b);
  __impl::emit<outstream>("]");
}

template <typename A, typename B, int outstream = stdout_fileno>
void
printk(pair<A, B> &&p)
{
  printk<A, B, outstream>(static_cast<const pair<A, B> &>(p));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// printkn: appends newline

// char array
template <typename T, int outstream = stdout_fileno>
void
printkn(T& c)
{
  printk<outstream>(c);
  __impl::emit_nl<outstream>();
}

// char pointer
template <typename T, int outstream = stdout_fileno>
  requires is_char_ptr<T>
void
printkn(const T &c)
{
  printk<outstream, T>(c);
  __impl::emit_nl<outstream>();
}

// raw sized print + newline
template <int outstream = stdout_fileno>
void
sprintkn(const char *c, usize len)
{
  __impl::emit<outstream>(c, len);
  __impl::emit_nl<outstream>();
}

// class with .c_str(): char
template <has_cstr T, int outstream = stdout_fileno>
  requires(micron::is_class_v<T> && micron::is_same_v<typename T::value_type, char>)
void
printkn(const T &str)
{
  __impl::emit<outstream>(str.c_str());
  __impl::emit_nl<outstream>();
}

// class with .c_str(): char8_t
template <has_cstr T, int outstream = stdout_fileno>
  requires(micron::is_class_v<T> && micron::is_same_v<typename T::value_type, char8_t>)
void
printkn(const T &str)
{
  __impl::emit<outstream>(str.c_str());
  __impl::emit_nl<outstream>();
}

// class with .c_str(): wchar_t
template <has_cstr T, int outstream = stdout_fileno>
  requires(micron::is_class_v<T> && micron::is_same_v<typename T::value_type, wchar_t>)
void
printkn(const T &str)
{
  if constexpr ( outstream == stdout_fileno )
    wfput(str.w_str(), io::stdout);
  else
    wfput(str.w_str(), io::stderr);
  __impl::emit_nl<outstream>();
}

// class with .c_str(): char32_t
template <has_cstr T, int outstream = stdout_fileno>
  requires(micron::is_class_v<T> && micron::is_same_v<typename T::value_type, char32_t>)
void
printkn(const T &str)
{
  if constexpr ( outstream == stdout_fileno )
    io::unifput(str.uni_str(), io::stdout);
  else
    io::unifput(str.uni_str(), io::stderr);
  __impl::emit_nl<outstream>();
}

// non-char pointer
template <typename T, int outstream = stdout_fileno>
  requires(micron::is_pointer_v<T> && !is_char_ptr<T>)
void
printkn(const T &x)
{
  printk<T, outstream>(x);
  __impl::emit_nl<outstream>();
}

// arithmetic
template <typename T, int outstream = stdout_fileno>
  requires micron::is_arithmetic_v<T>
void
printkn(const T &x)
{
  printk<T, outstream>(x);
  __impl::emit_nl<outstream>();
}

// volatile arithmetic
template <typename T, int outstream = stdout_fileno>
  requires micron::is_arithmetic_v<T>
void
printkn(const volatile T &x)
{
  T copy = const_cast<const T &>(x);
  printk<T, outstream>(copy);
  __impl::emit_nl<outstream>();
}

// volatile pointer
template <typename T, int outstream = stdout_fileno>
  requires(micron::is_pointer_v<T> && !is_char_ptr<T>)
void
printkn(const volatile T &x)
{
  T copy = const_cast<const T &>(x);
  printk<T, outstream>(copy);
  __impl::emit_nl<outstream>();
}

// single byte
template <typename T, int outstream = stdout_fileno>
  requires(micron::is_same_v<T, byte> && sizeof(T) == 1)
void
printkn(T &x)
{
  char c[3] = { static_cast<char>(x), '\n', '\0' };
  __impl::emit<outstream>(c);
}

// byte buffer
template <typename T, int outstream = stdout_fileno>
  requires micron::is_same_v<T, byte *>
void
printkn(T ptr, usize len)
{
  printk<T, outstream>(ptr, len);
  __impl::emit_nl<outstream>();
}

// container (non-map)
template <is_container T, int outstream = stdout_fileno>
  requires(!is_tagged_map<T> && !is_swiss_map<T>)
void
printkn(const T &ctr)
{
  printk<T, outstream>(ctr);
  __impl::emit_nl<outstream>();
}

template <is_container T, int outstream = stdout_fileno>
  requires(!is_tagged_map<T> && !is_swiss_map<T>)
void
printkn(T &&ctr)
{
  printk<T, outstream>(static_cast<const T &>(ctr));
  __impl::emit_nl<outstream>();
}

// tagged map
template <is_tagged_map T, int outstream = stdout_fileno>
void
printkn(const T &m)
{
  printk<T, outstream>(m);
  __impl::emit_nl<outstream>();
}

template <is_tagged_map T, int outstream = stdout_fileno>
void
printkn(T &&m)
{
  printk<T, outstream>(static_cast<const T &>(m));
  __impl::emit_nl<outstream>();
}

// swiss map
template <is_swiss_map T, int outstream = stdout_fileno>
void
printkn(const T &m)
{
  printk<T, outstream>(m);
  __impl::emit_nl<outstream>();
}

template <is_swiss_map T, int outstream = stdout_fileno>
void
printkn(T &&m)
{
  printk<T, outstream>(static_cast<const T &>(m));
  __impl::emit_nl<outstream>();
}

// pair
template <typename A, typename B, int outstream = stdout_fileno>
void
printkn(const pair<A, B> &p)
{
  printk<A, B, outstream>(p);
  __impl::emit_nl<outstream>();
}

template <typename A, typename B, int outstream = stdout_fileno>
void
printkn(pair<A, B> &&p)
{
  printk<A, B, outstream>(static_cast<const pair<A, B> &>(p));
  __impl::emit_nl<outstream>();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// binary hex dump

template <is_container T>
void
bin(const T &data)
{
  char buf[4096];
  usize cnt = 0;
  usize remaining = data.size();
  while ( remaining > 0 ) {
    usize chunk = remaining > 2048 ? 2048 : remaining;
    const auto *src = reinterpret_cast<const u8 *>(&data[cnt]);
    for ( usize i = 0; i < chunk; ++i ) {
      buf[i * 2] = micron::__impl::__hex_lower[src[i] >> 4];
      buf[i * 2 + 1] = micron::__impl::__hex_lower[src[i] & 0xF];
    }
    buf[chunk * 2] = '\0';
    __impl::emit<stdout_fileno>(buf, chunk * 2);
    cnt += chunk;
    remaining -= chunk;
  }
}

template <has_cstr T>
void
bin(const T &data)
{
  char buf[4096];
  usize cnt = 0;
  usize remaining = data.size();
  while ( remaining > 0 ) {
    usize chunk = remaining > 2048 ? 2048 : remaining;
    const auto *src = reinterpret_cast<const u8 *>(&data[cnt]);
    for ( usize i = 0; i < chunk; ++i ) {
      buf[i * 2] = micron::__impl::__hex_lower[src[i] >> 4];
      buf[i * 2 + 1] = micron::__impl::__hex_lower[src[i] & 0xF];
    }
    buf[chunk * 2] = '\0';
    __impl::emit<stdout_fileno>(" 0x");
    __impl::emit<stdout_fileno>(buf, chunk * 2);
    cnt += chunk;
    remaining -= chunk;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// high-level print functions

void
print_buffer(char **buf [[maybe_unused]])
{
}

inline void
flush(void)
{
  io::fflush(stdout);
}

template <typename... T>
void
print(T &&...str)
{
  (printk(str), ...);
  flush();
}

template <typename... T>
inline void
print(const T &...str)
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
    printk<stdout_fileno>("\n");
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
    printk<stdout_fileno>("\n");
  } else {
    (printkn(str), ...);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// error stream functions

template <typename... T>
void
error(T &...str)
{
  (printk<decltype(str), stderr_fileno>(str), ...);
}

template <typename... T>
inline void
error(const T &...str)
{
  (printk<decltype(str), stderr_fileno>(str), ...);
}

template <typename... T>
void
errorn(T &...str)
{
  (printkn<decltype(str), stderr_fileno>(str), ...);
}

template <typename... T>
inline void
errorn(const T &...str)
{
  (printkn<decltype(str), stderr_fileno>(str), ...);
}

template <typename... T>
inline void
errorln(const T &...str)
{
  if constexpr ( sizeof...(T) > 1 ) {
    (printk<decltype(str), stderr_fileno>(str), ...);
    printk<stderr_fileno>("\n");
  } else {
    (printkn<decltype(str), stderr_fileno>(str), ...);
  }
}

template <typename... T>
void
errorln(T &...str)
{
  (printk<decltype(str), stderr_fileno>(str), ...);
  printk<stderr_fileno>("\n");
}

}     // namespace io
};     // namespace micron
