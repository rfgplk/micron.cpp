//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// behavioral suite for io::flash: engine/dead-engine, plumbing rw vs posix ground truth, short-io
// continuation, queue+drain, linked chains, whole-file read/write/batch, porcelain cursor/universal,
// fp combinators, and a throughput smoke.

#include "snowball/snowball.hpp"

#include "../src/io/flash.hpp"
#include "../src/io/fsys.hpp"
#include "../src/string/strings.hpp"
#include "../src/vector/vector.hpp"

namespace mf = micron::io::flash;
namespace mio = micron::io;
namespace px = micron::posix;
namespace mc = micron;

static int FAILS = 0;
static constexpr const char *BASE = "/tmp/micron_io_flash";

// build BASE/name into a char buffer (no string-API dependency)
static mio::path_t
P(const char *name)
{
  char buf[256];
  usize i = 0;
  for ( const char *p = BASE; *p; p++ ) buf[i++] = *p;
  buf[i++] = '/';
  for ( const char *p = name; *p; p++ ) buf[i++] = *p;
  buf[i] = '\0';
  return mio::path_t(buf);
}

// BASE/<prefix><int><suffix>
static mio::path_t
Pn(const char *prefix, int v, const char *suffix)
{
  char buf[256];
  usize i = 0;
  for ( const char *p = BASE; *p; p++ ) buf[i++] = *p;
  buf[i++] = '/';
  for ( const char *p = prefix; *p; p++ ) buf[i++] = *p;
  char num[16];
  int j = 15;
  num[j--] = '\0';
  if ( v == 0 ) num[j--] = '0';
  for ( int x = v; x > 0; x /= 10 ) num[j--] = static_cast<char>('0' + (x % 10));
  for ( const char *p = &num[j + 1]; *p; p++ ) buf[i++] = *p;
  for ( const char *p = suffix; *p; p++ ) buf[i++] = *p;
  buf[i] = '\0';
  return mio::path_t(buf);
}

// content string "<prefix><int><suffix>"
static mc::string
Cn(const char *prefix, int v, const char *suffix)
{
  char buf[64];
  usize i = 0;
  for ( const char *p = prefix; *p; p++ ) buf[i++] = *p;
  char num[16];
  int j = 15;
  num[j--] = '\0';
  if ( v == 0 ) num[j--] = '0';
  for ( int x = v; x > 0; x /= 10 ) num[j--] = static_cast<char>('0' + (x % 10));
  for ( const char *p = &num[j + 1]; *p; p++ ) buf[i++] = *p;
  for ( const char *p = suffix; *p; p++ ) buf[i++] = *p;
  buf[i] = '\0';
  return mc::string(buf);
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  (void)mio::mkdir(mio::path_t(BASE));

  mf::engine &eng = mf::default_engine();
  if ( !eng.live() ) {
    sb::print("io_uring unavailable; flash tests SKIPPED");
    return 1;
  }

  // %%%% T1 engine

  sb::test_case("T1: engine init reports a live tier; dead engine returns bad_syscall");
  {
    sb::check(eng.live());
    sb::check(eng.level() >= mf::tier::basic);
    mf::engine dead;      // never init'd
    sb::check(!dead.live());
    byte b[4];
    sb::check(mf::read(3, b, 4, dead) == -micron::error::bad_syscall);
    auto r = mf::read_file(P("nope"), dead);
    sb::check(r.is_second());
  }
  sb::end_test_case();

  // %%%% T2 plumbing roundtrip vs posix

  sb::test_case("T2: pwrite/pread roundtrip matches posix ground truth");
  {
    mio::path_t p = P("plumb.bin");
    i32 fd = static_cast<i32>(px::openat(px::at_fdcwd, p.c_str(), px::o_rdwr | px::o_create | px::o_trunc, px::mode_file));
    sb::require(fd >= 0);
    byte src[4096];
    for ( usize i = 0; i < sizeof(src); i++ ) src[i] = static_cast<byte>((i * 7 + 3) & 0xff);
    sb::check(mf::pwrite(fd, src, sizeof(src), 0) == 4096);
    byte dst[4096] = {};
    sb::check(mf::pread(fd, dst, sizeof(dst), 0) == 4096);
    bool ok = true;
    for ( usize i = 0; i < sizeof(src); i++ )
      if ( dst[i] != src[i] ) ok = false;
    sb::check(ok);
    // posix cross-check
    byte pv[4096] = {};
    px::pread(fd, pv, sizeof(pv), 0);
    bool ok2 = true;
    for ( usize i = 0; i < sizeof(src); i++ )
      if ( pv[i] != src[i] ) ok2 = false;
    sb::check(ok2);
    px::close(fd);
    (void)mio::unlink(p);
  }
  sb::end_test_case();

  // %%%% T3 short-read continuation on a large file

  sb::test_case("T3: 4 MiB read via the continuation loop returns the whole file");
  {
    mio::path_t p = P("big.bin");
    const usize N = 4u * 1024 * 1024;
    mc::string blob;
    blob.reserve(N);
    for ( usize i = 0; i < N; i++ ) blob.push_back(static_cast<char>((i * 131 + 17) & 0xff));
    sb::require(mio::write_file(p, blob) == static_cast<max_t>(N));

    i32 fd = static_cast<i32>(px::openat(px::at_fdcwd, p.c_str(), px::o_rdonly, 0));
    sb::require(fd >= 0);
    mc::vector<byte> buf(N + 1);
    buf.set_size(N);
    max_t r = mf::pread(fd, buf.data(), N, 0);
    sb::check(r == static_cast<max_t>(N));
    bool ok = true;
    for ( usize i = 0; i < N; i++ )
      if ( buf[i] != static_cast<byte>(blob[i]) ) {
        ok = false;
        break;
      }
    sb::check(ok);
    px::close(fd);
    (void)mio::unlink(p);
  }
  sb::end_test_case();

  // %%%% T4 queue + drain

  sb::test_case("T4: queued preads reaped by user_data via drain");
  {
    mio::path_t p = P("queue.bin");
    byte data[512];
    for ( usize i = 0; i < sizeof(data); i++ ) data[i] = static_cast<byte>(i & 0xff);
    i32 fd = static_cast<i32>(px::openat(px::at_fdcwd, p.c_str(), px::o_rdwr | px::o_create | px::o_trunc, px::mode_file));
    sb::require(fd >= 0);
    px::pwrite(fd, data, sizeof(data), 0);

    byte slots[8][64];
    for ( int i = 0; i < 8; i++ ) (void)mf::queue_read(eng, fd, slots[i], 64, static_cast<u64>(i * 64), static_cast<u64>(i));
    (void)mf::submit_wait(eng, 8);
    int seen = 0;
    i32 results[8] = {};
    (void)mf::drain(eng, 8, [&](u64 ud, i32 res) {
      if ( ud < 8 ) results[ud] = res;
      seen++;
    });
    sb::check(seen == 8);
    bool ok = true;
    for ( int i = 0; i < 8; i++ ) {
      if ( results[i] != 64 ) ok = false;
      for ( int j = 0; j < 64; j++ )
        if ( slots[i][j] != static_cast<byte>((i * 64 + j) & 0xff) ) ok = false;
    }
    sb::check(ok);
    px::close(fd);
    (void)mio::unlink(p);
  }
  sb::end_test_case();

  // %%%% T5 chain open->read->close

  if ( eng.level() == mf::tier::fixed ) {
    sb::test_case("T5: linked open->read->close chain reads a file in one submission");
    {
      mio::path_t p = P("chain5.txt");
      const char msg[] = "chained whole-file read via fixed-file slot";
      sb::require(mio::write_file(p, mc::string(msg)) > 0);
      char buf[80] = {};
      mf::chain ch(eng);
      ch.open(p.c_str(), px::o_rdonly).read(buf, sizeof(msg) - 1, 0).close();
      i32 rc = ch.run();
      sb::check(rc == 0);
      sb::check(ch.res(1) == static_cast<i32>(sizeof(msg) - 1));
      bool ok = true;
      for ( usize i = 0; i < sizeof(msg) - 1; i++ )
        if ( buf[i] != msg[i] ) ok = false;
      sb::check(ok);
      (void)mio::unlink(p);
    }
    sb::end_test_case();

    // %%%% T6 chain failure + slot/fd stability
    sb::test_case("T6: chain on a missing file yields -no_entry (never -ECANCELED), no slot leak");
    {
      i32 rc0 = 0;
      // run the failing chain far more times than the fixed-file table has slots; if slots (or fds)
      // leaked, acquire_slot would starve and a later legitimate chain would stop working
      for ( int i = 0; i < 512; i++ ) {
        mf::chain ch(eng);
        ch.open(P("does_not_exist_xyz").c_str(), px::o_rdonly).read(nullptr, 8, 0).close();
        rc0 = ch.run();
      }
      sb::check(rc0 == -micron::error::no_entry);
      sb::check(rc0 != -micron::error::operation_canceled);
      // a real chain still succeeds afterwards -> no slot exhaustion
      mio::path_t p = P("chain6_ok.txt");
      sb::require(mio::write_file(p, mc::string("still-works")) == 11);
      char buf[16] = {};
      mf::chain ok(eng);
      ok.open(p.c_str(), px::o_rdonly).read(buf, 11, 0).close();
      sb::check(ok.run() == 0);
      sb::check(buf[0] == 's' && buf[10] == 's');
      (void)mio::unlink(p);
    }
    sb::end_test_case();
  }

  // %%%% T8 whole-file read/write/append/sync

  sb::test_case("T8: read_file / write_file / append_file / write_file_sync roundtrips");
  {
    mio::path_t p = P("whole.txt");
    sb::check(mf::write_file(p, mc::string("hello ")) == 6);
    sb::check(mf::append_file(p, mc::string("world")) == 5);
    auto r = mf::read_file(p);
    sb::require(r.is_first());
    sb::check(r.cast<mc::string>() == mc::string("hello world"));
    sb::check(mf::write_file_sync(p, mc::string("synced")) == 6);
    auto r2 = mf::read_file(p);
    sb::check(r2.is_first() && r2.cast<mc::string>() == mc::string("synced"));
    // size-0 virtual file
    auto rv = mf::read_file(mio::path_t("/proc/self/status"));
    sb::check(rv.is_first() && rv.cast<mc::string>().size() > 0);
    (void)mio::unlink(p);
  }
  sb::end_test_case();

  // %%%% T8b caller-provided-target overloads

  sb::test_case("T8b: read_file(p, target) fills in place; read_files(paths, target) appends");
  {
    mio::path_t p = P("whole2.txt");
    sb::require(mf::write_file(p, mc::string("hello world")) == 11);

    mc::string s;
    sb::check(mf::read_file(p, s) == 11);
    sb::check(s == mc::string("hello world"));
    sb::require(mf::write_file(p, mc::string("abc")) == 3);
    sb::check(mf::read_file(p, s) == 3);      // reuse: replaces and shrinks
    sb::check(s == mc::string("abc") && s.size() == 3);

    mc::vector<byte> v;
    sb::check(mf::read_file(p, v) == 3);
    sb::check(v.size() == 3 && v[0] == static_cast<byte>('a'));

    // explicit engine arg + dead engine
    sb::check(mf::read_file(p, s, mf::default_engine()) == 3);
    mf::engine dead;      // never init'd
    sb::check(mf::read_file(p, s, dead) == -micron::error::bad_syscall);

    sb::check(mf::read_file(P("nope_t"), s) < 0);      // missing -> negative, target untouched
    sb::check(s == mc::string("abc"));

    sb::check(mf::read_file(mio::path_t("/proc/self/status"), s) > 0);      // size-0 virtual file
    sb::check(s.size() > 0);

    // batch into a caller vector: appends in input order, returns the success count
    mc::vector<mio::path_t> paths;
    paths.push_back(p);
    paths.push_back(P("ghost_t"));
    mc::vector<mc::option<mc::string, mio::error_t>> results;
    sb::check(mf::read_files(paths, results) == 1);
    sb::require(results.size() == 2);
    sb::check(results[0].is_first() && results[0].cast<mc::string>() == mc::string("abc"));
    sb::check(results[1].is_second());
    sb::check(mf::read_files(paths, results) == 1);      // append semantics
    sb::check(results.size() == 4);
    (void)mio::unlink(p);
  }
  sb::end_test_case();

  // %%%% T9 batch read_files

  sb::test_case("T9: read_files loads N files with input-order per-item results");
  {
    mc::vector<mio::path_t> paths;
    for ( int i = 0; i < 32; i++ ) {
      mio::path_t p = Pn("batch_", i, ".txt");
      sb::require(mio::write_file(p, Cn("file-", i, "-payload")) > 0);
      paths.push_back(p);
    }
    auto results = mf::read_files(paths);
    sb::require(results.size() == 32);
    bool ok = true;
    for ( int i = 0; i < 32; i++ ) {
      if ( !results[i].is_first() ) {
        ok = false;
        continue;
      }
      if ( results[i].cast<mc::string>() != Cn("file-", i, "-payload") ) ok = false;
    }
    sb::check(ok);
    for ( int i = 0; i < 32; i++ ) (void)mio::unlink(paths[i]);
  }
  sb::end_test_case();

  // %%%% T10 batch partial failure

  sb::test_case("T10: read_files reports per-item errors without poisoning the batch");
  {
    mc::vector<mio::path_t> paths;
    mio::path_t a = P("exists_a.txt");
    mio::path_t b = P("exists_b.txt");
    sb::require(mio::write_file(a, mc::string("AAA")) == 3);
    sb::require(mio::write_file(b, mc::string("BBBB")) == 4);
    paths.push_back(a);
    paths.push_back(P("ghost1"));
    paths.push_back(b);
    paths.push_back(P("ghost2"));
    auto results = mf::read_files(paths);
    sb::require(results.size() == 4);
    sb::check(results[0].is_first() && results[0].cast<mc::string>() == mc::string("AAA"));
    sb::check(results[1].is_second());
    sb::check(results[2].is_first() && results[2].cast<mc::string>() == mc::string("BBBB"));
    sb::check(results[3].is_second());
    (void)mio::unlink(a);
    (void)mio::unlink(b);
  }
  sb::end_test_case();

  // %%%% T11 stat_many / rename / remove / mkdir / move

  sb::test_case("T11: stat_many + rename/remove/mkdir/move via the ring");
  {
    mio::path_t f = P("stat_me.bin");
    sb::require(mio::write_file(f, mc::string("0123456789")) == 10);
    mc::vector<mio::path_t> ps;
    ps.push_back(f);
    ps.push_back(P("stat_ghost"));
    auto sr = mf::stat_many(ps);
    sb::require(sr.size() == 2);
    sb::check(sr[0].is_first() && sr[0].cast<px::statx_t>().stx_size == 10);
    sb::check(sr[1].is_second());

    mio::path_t g = P("stat_renamed.bin");
    if ( micron::kernel::has(micron::kernel::feature::uring_renameat) ) {
      sb::check(mf::rename(f, g) == 0);
      sb::check(mio::file_size(g) == 10);
      sb::check(mf::remove(g) == 0);
      sb::check(!mio::exists(g.c_str()));
    } else {
      (void)mio::unlink(f);
    }

    if ( micron::kernel::has(micron::kernel::feature::uring_mkdirat) ) {
      mio::path_t d = P("newdir");
      (void)mio::rmdir(d);
      sb::check(mf::mkdir(d) == 0);
      sb::check(mio::file_type(d) == px::node_types::directory);
      (void)mio::rmdir(d);
    }
  }
  sb::end_test_case();

  // %%%% T12 copy

  sb::test_case("T12: copy produces a byte-equal file");
  {
    mio::path_t a = P("copy_src.bin");
    mio::path_t b = P("copy_dst.bin");
    mc::string blob;
    for ( int i = 0; i < 200000; i++ ) blob.push_back(static_cast<char>((i * 5 + 1) & 0xff));
    sb::require(mio::write_file(a, blob) == static_cast<max_t>(blob.size()));
    max_t c = mf::copy(a, b);
    sb::check(c == static_cast<max_t>(blob.size()));
    auto rb = mf::read_file(b);
    sb::require(rb.is_first());
    sb::check(rb.cast<mc::string>().size() == blob.size());
    sb::check(rb.cast<mc::string>() == blob);
    (void)mio::unlink(a);
    (void)mio::unlink(b);
  }
  sb::end_test_case();

  // %%%% T13 porcelain file

  sb::test_case("T13: flash::file cursor / universal write-read / modify");
  {
    mio::path_t p = P("porc.txt");
    {
      mf::file f(p, mio::modes::write);
      sb::require(f.valid());
      sb::check(f.write(mc::string("abcdefgh")) == 8);
    }
    {
      mf::file f(p, mio::modes::read);
      sb::require(f.valid());
      char b[4] = {};
      sb::check(f.read(b, 4) == 4);
      sb::check(f.tell() == 4);
      char b2[4] = {};
      sb::check(f.read_at(0, b2, 4) == 4);
      sb::check(f.tell() == 4);      // read_at leaves the cursor
      sb::check(b[0] == 'a' && b2[0] == 'a');
      sb::check(f.size() == 8);
    }
    // modify: uppercase
    max_t m = mf::modify(p, [](mc::string s) {
      for ( usize i = 0; i < s.size(); i++ )
        if ( s[i] >= 'a' && s[i] <= 'z' ) s[i] = static_cast<char>(s[i] - 32);
      return s;
    });
    sb::check(m >= 0);
    auto r = mf::read_file(p);
    sb::check(r.is_first() && r.cast<mc::string>() == mc::string("ABCDEFGH"));
    (void)mio::unlink(p);
  }
  sb::end_test_case();

  // %%%% T14 fp combinators

  sb::test_case("T14: with_file / map_files / fold_files / curried");
  {
    mio::path_t p = P("fp.txt");
    // curried write (invoked directly; micron::string overloads operator| so the pipe form is
    // reserved for non-string data)
    auto sink = mf::write_file_c(p);
    max_t w = sink(mc::string("piped-content"));
    sb::check(w == 13);
    // with_file returns a value
    auto sz = mf::with_file(p, mio::modes::read, [](mf::file &f) { return f.size(); });
    sb::require(sz.is_first());
    sb::check(sz.cast<max_t>() == 13);

    mc::vector<mio::path_t> paths;
    for ( int i = 0; i < 4; i++ ) {
      mio::path_t fp = Pn("fp_", i, "");
      mc::string c;
      c.push_back(static_cast<char>('0' + i));
      sb::require(mio::write_file(fp, c) == 1);
      paths.push_back(fp);
    }
    // map_files: length of each
    auto lens = mf::map_files(paths, [](mc::string s) { return s.size(); });
    sb::require(lens.size() == 4);
    bool ok = true;
    for ( int i = 0; i < 4; i++ )
      if ( !lens[i].is_first() || lens[i].cast<usize>() != 1 ) ok = false;
    sb::check(ok);
    // fold_files: concatenate
    auto cat = mf::fold_files<mc::string>(paths, mc::string(""), [](mc::string acc, const mc::string &c) {
      acc += c;
      return acc;
    });
    sb::require(cat.is_first());
    sb::check(cat.cast<mc::string>() == mc::string("0123"));
    for ( int i = 0; i < 4; i++ ) (void)mio::unlink(paths[i]);
    (void)mio::unlink(p);
  }
  sb::end_test_case();

  // %%%% T17 chain on a dead engine

  sb::test_case("T17: chain builders on a dead engine report, never fault");
  {
    // ring::get_sqe() dereferences sq_tail unconditionally, and on a dead engine that is null.
    // every other entry point is __guard-protected but the chain builders are not, so this used
    // to be a reachable null-deref rather than an error return.
    mf::engine dead;
    sb::check(!dead.live());
    mf::chain ch(dead);
    byte buf[8] = {};
    ch.open(P("never_opened").c_str(), px::o_rdonly).read(buf, sizeof(buf), 0).close();
    sb::check(ch.run() == -micron::error::bad_syscall);

    // ...and a chain with no owning open() must not silently target fd -1
    mf::chain orphan(eng);
    sb::check(orphan.read(buf, sizeof(buf), 0).run() < 0);
  }
  sb::end_test_case();

  // %%%% T18 wave isolation

  sb::test_case("T18: an abandoned completion cannot corrupt the next batch");
  {
    // strand a cqe on the ring: queue a read, submit, and never reap it. tagged user_data means
    // the next batch must consume-and-drop it rather than write it into a result slot.
    mio::path_t decoy = P("wave_decoy.bin");
    sb::require(mio::write_file(decoy, mc::string("0123456789")) > 0);
    i32 dfd = static_cast<i32>(px::openat(px::at_fdcwd, decoy.c_str(), px::o_rdonly, 0u));
    sb::require(dfd >= 0);
    byte junk[8] = {};
    (void)mf::queue_read(eng, dfd, junk, 8, 0, 0 /* raw 0: collides with a slot index pre-tagging */);
    (void)mf::submit(eng);

    mc::vector<mio::path_t> paths;
    for ( int i = 0; i < 6; i++ ) {
      mio::path_t p = Pn("wv_", i, ".txt");
      sb::require(mio::write_file(p, Cn("wave-", i, "-body")) > 0);
      paths.push_back(p);
    }
    auto got = mf::read_files(paths);
    sb::require(got.size() == 6);
    bool ok = true;
    for ( int i = 0; i < 6; i++ ) {
      if ( !got[i].is_first() ) {
        ok = false;
        continue;
      }
      if ( got[i].cast<mc::string>() != Cn("wave-", i, "-body") ) ok = false;
    }
    sb::check(ok);
    px::close(dfd);
    (void)mio::unlink(decoy);
    for ( int i = 0; i < 6; i++ ) (void)mio::unlink(paths[i]);
  }
  sb::end_test_case();

  // %%%% T19 batch larger than one window

  sb::test_case("T19: read_files/write_files correct when n exceeds the batch window");
  {
    // the window is capped at 64, so 150 forces multiple passes and exercises the roll-over path
    const int NF = 150;
    mc::vector<mio::path_t> paths;
    for ( int i = 0; i < NF; i++ ) {
      mio::path_t p = Pn("win_", i, ".txt");
      sb::require(mio::write_file(p, Cn("window-", i, "-payload")) > 0);
      paths.push_back(p);
    }
    auto got = mf::read_files(paths);
    sb::require(got.size() == static_cast<usize>(NF));
    bool ok = true;
    for ( int i = 0; i < NF; i++ ) {
      if ( !got[i].is_first() || got[i].cast<mc::string>() != Cn("window-", i, "-payload") ) ok = false;
    }
    sb::check(ok);

    // same for the write side
    mc::vector<mc::string> bodies;
    mc::vector<mf::write_spec> specs;
    for ( int i = 0; i < NF; i++ ) bodies.push_back(Cn("wr-", i, "-out"));
    for ( int i = 0; i < NF; i++ )
      specs.push_back(mf::write_spec{ Pn("wout_", i, ".txt"), reinterpret_cast<const byte *>(bodies[i].c_str()), bodies[i].size() });
    auto wres = mf::write_files(specs);
    sb::require(wres.size() == static_cast<usize>(NF));
    bool wok = true;
    for ( int i = 0; i < NF; i++ ) {
      if ( !wres[i].is_first() || wres[i].cast<max_t>() != static_cast<max_t>(bodies[i].size()) ) wok = false;
      auto rb = mio::read_file(Pn("wout_", i, ".txt"));
      if ( !rb.is_first() || rb.cast<mc::string>() != bodies[i] ) wok = false;
    }
    sb::check(wok);
    for ( int i = 0; i < NF; i++ ) {
      (void)mio::unlink(paths[i]);
      (void)mio::unlink(Pn("wout_", i, ".txt"));
    }
  }
  sb::end_test_case();

  // %%%% T20 whole-file reads across the probe boundary

  sb::test_case("T20: read_file exact at, below and above the 64 KiB probe size");
  {
    // read_file has no statx size probe below 64 KiB and takes a different path at/above it, so
    // the boundary itself is the interesting case (an off-by-one here silently truncates).
    const usize sizes[5] = { 0, 1, 65535, 65536, 65537 };
    bool ok = true;
    for ( usize k = 0; k < 5; k++ ) {
      mio::path_t p = Pn("probe_", static_cast<int>(k), ".bin");
      mc::string body;
      for ( usize i = 0; i < sizes[k]; i++ ) body.push_back(static_cast<char>('a' + (i % 26)));
      sb::require(mf::write_file(p, body) == static_cast<max_t>(sizes[k]));

      auto r = mf::read_file<mc::string>(p);
      if ( !r.is_first() || r.cast<mc::string>().size() != sizes[k] )
        ok = false;
      else if ( sizes[k] > 0 && r.cast<mc::string>() != body )
        ok = false;

      mc::string target;
      const max_t n = mf::read_file(p, target);
      if ( n != static_cast<max_t>(sizes[k]) || target.size() != sizes[k] )
        ok = false;
      else if ( sizes[k] > 0 && target != body )
        ok = false;

      (void)mio::unlink(p);
    }
    sb::check(ok);
  }
  sb::end_test_case();

  // %%%% T21 write_file_sync durability + copy beyond one slab

  sb::test_case("T21: write_file_sync round-trips; copy exceeds a single pool slab");
  {
    mio::path_t p = P("sync_once.bin");
    mc::string body;
    for ( int i = 0; i < 5000; i++ ) body.push_back(static_cast<char>('A' + (i % 26)));
    sb::check(mf::write_file_sync(p, body) == static_cast<max_t>(body.size()));
    auto back = mf::read_file<mc::string>(p);
    sb::check(back.is_first() && back.cast<mc::string>() == body);

    // copy now runs on a 256 KiB pool slab; go well past one slab so the chunk loop is exercised
    mio::path_t src = P("copy_big_src.bin");
    mio::path_t dst = P("copy_big_dst.bin");
    mc::string big;
    for ( int i = 0; i < 700 * 1024; i++ ) big.push_back(static_cast<char>(i & 0xff));
    sb::require(mf::write_file(src, big) == static_cast<max_t>(big.size()));
    sb::check(mf::copy(src, dst) == static_cast<max_t>(big.size()));
    auto cb = mf::read_file<mc::string>(dst);
    sb::check(cb.is_first() && cb.cast<mc::string>().size() == big.size());
    sb::check(cb.is_first() && cb.cast<mc::string>() == big);

    (void)mio::unlink(p);
    (void)mio::unlink(src);
    (void)mio::unlink(dst);
  }
  sb::end_test_case();

  sb::test_case("T22: a pool-less engine falls back to the stack buffer instead of unwrapping an error");
  {
    // fixed_bufs = 0 is the documented "no pool" engine, so acquire_buf hands back the ERROR
    // alternative. copy() must read that as "no slab, use the stack", never as a live pool_buf --
    // unwrapping option's inactive alternative builds one out of indeterminate bytes, and a
    // nonzero stale __e makes valid() lie, so copy then reads into a garbage pointer.
    mf::engine eng;
    mf::engine_opts o{};
    o.fixed_bufs = 0;
    sb::require(eng.init(o) == 0);
    sb::check(!eng.has_pool());

    auto slab = mf::acquire_buf(eng);
    sb::check(!slab.is_first());      // error alternative, not a plausible-looking pool_buf

    mio::path_t src = P("nopool_src.bin");
    mio::path_t dst = P("nopool_dst.bin");
    mc::string body;
    for ( int i = 0; i < 40 * 1024; i++ ) body.push_back(static_cast<char>((i * 31 + 7) & 0xff));
    sb::require(mf::write_file(src, body, eng) == static_cast<max_t>(body.size()));
    sb::check(mf::copy(src, dst, eng) == static_cast<max_t>(body.size()));
    auto back = mf::read_file<mc::string>(dst, eng);
    sb::check(back.is_first() && back.cast<mc::string>() == body);

    (void)mio::unlink(src);
    (void)mio::unlink(dst);
  }
  sb::end_test_case();

  sb::test_case("T23: an sqpoll engine really performs batched work (never 'success' over nothing)");
  {
    // on an sqpoll ring enter() returns 0 -- the poll thread drains the sq itself -- so deriving
    // the in-flight count from submit() abandons every wave unreaped while still reporting
    // success: destinations get created and O_TRUNC'd to zero, their fds leak, and stat_many
    // hands back uninitialized statx bytes in the success alternative.
    mf::engine eng;
    mf::engine_opts o{};
    o.sqpoll = true;
    if ( eng.init(o) != 0 ) {
      sb::print("  sqpoll engine unavailable (privileges); T23 SKIPPED");
    } else {
      mio::path_t pa = P("sqp_a.bin");
      mio::path_t pb = P("sqp_b.bin");
      mc::string a, b;
      for ( int i = 0; i < 9000; i++ ) a.push_back(static_cast<char>('a' + (i % 26)));
      for ( int i = 0; i < 1234; i++ ) b.push_back(static_cast<char>('A' + (i % 26)));

      mc::vector<mf::write_spec> specs;
      specs.push_back(mf::write_spec{ pa, reinterpret_cast<const byte *>(a.data()), a.size() });
      specs.push_back(mf::write_spec{ pb, reinterpret_cast<const byte *>(b.data()), b.size() });
      auto w = mf::write_files(specs, eng);
      sb::require(w.size() == 2);
      sb::check(w[0].is_first() && w[1].is_first());
      if ( w[0].is_first() ) sb::check(w[0].cast<max_t>() == static_cast<max_t>(a.size()));

      mc::vector<mio::path_t> paths;
      paths.push_back(pa);
      paths.push_back(pb);
      auto rr = mf::read_files(paths, eng);
      sb::require(rr.size() == 2);
      sb::check(rr[0].is_first() && rr[0].cast<mc::string>() == a);
      sb::check(rr[1].is_first() && rr[1].cast<mc::string>() == b);

      auto sm = mf::stat_many(paths, eng);
      sb::require(sm.size() == 2);
      sb::check(sm[0].is_first() && sm[0].cast<px::statx_t>().stx_size == a.size());
      sb::check(sm[1].is_first() && sm[1].cast<px::statx_t>().stx_size == b.size());

      (void)mio::unlink(pa);
      (void)mio::unlink(pb);
    }
  }
  sb::end_test_case();

  sb::test_case("T24: a pending queue_* op does not deadlock a batch call on the same engine");
  {
    // queue_* stages without submitting, so to_submit is nonzero when the batch starts. counting
    // those foreign sqes as part of this wave makes the reap loop wait for completions that can
    // never match its tag+gen -- read_files then blocks in io_uring_enter forever.
    mf::engine eng;
    sb::require(eng.init(mf::engine_opts{}) == 0);
    mio::path_t p = P("queued_coexist.bin");
    mc::string body;
    for ( int i = 0; i < 5000; i++ ) body.push_back(static_cast<char>((i * 17 + 3) & 0xff));
    sb::require(mf::write_file(p, body, eng) == static_cast<max_t>(body.size()));

    i32 fd = static_cast<i32>(px::openat(px::at_fdcwd, p.c_str(), px::o_rdonly, 0));
    sb::require(fd >= 0);
    byte qbuf[64]{};
    (void)mf::queue_read(eng, fd, qbuf, sizeof(qbuf), 0, 0x99u);      // staged, deliberately not submitted

    mc::vector<mio::path_t> paths;
    paths.push_back(p);
    auto rr = mf::read_files(paths, eng);      // must return rather than hang
    sb::require(rr.size() == 1);
    sb::check(rr[0].is_first() && rr[0].cast<mc::string>() == body);

    px::close(fd);
    (void)mio::unlink(p);
  }
  sb::end_test_case();

  // %%%% T16 throughput smoke (printed, not asserted)

  sb::test_case("T16: throughput smoke -- flash read_files vs posix loop (printed)");
  {
    mc::vector<mio::path_t> paths;
    const int NF = 24;
    mc::string blob;
    for ( int i = 0; i < 1024 * 1024; i++ ) blob.push_back(static_cast<char>(i & 0xff));
    for ( int i = 0; i < NF; i++ ) {
      mio::path_t p = Pn("tp_", i, "");
      (void)mio::write_file(p, blob);
      paths.push_back(p);
    }
    micron::timespec_t t0{}, t1{}, t2{};
    micron::clock_gettime(micron::clock_monotonic, t0);
    auto fr = mf::read_files(paths);
    micron::clock_gettime(micron::clock_monotonic, t1);
    usize total = 0;
    for ( usize i = 0; i < fr.size(); i++ )
      if ( fr[i].is_first() ) total += fr[i].cast<mc::string>().size();
    for ( int i = 0; i < NF; i++ ) {
      auto r = mio::read_file(paths[i]);
      (void)r;
    }
    micron::clock_gettime(micron::clock_monotonic, t2);
    const i64 flash_ns = (t1.tv_sec - t0.tv_sec) * 1000000000ll + (t1.tv_nsec - t0.tv_nsec);
    const i64 posix_ns = (t2.tv_sec - t1.tv_sec) * 1000000000ll + (t2.tv_nsec - t1.tv_nsec);
    sb::print("  flash read_files: ", flash_ns / 1000, " us   posix loop: ", posix_ns / 1000, " us   (", total / (1024 * 1024), " MiB)");
    sb::check(total == static_cast<usize>(NF) * 1024 * 1024);
    for ( int i = 0; i < NF; i++ ) (void)mio::unlink(paths[i]);
  }
  sb::end_test_case();

  (void)mio::rmdir(mio::path_t(BASE));

  sb::require(FAILS == 0);
  sb::print("=== ALL FLASH TESTS PASSED ===");
  return 1;
}
