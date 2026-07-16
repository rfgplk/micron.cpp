//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile-validity gate: the micron::io porcelain compiles on every arch/opt.
// Not run (would emit to stdout / touch the fs); this only checks the template surface.

#include "../../src/io.hpp"

#include "../../src/io/bin.hpp"
#include "../../src/io/cached_file.hpp"
#include "../../src/io/console.hpp"
#include "../../src/io/echo.hpp"
#include "../../src/io/file.hpp"
#include "../../src/io/filesystem.hpp"
#include "../../src/io/fp.hpp"
#include "../../src/io/fsys.hpp"
#include "../../src/io/ftw.hpp"
#include "../../src/io/os/dir.hpp"
#include "../../src/io/pipe.hpp"
#include "../../src/io/serial.hpp"

#include "../../src/function.hpp"

#include "../../src/list.hpp"
#include "../../src/maps/heap_swiss.hpp"
#include "../../src/string/strings.hpp"
#include "../../src/vector/vector.hpp"

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 0
// freestanding 32-bit links have no libgcc: pick up the weak 64-bit div/mod shims here
// until the installed start.cpp snapshot (which includes them via __gcc_math_syms.hpp)
// is refreshed
#include "../../src/math/__gcc_int_syms.hpp"
#endif

namespace mc = micron;

struct pod_t {
  int a;
  float b;
};

// tier-(d) safety: trivially-copyable callables must never byte-dump (write) or be
// byte-filled (read). concept wrappers because requires-expressions outside a template
// hard-error instead of evaluating to false
struct tc_functor {
  int v;
  int
  operator()(int) const
  {
    return v;
  }
};
template<typename T>
concept __can_write = requires(micron::io::file &g, T t) { g.write(t); };
template<typename T>
concept __can_write_cl = requires(micron::io::file &g, const T t) { g.write(t); };
template<typename T>
concept __can_read = requires(micron::io::file &g, T t) { g.read(t); };
template<typename T>
concept __reads_as_consumer = requires(micron::io::file &g, T t) {
  { g.read(t) } -> micron::same_as<micron::option<int, micron::io::error_t>>;
};
inline constexpr auto __generic_lambda = [](auto) { };
static_assert(!__can_write<tc_functor>);                     // no closure byte-dump
static_assert(!__can_write_cl<tc_functor>);                  // the const-lvalue trap
static_assert(__reads_as_consumer<tc_functor>);              // consumer read, NOT tier-d fill
static_assert(!__can_write<decltype(__generic_lambda)>);     // generic lambdas constrained away
static_assert(!__can_read<decltype(__generic_lambda)>);
static_assert(__can_write<pod_t>);                           // PODs still tier (d)
static_assert(__can_read<pod_t>);

int
main()
{
  // historical print surface
  micron::io::print("compiletest\n");
  micron::io::println("line");
  micron::console("value: ", 42);
  micron::console(true);

  // echo family incl. redirects and format strings
  micron::io::echo("echo ", 1, 2.0, true);
  micron::io::echon("no newline");
  micron::io::echof("fmt {} {:x} {:>4}", 1, 255, "r");
  micron::io::echofn("{}", 9);
  micron::io::echo(micron::io::stderr, "to stderr");

  // porcelain file: every marshalling tier instantiates
  micron::io::file f("/tmp/compiletest_io", micron::io::rwc);
  micron::string s("str");
  micron::vector<u32> v;
  micron::list<i32> l;
  micron::vector<micron::string> vs;
  micron::hswiss<u64, u64> m;
  pod_t p{ 1, 2.f };
  (void)f.write(s);           // tier a
  (void)f.write(v);           // tier b
  (void)f.write(l);           // tier c (framed)
  (void)f.write(vs);          // tier c (non-TC contiguous)
  (void)f.write(m);           // tier c (map)
  (void)f.write(p);           // tier d
  (void)f.write("lit");       // literal
  (void)f.write(s.data(), 1); // ptr+len
  (void)f.read<micron::string>();
  (void)f.read<micron::vector<u32>>();
  (void)f.read<micron::list<i32>>();
  (void)f.atomic_replace(s);
  f >> s;
  f << s;

  // resident editing + binary analysis types
  micron::io::cached_file<micron::string> cf("/tmp/compiletest_io", micron::io::rw);
  (void)cf.load();
  (void)cf.append(s);
  (void)cf.flush();
  micron::io::binary<micron::string> b("/tmp/compiletest_io", micron::io::rd, 4096);
  (void)b.read_u32le_at(0);
  (void)b.analyse_file();

  // filesystem family
  micron::io::filesystem<micron::io::rwc> fs;
  (void)fs.write("/tmp/compiletest_io", s);
  (void)fs.read("/tmp/compiletest_io", s);
  (void)fs.exists("/tmp/compiletest_io");
  micron::io::concurrent_filesystem<> cfs;
  (void)cfs.apply("/tmp/compiletest_io", micron::io::rd, [](micron::io::file &h) { return h.pos(); });
  micron::io::basic_filesystem<micron::io::rd, micron::null_lock, 4> lru;
  (void)lru.exists("/tmp/compiletest_io");
  micron::io::rooted_filesystem rfs("/tmp");
  (void)rfs.exists("compiletest_io");

  // oneshots
  (void)micron::io::exists("/tmp/compiletest_io");
  if ( auto r = micron::io::read_file("/tmp/compiletest_io"); r.is_first() ) (void)r;
  (void)micron::io::write_file("/tmp/compiletest_io", v);
  (void)micron::io::copy("/tmp/compiletest_io", "/tmp/compiletest_io2");
  (void)micron::io::unlink("/tmp/compiletest_io2");
  (void)micron::io::touch("/tmp/compiletest_io");

  // MFR1 serialize
  (void)micron::io::serialize::frame(f, v);
  (void)micron::io::serialize::unframe<micron::vector<u32>>(f);
  (void)micron::io::serialize::serialize(f, p);

  // container-aware format()
  (void)micron::format::format("v = {}", v);

  // functional layer: file members (fn.hpp / __lines.hpp)
  (void)f.write([] { return micron::string("p"); });      // producer (lambda)
  (void)f.write(+[] { return micron::string("p"); });     // producer (fn pointer)
  if ( auto r = f.read([](micron::string sv) { return sv.size(); }); r.is_first() ) (void)r;
  (void)f.read([](micron::vector<u32> vv) { (void)vv; });      // void consumer -> option<unit_t, _>
  (void)f.modify([](micron::string sv) { return sv; });        // pure T -> T
  (void)f.modify([](micron::string &sv) { sv += "x"; });       // in-place void(T&)
  (void)f.modify_atomic([](micron::string sv) { return sv; });
  (void)f.read_with([](const byte *, usize) { });
  (void)f.write_with([](byte *, usize) -> usize { return 0; });
  (void)f.each_line([](const char *, usize) { });
  (void)f.each_line([](const micron::string &) { });
  (void)f.fold_lines(usize{ 0 }, [](usize a, const char *, usize) { return a + 1; });
  (void)f.fold_lines(usize{ 0 }, [](usize a, const micron::string &) { return a + 1; });
  for ( const auto &ln : f.lines() ) (void)ln;
  {
    auto lr = f.lines();
    (void)lr.error();
    (void)static_cast<bool>(lr);
  }

  // functional layer: free functions (fp.hpp) + combinator interop (function.hpp)
  (void)micron::io::with_file("/tmp/compiletest_io", micron::io::rd, [](micron::io::file &h) { return h.size(); });
  (void)micron::io::with_file("/tmp/compiletest_io", [](micron::io::file &h) { (void)h; });
  (void)micron::io::modify("/tmp/compiletest_io", [](micron::string sv) { return sv; });
  (void)micron::io::read_lines("/tmp/compiletest_io");
  (void)micron::io::to_option(0);
  (void)(s | micron::io::write_file_c("/tmp/compiletest_io"));
  (void)micron::io::append_file_c("/tmp/compiletest_io")(s);
  (void)micron::io::modify_c([](micron::string sv) { return sv; })("/tmp/compiletest_io");
  (void)micron::and_then(micron::io::read_file("/tmp/compiletest_io"),
                         [](micron::string sv) { return micron::option<usize, micron::io::error_t>{ sv.size() }; });
  (void)micron::fmap([](micron::vector<micron::string> ls) { return ls.size(); }, micron::io::read_lines("/tmp/compiletest_io"));
  if ( false ) (void)micron::io::interact([](micron::string &&sv) { return micron::move(sv); });

  // functional members across the porcelain types
  (void)cf.modify([](micron::string &sv) { sv += "x"; });
  (void)cf.modify([](micron::string sv) { return sv; });
  (void)b.fold(usize{ 0 }, [](usize acc, micron::io::bin_range_t w) { return acc + w.size(); });
  (void)b.each_window([](micron::io::bin_range_t) { });
  micron::io::dir dd("/tmp");
  (void)dd.fold(usize{ 0 }, [](usize acc, const micron::posix::__impl_dir &) { return acc + 1; });
  (void)micron::io::ftw_files(micron::io::path("/tmp"), [](const micron::io::path_t &) { });
  (void)micron::io::ftw(micron::io::path("/tmp"), [](const micron::io::path_t &) { return false; });      // early-stop shape
  (void)micron::io::ftw_fold(micron::io::path("/tmp"), usize{ 0 }, [](usize acc, const micron::io::path_t &) { return acc + 1; });
  micron::io::stream<> st;
  (void)st.fold(u32{ 0 }, [](u32 acc, byte bb) { return acc + bb; });
  (void)st.each_line([](const micron::string &) { });
  micron::io::upipe up;
  (void)up.write_with([](byte *, usize) -> usize { return 0; });
  up.close_write();
  (void)up.each_chunk([](const byte *, usize) { });

  (void)micron::io::unlink("/tmp/compiletest_io");
  return 0;
}
