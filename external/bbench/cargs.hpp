#pragma once

#include <micron/array.hpp>
#include <micron/bits/__exceptions.hpp>
#include <micron/concepts.hpp>
#include <micron/except.hpp>
#include <micron/memory/cmemory.hpp>
#include <micron/string/sstring.hpp>
#include <micron/sum.hpp>
#include <micron/type_traits.hpp>
#include <micron/types.hpp>
#include <micron/vector.hpp>

#ifdef CARGS_EXC
#include <micron/control.hpp>
#endif

namespace cargs
{
typedef void (*gfcallback)(int, const char *, const char *);
typedef void (*fcallback)(const char *, const char *);

enum class Accepts : int { czero, cvoid, cpath, cparam, coperand, ckey, cbool, cint, clong, cfloat, cdouble, cstring };

namespace
{
template<usize N> struct packet {
  micron::sstring<N> key;
  micron::sstring<N> data;
  Accepts type = Accepts::czero;
};

template<typename T> struct get_arity: get_arity<decltype(&T::operator())> {
};

template<typename R, typename... Args> struct get_arity<R (*)(Args...)>: micron::integral_constant<unsigned, sizeof...(Args)> {
};

template<typename R, typename C, typename... Args>
struct get_arity<R (C::*)(Args...)>: micron::integral_constant<unsigned, sizeof...(Args)> {
};

template<typename R, typename C, typename... Args>
struct get_arity<R (C::*)(Args...) const>: micron::integral_constant<unsigned, sizeof...(Args)> {
};
};      // namespace

template<typename Fn> struct param_packet {
  constexpr param_packet() : param(nullptr), abbr(nullptr), description(nullptr), acc(Accepts::czero), ptr(nullptr) { }

  template<usize N, usize M, usize L>
  constexpr param_packet(const char (&s)[N], const char (&s2)[M], const char (&s3)[L], const Accepts a = Accepts::czero)
      : param(s), abbr(s2), description(s3), acc(a), ptr(nullptr)
  {
  }

  template<usize N, usize M, usize L>
  constexpr param_packet(const char (&s)[N], const char (&s2)[M], const char (&s3)[L], const Accepts a, Fn _ptr)
      : param(s), abbr(s2), description(s3), acc(a), ptr(_ptr)
  {
  }

  constexpr param_packet(const param_packet &o) : param(o.param), abbr(o.abbr), description(o.description), acc(o.acc), ptr(o.ptr) { }

  constexpr Accepts
  accepts() const
  {
    return acc;
  }

  constexpr param_packet &
  operator=(const param_packet &o)
  {
    param = o.param;
    abbr = o.abbr;
    description = o.description;
    acc = o.acc;
    ptr = o.ptr;
    return *this;
  }

  template<usize N>
  param_packet &
  operator()(const packet<N> &p)
  {
    if constexpr ( micron::is_pointer_v<Fn> ) {
      if ( ptr ) {
        if constexpr ( get_arity<Fn>() == 0 ) ptr();
        if constexpr ( get_arity<Fn>() == 1 ) ptr(p.key.data());
        if constexpr ( get_arity<Fn>() == 2 ) ptr(p.key.data(), p.data.data());
      }
    }
    return *this;
  }

  const char *param;
  const char *abbr;
  const char *description;
  Accepts acc;
  Fn ptr;
};

template<typename Fn = nullptr_t, usize N, usize M, usize L>
inline constexpr param_packet<Fn>
make_packet(const char (&s)[N], const char (&s2)[M], const char (&s3)[L])
{
  return param_packet<Fn>(s, s2, s3);
}

template<typename Fn = nullptr_t, usize N, usize M, usize L>
inline constexpr param_packet<Fn>
make_packet(const char (&s)[N], const char (&s2)[M], const char (&s3)[L], const Accepts s4)
{
  return param_packet<Fn>(s, s2, s3, s4);
}

template<typename Fn = nullptr_t, usize N, usize M, usize L>
inline constexpr param_packet<Fn>
make_packet(const char (&s)[N], const char (&s2)[M], const char (&s3)[L], const Accepts s4, Fn ptr = nullptr)
{
  return param_packet<Fn>(s, s2, s3, s4, ptr);
}

template<usize P_COUNT = 0, typename C = nullptr_t, gfcallback F = nullptr, char16_t FLAG = '-', i64 MAX_LENGTH = 32, i64 MAX_BUFFER = 127>
class parser
{
  static constexpr gfcallback global_callback = F;

  static constexpr usize __param_storage = (P_COUNT == 0) ? 1 : P_COUNT;
  micron::array<packet<MAX_BUFFER>, MAX_LENGTH> args;
  micron::array<param_packet<C>, __param_storage> parameters;

  static constexpr i64
  __to_i64(const char *s) noexcept
  {
    if ( !s ) return 0;
    i64 acc = 0;
    bool neg = false;
    while ( *s == ' ' || *s == '\t' ) ++s;
    if ( *s == '-' ) {
      neg = true;
      ++s;
    } else if ( *s == '+' )
      ++s;
    while ( *s >= '0' && *s <= '9' ) {
      acc = acc * 10 + (*s - '0');
      ++s;
    }
    return neg ? -acc : acc;
  }

  static constexpr f64
  __to_f64(const char *s) noexcept
  {
    if ( !s ) return 0.0;
    f64 acc = 0.0;
    bool neg = false;
    while ( *s == ' ' || *s == '\t' ) ++s;
    if ( *s == '-' ) {
      neg = true;
      ++s;
    } else if ( *s == '+' )
      ++s;
    while ( *s >= '0' && *s <= '9' ) {
      acc = acc * 10.0 + (*s - '0');
      ++s;
    }
    if ( *s == '.' ) {
      ++s;
      f64 frac = 0.1;
      while ( *s >= '0' && *s <= '9' ) {
        acc += (*s - '0') * frac;
        frac *= 0.1;
        ++s;
      }
    }
    if ( *s == 'e' || *s == 'E' ) {
      ++s;
      bool eneg = false;
      i32 exp = 0;
      if ( *s == '-' ) {
        eneg = true;
        ++s;
      } else if ( *s == '+' )
        ++s;
      while ( *s >= '0' && *s <= '9' ) {
        exp = exp * 10 + (*s - '0');
        ++s;
      }
      f64 p = 1.0;
      for ( i32 i = 0; i < exp; ++i ) p *= 10.0;
      acc = eneg ? acc / p : acc * p;
    }
    return neg ? -acc : acc;
  }

  inline void
  to_number(micron::any<long, long long, double, bool> &out, const char *str, const Accepts type) const
  {
    switch ( type ) {
    case Accepts::cint:
      out = static_cast<long>(__to_i64(str));
      break;
    case Accepts::clong:
      out = static_cast<long long>(__to_i64(str));
      break;
    case Accepts::cfloat:
    case Accepts::cdouble:
      out = static_cast<double>(__to_f64(str));
      break;
    default:
      out = false;
      break;
    }
  }

  inline micron::any<long, long long, double, bool>
  to_number(const char *str, const Accepts type) const
  {
    micron::any<long, long long, double, bool> ret;
    switch ( type ) {
    case Accepts::cint:
      ret = static_cast<long>(__to_i64(str));
      return ret;
    case Accepts::clong:
      ret = static_cast<long long>(__to_i64(str));
      return ret;
    case Accepts::cfloat:
    case Accepts::cdouble:
      ret = static_cast<double>(__to_f64(str));
      return ret;
    default:
      ret = false;
      return ret;
    }
  }

  template<usize M>
  static inline void
  __append_cstr(micron::sstring<M> &dst, const char *src)
  {
    if ( !src ) return;
    while ( *src ) dst += *src++;
  }

  inline bool
  number(const char *str) const
  {
    if ( str == nullptr ) return false;
    for ( ; *str; ++str )
      if ( !(*str >= '0' && *str <= '9') && *str != '.' ) return false;
    return true;
  }

  inline bool
  number(const micron::sstring<MAX_BUFFER> &arg) const
  {
    for ( auto &c : arg )
      if ( !(c >= '0' && c <= '9') && c != '.' ) return false;
    return true;
  }

  inline bool
  empty(char *str) const
  {
    if ( str == nullptr ) return true;
    while ( *str ) {
      if ( *str != FLAG && *str != 0 ) return false;
      str++;
    }
    return true;
  }

  inline bool
  param(char *str) const
  {
    if ( str == nullptr ) return false;
    return (*str == FLAG);
  }

  inline bool
  flag(char *ptr) const
  {
    char c = 0;
    while ( *ptr ) {
      if ( *ptr++ == FLAG )
        if ( c++ > 0 ) return true;
    }
    return false;
  }

  inline char *
  trim(char *ptr) const
  {
    while ( *ptr ) {
      if ( *ptr == FLAG )
        ptr++;
      else
        break;
    }
    return ptr;
  }

  inline __attribute__((always_inline)) void
  callbacks_handler()
  {
    int c = 0;
    do {
      if ( args[c].type == Accepts::czero ) break;
      for ( auto &n : parameters ) {

        if ( (n.param && args[c].key == n.param) || (n.abbr && args[c].key == n.abbr) ) n(args[c]);
      }
      if constexpr ( global_callback ) global_callback(c, args[c].key.data(), args[c].data.data());
    } while ( ++c );
  }

  inline __attribute__((always_inline)) void
  general_parse(int argc, char **_args)
  {
    if ( argc > MAX_LENGTH )
#ifdef CARGS_EXC
      micron::halt();
#elif CARGS_STD_EXC
      micron::exc<micron::except::runtime_error>("Too many arguments present");
#else
      return;
#endif
    if ( argc == 1 ) return;

    __append_cstr(args[0].key, _args[0]);
    args[0].type = Accepts::cpath;
    for ( int i = 1; i < argc; i++ ) {
      if ( micron::strlen(_args[i]) > MAX_BUFFER )
#ifdef CARGS_EXC
        micron::halt();
#elif CARGS_STD_EXC
        micron::exc<micron::except::runtime_error>("Argument is too long");
#else
        continue;
#endif
      bool fl = flag(_args[i]);
      bool pm = (fl == true) ? false : param(_args[i]);

      auto *ch = trim(_args[i]);
      for ( auto &n : parameters ) {
        if ( n.param != nullptr && micron::strcmp(n.param, ch) == 0 && fl ) {
          __append_cstr(args[i].key, ch);
          args[i].type = Accepts::ckey;
          goto data;
        } else if ( n.abbr != nullptr && micron::strcmp(n.abbr, ch) == 0 && pm ) {
          __append_cstr(args[i].key, ch);
          args[i].type = Accepts::cparam;
          goto data;
        } else {
          __append_cstr(args[i].data, _args[i]);
          args[i].type = Accepts::coperand;
          goto data;
        }
        continue;
      data:
        if ( n.accepts() != Accepts::czero ) {
          if ( !(i + 1 < argc) ) break;
          __append_cstr(args[i].data, _args[i + 1]);
          args[i].type = n.accepts();
        }
        break;
      }
    }
    callbacks_handler();
  }

  inline __attribute__((always_inline)) void
  general_parse_np(int argc, char **_args)
  {
    if ( argc > MAX_LENGTH )
#ifdef CARGS_EXC
      micron::halt();
#elif CARGS_STD_EXC
      micron::exc<micron::except::runtime_error>("Too many arguments present");
#else
      return;
#endif
    if ( argc == 1 ) return;

    __append_cstr(args[0].key, _args[0]);
    args[0].type = Accepts::cpath;
    for ( int i = 1; i < argc; i++ ) {
      if ( micron::strlen(_args[i]) > MAX_BUFFER )
#ifdef CARGS_EXC
        micron::halt();
#elif CARGS_STD_EXC
        micron::exc<micron::except::runtime_error>("Argument is too long");
#else
        continue;
#endif
      bool fl = flag(_args[i]);
      bool pm = (fl == true) ? false : param(_args[i]);
      bool emp = empty(_args[i]);

      if ( !emp ) {
        if ( fl ) {
          __append_cstr(args[i].key, trim(_args[i]));
          args[i].type = Accepts::ckey;
          continue;
        } else if ( pm ) {
          __append_cstr(args[i].key, trim(_args[i]));
          args[i].type = Accepts::cparam;
          continue;
        }
      }
      if ( args[i - 1].type == Accepts::ckey ) {
        __append_cstr(args[i - 1].data, _args[i]);
        args[i - 1].type = Accepts::cstring;
        args[i].type = Accepts::czero;
      } else {
        __append_cstr(args[i].data, _args[i]);
        args[i].type = Accepts::coperand;
      }
    }
    callbacks_handler();
  }

public:
  parser() = delete;

  template<class... Y> constexpr parser(Y &&...ls) : parameters{ ls... } { }

  explicit parser(int argc, char **_args) { general_parse_np(argc, _args); }

  parser(const parser &o) : args(o.args) { }

  parser(parser &&o) : args(micron::move(o.args)) { }

  parser &
  operator=(const parser &o)
  {
    args = o.args;
    return *this;
  }

  parser &
  operator=(parser &&o)
  {
    args = micron::move(o.args);
    return *this;
  }

  parser &
  operator()(int argc, char **argv)
  {
    general_parse(argc, argv);
    return *this;
  }

  const auto &
  get()
  {
    return args;
  }

  template<typename Y = micron::sstring<MAX_BUFFER>>
  micron::vector<Y>
  list()
  {
    micron::vector<Y> tmp;
    for ( auto &n : args )
      if ( n.type >= Accepts::ckey ) tmp.emplace_back(n.key);
    return tmp;
  }

  template<typename Y = micron::sstring<MAX_BUFFER>>
  micron::vector<Y>
  data()
  {
    micron::vector<Y> tmp;
    for ( auto &n : args )
      if ( n.type >= Accepts::ckey ) tmp.emplace_back(n.data);
    return tmp;
  }

  template<typename Y = micron::sstring<MAX_BUFFER>>
  micron::vector<Y>
  params()
  {
    micron::vector<Y> tmp;
    for ( auto &n : args )
      if ( n.type == Accepts::cparam ) tmp.emplace_back(n.key);
    return tmp;
  }

  template<typename Y = micron::sstring<MAX_BUFFER>>
  micron::vector<Y>
  operands()
  {
    micron::vector<Y> tmp;
    for ( auto &n : args )
      if ( n.type == Accepts::coperand ) tmp.emplace_back(n.data);
    return tmp;
  }

  inline micron::any<long, long long, double, bool>
  get(const char *const str)
  {
    for ( auto &n : args )
      if ( micron::strcmp(str, n.key.data()) == 0 ) return (to_number(n.data.data(), n.type));
    return micron::any<long, long long, double, bool>{ false };
  }

  inline auto
  operator()(const char *const str)
  {
    for ( auto &n : args )
      if ( micron::strcmp(str, n.key.data()) == 0 ) return n.data;
    return micron::sstring<MAX_BUFFER>();
  }

  inline bool
  operator[](const char *const str)
  {
    for ( auto &n : args )
      if ( micron::strcmp(str, n.key.data()) == 0 ) return true;
    return false;
  }

  bool
  operator[](std::initializer_list<const char *const> &&list)
  {
    for ( auto &n : args )
      for ( auto &c : list )
        if ( micron::strcmp(c, n.key.data()) == 0 ) return true;
    return false;
  }
};
};      // namespace cargs
