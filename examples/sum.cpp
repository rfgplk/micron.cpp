// sum.cpp
// micron's sum types (src/sum.hpp): any<Ts...> and option<T, F>.
//
// See also:
//   examples/match.cpp        — pattern-match on any<>/option<>
//   examples/function.cpp     — option monad combinators
//   examples/algorithm_fp.cpp — option in fp:: pipelines
//
// any<Ts...>   — tagged union that holds exactly one of Ts at a time.
//                Like std::variant but with slightly different API:
//                  is<T>()    instead of std::holds_alternative
//                  cast<T>()  instead of std::get (UB if wrong type)
//                  emplace<T>(args...)  to change active type
//                  index()    returns discriminant (0..N-1, npos if empty)
//                  tag<T>{}   disambiguation sentinel in constructors
//
// option<T,F>  — binary variant: exactly T OR F (like Either/Result).
//                  is_first() / is_second()
//                  cast<T>() / cast<F>()
//                  Monad combinators: fmap, bind, and_then, or_else,
//                  bimap, fold, tap, ensure, join, etc. (in function.hpp)

#include "../src/sum.hpp"
#include "../src/function.hpp"
#include "../src/except.hpp"
#include "../src/io/console.hpp"

// ================================================================
// any<Ts...> examples
// ================================================================

static void
demo_any()
{
  micron::io::println("--- any<> ---");

  // --- Construction from value ---
  micron::any<int, f64, bool> a(42);
  micron::io::println("holds int: ", a.is<int>(), " index=", a.index());

  // --- Cast to active type ---
  micron::io::println("cast<int>=", a.cast<int>());

  // --- Assignment changes the active type ---
  a = static_cast<f64>(3.14);
  micron::io::println("now f64: is<f64>=", a.is<f64>(), " cast=", a.cast<f64>());

  a = true;
  micron::io::println("now bool: is<bool>=", a.is<bool>());

  // --- tag<T> constructor: default-construct a specific type ---
  micron::any<int, f64, bool> tagged(micron::tag<f64>{});
  micron::io::println("tag<f64> default: is<f64>=", tagged.is<f64>(), " val=", tagged.cast<f64>());

  // --- tag<T> + args: construct with arguments ---
  micron::any<int, f64, bool> with_args(micron::tag<int>{}, 99);
  micron::io::println("tag<int>(99): cast<int>=", with_args.cast<int>());

  // --- emplace<T> — change active type in place ---
  micron::any<int, f64, bool> emp;
  emp.emplace<int>(100);
  micron::io::println("emplace<int>(100): cast<int>=", emp.cast<int>());
  emp.emplace<f64>(2.718);
  micron::io::println("emplace<f64>: cast<f64>=", emp.cast<f64>());

  // --- has_value / reset ---
  micron::any<int, f64> hv(42);
  micron::io::println("has_value=", hv.has_value());
  hv.reset();
  micron::io::println("after reset: has_value=", hv.has_value());

  // --- Copy / move semantics ---
  micron::any<int, f64> src(static_cast<f64>(3.14));
  micron::any<int, f64> cpyd = src;          // copy
  micron::any<int, f64> mvd  = micron::move(src);   // move
  micron::io::println("copy: is<f64>=", cpyd.is<f64>());
  micron::io::println("move: is<f64>=", mvd.is<f64>());

  // --- Implicit conversion operators ---
  micron::any<int, f64> conv(123);
  int &ref = conv;   // implicit operator T& for active type
  ref = 456;
  micron::io::println("implicit ref assignment: cast<int>=", conv.cast<int>());

  // --- index() returns ordinal position in the type list ---
  micron::any<int, f64, bool> idx_test(static_cast<f64>(3.14));
  micron::io::println("index() of f64 = ", idx_test.index());   // 1 (second type)
}

// ================================================================
// option<T, F> examples — binary Either type
// ================================================================

// Return type: success = int, error = micron::runtime
static micron::option<int, micron::runtime>
safe_div(int a, int b)
{
  if ( b == 0 )
    return micron::runtime("division by zero");
  return a / b;
}

// Chain of fallible operations
static micron::option<int, micron::runtime>
safe_sqrt(int x)
{
  if ( x < 0 )
    return micron::runtime("negative value");
  // Simplified integer sqrt
  int r = 0;
  while ( (r + 1) * (r + 1) <= x ) ++r;
  return r;
}

static void
demo_option()
{
  micron::io::println("--- option<> ---");

  // --- Construction from first/second type ---
  micron::option<int, micron::runtime> ok(42);
  micron::option<int, micron::runtime> err(micron::runtime("oops"));

  micron::io::println("ok.is_first()=", ok.is_first());
  micron::io::println("err.is_second()=", err.is_second());

  // --- cast<T> to extract ---
  micron::io::println("ok.cast<int>=", ok.cast<int>());
  micron::io::println("err.cast<runtime>.what()=", err.cast<micron::runtime>().what());

  // --- fmap — transform success value; error passes through ---
  // fmap dispatches via the option-specific overload in function.hpp
  micron::option<int, micron::runtime> doubled = micron::fmap(
      [](int x) { return x * 2; }, ok);
  micron::io::println("fmap(*2): is_first=", doubled.is_first(), " val=", doubled.cast<int>());

  micron::option<int, micron::runtime> still_err = micron::fmap(
      [](int x) { return x * 2; }, err);
  micron::io::println("fmap on error: is_second=", still_err.is_second());

  // --- and_then (bind) — chain fallible operations ---
  auto chained = micron::and_then(safe_div(100, 5), safe_sqrt);
  micron::io::println("div(100,5)=20 then sqrt: ", chained.cast<int>());

  auto chain_err = micron::and_then(safe_div(1, 0), safe_sqrt);
  micron::io::println("div(1,0) error propagates: is_second=", chain_err.is_second());

  // --- or_else — recover from error branch ---
  auto recovered = micron::or_else(err,
      [](const micron::runtime &) -> micron::option<int, micron::runtime> {
        return -1;   // provide a fallback value
      });
  micron::io::println("or_else recovery: val=", recovered.cast<int>());

  // --- fold — unwrap either branch into a single value ---
  // fold(opt, fn_success, fn_error) -> common type
  int result = micron::fold(safe_div(10, 2),
      [](int v) { return v; },
      [](const micron::runtime &) { return -999; });
  micron::io::println("fold(10/2): ", result);

  int err_result = micron::fold(safe_div(10, 0),
      [](int v) { return v; },
      [](const micron::runtime &) { return -999; });
  micron::io::println("fold(10/0): ", err_result);

  // --- bimap — transform BOTH branches independently ---
  auto mapped = micron::bimap(
      safe_div(20, 4),
      [](int v) { return v * 10; },                   // success: multiply
      [](const micron::runtime &e) {                   // error: rewrap
        return micron::runtime("remapped error");
      });
  micron::io::println("bimap success: ", mapped.cast<int>());   // 50

  // --- tap — side-effect on success, option unchanged ---
  bool side_ran = false;
  auto after_tap = micron::tap(ok, [&side_ran](int v) {
    side_ran = true;
    micron::io::println("tap: value=", v);
  });
  micron::io::println("tap ran=", side_ran, " still ok=", after_tap.is_first());

  // --- ensure — filter: if pred fails, replace with error ---
  auto big = micron::ensure(ok, [](int v) { return v > 100; }, micron::runtime("too small"));
  micron::io::println("ensure(>100): is_second=", big.is_second());

  auto small_ok = micron::ensure(ok, [](int v) { return v > 0; }, micron::runtime("not positive"));
  micron::io::println("ensure(>0): is_first=", small_ok.is_first());

  // --- join — flatten option<option<T,E>, E> -> option<T,E> ---
  micron::option<micron::option<int, micron::runtime>, micron::runtime>
      nested(micron::option<int, micron::runtime>(42));
  auto flat = micron::join(nested);
  micron::io::println("join nested: val=", flat.cast<int>());

  // --- map_error — transform the error type ---
  micron::option<int, micron::runtime> orig_err(micron::runtime("original"));
  auto new_err = micron::map_error(orig_err,
      [](const micron::runtime &e) {
        return micron::logic("converted error");
      });
  micron::io::println("map_error: is_second=", new_err.is_second());
}

int
main()
{
  demo_any();
  micron::io::println("");
  demo_option();
  return 0;
}
