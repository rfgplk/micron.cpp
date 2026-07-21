//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#if !defined(__cplusplus) || __cplusplus < 202400L
#error "snowball_fuzz requires C++26 (compile with -std=c++26)."
#endif

#include "snowball.hpp"

#include "../../src/array/array.hpp"
#include "../../src/concepts.hpp"
#include "../../src/numerics.hpp"
#include "../../src/rng.hpp"
#include "../../src/string/strings.hpp"
#include "../../src/tuple.hpp"
#include "../../src/type_traits.hpp"
#include "../../src/vector/vector.hpp"

#include "../../src/function.hpp"
#include "../../src/linux/process/fork.hpp"
#include "../../src/linux/process/wait.hpp"

#include <initializer_list>
#include <new>

#if defined(__cpp_impl_reflection) && __has_include(<meta>)
#define SNOWBALL_FUZZ_REFLECTION 1
#include <meta>
#else
#define SNOWBALL_FUZZ_REFLECTION 0
#endif

namespace snowball
{
namespace fuzzing
{

using u8 = ::u8;
using u16 = ::u16;
using u32 = ::u32;
using u64 = ::u64;
using i8 = ::i8;
using i16 = ::i16;
using i32 = ::i32;
using i64 = ::i64;
using usize = ::usize;
using byte = ::byte;

template<class> inline constexpr bool __always_false = false;

struct rng {
  micron::math::rng::xoshiro256ss __e;

  static rng
  from_seed(u64 s) noexcept
  {
    rng r;
    r.__e = micron::math::rng::xoshiro256ss::from_seed(s ? s : 0xdeadbeefULL);
    return r;
  }

  u64
  next(void) noexcept
  {
    return __e.next();
  }
};

inline u64
__below(rng &r, u64 n) noexcept
{
  if ( n == 0 ) return 0;
  u64 threshold = (0ULL - n) % n;
  u64 x;
  do {
    x = r.next();
  } while ( x < threshold );
  return x % n;
}

inline bool
__one_of(rng &r, u64 n) noexcept
{
  return __below(r, n) == 0;
}

template<class T>
inline T
__uniform_int(rng &r, T lo, T hi) noexcept
{
  if ( hi <= lo ) return lo;
  using U = micron::make_unsigned_t<T>;
  U span = static_cast<U>(static_cast<U>(hi) - static_cast<U>(lo));
  u64 count = static_cast<u64>(span) + 1ULL;
  if ( count == 0 ) return static_cast<T>(r.next());
  return static_cast<T>(static_cast<U>(lo) + static_cast<U>(__below(r, count)));
}

inline u64
__biased_below(rng &r, u64 n, u32 k) noexcept
{
  if ( n == 0 ) return 0;
  u64 m = __below(r, n);
  for ( u32 i = 1; i < k; ++i ) {
    u64 t = __below(r, n);
    if ( t < m ) m = t;
  }
  return m;
}

struct gen_state {
  u32 special_pct = 61;
  u32 size_bias_k = 3;
  u32 depth = 0;
  u32 max_depth = 8;
};

template<class G>
concept value_generator = requires(const G g, rng &r, gen_state &s, typename G::value_type &v) {
  typename G::value_type;
  { g.generate(r, s) };
  g.mutate(v, r, s);
};

template<class T> T __aggregate_default(rng &r, gen_state &s) noexcept;
template<class E> E __enum_default(rng &r, gen_state &s) noexcept;

inline constexpr long long __special_int_seeds[] = { 0,
                                                     1,
                                                     2,
                                                     3,
                                                     4,
                                                     5,
                                                     7,
                                                     8,
                                                     15,
                                                     16,
                                                     17,
                                                     31,
                                                     32,
                                                     63,
                                                     64,
                                                     127,
                                                     128,
                                                     129,
                                                     255,
                                                     256,
                                                     257,
                                                     511,
                                                     512,
                                                     1023,
                                                     1024,
                                                     4095,
                                                     4096,
                                                     32767,
                                                     32768,
                                                     65535,
                                                     65536,
                                                     2147483647LL,
                                                     2147483648LL,
                                                     4294967295LL,
                                                     4294967296LL,
                                                     9223372036854775807LL,
                                                     -1,
                                                     -2,
                                                     -3,
                                                     -8,
                                                     -16,
                                                     -128,
                                                     -129,
                                                     -256,
                                                     -32768,
                                                     -32769,
                                                     -2147483648LL };
inline constexpr usize __special_int_count = sizeof(__special_int_seeds) / sizeof(__special_int_seeds[0]);

template<class T> struct special_ints_gen {
  using value_type = T;

  T
  generate(rng &r, gen_state &) const noexcept
  {
    return static_cast<T>(__special_int_seeds[__below(r, __special_int_count)]);
  }

  void
  mutate(T &v, rng &r, gen_state &s) const noexcept
  {
    v = generate(r, s);
  }
};

template<class T = int>
special_ints_gen<T>
special_ints(void) noexcept
{
  return {};
}

template<class F>
inline F
__float_special(rng &r) noexcept
{
  using L = micron::numeric_limits<F>;
  const F vals[] = { static_cast<F>(0), -static_cast<F>(0), static_cast<F>(1), static_cast<F>(-1), static_cast<F>(0.5), L::min(),
                     L::max(),          L::lowest(),        L::epsilon(),      L::infinity(),      -L::infinity(),      L::quiet_NaN() };
  return vals[__below(r, sizeof(vals) / sizeof(vals[0]))];
}

template<class F>
inline F
__float_magnitude(rng &r) noexcept
{
  u64 bits = r.next();
  F frac = static_cast<F>(bits >> 11) / static_cast<F>(9007199254740992.0);
  const F scales[] = { static_cast<F>(1),   static_cast<F>(10),   static_cast<F>(100),  static_cast<F>(1e3),
                       static_cast<F>(1e6), static_cast<F>(1e-3), static_cast<F>(1e-6), static_cast<F>(1e9) };
  F mag = frac * scales[__below(r, sizeof(scales) / sizeof(scales[0]))];
  return (bits & 1u) ? -mag : mag;
}

template<class T> struct spec {
  using value_type = T;

  T
  generate(rng &r, gen_state &s) const noexcept
  {
    if constexpr ( micron::is_same_v<T, bool> ) {
      return static_cast<bool>(r.next() & 1u);
    } else if constexpr ( micron::is_integral_v<T> ) {
      if ( __below(r, 100) < s.special_pct ) return special_ints_gen<T>{}.generate(r, s);
      u32 w = static_cast<u32>(__biased_below(r, sizeof(T) * 8, 2)) + 1;
      u64 mask = (w >= 64) ? ~0ULL : ((1ULL << w) - 1ULL);
      return static_cast<T>(r.next() & mask);
    } else if constexpr ( micron::is_floating_point_v<T> ) {
      if ( __below(r, 100) < s.special_pct ) return __float_special<T>(r);
      return __float_magnitude<T>(r);
    } else if constexpr ( micron::is_enum_v<T> ) {
      return __enum_default<T>(r, s);
    } else {
      return __aggregate_default<T>(r, s);
    }
  }

  void
  mutate(T &v, rng &r, gen_state &s) const noexcept
  {
    if constexpr ( micron::is_integral_v<T> && !micron::is_same_v<T, bool> ) {
      switch ( __below(r, 4) ) {
      case 0:
        v = generate(r, s);
        break;
      case 1:
        v = static_cast<T>(v ^ (static_cast<T>(1) << __below(r, sizeof(T) * 8)));
        break;
      case 2:
        v = static_cast<T>(v + static_cast<T>(static_cast<i64>(__below(r, 9)) - 4));
        break;
      default:
        v = static_cast<T>(~v);
        break;
      }
    } else {
      v = generate(r, s);
    }
  }
};

template<class T> struct range_gen {
  using value_type = T;
  T lo{};
  T hi{};
  T align = 0;

  range_gen
  aligned(T k) const noexcept
  {
    range_gen c = *this;
    c.align = k;
    return c;
  }

  T
  generate(rng &r, gen_state &) const noexcept
  {
    if ( align > 1 ) {
      using U = micron::make_unsigned_t<T>;
      U steps = static_cast<U>((static_cast<U>(hi) - static_cast<U>(lo)) / static_cast<U>(align)) + 1u;
      return static_cast<T>(static_cast<U>(lo) + static_cast<U>(__below(r, static_cast<u64>(steps))) * static_cast<U>(align));
    }
    return __uniform_int<T>(r, lo, hi);
  }

  void
  mutate(T &v, rng &r, gen_state &s) const noexcept
  {
    v = generate(r, s);
  }
};

template<class T>
range_gen<T>
range(T lo, T hi) noexcept
{
  return range_gen<T>{ lo, hi, 0 };
}

template<class T> struct one_of_gen {
  using value_type = T;
  micron::vector<T> vals;

  T
  generate(rng &r, gen_state &) const noexcept
  {
    return vals.empty() ? T{} : vals[__below(r, vals.size())];
  }

  void
  mutate(T &v, rng &r, gen_state &s) const noexcept
  {
    v = generate(r, s);
  }
};

template<class T>
one_of_gen<T>
one_of(std::initializer_list<T> vs) noexcept
{
  return one_of_gen<T>{ micron::vector<T>(vs) };
}

template<class T, bool = micron::is_enum_v<T>> struct __flag_uint {
  using type = micron::make_unsigned_t<T>;
};

template<class T> struct __flag_uint<T, true> {
  using type = micron::make_unsigned_t<micron::underlying_type_t<T>>;
};
template<class T> using __flag_uint_t = typename __flag_uint<T>::type;

template<class T> struct flags_gen {
  using value_type = T;
  micron::vector<T> bits;

  T
  generate(rng &r, gen_state &) const noexcept
  {
    using U = __flag_uint_t<T>;
    if ( bits.empty() || __one_of(r, 10) ) return static_cast<T>(0);
    U acc = 0;
    for ( const T &b : bits )
      if ( r.next() & 1u ) acc |= static_cast<U>(b);
    return static_cast<T>(acc);
  }

  void
  mutate(T &v, rng &r, gen_state &) const noexcept
  {
    if ( bits.empty() ) return;
    using U = __flag_uint_t<T>;
    v = static_cast<T>(static_cast<U>(v) ^ static_cast<U>(bits[__below(r, bits.size())]));
  }
};

template<class T>
flags_gen<T>
flags(std::initializer_list<T> vs) noexcept
{
  return flags_gen<T>{ micron::vector<T>(vs) };
}

enum class alpha { ascii, case_mix, digits, full, utf8, filename };

template<class Ch = char> struct char_gen {
  using value_type = Ch;
  alpha cls = alpha::ascii;

  Ch
  generate(rng &r, gen_state &) const noexcept
  {
    u64 x = r.next();
    switch ( cls ) {
    case alpha::digits:
      return static_cast<Ch>('0' + (x % 10u));
    case alpha::case_mix: {
      u64 k = x % 52u;
      return static_cast<Ch>(k < 26u ? ('A' + k) : ('a' + (k - 26u)));
    }
    case alpha::ascii:
      return static_cast<Ch>(0x20 + (x % (0x7fu - 0x20u)));
    case alpha::filename: {
      static const char fc[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-";
      return static_cast<Ch>(fc[x % (sizeof(fc) - 1)]);
    }
    case alpha::utf8:
    case alpha::full:
    default:
      if constexpr ( sizeof(Ch) == 1 )
        return static_cast<Ch>(1u + (x % 255u));
      else
        return static_cast<Ch>(1u + (x % 0x10FFFEu));
    }
  }

  void
  mutate(Ch &c, rng &r, gen_state &s) const noexcept
  {
    c = generate(r, s);
  }
};

inline char_gen<char>
chars(alpha a) noexcept
{
  return char_gen<char>{ a };
}

inline constexpr u32 __str_lens[] = { 0, 1, 2, 3, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129, 255, 256, 257, 511, 512, 1023 };
inline constexpr usize __str_lens_count = sizeof(__str_lens) / sizeof(__str_lens[0]);

template<class Str = micron::string> struct string_gen {
  using value_type = Str;
  alpha cls = alpha::ascii;
  usize lo = 0;
  usize hi = 256;
  bool boundary = true;

  string_gen
  len(usize a, usize b) const noexcept
  {
    string_gen c = *this;
    c.lo = a;
    c.hi = b;
    return c;
  }

  Str
  generate(rng &r, gen_state &s) const noexcept
  {
    usize n;
    if ( boundary && __one_of(r, 2) ) {
      n = __str_lens[__below(r, __str_lens_count)];
      if ( n < lo ) n = lo;
      if ( n > hi ) n = hi;
    } else {
      n = lo + static_cast<usize>(__biased_below(r, static_cast<u64>(hi - lo) + 1ULL, s.size_bias_k));
    }
    Str out;
    out.reserve(n);
    char_gen<char> cg{ cls };
    for ( usize i = 0; i < n; ++i ) out.push_back(cg.generate(r, s));
    return out;
  }

  void
  mutate(Str &v, rng &r, gen_state &s) const noexcept
  {
    v = generate(r, s);
  }
};

inline string_gen<>
string_of(alpha a) noexcept
{
  return string_gen<>{ a };
}

template<class Elem> struct vector_gen {
  using elem_type = typename Elem::value_type;
  using value_type = micron::vector<elem_type>;
  Elem elem;
  usize lo = 0;
  usize hi = 16;

  vector_gen
  len(usize a, usize b) const noexcept
  {
    vector_gen c = *this;
    c.lo = a;
    c.hi = b;
    return c;
  }

  value_type
  generate(rng &r, gen_state &s) const noexcept
  {
    usize n = lo + static_cast<usize>(__biased_below(r, static_cast<u64>(hi - lo) + 1ULL, s.size_bias_k));
    value_type out;
    out.reserve(n);
    for ( usize i = 0; i < n; ++i ) out.emplace_back(elem.generate(r, s));
    return out;
  }

  void
  mutate(value_type &v, rng &r, gen_state &s) const noexcept
  {
    v = generate(r, s);
  }
};

template<class Elem>
vector_gen<Elem>
vector_of(Elem e) noexcept
{
  return vector_gen<Elem>{ e };
}

template<usize N, class Elem> struct array_gen {
  using elem_type = typename Elem::value_type;
  using value_type = micron::array<elem_type, N>;
  Elem elem;

  value_type
  generate(rng &r, gen_state &s) const noexcept
  {
    value_type out{};
    for ( usize i = 0; i < N; ++i ) out[i] = elem.generate(r, s);
    return out;
  }

  void
  mutate(value_type &v, rng &r, gen_state &s) const noexcept
  {
    for ( usize i = 0; i < N; ++i ) elem.mutate(v[i], r, s);
  }
};

template<usize N, class Elem>
array_gen<N, Elem>
array_of(Elem e) noexcept
{
  return array_gen<N, Elem>{ e };
}

template<class T> class generator
{
  static constexpr usize __sbo = 64;
  alignas(16) unsigned char __buf[__sbo];
  T (*__gen)(const void *, rng &, gen_state &) = nullptr;
  void (*__mut)(const void *, T &, rng &, gen_state &) = nullptr;
  void (*__copy)(void *, const void *) = nullptr;
  void (*__destroy)(void *) = nullptr;

public:
  using value_type = T;

  template<class G>
    requires(!micron::is_same_v<micron::remove_cvref_t<G>, generator> && value_generator<micron::remove_cvref_t<G>>
             && micron::is_same_v<typename micron::remove_cvref_t<G>::value_type, T>)
  generator(G &&g) noexcept
  {
    using GG = micron::remove_cvref_t<G>;
    static_assert(sizeof(GG) <= __sbo, "snowball_fuzz: generator too large for SBO; simplify or nest less.");
    ::new (static_cast<void *>(__buf)) GG(micron::forward<G>(g));
    __gen = [](const void *p, rng &r, gen_state &s) -> T { return static_cast<const GG *>(p)->generate(r, s); };
    __mut = [](const void *p, T &v, rng &r, gen_state &s) { static_cast<const GG *>(p)->mutate(v, r, s); };
    __copy = [](void *d, const void *p) { ::new (d) GG(*static_cast<const GG *>(p)); };
    __destroy = [](void *p) { static_cast<GG *>(p)->~GG(); };
  }

  generator(const generator &o) noexcept : __gen(o.__gen), __mut(o.__mut), __copy(o.__copy), __destroy(o.__destroy)
  {
    __copy(__buf, o.__buf);
  }

  generator &
  operator=(const generator &o) noexcept
  {
    if ( this != &o ) {
      __destroy(__buf);
      __gen = o.__gen;
      __mut = o.__mut;
      __copy = o.__copy;
      __destroy = o.__destroy;
      __copy(__buf, o.__buf);
    }
    return *this;
  }

  ~generator() { __destroy(__buf); }

  T
  generate(rng &r, gen_state &s) const
  {
    return __gen(__buf, r, s);
  }

  void
  mutate(T &v, rng &r, gen_state &s) const
  {
    __mut(__buf, v, r, s);
  }
};

template<class T> struct weighted_gen {
  using value_type = T;
  micron::vector<micron::pair<u32, generator<T>>> arms;

  T
  generate(rng &r, gen_state &s) const noexcept
  {
    if ( arms.empty() ) return T{};
    u64 total = 0;
    for ( const auto &a : arms ) total += a.first;
    u64 pick = __below(r, total ? total : 1);
    u64 acc = 0;
    for ( const auto &a : arms ) {
      acc += a.first;
      if ( pick < acc ) return a.second.generate(r, s);
    }
    return arms.back().second.generate(r, s);
  }

  void
  mutate(T &v, rng &r, gen_state &s) const noexcept
  {
    v = generate(r, s);
  }
};

template<class T>
weighted_gen<T>
weighted(std::initializer_list<micron::pair<u32, generator<T>>> arms) noexcept
{
  return weighted_gen<T>{ micron::vector<micron::pair<u32, generator<T>>>(arms) };
}

#if SNOWBALL_FUZZ_REFLECTION

template<class E>
  requires micron::is_enum_v<E>
one_of_gen<E>
one_of(void)
{
  one_of_gen<E> g;
  template for ( constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E)) ) g.vals.push_back([:e:]);
  return g;
}

template<class E>
  requires micron::is_enum_v<E>
flags_gen<E>
flags(void)
{
  flags_gen<E> g;
  template for ( constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E)) ) g.bits.push_back([:e:]);
  return g;
}

template<class E>
E
__enum_default(rng &r, gen_state &s) noexcept
{
  return one_of<E>().generate(r, s);
}

template<class T>
T
__aggregate_default(rng &r, gen_state &s) noexcept
{
  T out{};
  ++s.depth;
  template for ( constexpr auto m :
                 std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked())) )
  {
    using M = micron::remove_cvref_t<typename[:std::meta::type_of(m):]>;
    out.[:m:] = spec<M>{}.generate(r, s);
  }
  --s.depth;
  return out;
}

template<std::meta::info Target, class G> struct __member_override {
  static constexpr std::meta::info target = Target;
  G gen;
};

template<std::meta::info Member, class... Ovs>
consteval bool
__has_override(void)
{
  return (false || ... || (Ovs::target == Member));
}

template<std::meta::info Target, class G, class T>
inline void
__assign_override(const __member_override<Target, G> &ov, T &out, rng &r, gen_state &s) noexcept
{
  out.[:Target:] = ov.gen.generate(r, s);
}

template<class T, class... Ov> struct reflect_gen {
  using value_type = T;
  micron::tuple<Ov...> overrides;

  template<std::meta::info Target, class G>
  reflect_gen<T, Ov..., __member_override<Target, G>>
  with(G g) const noexcept
  {
    return { micron::tuple_cat(overrides, micron::make_tuple(__member_override<Target, G>{ g })) };
  }

  T
  generate(rng &r, gen_state &s) const noexcept
  {
    T out{};
    ++s.depth;

    template for ( constexpr auto m :
                   std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked())) )
    {
      if constexpr ( !__has_override<m, Ov...>() ) {
        using M = micron::remove_cvref_t<typename[:std::meta::type_of(m):]>;
        out.[:m:] = spec<M>{}.generate(r, s);
      }
    }

    [&]<usize... I>(micron::index_sequence<I...>) {
      ((__assign_override(micron::get<I>(overrides), out, r, s)), ...);
    }(micron::make_index_sequence<sizeof...(Ov)>{});
    --s.depth;
    return out;
  }

  void
  mutate(T &v, rng &r, gen_state &s) const noexcept
  {
    v = generate(r, s);
  }
};

template<class T>
reflect_gen<T>
reflect(void) noexcept
{
  return {};
}

#else

template<class T>
T
__aggregate_default(rng &, gen_state &) noexcept
{
  static_assert(__always_false<T>, "snowball_fuzz: auto-deriving a generator for this aggregate needs C++26 reflection "
                                   "(compile with -freflection). Pass an explicit generator instead.");
  return T{};
}

template<class E>
E
__enum_default(rng &, gen_state &) noexcept
{
  static_assert(__always_false<E>, "snowball_fuzz: auto-deriving a generator for this enum needs C++26 reflection "
                                   "(compile with -freflection). Use one_of({...}) / flags({...}) with explicit values.");
  return E{};
}

#endif

struct run_config {
  u64 seed = 0;
  bool print_seed_on_fail = true;
  usize count = 1000;
  gen_state state{};
};

namespace __fz
{
inline void
emit(const char *s) noexcept
{
  micron::io::print(s);
}

inline void
emit_uint(unsigned long long v) noexcept
{
  micron::io::print(static_cast<u64>(v));
}
}      // namespace __fz

template<class Tuple> struct __decay_tuple;

template<class... A> struct __decay_tuple<micron::tuple<A...>> {
  using type = micron::tuple<micron::remove_cvref_t<A>...>;
};

template<class Fn, class... Gens>
void
check_property(const char *name, Fn &&fn, run_config cfg, Gens... gens)
{
  using traits = function_traits<micron::remove_cvref_t<Fn>>;
  static_assert(sizeof...(Gens) == 0 || sizeof...(Gens) == traits::arity,
                "snowball_fuzz: pass either no generators or exactly one per function parameter.");

  test_case(name);
  u64 seed = cfg.seed;
  if ( seed == 0 ) {
    seed = static_cast<u64>(__impl::__cycle_counter());
    if ( seed == 0 ) seed = 0xdeadbeefULL;
  }
  rng r = rng::from_seed(seed);

  for ( usize i = 0; i < cfg.count; ++i ) {
    auto args = [&]<usize... Is>(micron::index_sequence<Is...>) {
      if constexpr ( sizeof...(Gens) == 0 ) {
        using Args = typename __decay_tuple<typename traits::args_tuple>::type;
        Args a{};
        ((micron::get<Is>(a) = spec<micron::tuple_element_t<Is, Args>>{}.generate(r, cfg.state)), ...);
        return a;
      } else {
        micron::tuple<typename micron::remove_cvref_t<Gens>::value_type...> a{};
        auto gt = micron::tie(gens...);
        ((micron::get<Is>(a) = micron::get<Is>(gt).generate(r, cfg.state)), ...);
        return a;
      }
    }(micron::make_index_sequence<traits::arity>{});

#if defined(__cpp_exceptions)
    try {
      micron::apply(fn, args);
    } catch ( ... ) {
      __fz::emit("\033[34msnowball_fuzz check_property() failure:\033[0m exception at iteration ");
      __fz::emit_uint(static_cast<unsigned long long>(i));
      if ( cfg.print_seed_on_fail ) {
        __fz::emit(" seed=");
        __fz::emit_uint(static_cast<unsigned long long>(seed));
      }
      __fz::emit("\n\r");
      should_print_stack();
      __require_clbck();
      __abort();
    }
#else

    micron::apply(fn, args);
#endif
  }
  end_test_case();
}

using resource_id = u32;
using call_id = u32;
inline constexpr u32 __no_index = 0xffffffffu;

template<class T> struct resource_handle {
  resource_id id;
  using type = T;
};

struct any_value {
  void *obj = nullptr;
  void (*destroy_)(void *) = nullptr;

  any_value() = default;
  any_value(const any_value &) = delete;
  any_value &operator=(const any_value &) = delete;

  any_value(any_value &&o) noexcept : obj(o.obj), destroy_(o.destroy_)
  {
    o.obj = nullptr;
    o.destroy_ = nullptr;
  }

  any_value &
  operator=(any_value &&o) noexcept
  {
    reset();
    obj = o.obj;
    destroy_ = o.destroy_;
    o.obj = nullptr;
    o.destroy_ = nullptr;
    return *this;
  }

  ~any_value() { reset(); }

  void
  reset() noexcept
  {
    if ( obj && destroy_ ) destroy_(obj);
    obj = nullptr;
    destroy_ = nullptr;
  }

  template<class T>
  void
  set(T v)
  {
    reset();
    obj = new T(static_cast<T &&>(v));
    destroy_ = [](void *p) { delete static_cast<T *>(p); };
  }

  template<class T>
  const T &
  as() const
  {
    return *static_cast<const T *>(obj);
  }

  bool
  live() const noexcept
  {
    return obj != nullptr;
  }
};

struct any_generator {
  void *gen = nullptr;
  void (*produce_)(const void *, rng &, gen_state &, void *) = nullptr;
  void (*destroy_)(void *) = nullptr;
  void *(*clone_)(const void *) = nullptr;

  any_generator() = default;

  any_generator(const any_generator &o) : produce_(o.produce_), destroy_(o.destroy_), clone_(o.clone_)
  {
    gen = o.gen ? o.clone_(o.gen) : nullptr;
  }

  any_generator &
  operator=(const any_generator &o)
  {
    if ( this != &o ) {
      reset();
      produce_ = o.produce_;
      destroy_ = o.destroy_;
      clone_ = o.clone_;
      gen = o.gen ? o.clone_(o.gen) : nullptr;
    }
    return *this;
  }

  any_generator(any_generator &&o) noexcept : gen(o.gen), produce_(o.produce_), destroy_(o.destroy_), clone_(o.clone_) { o.gen = nullptr; }

  any_generator &
  operator=(any_generator &&o) noexcept
  {
    reset();
    gen = o.gen;
    produce_ = o.produce_;
    destroy_ = o.destroy_;
    clone_ = o.clone_;
    o.gen = nullptr;
    return *this;
  }

  ~any_generator() { reset(); }

  void
  reset() noexcept
  {
    if ( gen && destroy_ ) destroy_(gen);
    gen = nullptr;
  }

  template<class A, class G>
  static any_generator
  of(G g)
  {
    any_generator a;
    a.gen = new G(static_cast<G &&>(g));
    a.produce_ = [](const void *gp, rng &r, gen_state &s, void *out) {
      *static_cast<A *>(out) = static_cast<A>(static_cast<const G *>(gp)->generate(r, s));
    };
    a.destroy_ = [](void *p) { delete static_cast<G *>(p); };
    a.clone_ = [](const void *p) -> void * { return new G(*static_cast<const G *>(p)); };
    return a;
  }

  template<class A>
  A
  produce(rng &r, gen_state &s) const
  {
    A a{};
    produce_(gen, r, s, &a);
    return a;
  }
};

struct binding {
  bool consumes = false;
  resource_id want = 0;
  any_generator gen;
};

struct exec_ctx;
struct call_instance;

struct registered_call {
  micron::string name;
  u16 arity = 0;
  micron::vector<binding> args;
  resource_id produces = 0;
  u32 weight = 10;
  micron::function<void(exec_ctx &, const call_instance &)> invoke;
};

struct resource_kind {
  resource_id id = 0;
  micron::string name;
  micron::vector<call_id> ctors;
};

struct producer_ref {
  u32 call_index = __no_index;
  bool bound = false;
};

struct call_instance {
  call_id call = 0;
  u32 index = 0;
  u64 seed = 0;
  micron::vector<producer_ref> bindings;
};

struct program {
  micron::vector<call_instance> calls;
  u64 seed = 0;
};

struct scenario;

struct exec_ctx {
  rng r;
  gen_state state;
  micron::vector<any_value> produced;
  const scenario *sc = nullptr;
};

template<class A>
inline void
__resolve_arg(A &out, exec_ctx &ex, const call_instance &ci, const registered_call &rc, usize i)
{
  const binding &b = rc.args[i];
  if ( b.consumes ) {
    const producer_ref &pr = ci.bindings[i];
    if ( pr.bound && pr.call_index < ex.produced.size() && ex.produced[pr.call_index].live() )
      out = ex.produced[pr.call_index].as<A>();
    else
      out = A{};
  } else {
    out = b.gen.template produce<A>(ex.r, ex.state);
  }
}

template<class Fn> inline void __invoke_impl(const Fn &fn, exec_ctx &ex, const call_instance &ci);

enum class outcome : u8 { ok, exited, signaled };

struct exec_result {
  outcome st = outcome::ok;
  int code = 0;
  int signal = 0;
  u32 fail_call = __no_index;

  bool
  failed() const noexcept
  {
    return st != outcome::ok;
  }

  bool
  matches(const exec_result &o) const noexcept
  {
    if ( st != o.st ) return false;
    if ( st == outcome::exited ) return code == o.code;
    if ( st == outcome::signaled ) return signal == o.signal;
    return true;
  }
};

struct run_options {
  u64 seed = 0;
  usize iterations = 1000;
  u32 max_calls = 20;
  bool minimize = true;
  bool abort_on_failure = true;
  bool verbose = false;
};

struct run_report {
  bool found_failure = false;
  u64 seed = 0;
  usize iteration = 0;
  u64 program_seed = 0;
  exec_result result;
  program failing;
  program minimized;
};

template<class Fn, usize Cursor> struct call_builder;

struct scenario {
  micron::vector<resource_kind> res_{ resource_kind{} };
  micron::vector<registered_call> calls_;
  micron::vector<micron::vector<u32>> prio_;
  micron::vector<u32> base_;
  bool table_built_ = false;

  template<class T>
  resource_handle<T>
  resource(const char *name)
  {
    resource_id id = static_cast<resource_id>(res_.size());
    res_.push_back(resource_kind{ id, micron::string(name), {} });
    return resource_handle<T>{ id };
  }

  template<class Fn>
  call_builder<micron::remove_cvref_t<Fn>, 0>
  call(const char *name, Fn fn)
  {
    call_id id = static_cast<call_id>(calls_.size());
    registered_call rc;
    rc.name = name;
    rc.arity = static_cast<u16>(function_traits<micron::remove_cvref_t<Fn>>::arity);
    rc.args.resize(rc.arity);
    calls_.push_back(micron::move(rc));
    table_built_ = false;
    return call_builder<micron::remove_cvref_t<Fn>, 0>{ this, id, micron::move(fn), true };
  }

  template<class Fn, usize I, class G>
  void
  __bind_gen(call_id id, G g)
  {
    using A = micron::remove_cvref_t<typename function_traits<Fn>::template arg_type<I>>;
    binding b;
    b.consumes = false;
    b.gen = any_generator::of<A>(micron::move(g));
    calls_[id].args[I] = micron::move(b);
  }

  template<class Fn, usize I, class T>
  void
  __bind_res(call_id id, resource_id kind)
  {
    using A = micron::remove_cvref_t<typename function_traits<Fn>::template arg_type<I>>;
    static_assert(micron::is_same_v<A, T>, "snowball_fuzz: consumed resource type must match the parameter type.");
    binding b;
    b.consumes = true;
    b.want = kind;
    calls_[id].args[I] = micron::move(b);
  }

  template<class Fn, class T>
  void
  __set_produces(call_id id, resource_id kind)
  {
    using R = typename function_traits<Fn>::return_type;
    static_assert(micron::is_same_v<micron::remove_cvref_t<R>, T>, "snowball_fuzz: produced resource type must match the return type.");
    calls_[id].produces = kind;
    res_[kind].ctors.push_back(id);
  }

  void
  __set_weight(call_id id, u32 w)
  {
    calls_[id].weight = w;
  }

  template<class Fn>
  void
  __finalize(call_id id, Fn fn)
  {
    calls_[id].invoke = [fn = micron::move(fn)](exec_ctx &ex, const call_instance &ci) { __invoke_impl(fn, ex, ci); };
  }

  void
  build_choice_table()
  {
    usize n = calls_.size();
    base_.assign(n, 1);
    prio_.assign(n, micron::vector<u32>(n, 0));
    for ( usize b = 0; b < n; ++b ) base_[b] = calls_[b].weight ? calls_[b].weight : 1;
    for ( usize a = 0; a < n; ++a )
      for ( usize b = 0; b < n; ++b ) {
        u32 p = base_[b];

        if ( calls_[a].produces ) {
          for ( const binding &bd : calls_[b].args )
            if ( bd.consumes && bd.want == calls_[a].produces ) p += 15;
        }
        prio_[a][b] = p;
      }
    table_built_ = true;
  }

  u32
  __pick_weighted(rng &r, const micron::vector<u32> &row) const
  {
    u32 total = 0;
    for ( u32 w : row ) total += w;
    if ( total == 0 ) return static_cast<u32>(__below(r, row.size()));
    u32 pick = static_cast<u32>(__below(r, total));
    u32 acc = 0;
    for ( usize i = 0; i < row.size(); ++i ) {
      acc += row[i];
      if ( pick < acc ) return static_cast<u32>(i);
    }
    return static_cast<u32>(row.size() - 1);
  }

  call_id
  __choose_next(rng &r, const program &p) const
  {
    if ( p.calls.empty() || __one_of(r, 20) ) return __pick_weighted(r, base_);
    call_id bias = p.calls[__below(r, p.calls.size())].call;
    return __pick_weighted(r, prio_[bias]);
  }

  u32
  __append_call(program &p, rng &r, micron::vector<micron::vector<u32>> &live, call_id c, u32 depth) const
  {
    const registered_call &rc = calls_[c];
    micron::vector<producer_ref> binds(rc.arity);
    for ( u16 i = 0; i < rc.arity; ++i ) {
      if ( !rc.args[i].consumes ) {
        binds[i] = producer_ref{ __no_index, false };
        continue;
      }
      resource_id k = rc.args[i].want;
      u32 idx = __no_index;
      if ( !live[k].empty() && !(depth > 0 && __one_of(r, 4)) ) idx = live[k][__below(r, live[k].size())];
      if ( idx == __no_index && depth < 3 && !res_[k].ctors.empty() ) {
        call_id ctor = res_[k].ctors[__below(r, res_[k].ctors.size())];
        __append_call(p, r, live, ctor, depth + 1);
        if ( !live[k].empty() ) idx = live[k].back();
      }
      binds[i] = (idx == __no_index) ? producer_ref{ __no_index, false } : producer_ref{ idx, true };
    }
    call_instance ci;
    ci.call = c;
    ci.index = static_cast<u32>(p.calls.size());
    ci.seed = r.next();
    ci.bindings = micron::move(binds);
    p.calls.push_back(micron::move(ci));
    if ( rc.produces ) live[rc.produces].push_back(static_cast<u32>(p.calls.size() - 1));
    return static_cast<u32>(p.calls.size() - 1);
  }

  program
  generate(u64 seed, u32 max_calls) const
  {
    program p;
    p.seed = seed;
    rng r = rng::from_seed(seed ? seed : 0xdeadbeefULL);
    micron::vector<micron::vector<u32>> live(res_.size());
    while ( p.calls.size() < max_calls ) {
      call_id c = __choose_next(r, p);
      __append_call(p, r, live, c, 0);
    }
    return p;
  }

  exec_result
  __execute(const program &p) const
  {
    int pid = micron::fork();
    if ( pid == 0 ) {
      exec_ctx ex;
      ex.sc = this;
      ex.produced.resize(p.calls.size());
#if defined(__cpp_exceptions)
      try {
#endif
        for ( const call_instance &ci : p.calls ) {
          ex.r = rng::from_seed(ci.seed ? ci.seed : (static_cast<u64>(ci.index) + 1));
          calls_[ci.call].invoke(ex, ci);
        }
#if defined(__cpp_exceptions)
      } catch ( ... ) {
        micron::sys_exit(7);
      }
#endif
      micron::sys_exit(0);
    }
    int status = 0;
    micron::waitpid(pid, &status, 0);
    exec_result res;
    if ( micron::wifsignaled(status) ) {
      res.st = outcome::signaled;
      res.signal = micron::wtermsig(status);
    } else if ( micron::wifexited(status) ) {
      int code = micron::wexitstatus(status);
      if ( code == 0 )
        res.st = outcome::ok;
      else {
        res.st = outcome::exited;
        res.code = code;
      }
    }
    return res;
  }

  static program
  __keep(const program &p, const micron::vector<bool> &keep)
  {
    micron::vector<u32> newidx(p.calls.size(), __no_index);
    u32 run = 0;
    for ( usize i = 0; i < p.calls.size(); ++i )
      if ( keep[i] ) newidx[i] = run++;
    program q;
    q.seed = p.seed;
    for ( usize i = 0; i < p.calls.size(); ++i ) {
      if ( !keep[i] ) continue;
      call_instance ci = p.calls[i];
      ci.index = newidx[i];
      for ( producer_ref &pr : ci.bindings ) {
        if ( pr.bound ) {
          if ( pr.call_index < newidx.size() && newidx[pr.call_index] != __no_index )
            pr.call_index = newidx[pr.call_index];
          else
            pr = producer_ref{ __no_index, false };
        }
      }
      q.calls.push_back(micron::move(ci));
    }
    return q;
  }

  micron::vector<bool>
  __closure(const program &p, u32 target) const
  {
    micron::vector<bool> keep(p.calls.size(), false);
    if ( target >= p.calls.size() ) return micron::vector<bool>(p.calls.size(), true);
    micron::vector<u32> stack{ target };
    while ( !stack.empty() ) {
      u32 c = stack.back();
      stack.pop_back();
      if ( keep[c] ) continue;
      keep[c] = true;
      for ( const producer_ref &pr : p.calls[c].bindings )
        if ( pr.bound && pr.call_index < p.calls.size() && !keep[pr.call_index] ) stack.push_back(pr.call_index);
    }
    return keep;
  }

  program
  minimize(const program &orig, const exec_result &target) const
  {
    program p = orig;
    auto pred = [&](const program &cand) { return __execute(cand).matches(target); };

    if ( !p.calls.empty() ) {
      micron::vector<bool> keep = __closure(p, static_cast<u32>(p.calls.size() - 1));
      program q = __keep(p, keep);
      if ( q.calls.size() < p.calls.size() && pred(q) ) p = micron::move(q);
    }
    bool changed = true;
    while ( changed ) {
      changed = false;
      for ( usize i = p.calls.size(); i-- > 0; ) {
        micron::vector<bool> keep(p.calls.size(), true);
        keep[i] = false;
        program q = __keep(p, keep);
        if ( pred(q) ) {
          p = micron::move(q);
          changed = true;
          break;
        }
      }
    }
    return p;
  }

  void
  __print_program(const program &p, u32 fail_call) const
  {
    __fz::emit("program (seed=");
    __fz::emit_uint(p.seed);
    __fz::emit(", calls=");
    __fz::emit_uint(p.calls.size());
    __fz::emit("):\n\r");
    for ( const call_instance &ci : p.calls ) {
      __fz::emit("  #");
      __fz::emit_uint(ci.index);
      __fz::emit(" ");
      __fz::emit(calls_[ci.call].name.c_str());
      for ( usize i = 0; i < ci.bindings.size(); ++i ) {
        if ( calls_[ci.call].args[i].consumes ) {
          __fz::emit(" [<-#");
          if ( ci.bindings[i].bound )
            __fz::emit_uint(ci.bindings[i].call_index);
          else
            __fz::emit("?");
          __fz::emit("]");
        }
      }
      if ( calls_[ci.call].produces ) __fz::emit(" ->res");
      if ( ci.index == fail_call ) __fz::emit("   <== FAILED");
      __fz::emit("\n\r");
    }
  }

  program
  mutate_program(const program &src, rng &r) const
  {
    program p = src;
    u32 op = static_cast<u32>(__below(r, 100));
    if ( p.calls.empty() || (op < 35 && p.calls.size() < 40) ) {

      micron::vector<micron::vector<u32>> live(res_.size());
      for ( usize k = 0; k < p.calls.size(); ++k ) {
        const registered_call &rc = calls_[p.calls[k].call];
        if ( rc.produces ) live[rc.produces].push_back(static_cast<u32>(k));
      }
      __append_call(p, r, live, __choose_next(r, p), 0);
    } else if ( op < 55 && p.calls.size() > 1 ) {

      u32 i = static_cast<u32>(__below(r, p.calls.size()));
      micron::vector<bool> keep(p.calls.size(), true);
      keep[i] = false;
      p = __keep(p, keep);
    } else if ( op < 80 ) {

      p.calls[__below(r, p.calls.size())].seed = r.next();
    } else {

      u32 j = static_cast<u32>(__below(r, p.calls.size()));
      const registered_call &rc = calls_[p.calls[j].call];
      micron::vector<u16> cons;
      for ( u16 i = 0; i < rc.arity; ++i )
        if ( rc.args[i].consumes ) cons.push_back(i);
      if ( !cons.empty() ) {
        u16 ai = cons[__below(r, cons.size())];
        resource_id k = rc.args[ai].want;
        micron::vector<u32> prod;
        for ( u32 e = 0; e < j; ++e )
          if ( calls_[p.calls[e].call].produces == k ) prod.push_back(e);
        if ( !prod.empty() ) p.calls[j].bindings[ai] = producer_ref{ prod[__below(r, prod.size())], true };
      }
    }
    return p;
  }

  run_report
  run(run_options opts)
  {
    if ( !table_built_ ) build_choice_table();
    run_report rep;
    u64 master = opts.seed;
    if ( master == 0 ) {
      master = static_cast<u64>(__impl::__cycle_counter());
      if ( master == 0 ) master = 0xdeadbeefULL;
    }
    rep.seed = master;
    rng mr = rng::from_seed(master);
    micron::vector<program> corpus;

    for ( usize it = 0; it < opts.iterations; ++it ) {
      u64 pseed = mr.next();
      program p;
      if ( !corpus.empty() && !__one_of(mr, 4) )
        p = mutate_program(corpus[__below(mr, corpus.size())], mr);
      else
        p = generate(pseed, opts.max_calls);
      exec_result res = __execute(p);
      if ( res.failed() ) {
        rep.found_failure = true;
        rep.iteration = it;
        rep.program_seed = pseed;
        rep.result = res;
        rep.failing = p;
        if ( opts.minimize )
          rep.minimized = minimize(p, res);
        else
          rep.minimized = p;

        __fz::emit("\033[34msnowball_fuzz scenario failure:\033[0m ");
        if ( res.st == outcome::signaled ) {
          __fz::emit("signal ");
          __fz::emit_uint(static_cast<unsigned long long>(res.signal));
        } else {
          __fz::emit("exit ");
          __fz::emit_uint(static_cast<unsigned long long>(res.code));
        }
        __fz::emit(" at iteration ");
        __fz::emit_uint(static_cast<unsigned long long>(it));
        __fz::emit(" seed=");
        __fz::emit_uint(master);
        __fz::emit("\n\r");
        __fz::emit("minimized ");
        __print_program(rep.minimized, rep.minimized.calls.empty() ? __no_index : res.fail_call);

        if ( opts.abort_on_failure ) {
          should_print_stack();
          __require_clbck();
          __abort();
        }
        return rep;
      }

      if ( corpus.size() < 256 )
        corpus.push_back(micron::move(p));
      else
        corpus[__below(mr, corpus.size())] = micron::move(p);
    }
    return rep;
  }
};

template<class Fn>
inline void
__invoke_impl(const Fn &fn, exec_ctx &ex, const call_instance &ci)
{
  using traits = function_traits<Fn>;
  const registered_call &rc = ex.sc->calls_[ci.call];
  auto args = [&]<usize... I>(micron::index_sequence<I...>) {
    micron::tuple<micron::remove_cvref_t<typename traits::template arg_type<I>>...> t{};
    ((__resolve_arg(micron::get<I>(t), ex, ci, rc, I)), ...);
    return t;
  }(micron::make_index_sequence<traits::arity>{});

  if constexpr ( micron::is_void_v<typename traits::return_type> ) {
    micron::apply(fn, args);
  } else {
    auto ret = micron::apply(fn, args);
    if ( rc.produces ) ex.produced[ci.index].set(micron::move(ret));
  }
}

template<class Fn, usize Cursor> struct call_builder {
  scenario *sc;
  call_id id;
  Fn fn;
  bool active;

  using traits = function_traits<Fn>;

  call_builder(scenario *s, call_id i, Fn f, bool a) : sc(s), id(i), fn(micron::move(f)), active(a) { }

  call_builder(const call_builder &) = delete;
  call_builder &operator=(const call_builder &) = delete;

  call_builder(call_builder &&o) noexcept : sc(o.sc), id(o.id), fn(micron::move(o.fn)), active(o.active) { o.active = false; }

  template<class G>
  call_builder<Fn, Cursor + 1>
  arg(G g)
  {
    static_assert(Cursor < traits::arity, "snowball_fuzz: too many .arg()/.consumes() for this callable.");
    sc->template __bind_gen<Fn, Cursor, G>(id, micron::move(g));
    active = false;
    return call_builder<Fn, Cursor + 1>{ sc, id, micron::move(fn), true };
  }

  template<class T>
  call_builder<Fn, Cursor + 1>
  consumes(resource_handle<T> h)
  {
    static_assert(Cursor < traits::arity, "snowball_fuzz: too many .arg()/.consumes() for this callable.");
    sc->template __bind_res<Fn, Cursor, T>(id, h.id);
    active = false;
    return call_builder<Fn, Cursor + 1>{ sc, id, micron::move(fn), true };
  }

  template<class T>
  call_builder<Fn, Cursor>
  produces(resource_handle<T> h)
  {
    sc->template __set_produces<Fn, T>(id, h.id);
    active = false;
    return call_builder<Fn, Cursor>{ sc, id, micron::move(fn), true };
  }

  call_builder<Fn, Cursor>
  weight(u32 w)
  {
    sc->__set_weight(id, w);
    active = false;
    return call_builder<Fn, Cursor>{ sc, id, micron::move(fn), true };
  }

  ~call_builder()
  {
    if ( active ) {
      static_assert(true, "");
      sc->template __finalize<Fn>(id, micron::move(fn));
    }
  }
};

};      // namespace fuzzing
};      // namespace snowball

namespace sbf = snowball::fuzzing;
