// function.cpp
// micron::function<R(Args...)> and its FP combinators (src/function.hpp).
//
// See also:
//   examples/algorithm_fp.cpp — fmap / scanl / zip_with on containers
//   examples/sum.cpp          — option<T,F> as a return-monad
//   examples/match.cpp        — type-dispatch on any<>/option<>
//
// micron::function is a type-erased function wrapper with:
//   - 48-byte small-object optimization (no heap for small callables)
//   - Copy requires stored type to be copy-constructible
//   - Tracks is_noexcept() of the stored callable
//   - target<G>() — retrieve the stored callable if type matches exactly
//
// FP combinators (free functions in micron namespace):
//   fmap(f, g)         — compose: returns function h where h(x) = f(g(x))
//   bind(g, f)         — monadic bind: h(x) = f(g(x))(x) (function monad)
//   pure(b)            — const function ignoring input: h(x) = b
//   ap(ff, fg)         — applicative: h(x) = ff(x)(fg(x))
//   curry(f)           — n-ary to chain of 1-ary
//   partial(f, args..) — bind leading arguments
//   lazy(f)            — memoize nullary function
//   flip(f)            — swap first two args
//   const_fn(a)        — function always returning a
//   operator|          — pipe:          a | f      == f(a)
//   operator>>         — compose LTR:   (f >> g)(x) == g(f(x))
//   operator<<         — compose RTL:   (f << g)(x) == f(g(x))
//   bind_method        — bind member function pointer + object

#include "../src/function.hpp"
#include "../src/io/console.hpp"
#include "../src/vector/vector.hpp"
#include "../src/algorithm/fp.hpp"

int
main()
{
  // ================================================================
  // micron::function<R(Args...)> basics
  // ================================================================

  // Store a lambda
  micron::function<int(int)> fn = [](int x) { return x * 2; };
  micron::io::println("fn(5)=", fn(5));

  // Empty check
  micron::function<int(int)> empty;
  micron::io::println("empty fn bool=", bool(empty), " !empty=", !empty);

  // Copy
  auto fn2 = fn;
  micron::io::println("fn2(7)=", fn2(7));

  // Move
  auto fn3 = micron::move(fn2);
  micron::io::println("fn3(3)=", fn3(3));

  // Reassign
  fn3 = [](int x) { return x + 100; };
  micron::io::println("reassigned fn3(5)=", fn3(5));

  // swap
  micron::function<int(int)> fa = [](int x) { return x * x; };
  micron::function<int(int)> fb = [](int x) { return x + 1; };
  fa.swap(fb);
  micron::io::println("after swap: fa(4)=", fa(4));   // was fb: 4+1=5

  // is_noexcept — checks noexcept attribute of stored callable
  micron::function<int()> ne_fn = []() noexcept { return 42; };
  micron::function<int()> ex_fn = []() { return 0; };
  micron::io::println("ne_fn.is_noexcept()=", ne_fn.is_noexcept());
  micron::io::println("ex_fn.is_noexcept()=", ex_fn.is_noexcept());

  // nullptr equality
  micron::io::println("empty == nullptr: ", (empty == nullptr));

  // ================================================================
  // bind_method — wrap member function pointer
  // ================================================================

  struct Adder {
    int base;
    int add(int x) const { return base + x; }
  };
  Adder adder{100};
  auto add_fn = micron::bind_method(&Adder::add, &adder);
  micron::io::println("bind_method add(5)=", add_fn(5));   // 105

  // ================================================================
  // FUNCTION COMPOSITION
  // ================================================================

  // Must be micron::function<> so ADL finds micron::operator>>/<<
  micron::function<int(int)> double_it = [](int x) { return x * 2; };
  micron::function<int(int)> add_ten   = [](int x) { return x + 10; };
  micron::function<int(int)> square    = [](int x) { return x * x; };

  // operator| (pipe): value | fn  ==  fn(value)
  int piped = 5 | double_it | add_ten;
  micron::io::println("5 | double | add_ten = ", piped);   // 20

  // operator>> (compose LTR): (f >> g)(x) == g(f(x))
  auto double_then_add = double_it >> add_ten;
  micron::io::println("(double >> add)(3)=", double_then_add(3));   // 16

  // operator<< (compose RTL): (f << g)(x) == f(g(x))
  auto add_then_double = double_it << add_ten;
  micron::io::println("(double << add)(3)=", add_then_double(3));   // 26

  // Chain three functions
  auto pipeline = double_it >> add_ten >> square;
  micron::io::println("double >> add >> square (2) = ", pipeline(2));  // (2*2+10)^2=196

  // ================================================================
  // fmap / bind / pure / ap on micron::function
  // ================================================================

  // NOTE: micron::fmap(f, function<>) is defined but stores a mutable lambda in
  // a const vtable slot — a library-side limitation. Use operator>> for composition.
  micron::function<int(int)> gfn = [](int x) { return x + 5; };
  auto mapped = gfn >> [](int x) { return x * 3; };
  micron::io::println("(+5 >> *3): (2) = ", mapped(2));   // (2+5)*3=21

  // ================================================================
  // curry — convert f(a,b,c) into f(a)(b)(c)
  // ================================================================

  auto add3 = [](int a, int b, int c) { return a + b + c; };
  auto curried = micron::curry(add3);
  auto add1_2  = curried(1)(2);
  micron::io::println("curry(add3)(1)(2)(3)=", add1_2(3));   // 6

  // ================================================================
  // partial — bind leading arguments
  // ================================================================

  auto multiply = [](int a, int b) { return a * b; };
  auto triple   = micron::partial(multiply, 3);   // binds a=3
  micron::io::println("partial(multiply,3)(7)=", triple(7));   // 21

  auto add_curried = [](int a, int b, int c) { return a + b + c; };
  auto add_1_2     = micron::partial(add_curried, 1, 2);
  micron::io::println("partial(add,1,2)(3)=", add_1_2(3));   // 6

  // ================================================================
  // flip — swap the first two arguments
  // ================================================================

  auto divide = [](int a, int b) { return a / b; };
  auto rdivide = micron::flip(divide);   // rdivide(b,a) == divide(a,b)
  micron::io::println("flip(divide)(2,10)=", rdivide(2, 10));   // 10/2=5

  // ================================================================
  // const_fn — function that always returns a constant, ignoring args
  // ================================================================

  auto always7 = micron::const_fn(7);
  micron::io::println("const_fn(7)(anything)=", always7(42));   // 7

  // ================================================================
  // lazy — memoize nullary function (computed once, cached)
  // ================================================================

  int compute_count = 0;
  auto expensive = micron::lazy([&compute_count]() {
    ++compute_count;
    return 42;
  });

  // force() triggers computation (or returns cached result)
  int r1 = micron::force(expensive);
  int r2 = micron::force(expensive);   // should NOT recompute
  micron::io::println("lazy force x2: r1=", r1, " r2=", r2, " computed=", compute_count);

  // ================================================================
  // function<> as a first-class value passed to fp:: algorithms
  // ================================================================

  micron::vector<int> nums({1, 2, 3, 4, 5});
  micron::function<int(int)> dbl = [](int x) { return x * 2; };

  // fmap from fpalgorithm.hpp accepts micron::function directly
  auto result = micron::fmap(dbl, nums);
  micron::io::print("fmap(dbl, nums): ");
  for ( int x : result ) micron::io::print(x, " ");
  micron::io::println("");

  return 0;
}
