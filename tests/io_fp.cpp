//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// behavioral suite for the functional io layer: file::write(fn)/read(fn)/modify/
// modify_atomic/read_with/write_with/each_line/fold_lines/lines(), the fp.hpp free
// functions (with_file/modify/read_lines/_c set), combinator interop, and the
// functional members on cached_file/dir/ftw/binary/stream/pipes

#include "snowball/snowball.hpp"

#include "../src/function.hpp"
#include "../src/io/bin.hpp"
#include "../src/io/cached_file.hpp"
#include "../src/io/fp.hpp"
#include "../src/io/ftw.hpp"
#include "../src/io/os/dir.hpp"
#include "../src/io/pipe.hpp"
#include "../src/list.hpp"
#include "../src/vector/vector.hpp"

namespace mio = micron::io;

static void
put(const mio::path_t &p, const char *content)
{
  sb::require(mio::write_file(p, mc::string(content)) >= 0, true);
}

int
main()
{
  (void)mio::mkdir(mio::path_t("/tmp/micron_io_fp"));
  const mio::path_t base("/tmp/micron_io_fp/f.txt");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // producer write / consumer read roundtrips
  {
    mio::file f(base, mio::rwc);
    sb::require(f.write([] { return mc::string("hello\nworld\r\nlast"); }), 17);
    f.rewind();
    auto ro = f.read([](mc::string s) { return s.size(); });
    sb::require(ro.is_first(), true);
    sb::require(ro.cast<usize>(), 17ull);
    f.rewind();
    auto vo = f.read([](mc::string s) { (void)s; });      // void consumer -> unit_t
    sb::require(vo.is_first(), true);

    // tier-b roundtrip through the functional forms
    sb::require(f.modify([](mc::string) { return mc::string(); }), 0);
    f.rewind();
    sb::require(f.write([] {
      mc::vector<u32> v;
      for ( u32 i = 0; i < 64; ++i ) v.push_back(i * 3u);
      return v;
    }),
                256);
    f.rewind();
    auto vr = f.read([](mc::vector<u32> v) { return v.size(); });
    sb::require(vr.is_first(), true);
    sb::require(vr.cast<usize>(), 64ull);

    // tier-c (MFR1 framed) roundtrip
    sb::require(f.modify([](mc::string) { return mc::string(); }), 0);
    f.rewind();
    sb::require(f.write([] {
      mc::list<i32> l;
      l.push_back(7);
      l.push_back(11);
      l.push_back(13);
      return l;
    }) > 0,
                true);
    f.rewind();
    auto lr = f.read([](mc::list<i32> l) { return l.size(); });
    sb::require(lr.is_first(), true);
    sb::require(lr.cast<usize>(), 3ull);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // modify: shrink truncates, grow works, empty file seeds, missing path errors
  {
    mio::file f(base, mio::rwc);
    sb::require(f.write(mc::string("0123456789")), 10);
    sb::require(f.modify([](mc::string) { return mc::string("xy"); }), 2);      // shrink
    f.rewind();
    auto now = f.read([](mc::string s) { return s; });
    sb::require(now.is_first(), true);
    sb::require(now.cast<mc::string>() == mc::string("xy"), true);      // tail truncated
    sb::require(f.modify([](mc::string &s) { s += "z123"; }), 6);       // in-place grow
    sb::require(mio::modify(base, [](mc::string s) {
      s += "!";
      return s;
    }),
                7);
    sb::require(mio::modify(mio::path_t("/tmp/micron_io_fp/missing"), [](mc::string s) { return s; }) < 0, true);

    const mio::path_t empty_p("/tmp/micron_io_fp/empty");
    put(empty_p, "");
    sb::require(mio::modify(empty_p, [](mc::string s) {
      s += "seeded";
      return s;
    }),
                6);
  }

  // modify_atomic: content replaced, inode changes; tmpfile handles fail at rename
  {
    const mio::path_t ap("/tmp/micron_io_fp/atomic.txt");
    put(ap, "before");
    u64 ino_before = 0;
    {
      mio::file f(ap, mio::rw);
      ino_before = static_cast<u64>(f.inode());
      sb::require(f.modify_atomic([](mc::string) { return mc::string("after!"); }), 6);
    }
    auto disk = mio::read_file(ap);
    sb::require(disk.is_first(), true);
    sb::require(disk.cast<mc::string>() == mc::string("after!"), true);
    mio::file f2(ap, mio::rd);
    sb::require(static_cast<u64>(f2.inode()) != ino_before, true);

    mio::file tf = mio::tmpfile(mio::path_t("/tmp/micron_io_fp"));
    if ( tf.valid() )      // O_TMPFILE support
      sb::require(tf.modify_atomic([](mc::string s) { return s; }) < 0, true);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // byte-window streaming: > window roundtrip, producer overrun
  {
    mio::file f(base, mio::rwc);
    usize fed = 0;
    sb::require(f.write_with([&](byte *dst, usize cap) -> usize {
      if ( fed >= 40000 ) return 0;
      usize n = cap < 1000 ? cap : 1000;
      for ( usize i = 0; i < n; ++i ) dst[i] = static_cast<byte>('A' + ((fed + i) % 26));
      fed += n;
      return n;
    }),
                40000);
    f.rewind();
    usize seen = 0;
    u64 sum = 0;
    sb::require(f.read_with([&](const byte *p, usize n) {
      seen += n;
      for ( usize i = 0; i < n; ++i ) sum += p[i];
    }),
                40000);
    sb::require(seen, 40000ull);
    u64 expect = 0;
    for ( usize i = 0; i < 40000; ++i ) expect += static_cast<u64>('A' + (i % 26));
    sb::require(sum, expect);
    sb::require(f.write_with([](byte *, usize cap) -> usize { return cap + 1; }), static_cast<max_t>(-mc::error::invalid_arg));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // line api: policy fixtures
  {
    const mio::path_t lp("/tmp/micron_io_fp/lines.txt");
    mio::file lf(lp, mio::rwc);

    auto count_lines = [&](const char *content) -> max_t {
      sb::require(lf.modify([](mc::string) { return mc::string(); }) >= 0, true);
      sb::require(lf.write(mc::string(content)) >= 0, true);
      lf.rewind();
      return lf.each_line([](const mc::string &) { });
    };
    sb::require(count_lines(""), 0);              // empty file
    sb::require(count_lines("a\nb\n"), 2);        // trailing '\n': no empty final line
    sb::require(count_lines("a\nb"), 2);          // final unterminated line emitted
    sb::require(count_lines("a\n\n"), 2);         // "a", ""
    sb::require(count_lines("\n"), 1);            // one empty line

    // CRLF strip + content check via fold_lines (both shapes)
    sb::require(lf.modify([](mc::string) { return mc::string("x\r\ny\r\nz"); }), 7);
    lf.rewind();
    auto cat = lf.fold_lines(mc::string(), [](mc::string acc, const mc::string &ln) {
      acc += ln;
      return acc;
    });
    sb::require(cat.is_first(), true);
    sb::require(cat.cast<mc::string>() == mc::string("xyz"), true);
    lf.rewind();
    auto bytes = lf.fold_lines(usize{ 0 }, [](usize acc, const char *, usize n) { return acc + n; });
    sb::require(bytes.is_first(), true);
    sb::require(bytes.cast<usize>(), 3ull);

    // '\r' with no terminator is kept
    sb::require(lf.modify([](mc::string) { return mc::string("a\r"); }), 2);
    lf.rewind();
    lf.each_line([](const mc::string &ln) { sb::require(ln == mc::string("a\r"), true); });

    // huge line (1 MiB) crossing many 4096 windows
    {
      mc::string huge;
      huge.reserve(1048600);
      for ( usize i = 0; i < 1048576; ++i ) huge.push_back('q');
      huge += "\ntail";
      sb::require(mio::write_file(lp, huge) >= 0, true);
      mio::file hf(lp, mio::rd);
      usize first_len = 0, cnt = 0;
      hf.each_line([&](const char *, usize n) {
        if ( cnt == 0 ) first_len = n;
        ++cnt;
      });
      sb::require(cnt, 2ull);
      sb::require(first_len, 1048576ull);
    }

    // lazy lines(): range-for, error() clean, early break, closed-fd error
    put(lp, "1\n22\n333\n");
    {
      mio::file f(lp, mio::rd);
      usize n = 0, chars = 0;
      auto rng = f.lines();
      for ( const auto &ln : rng ) {
        ++n;
        chars += ln.size();
      }
      sb::require(n, 3ull);
      sb::require(chars, 6ull);
      sb::require(rng.error(), 0);
      sb::require(static_cast<bool>(rng), true);
    }
    {
      mio::file f(lp, mio::rd);
      usize n = 0;
      for ( const auto &ln : f.lines() ) {
        (void)ln;
        ++n;
        break;      // early break: range destructs cleanly
      }
      sb::require(n, 1ull);
    }
    {
      mio::file dead;      // invalid fd
      auto rng = dead.lines();
      usize n = 0;
      for ( const auto &ln : rng ) {
        (void)ln;
        ++n;
      }
      sb::require(n, 0ull);
      sb::require(rng.error() < 0, true);
      sb::require(static_cast<bool>(rng), false);
    }

    // read_lines eager bridge
    auto rl = mio::read_lines(lp);
    sb::require(rl.is_first(), true);
    sb::require(rl.cast<mc::vector<mc::string>>().size(), 3ull);

    // virtual (/proc) file: fstat-size-0 growth path
    mio::file pf("/proc/self/status", mio::rd);
    sb::require(pf.each_line([](const mc::string &) { }) > 5, true);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // with_file bracket + combinator interop
  {
    put(base, "data");
    auto sz = mio::with_file(base, mio::modes::read, [](mio::file &f) { return static_cast<usize>(f.size()); });
    sb::require(sz.is_first(), true);
    sb::require(sz.cast<usize>(), 4ull);
    auto vd = mio::with_file(base, [](mio::file &f) { (void)f; });
    sb::require(vd.is_first(), true);
    auto er = mio::with_file(mio::path_t("/tmp/micron_io_fp/nope"), [](mio::file &) { return 1; });
    sb::require(er.is_second(), true);
    sb::require(er.cast<mio::error_t>().code, 2);      // ENOENT

    auto counted = mc::fmap([](mc::vector<mc::string> ls) { return ls.size(); }, mio::read_lines(base));
    sb::require(counted.is_first(), true);
    sb::require(counted.cast<usize>(), 1ull);

    auto chained
        = mc::and_then(mio::read_file(base), [](mc::string s) { return mc::option<usize, mio::error_t>{ s.size() }; });
    sb::require(chained.is_first(), true);
    sb::require(chained.cast<usize>(), 4ull);

    auto fallback = mc::or_else(mio::read_file(mio::path_t("/tmp/micron_io_fp/nope")),
                                [](mio::error_t) { return mc::option<mc::string, mio::error_t>{ mc::string("fb") }; });
    sb::require(fallback.is_first(), true);
    sb::require(fallback.cast<mc::string>() == mc::string("fb"), true);

    sb::require(mc::value(mio::read_file(mio::path_t("/tmp/micron_io_fp/nope")), mc::string("dflt")) == mc::string("dflt"),
                true);

    auto lifted = mio::to_option(mio::write_file(base, mc::string("zz")));
    sb::require(lifted.is_first(), true);
    sb::require(lifted.cast<max_t>(), 2);

    // curried forms, data-last, OCaml pipe
    max_t piped = mc::string("data") | mio::write_file_c(base);
    sb::require(piped, 4);
    sb::require(mio::append_file_c(base)(mc::string("+1")), 2);
    auto mc_fn = mio::modify_c([](mc::string s) {
      s += "!";
      return s;
    });
    sb::require(mc_fn(base), 7);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // cached_file::modify — resident only until explicit persist
  {
    const mio::path_t cp("/tmp/micron_io_fp/cache.txt");
    put(cp, "base");
    mio::cached_file<mc::string> cf(cp.c_str(), mio::modes::readwrite);
    cf.load();
    cf.modify([](mc::string &s) { s += "-x"; });
    sb::require(cf.get() == mc::string("base-x"), true);
    auto disk = mio::read_file(cp);
    sb::require(disk.is_first(), true);
    sb::require(disk.cast<mc::string>() == mc::string("base"), true);      // untouched
    cf.set_start();
    cf.write();
    auto disk2 = mio::read_file(cp);
    sb::require(disk2.is_first(), true);
    sb::require(disk2.cast<mc::string>() == mc::string("base-x"), true);      // persisted
    cf.modify([](mc::string s) {
      s += "!";
      return s;
    });
    sb::require(cf.get().size(), 7ull);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // dir::fold == for_each_entry universe
  {
    mio::dir d("/tmp/micron_io_fp");
    usize folded = d.fold(usize{ 0 }, [](usize a, const mc::posix::__impl_dir &) { return a + 1; });
    usize walked = 0;
    d.for_each_entry([&](const mc::posix::__impl_dir &) { ++walked; });
    sb::require(folded, walked);
    sb::require(folded >= 3, true);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // ftw: visitor == collect, early stop, fold
  {
    auto collected = mio::ftw_files(mio::path("/tmp/micron_io_fp"));
    usize visited = mio::ftw_files(mio::path("/tmp/micron_io_fp"), [](const mio::path_t &) { });
    sb::require(visited, collected.size());
    usize early = 0;
    mio::ftw_files(mio::path("/tmp/micron_io_fp"), [&](const mio::path_t &) {
      ++early;
      return false;
    });
    sb::require(early, 1ull);
    usize folded = mio::ftw_fold(mio::path("/tmp/micron_io_fp"), usize{ 0 }, [](usize a, const mio::path_t &) { return a + 1; });
    sb::require(folded, mio::ftw_all(mio::path("/tmp/micron_io_fp")).size());
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // binary: whole-file window fold on a multi-window file
  {
    const mio::path_t bp("/tmp/micron_io_fp/bin.dat");
    mc::string big;
    big.reserve(200001);
    for ( usize i = 0; i < 200000; ++i ) big.push_back(static_cast<char>('a' + (i % 7)));
    sb::require(mio::write_file(bp, big), 200000);
    mio::binary<mc::string> b(bp.c_str(), mio::modes::read, 4096);
    auto fo = b.fold(usize{ 0 }, [](usize a, mio::bin_range_t w) { return a + w.size(); });
    sb::require(fo.is_first(), true);
    sb::require(fo.cast<usize>(), 200000ull);
    sb::require(b.each_window([](mio::bin_range_t) { }), 200000);
    mio::binary<mc::string> nb;      // no window buffer -> EINVAL
    sb::require(nb.each_window([](mio::bin_range_t) { }) < 0, true);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // stream: fold + each_line over buffered bytes
  {
    mio::stream<> st;
    const char txt[] = "a\nb\r\nc";
    st.append(reinterpret_cast<const byte *>(txt), 6);
    mc::string cat;
    sb::require(st.each_line([&](const mc::string &ln) { cat += ln; }), 3ull);
    sb::require(cat == mc::string("abc"), true);
    u32 sum = st.fold(u32{ 0 }, [](u32 a, byte bb) { return a + bb; });
    sb::require(sum, static_cast<u32>('a' + '\n' + 'b' + '\r' + '\n' + 'c'));
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // pipes: drain loops and producers (single process; capacity 64K)
  {
    mio::upipe up;
    micron::buffer payload(16384);
    for ( usize i = 0; i < 16384; ++i ) payload.begin()[i] = static_cast<byte>(i & 0xff);
    (void)up.write_bytes(payload.begin(), 16384);
    up.close_write();
    usize total = 0;
    u64 sum = 0, expect = 0;
    for ( usize i = 0; i < 16384; ++i ) expect += (i & 0xff);
    sb::require(up.each_chunk([&](const byte *p, usize len) {
      total += len;
      for ( usize i = 0; i < len; ++i ) sum += p[i];
    }),
                16384);
    sb::require(total, 16384ull);
    sb::require(sum, expect);
    sb::require(up.write_with([](byte *, usize) { return usize{ 0 }; }) < 0, true);      // write end closed

    mio::upipe up2;
    usize fed = 0;
    sb::require(up2.write_with([&](byte *dst, usize cap) -> usize {
      if ( fed >= 12288 ) return 0;
      usize n = cap < 4096 ? cap : 4096;
      for ( usize i = 0; i < n; ++i ) dst[i] = static_cast<byte>('q');
      fed += n;
      return n;
    }),
                12288);
    up2.close_write();
    usize got = 0;
    sb::require(up2.each_chunk([&](const byte *, usize len) { got += len; }), 12288);
    sb::require(got, 12288ull);

    mio::upipe up3;
    up3.set_read_nonblocking();
    sb::require(up3.each_chunk([](const byte *, usize) { }), 0);      // EAGAIN -> clean drain

    mc::posix::unlink("/tmp/micron_io_fp_fifo");
    mio::npipe np(mc::string("/tmp/micron_io_fp_fifo"));
    np.write(mc::string("hellohello"));
    np.set_nonblocking();
    usize ngot = 0;
    sb::require(np.each_chunk([&](const byte *, usize len) { ngot += len; }), 10);
    sb::require(ngot, 10ull);
    np.close();
    np.unlink();
  }

  micron::io::println("io_fp: all sections passed");
  return 0;
}
