// match.cpp
// micron's compile-time pattern matching (src/match.hpp).
//
// See also:
//   examples/sum.cpp        — sum-type primitives (any<Ts...>, option<T,F>)
//   examples/algorithm_fp.cpp — option-returning fp helpers
//
// match<Fs...>(value) dispatches a value to handler functions.
// The Fs are function pointers or lambdas provided as NON-TYPE template
// args. Only handlers whose argument type exactly matches the value's
// type fire. lazy_match<Fs...> allows implicit conversions.
//
// Supports matching on:
//   match<Fs...>(T)              — scalar value
//   match<Fs...>(pair<A,B>)      — both elements dispatched
//   match<Fs...>(tuple<Ts...>)   — all elements dispatched
//   match<Fs...>(any<Ts...>)     — active alternative dispatched
//   match<Fs...>(option<T,F>)    — active branch dispatched

#include "../src/match.hpp"
#include "../src/sum.hpp"
#include "../src/io/console.hpp"
#include "../src/tuple.hpp"

// Handlers must be function pointers (or lambdas captured in constexpr context)
// because they're non-type template arguments. Use static or free functions.

static void handle_int(int x)    { micron::io::println("int handler: ", x); }
static void handle_float(float x){ micron::io::println("float handler: ", x); }
static void handle_double(double x){ micron::io::println("double handler: ", x); }
static void handle_bool(bool b)  { micron::io::println("bool handler: ", b); }

int
main()
{
  // ================================================================
  // Scalar matching — dispatch by exact type
  // Only the handler whose parameter type matches the value type fires.
  // ================================================================

  micron::io::println("--- scalar match ---");

  // int fires handle_int only; handle_float and handle_double are skipped
  micron::match<handle_int, handle_float, handle_double>(42);

  // float fires handle_float only
  micron::match<handle_int, handle_float, handle_double>(3.14f);

  // double fires handle_double only
  micron::match<handle_int, handle_float, handle_double>(2.718);

  // ================================================================
  // lazy_match — permissive: allows convertible types to match
  // int can match handle_float (int is convertible to float)
  // ================================================================

  micron::io::println("--- lazy_match (permissive) ---");
  // All three handlers will fire because int is convertible to float and double
  micron::lazy_match<handle_int, handle_float, handle_double>(100);

  // ================================================================
  // Pair matching — both elements are dispatched to the same handler set
  // ================================================================

  micron::io::println("--- pair match ---");

  micron::pair<int, float> pf{10, 2.5f};
  // handle_int fires for pf.a (int), handle_float fires for pf.b (float)
  micron::match<handle_int, handle_float>(pf);

  micron::pair<bool, double> pb{true, 1.618};
  micron::match<handle_bool, handle_double>(pb);

  // ================================================================
  // Tuple matching — all N elements dispatched
  // ================================================================

  micron::io::println("--- tuple match ---");

  micron::tuple<int, float, bool> t{7, 1.0f, false};
  // Each element is dispatched independently: int->handle_int, float->handle_float, bool->handle_bool
  micron::match<handle_int, handle_float, handle_bool>(t);

  // ================================================================
  // any<Ts...> matching — only the active alternative dispatches
  // ================================================================

  micron::io::println("--- any<> match ---");

  micron::any<int, float, bool> a1(42);
  micron::any<int, float, bool> a2(3.14f);
  micron::any<int, float, bool> a3(true);

  micron::match<handle_int, handle_float, handle_bool>(a1);   // int active
  micron::match<handle_int, handle_float, handle_bool>(a2);   // float active
  micron::match<handle_int, handle_float, handle_bool>(a3);   // bool active

  // ================================================================
  // option<T,F> matching — either T or F is active
  // ================================================================

  micron::io::println("--- option<> match ---");

  micron::option<int, float> ok(42);
  micron::option<int, float> err(1.5f);

  micron::match<handle_int, handle_float>(ok);    // int branch active
  micron::match<handle_int, handle_float>(err);   // float branch active

  // ================================================================
  // Practical use: heterogeneous dispatch with any<>
  // NOTE: match<Fs...> requires non-type template args — use free
  // functions or static function pointers, not local lambdas.
  // ================================================================

  micron::io::println("--- practical any<> dispatch ---");

  micron::vector<micron::any<int, double, bool>> messages;
  messages.push_back(micron::any<int, double, bool>(10));
  messages.push_back(micron::any<int, double, bool>(3.14));
  messages.push_back(micron::any<int, double, bool>(false));
  messages.push_back(micron::any<int, double, bool>(42));

  for ( auto &msg : messages )
    micron::match<handle_int, handle_double, handle_bool>(msg);

  return 0;
}
