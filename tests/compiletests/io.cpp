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
#include "../../src/io/fsys.hpp"
#include "../../src/io/serial.hpp"

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

  (void)micron::io::unlink("/tmp/compiletest_io");
  return 0;
}
