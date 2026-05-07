// strings.cpp
// Tour of micron's string types (src/string/).
//
// micron has a small family of string types, picked by where the
// memory lives and whether it is owning:
//
//   hstring<C>   — heap-allocated, mutable, the std::string analogue.
//                  Aliases: micron::string (= hstring<char>),
//                           str8, ustr8, wstr, ustr32.
//
//   sstring<N,C> — STACK-allocated, fixed N capacity, mutable.
//                  Aliases: sstr<N>  (char),
//                           bstr<N>  (byte),
//                           wsstring<N>, utfsstring<N>, etc.
//
//   string_view<S>  — non-owning, mutable-by-rebind view over an existing
//                     S (an hstring or sstring). Note: takes an STRING TYPE
//                     as the template parameter, not the char type.
//
//   cstring_view<S> — same, but constexpr-friendly.
//
//   rope             — chunked string for cheap concatenation (advanced).
//   radixstring      — bit-trie keyed string (advanced).
//
// micron::npos is in the micron namespace (not numeric_limits-style).
//
// Conversions in src/string/conversions/ provide:
//   int_to_string<I>(n)              — heap result (hstring)
//   uint_to_string<I>(n)             — heap result
//   int_to_string_stack<I,T,N>(n)    — sstring result, no heap
//   double_to_string(f64)            — heap result
//
// Key STL deltas:
//   - No std::string SSO toggle — pick hstring for heap, sstring for stack.
//   - Allocator-parametrised by default (allocator_serial<>).
//   - Strings expose c_str(), so println(str) prints them as text.
//   - hstring::find(char) works; the (hstring, pos) overload is currently
//     a stub returning npos — use char-by-char find or work with sstring's
//     find_substr if you need substring search today.

#include "../src/io/console.hpp"
#include "../src/string/conversions/floating_point.hpp"
#include "../src/string/conversions/integral.hpp"
#include "../src/string/string.hpp"
#include "../src/string/string_view.hpp"
#include "../src/string/strings.hpp"

int
main()
{
  // ================================================================
  // 1. hstring — the heap workhorse
  // ----------------------------------------------------------------
  // micron::string is the typical alias for hstring<char>.
  // ================================================================
  micron::io::println("-- 1. hstring --");

  micron::string s("hello");
  micron::io::println("s = '", s, "'  size=", s.size());

  // operator+= and concatenation
  s += ", world";
  micron::io::println("after +=: '", s, "'");

  // push_back of a char
  s.push_back('!');
  micron::io::println("after push_back('!'): '", s, "'");

  // operator+ between two strings (allocates a new one)
  micron::string greeting = micron::string("hello, ") + micron::string("micron");
  micron::io::println("greet  = '", greeting, "'");

  // substr by [pos, pos+cnt)
  micron::string sub = s.substr(0, 5);
  micron::io::println("substr(0,5) = '", sub, "'");

  // find a single character — returns micron::npos if not present
  usize pos = s.find(',');
  micron::io::println("find(',') = ", pos);

  // ================================================================
  // 2. sstring<N> — the stack string
  // ----------------------------------------------------------------
  // No heap allocation; capacity N is part of the type. Going past N
  // throws library_error at construction (or appending).
  // ================================================================
  micron::io::println("-- 2. sstring --");

  // sstr<N> alias = sstring<N, schar>
  micron::sstr<64> ss("on the stack");
  micron::io::println("ss = '", ss, "'  size=", ss.size());

  // Append works; capacity N=64 is enough for both pieces.
  ss += " — fixed cap";
  micron::io::println("after append: '", ss, "'");

  // bstr<N> = stack string of bytes — handy for raw token buffers
  micron::bstr<16> tag;
  tag.append_null("DEADBEEF");
  micron::io::println("byte tag = '", tag, "'");

  // ================================================================
  // 3. string_view — non-owning view
  // ----------------------------------------------------------------
  // Note: the template parameter is the STRING TYPE, e.g. micron::string,
  // not the char type. The view is a (begin, end) pair; it does not
  // null-terminate, so use it for read-only scanning.
  // ================================================================
  micron::io::println("-- 3. string_view --");

  micron::string base("the quick brown fox");
  micron::string_view<micron::string> view(base);
  micron::io::println("view.size = ", view.size(), " front = '", view.front(), "'");

  // sub-view by [a, b)
  auto quick = view.substr(4, 9);     // "quick"
  micron::io::print("view.substr(4,9) = '");
  for ( auto p = quick.begin(); p != quick.end(); ++p ) micron::io::print(*p);
  micron::io::println("'");

  // advance moves the view's start forward
  view.advance(4);
  micron::io::print("after advance(4) starts at '");
  micron::io::print(view.front());
  micron::io::println("'");

  // ================================================================
  // 4. Number <-> string conversions
  // ----------------------------------------------------------------
  // int_to_string returns a heap hstring. int_to_string_stack returns
  // a fixed-N sstring. Both are templated on the integer type, so they
  // pick the right buffer width at compile time.
  // ================================================================
  micron::io::println("-- 4. conversions --");

  micron::string n_heap = micron::int_to_string<i64>(-12345);
  micron::io::println("i64 -> heap  = '", n_heap, "'");

  micron::sstring<24, char> n_stack = micron::int_to_string_stack<i64, char, 24>(98765);
  micron::io::println("i64 -> stack = '", n_stack, "'");

  micron::string f_str = micron::double_to_string(3.141592653589793);
  micron::io::println("f64 -> str   = '", f_str, "'");

  // ================================================================
  // 5. Wide and unicode strings
  // ----------------------------------------------------------------
  // wstr   = hstring<wide>      — wchar_t
  // ustr8  = hstring<unicode8>  — char8_t
  // ustr32 = hstring<unicode32> — char32_t
  // The println overloads dispatch to wfput/unifput for these.
  // ================================================================
  micron::io::println("-- 5. wide / unicode --");

  micron::ustr8 u8 = u8"héllo µ";
  micron::io::println("u8 size  = ", u8.size());
  // Direct printing of u8 strings via println uses the unicode8 path.
  // (printing unicode here may or may not render depending on terminal)

  // ================================================================
  // 6. String literals -> sstring CTAD-friendly
  // ----------------------------------------------------------------
  // sstring<N,C> can be constructed from a char[M] literal as long as
  // M <= N. The compiler enforces this with a static_assert.
  // ================================================================
  micron::io::println("-- 6. literals --");

  micron::sstr<32> from_lit("compile-time sized");
  micron::io::println("from literal: '", from_lit, "'");

  // ================================================================
  // 7. Beyond this example
  // ----------------------------------------------------------------
  // src/string/rope.hpp        — chunked rope for cheap concat
  // src/string/radixstring.hpp — radix-trie string (long shared prefix)
  // src/string/format.hpp      — printf-style format machinery
  // src/string/conversions/    — full int/float <-> string family
  // ================================================================
  return 0;
}
