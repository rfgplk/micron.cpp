//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "snowball/snowball.hpp"

#include "../src/io/__serial_core.hpp"
#include "../src/io/cached_file.hpp"
#include "../src/io/file.hpp"
#include "../src/io/filesystem.hpp"
#include "../src/io/fp.hpp"
#include "../src/io/ftw.hpp"
#include "../src/io/realpath.hpp"
#include "../src/maps/heap_swiss.hpp"
#include "../src/string/strings.hpp"
#include "../src/vector/circle_vector.hpp"
#include "../src/vector/vector.hpp"

namespace mio = micron::io;
namespace si = micron::io::serialize::__impl;

static_assert(!mio::__readable_value<micron::circle_vector<int, 8>>, "non-contiguous container must not reach the object blit");
static_assert(mio::__readable_value<micron::string>, "strings must stay readable");
static_assert(mio::__readable_value<micron::vector<int>>, "contiguous trivial containers must stay readable");
static_assert(mio::__readable_value<micron::vector<micron::string>>, "framed containers must stay readable");

static_assert(si::map_like<micron::heap_swiss_map<micron::string, int>>, "real maps must still frame as maps");
static_assert(!si::map_like<micron::vector<micron::pair<micron::string, int>>>, "vector<pair> must not frame as a map");
static_assert(!si::map_like<micron::vector<micron::pair<int, int>>>, "vector<pair> must not frame as a map");

static void
put(const char *p, const char *content)
{
  mio::file f(p, mio::modes::readwritecreate);
  sb::require(f.write(micron::string(content)) >= 0, true);
}

static micron::string
slurp(const char *p)
{
  mio::file f(p, mio::modes::read);
  return f.read<micron::string>();
}

int
main()
{
  (void)mio::mkdir(mio::path_t("/tmp/micron_io_regress"));

  {

    micron::string deep("/tmp/micron_io_regress");
    for ( int i = 0; i < 12; ++i ) {
      deep += "/dddddddddddddddddddd";
      (void)mio::mkdir(mio::path_t(deep.c_str()));
    }
    sb::require(deep.size() > 255, true);

    micron::wsstring<128> wide_in;
    micron::bytecpy(reinterpret_cast<byte *>(wide_in.data()), reinterpret_cast<const byte *>(deep.c_str()), deep.size() + 1);

    auto resolved = mio::realpath<256>(wide_in);
    sb::require(resolved.size() < 256, true);

    micron::sstr<4096> ok_out;
    sb::require(mio::realpath<4096>("/tmp/micron_io_regress", ok_out) != nullptr, true);
    sb::require(ok_out.size() > 0, true);
  }

  {
    const char *p = "/tmp/micron_io_regress/atomic.txt";
    put(p, "a");
    mio::file f(p, mio::modes::readwrite);

    sb::require(f.modify_atomic([](micron::string s) { return s + "x"; }) >= 0, true);
    sb::require(f.modify_atomic([](micron::string s) { return s + "x"; }) >= 0, true);
    sb::require(slurp(p) == micron::string("axx"), true);

    f.seek_to(0);
    sb::require(f.read<micron::string>() == micron::string("axx"), true);
  }

  {
    const char *p = "/tmp/micron_io_regress/append.txt";
    put(p, "hello");
    mio::file f(p, mio::modes::appendread);

    sb::require(f.modify([](micron::string) { return micron::string("hi"); }) < 0, true);
    sb::require(slurp(p) == micron::string("hello"), true);
  }

  {
    const char *p = "/tmp/micron_io_regress/clamp.txt";
    put(p, "0123456789012345678901234567890123456789");
    mio::file f(p, mio::modes::readwrite);

    sb::require(f.modify([](micron::sstr<16> s) { return s; }) < 0, true);
    sb::require(slurp(p).size(), 40ull);
  }

  {
    const char *p = "/tmp/micron_io_regress/lines.txt";
    put(p, "hello\n\nworld\n");
    mio::file f(p, mio::modes::read);
    micron::vector<micron::string> got;
    f.each_line([&got](const micron::string &l) { got.push_back(micron::string(l.c_str())); });
    sb::require(got.size(), 3ull);
    sb::require(got[0] == micron::string("hello"), true);
    sb::require(got[1].size(), 0ull);

    sb::require(micron::strlen(got[1].c_str()), 0ull);
    sb::require(got[2] == micron::string("world"), true);
  }

  {
    const char *p = "/tmp/micron_io_regress/lines_crlf.txt";
    put(p, "hello\r\n\r\nworld\r\n");
    mio::file f(p, mio::modes::read);
    micron::vector<micron::string> got;
    f.each_line([&got](const micron::string &l) { got.push_back(micron::string(l.c_str())); });
    sb::require(got.size(), 3ull);
    sb::require(micron::strlen(got[1].c_str()), 0ull);
  }

  {
    const char *p = "/tmp/micron_io_regress/hdr.txt";
    put(p, "h1\nh2\n\nbody-first\nbody-second\n");
    mio::file f(p, mio::modes::read);
    {
      for ( const auto &l : f.lines() ) {
        if ( l.size() == 0 ) break;
      }
    }

    sb::require(f.read<micron::string>() == micron::string("body-first\nbody-second\n"), true);
  }

  {
    const char *p = "/tmp/micron_io_regress/trunc.txt";
    put(p, "stale-content");
    mio::filesystem fs;
    fs.open(mio::path_t(p), mio::modes::readwrite);
    fs.open(mio::path_t(p), mio::modes::readwritecreate);
    sb::require(slurp(p).size(), 0ull);
  }

  {
    const char *p = "/tmp/micron_io_regress/serviceable.txt";
    put(p, "content");
    mio::filesystem fs;
    mio::file &a = fs.open(mio::path_t(p), mio::modes::readwrite);
    mio::file &b = fs.open(mio::path_t(p), mio::modes::read);
    sb::require(a.__handle.fd == b.__handle.fd, true);
  }

  {
    const char *p = "/tmp/micron_io_regress/wide.txt";
    put(p, "abcd");
    mio::cached_file<micron::wstr> cf(p, mio::modes::read);
    sb::require(cf.load(), 4ll);

    sb::require(cf.count(), 4ull / sizeof(micron::wstr::value_type));
  }

  {
    micron::heap_swiss_map<micron::string, int> m;
    m.insert(micron::string("alpha"), 1);
    m.insert(micron::string("beta"), 2);

    max_t need = micron::io::serialize::framed_size(m);
    sb::require(need > 0, true);
    micron::buffer buf(static_cast<usize>(need));
    max_t n = micron::io::serialize::frame_into(buf.data(), buf.size(), m);
    sb::require(n > 0, true);

    micron::heap_swiss_map<micron::string, int> out;
    sb::require(micron::io::serialize::unframe_from(buf.data(), static_cast<usize>(n), out) >= 0, true);
    int *a = out.find(micron::string("alpha"));
    int *b = out.find(micron::string("beta"));
    sb::require(a != nullptr && b != nullptr, true);
    sb::require(*a, 1);
    sb::require(*b, 2);
  }

  {
    (void)mio::mkdir(mio::path_t("/tmp/micron_io_regress/tree"));
    (void)mio::mkdir(mio::path_t("/tmp/micron_io_regress/tree/sub"));
    put("/tmp/micron_io_regress/tree/top.txt", "x");
    put("/tmp/micron_io_regress/tree/sub/deep.txt", "y");

    bool escaped = false;
    usize seen = 0;
    try {
      mio::ftw_all(mio::path("/tmp/micron_io_regress/tree"), [&](const mio::path_t &x) {
        ++seen;

        if ( micron::format::contains(x, "deep.txt") ) micron::exc<micron::except::io_error>("visitor");
        return true;
      });
    } catch ( const micron::except::io_error & ) {
      escaped = true;
    }
    sb::require(escaped, true);
    sb::require(seen > 0, true);
  }

  {
    (void)mio::mkdir(mio::path_t("/tmp/micron_io_regress/tree2"));
    (void)mio::mkdir(mio::path_t("/tmp/micron_io_regress/tree2/open"));
    put("/tmp/micron_io_regress/tree2/open/f.txt", "z");
    auto all = mio::ftw_all(mio::path("/tmp/micron_io_regress/tree2"));
    sb::require(all.size() > 0, true);
  }

  micron::io::println("io_regress: all sections passed");
  return 0;
}
