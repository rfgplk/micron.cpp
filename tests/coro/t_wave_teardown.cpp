//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Regression: three ways io::coro::wave could lose.
//
// (1) A LEAKED DIRECT DESCRIPTOR. Each item staged open -LINK-> read -LINK-> close. A soft link
//     severs on failure, so any read that failed -- a directory answering -EISDIR, a bad sector
//     answering -EIO -- cancelled the close with -ECANCELED (measured on 7.1.5: open=0, read=-21,
//     close=-125; with IOSQE_IO_HARDLINK on the read the same chain gives close=0). run() only ever
//     looked at the open and read results, so __release_slots() put the slot back in the free bitmap
//     with the file still installed. That does not exhaust the table -- installing into an occupied
//     slot replaces and closes what was there -- but the file stays open, and its inode pinned,
//     until that particular slot happens to be reused, which may be never.
//
// (2) A noexcept push() THAT ABORTED. sstring's const char* ctor answers an over-long name with
//     [[noreturn]] exc<library_error>, and push() is noexcept: a 256-byte filename took the whole
//     process down rather than being rejected.
//
// (3) A USE-AFTER-FREE AT TEARDOWN. Unlike an __io_op, a wave does not live inside the frame that
//     awaits it -- destroying a suspended coroutine destroys the wave while the reactor still has a
//     resume queued against it. ~wave() drains the CQ itself, so it dispatches the very completion
//     that would resume a frame into released slots and an unmapped slab.

#define MICRON_CORO_URING

#include "../../src/coroio.hpp"

#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
namespace cio = micron::io::coro;
namespace posix = micron::posix;

static int FAILS = 0;

static constexpr const char *DIR = "/var/tmp/micron_wave_td";
static constexpr const char *SUBDIR = "subdir";          // opens fine, reads -EISDIR
static constexpr i32 c_eisdir = -21;
static constexpr u32 DIRS_PER_PASS = 8;                  // half the batch leaks, pre-fix
static constexpr u32 PASSES = (micron::coro::__io_file_slots / DIRS_PER_PASS) + 8u;

static void
mkdir_p(const char *p)
{
  micron::syscall(SYS_mkdirat, -100, p, 0755);
}

static void
path_of(char *out, const char *a, const char *b)
{
  usize k = 0;
  for ( const char *p = a; *p; ++p ) out[k++] = *p;
  out[k++] = '/';
  for ( const char *p = b; *p; ++p ) out[k++] = *p;
  out[k] = 0;
}

static bool
mkfile(const char *name, u32 seed, usize n)
{
  char full[512];
  path_of(full, DIR, name);
  const long fd = micron::syscall(SYS_openat, -100, full, posix::o_create | posix::o_rdwr | posix::o_trunc, 0644);
  if ( fd < 0 ) return false;
  byte buf[512];
  usize done = 0;
  u32 st = 0x9e3779b9u ^ seed;
  while ( done < n ) {
    const usize want = (n - done) > sizeof(buf) ? sizeof(buf) : (n - done);
    for ( usize i = 0; i < want; ++i ) {
      st ^= st << 13;
      st ^= st >> 17;
      st ^= st << 5;
      buf[i] = static_cast<byte>('a' + (st % 26u));
    }
    const long w = micron::syscall(SYS_write, fd, buf, want);
    if ( w <= 0 ) break;
    done += static_cast<usize>(w);
  }
  micron::syscall(SYS_close, fd);
  return done == n;
}

static i32
open_dir()
{
  return static_cast<i32>(micron::syscall(SYS_openat, -100, DIR, posix::o_rdonly | posix::o_directory, 0));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// (1) a failed read must not strand its direct descriptor

static micron::task<i32>
failed_reads_do_not_exhaust_the_table(i32 dirfd, const micron::vector<micron::sstr<64>> &good)
{
  cio::wave w;
  u32 stranded = 0;
  for ( u32 p = 0; p < PASSES; ++p ) {
    w.begin(dirfd);
    for ( u32 i = 0; i < DIRS_PER_PASS && !w.full(); ++i )
      if ( !w.push(SUBDIR) ) co_return -5000;
    for ( usize i = 0; i < good.size() && !w.full(); ++i )
      if ( !w.push(good[i].c_str()) ) co_return -5001;

    const i32 rc = co_await w.run();
    if ( rc != 0 ) co_return -5002;

    for ( usize i = 0; i < w.size(); ++i ) {
      const auto &r = w[i];
      if ( r.unstaged ) continue;
      if ( i < DIRS_PER_PASS ) {
        if ( r.err != c_eisdir ) co_return -5003;      // the directory must report, not read
        // THE regression: a soft link severed here and the close came back -ECANCELED, leaving the
        // direct descriptor installed in the ring's file table with the slot marked free
        if ( r.close_err != 0 ) co_return -5007;
        continue;
      }
      if ( r.err != 0 ) co_return -5004 - (static_cast<i32>(p) << 8);
      if ( r.data == nullptr || r.len == 0 ) co_return -5005;
      if ( r.close_err != 0 ) co_return -5008;      // a successful chain closes too
    }
    stranded += w.stranded();
    w.clear();
  }
  if ( stranded != 0 ) co_return -5006;
  co_return static_cast<i32>(PASSES);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// (3) the teardown handshake, driven directly
//
// getting a real frame destroyed mid-batch is a timing race; the handshake it depends on is not, so
// that is what gets asserted. These mirror ~wave() and __dispatch_cqe's wop branch exactly.

static bool
dispatcher_claims(micron::coro::__io_wave &wv)
{
  u32 sus = micron::coro::__io_st_suspended;
  return wv.__st.compare_exchange_strong(sus, micron::coro::__io_st_resumed, micron::memory_order_acq_rel, micron::memory_order_acquire);
}

static i32
teardown_handshake()
{
  {      // the owner gets there first: the completer must NOT resume
    micron::coro::__io_wave wv{};
    wv.__st.store(micron::coro::__io_st_suspended, micron::memory_order_release);
    if ( !micron::coro::io::__wave_abandon(wv) ) return -5100;
    if ( dispatcher_claims(wv) ) return -5101;      // would have been the use-after-free
    if ( !micron::coro::io::__wave_settle(wv) ) return -5102;      // nothing was handed out; no wait
  }
  {      // the completer gets there first: the owner must WAIT, not free
    micron::coro::__io_wave wv{};
    wv.__st.store(micron::coro::__io_st_suspended, micron::memory_order_release);
    if ( !dispatcher_claims(wv) ) return -5110;
    if ( micron::coro::io::__wave_abandon(wv) ) return -5111;      // too late to abandon
    wv.__fin.store(1, micron::memory_order_release);               // run() reports in
    if ( !micron::coro::io::__wave_settle(wv) ) return -5112;
  }
  {      // never staged: settle is a no-op, it must not wait on a __fin that will never come
    micron::coro::__io_wave wv{};
    if ( micron::coro::io::__wave_abandon(wv) ) return -5120;      // state is submitted, not suspended
    if ( !micron::coro::io::__wave_settle(wv) ) return -5121;
  }
  return 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the force-close fallback. The hardlink means the close normally runs on its own, so this path
// only fires on a severed chain -- exercise it directly rather than leave it unproven.
//
// WARNING: IORING_REGISTER_FILES_UPDATE reports the NUMBER OF SLOTS UPDATED (1), not 0. Reading a
// successful eviction as a failure would strand the bitmap bit on every single use.
static micron::task<i32>
force_close_evicts()
{
  const i32 rid = micron::coro::io::wave_ring_id();
  if ( rid < 0 ) co_return -5300;
  const i32 slot = micron::coro::io::wave_slot_acquire();
  if ( slot < 0 ) co_return -5301;

  if ( !micron::coro::io::wave_slot_close(rid, slot) ) co_return -5302;      // empty slot: still a success
  micron::coro::io::wave_slot_release(rid, slot);

  if ( micron::coro::io::wave_slot_close(rid, -1) ) co_return -5303;                                  // rejected
  if ( micron::coro::io::wave_slot_close(rid, static_cast<i32>(micron::coro::__io_file_slots)) ) co_return -5304;
  if ( micron::coro::io::wave_slot_close(-1, 0) ) co_return -5305;      // no such ring
  co_return 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// waves that come and go under load; the destructor runs hundreds of times

static micron::task<i32>
churn(i32 dirfd, const micron::vector<micron::sstr<64>> &good, u32 rounds)
{
  i32 ok = 0;
  for ( u32 r = 0; r < rounds; ++r ) {
    cio::wave w;      // constructed and destroyed every round
    w.begin(dirfd);
    for ( usize i = 0; i < good.size() && !w.full(); ++i ) (void)w.push(good[i].c_str());
    const i32 rc = co_await w.run();
    if ( rc != 0 ) co_return -5200;
    for ( usize i = 0; i < w.size(); ++i ) {
      if ( w[i].unstaged ) continue;
      if ( w[i].err != 0 ) co_return -5201;
      ++ok;
    }
    if ( w.stranded() != 0 ) co_return -5202;
  }
  co_return ok;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  sb::print("=== CORO WAVE TEARDOWN + SLOT RECLAIM ===");

  mkdir_p(DIR);
  {
    char sub[512];
    path_of(sub, DIR, SUBDIR);
    mkdir_p(sub);
  }

  micron::vector<micron::sstr<64>> good;
  for ( u32 i = 0; i < 8; ++i ) {
    micron::sstr<64> n("g");
    n += static_cast<char>('a' + i);
    n += ".dat";
    sb::require(mkfile(n.c_str(), i, 128u + i * 37u));
    good.push_back(n);
  }

  // %%%% no ring needed for these two %%%%

  sb::test_case("push() rejects what it cannot hold instead of terminating");
  {
    cio::wave w;
    w.begin(-100);

    char big[cio::wave_name_cap + 64];
    for ( usize i = 0; i < sizeof(big) - 1; ++i ) big[i] = 'x';
    big[sizeof(big) - 1] = 0;
    sb::check(!w.push(big));      // pre-fix: exc -> terminate/abort, not a return

    char exact[cio::wave_name_cap + 1];      // strlen == cap: still one too many for the nul
    for ( usize i = 0; i < cio::wave_name_cap; ++i ) exact[i] = 'y';
    exact[cio::wave_name_cap] = 0;
    sb::check(!w.push(exact));

    char fits[cio::wave_name_cap];      // strlen == cap - 1: the largest that does fit
    for ( usize i = 0; i < cio::wave_name_cap - 1; ++i ) fits[i] = 'z';
    fits[cio::wave_name_cap - 1] = 0;
    sb::check(w.push(fits));

    sb::check(!w.push(nullptr));
    sb::check(!w.push(""));
    sb::check(w.size() == 1);
  }
  sb::end_test_case();

  sb::test_case("the teardown handshake hands the frame to exactly one side");
  {
    const i32 rc = teardown_handshake();
    sb::check(rc == 0);
    if ( rc != 0 ) sb::print("  rc = ", rc);
  }
  sb::end_test_case();

  const i32 dirfd = open_dir();
  sb::require(dirfd >= 0);

  bool have_wave = false;
  coro::start_coroutine_runtime(2);
  have_wave = coro::sync_wait([]() -> micron::task<bool> { co_return cio::wave::available(); }());
  coro::stop_coroutine_runtime();

  if ( !have_wave ) {
    sb::print("no sparse fixed-file table (needs >= 5.19) - wave unavailable, skipping the ring cases");
    micron::syscall(SYS_close, dirfd);
    sb::require(FAILS == 0);
    sb::print("=== CORO WAVE TEARDOWN PARTIALLY SKIPPED ===");
    return 1;
  }

  sb::test_case("a read that fails does not strand its direct descriptor");
  {
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(failed_reads_do_not_exhaust_the_table(dirfd, good));
    coro::stop_coroutine_runtime();
    sb::check(rc > 0);
    if ( rc < 0 ) sb::print("  rc = ", rc);
    sb::print("  ", PASSES, " passes x ", DIRS_PER_PASS, " failed reads = ", PASSES * DIRS_PER_PASS, " descriptors through a ",
              micron::coro::__io_file_slots, "-slot table");
  }
  sb::end_test_case();

  sb::test_case("the force-close fallback evicts, and reports success as success");
  {
    coro::start_coroutine_runtime(2);
    const i32 rc = coro::sync_wait(force_close_evicts());
    coro::stop_coroutine_runtime();
    sb::check(rc == 0);
    if ( rc != 0 ) sb::print("  rc = ", rc);
  }
  sb::end_test_case();

  sb::test_case("hundreds of waves construct and destruct without stranding anything");
  {
    coro::start_coroutine_runtime(4);
    const i32 rc = coro::sync_wait(churn(dirfd, good, 200));
    coro::stop_coroutine_runtime();
    sb::check(rc > 0);
    if ( rc < 0 ) sb::print("  rc = ", rc);
  }
  sb::end_test_case();

  micron::syscall(SYS_close, dirfd);
  sb::require(FAILS == 0);
  sb::print("=== CORO WAVE TEARDOWN PASSED ===");
  return 1;
}
