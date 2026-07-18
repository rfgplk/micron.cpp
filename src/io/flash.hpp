//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../kernel.hpp"
#include "../memory_block.hpp"
#include "../sum.hpp"
#include "../type_traits.hpp"
#include "../vector/vector.hpp"

#include "../linux/sys/stat.hpp"
#include "../linux/sys/uring.hpp"

#include "bits.hpp"
#include "fn.hpp"
#include "os/os_file.hpp"
#include "paths.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// io::flash -- filesystem io backed ENTIRELY by io_uring, tuned for throughput and low latency.
//
// three api styles, matching the rest of io_v3:
//   plumbing  -- free fns over raw fds (flash::read/pread/write/fsync/..., i32 + fd_t overloads,
//                return max_t, negative == -errno); a queue layer (queue_*/submit/reap/drain) and a
//                linked-sqe chain builder for batching
//   porcelain -- class flash::file (RAII, cursor) + fsys-mirror free fns (open_file/read_file/
//                write_file/copy/rename/...); return option<T, io::error_t>; caller-target
//                overloads (read_file(p, target[, eng]) / read_files(paths, target)) return max_t
//   functional -- with_file/map_files/fold_files/modify + curried *_c combinators
//
// flash is SYNCHRONOUS-COMPLETING: every call submits and waits inline; batching (read_files et al)
// is where io_uring beats the synchronous posix layer. it does NOT use the coroutine runtime and
// coexists with it (separate ring instances).
//
// UNLIKE the posix fsys layer, flash::write_file/append_file/copy do NOT implicitly fsync (io_v3
// dropped implicit O_SYNC) -- use write_file_sync / open_opts.sync for durability.
//
// availability is runtime-gated by kernel version + ring probe (see flash::tier); there is no posix
// fallback -- below 5.6 the path-level surface returns -error::bad_syscall. one engine per thread.

namespace micron
{
namespace io
{
namespace flash
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// capability tier

enum class tier : u8 {
  none,          // ring dead -> every op returns -error::bad_syscall
  plumbing,      // 5.1-5.5: readv/writev/fsync/fixed only; path-level surface unavailable
  basic,         // 5.6+: op_read/write/openat/close/statx/fallocate/...; 2-submission path ops
  fixed          // 5.19+: sparse fixed-file table -> 1-submission whole-file chains
};

inline constexpr u32 auto_slots = 0xffffffffu;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// user_data namespace.
//
// the hazard is not that the producers overlap -- it is that a stray cqe is INDISTINGUISHABLE from a
// live one, so a completion left over from an abandoned op lands in someone else's result array.
// tagging makes strays identifiable, and therefore droppable:
//
//   bits 63..56  tag   1=oneshot 2=wave 3=chain 0xfe=reclaim(always ignored)
//   bits 55..40  gen   wave generation, bumped per wave -- wave N-1 leakage cannot count in wave N
//   bits 39..16  slot  per-wave item index (addresses 16M items; waves are ring-depth capped)
//   bits 15.. 8  step  0=statx 1=open 2=read 3=write 4=fsync 5=close
//
// every reap site matches on tag (+gen where it has one) and consumes-and-drops anything else, so a
// stray degrades to a wasted 16-byte read instead of a corrupted result.

namespace __ud
{
inline constexpr u8 tag_oneshot = 1;
inline constexpr u8 tag_wave = 2;
inline constexpr u8 tag_chain = 3;
inline constexpr u8 tag_reclaim = 0xfe;

[[gnu::always_inline]] inline constexpr u64
oneshot(u64 seq) noexcept
{
  return (static_cast<u64>(tag_oneshot) << 56) | (seq & 0x00ffffffffffffffull);
}

[[gnu::always_inline]] inline constexpr u64
wave(u16 gen, u32 slot, u8 step) noexcept
{
  return (static_cast<u64>(tag_wave) << 56) | (static_cast<u64>(gen) << 40) | (static_cast<u64>(slot & 0xffffffu) << 16)
         | (static_cast<u64>(step) << 8);
}

[[gnu::always_inline]] inline constexpr u64
chain_at(u16 gen, u32 idx) noexcept
{
  return (static_cast<u64>(tag_chain) << 56) | (static_cast<u64>(gen) << 40) | (static_cast<u64>(idx & 0xffffffu) << 16);
}

inline constexpr u64 reclaim = static_cast<u64>(tag_reclaim) << 56;

[[gnu::always_inline]] inline constexpr u8
tag_of(u64 ud) noexcept
{
  return static_cast<u8>(ud >> 56);
}

[[gnu::always_inline]] inline constexpr u16
gen_of(u64 ud) noexcept
{
  return static_cast<u16>((ud >> 40) & 0xffffu);
}

[[gnu::always_inline]] inline constexpr u32
slot_of(u64 ud) noexcept
{
  return static_cast<u32>((ud >> 16) & 0xffffffu);
}

[[gnu::always_inline]] inline constexpr u8
step_of(u64 ud) noexcept
{
  return static_cast<u8>((ud >> 8) & 0xffu);
}

// step ids
inline constexpr u8 st_statx = 0;
inline constexpr u8 st_open = 1;
inline constexpr u8 st_read = 2;
inline constexpr u8 st_write = 3;
inline constexpr u8 st_fsync = 4;
inline constexpr u8 st_close = 5;
};      // namespace __ud

// tunables; aggregate with defaults (open_opts style)
struct engine_opts {
  u32 entries = 256;        // sq depth (kernel gives cq = 2x)
  bool sqpoll = false;      // OPT-IN kernel poll thread (burns a core); rarely useful for sync-completing flash
  u32 sqpoll_idle_ms = 100;
  bool iopoll = false;                    // OPT-IN: dedicated O_DIRECT engines only
  u32 fixed_bufs = 8;                     // fixed-buffer slab count (0 = no pool; max 64)
  usize fixed_buf_size = 256 * 1024;      // slab size (page-aligned)
  u32 fixed_files = auto_slots;           // sparse fixed-file table size (auto_slots = entries; 0 = disabled; max 256)
  bool register_ring = true;              // register the ring fd when >=5.18
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// engine: owns one uring::ring tuned for file io. NOT thread-safe -- one engine per thread
// (defer_taskrun additionally requires completions be reaped on the submitting thread, which flash
// always does). init failure -> dead engine (tier::none): every op returns the bad_syscall error.

class engine
{
  uring::ring __r;
  byte *__pool = nullptr;      // fixed-buffer slab backing (page-aligned, mmap'd)
  usize __pool_len = 0;
  u64 __buf_free = 0;                    // free-slot bitmap over pool slabs
  u64 __file_free[4]{ 0, 0, 0, 0 };      // free-slot bitmap over the fixed-file table (<=256 slots)
  u32 __nbufs = 0, __nfiles = 0;
  usize __buf_sz = 0;
  u64 __seq = 0;            // internal user_data sequencer
  u16 __gen = 0;            // wave generation (see __ud)
  u32 __pool_want = 0;      // configured pool geometry; the slab is allocated on first use
  usize __pool_want_sz = 0;
  tier __tier = tier::none;

  void
  __free_pool() noexcept
  {
    if ( __pool != nullptr ) micron::munmap(reinterpret_cast<addr_t *>(__pool), __pool_len);
    __pool = nullptr;
    __pool_len = 0;
  }

  static u32
  __setup_flags(const engine_opts &o) noexcept
  {
    u32 f = 0;
    if ( o.sqpoll ) {
      f |= uring::setup_sqpoll;      // sqpoll drives its own reaping; skip the taskrun ladder
    } else {
      if ( kernel::has(kernel::feature::uring_single_issuer) && kernel::has(kernel::feature::uring_defer_taskrun) )
        f |= uring::setup_single_issuer | uring::setup_defer_taskrun;
      else if ( kernel::has(kernel::feature::uring_coop_taskrun) )
        f |= uring::setup_coop_taskrun;
    }
    if ( o.iopoll ) f |= uring::setup_iopoll;
    return f;
  }

public:
  engine(const engine &) = delete;
  engine &operator=(const engine &) = delete;
  engine(engine &&) = delete;
  engine &operator=(engine &&) = delete;

  engine() noexcept = default;

  explicit engine(const engine_opts &o) noexcept { init(o); }

  ~engine() { shutdown(); }

  i32
  init(const engine_opts &o = {}) noexcept
  {
    if ( __r.live() ) return 0;
    uring::params p{};
    p.flags = __setup_flags(o);
    if ( o.sqpoll ) p.sq_thread_idle = o.sqpoll_idle_ms;
    // params-carrying overload: the flags-only form builds a fresh params{} internally, which
    // silently dropped sq_thread_idle -- and sqpoll with idle==0 parks the poll thread immediately,
    // making sqpoll=true strictly worse than sqpoll=false.
    int rc = __r.init_best(o.entries, p);
    if ( rc != 0 ) {
      __tier = tier::none;
      return rc;
    }
    __tier = kernel::has(kernel::feature::uring_op_read) ? tier::basic : tier::plumbing;

    if ( o.register_ring && kernel::has(kernel::feature::uring_reg_ring_fd) ) (void)__r.register_ring_fd();

    // fixed-buffer pool geometry is recorded here but the slab is NOT allocated until the first
    // acquire_buf(). register_buffers pins every page via pin_user_pages, so eagerly building the
    // default 8 x 256 KiB pool cost every thread that ever touched default_engine() 2 MiB of
    // resident pinned RSS plus two syscalls -- for a pool the porcelain and batch layers never use.
    __pool_want = o.fixed_bufs > 64 ? 64 : o.fixed_bufs;
    __pool_want_sz = o.fixed_buf_size;
    if ( __pool_want_sz == 0 ) __pool_want = 0;

    // sparse fixed-file table -> tier::fixed (non-fatal: failure drops to tier::basic)
    if ( __tier == tier::basic && o.fixed_files != 0 && kernel::has(kernel::feature::uring_files_sparse) ) {
      u32 nf = o.fixed_files == auto_slots ? o.entries : o.fixed_files;
      if ( nf > 256 ) nf = 256;
      if ( __r.register_files_sparse(nf) == 0 ) {
        __nfiles = nf;
        for ( u32 i = 0; i < nf; i++ ) __file_free[i >> 6] |= 1ull << (i & 63);
        __tier = tier::fixed;
      }
    }
    return 0;
  }

  void
  shutdown() noexcept
  {
    if ( __r.live() ) {
      if ( __nbufs ) (void)__r.unregister_buffers();
      if ( __nfiles ) (void)__r.unregister_files();
      __r.shutdown();
    }
    __free_pool();
    __nbufs = __nfiles = 0;
    __buf_free = 0;
    __tier = tier::none;
  }

  [[nodiscard]] bool
  live() const noexcept
  {
    return __r.live();
  }

  [[nodiscard]] tier
  level() const noexcept
  {
    return __tier;
  }

  uring::ring &
  ring() noexcept
  {
    return __r;
  }

  // sqe acquisition that flushes the sq (enter(0)) when full. nullptr when the ring is dead, or
  // when the sq is STILL full after the flush. the liveness test is load-bearing: get_sqe()
  // dereferences sq_tail unconditionally, and on a dead ring that pointer is null -- the chain
  // builders reach here without an outer __guard.
  [[nodiscard]] uring::sqe *
  sqe() noexcept
  {
    if ( !__r.live() ) [[unlikely]]
      return nullptr;
    uring::sqe *s = __r.get_sqe();
    if ( s == nullptr ) [[unlikely]] {
      (void)__r.enter(0);
      s = __r.get_sqe();
    }
    return s;
  }

  // %%%% staged submission: fill stage(0..n-1), then publish(n) once. see ring::peek_sqe.
  // NEVER call submit()/submit_wait() between stage() and publish() -- the staged entries are not
  // yet visible to the kernel and an intervening enter() would split the batch.

  [[nodiscard]] uring::sqe *
  stage(u32 k) noexcept
  {
    if ( !__r.live() ) [[unlikely]]
      return nullptr;
    return __r.peek_sqe(k);
  }

  [[gnu::always_inline]] void
  publish(u32 n) noexcept
  {
    if ( n != 0 ) __r.advance_sq(n);
  }

  [[nodiscard]] u32
  sq_room() const noexcept
  {
    return __r.live() ? __r.sq_space_left() : 0;
  }

  // bump and return the wave generation
  [[nodiscard]] u16
  next_gen() noexcept
  {
    return ++__gen;
  }

  [[gnu::always_inline]] void
  advance() noexcept
  {
    __r.advance_sq();
  }

  [[gnu::always_inline]] i32
  submit() noexcept
  {
    return static_cast<i32>(__r.submit());
  }

  [[gnu::always_inline]] i32
  submit_wait(u32 n) noexcept
  {
    return static_cast<i32>(__r.submit_and_wait(n));
  }

  // tagged so a one-shot's completion can never be mistaken for a wave or chain entry
  [[nodiscard]] u64
  next_seq() noexcept
  {
    return __ud::oneshot(++__seq);
  }

  // reap cqes until the one tagged `ud` lands; returns its res (or a negative enter error). stray
  // cqes (from other in-flight work) are discarded -- safe for the single-op plumbing path.
  i32
  await(u64 ud) noexcept
  {
    for ( ;; ) {
      uring::cqe c{};
      int rc = __r.wait_cqe(&c);
      if ( rc < 0 ) return rc;
      if ( c.user_data == ud ) return c.res;
    }
  }

  // %%%% fixed-buffer pool

  // reports the CONFIGURED pool, whether or not the slab has been materialised yet
  [[nodiscard]] bool
  has_pool() const noexcept
  {
    return __nbufs != 0 || __pool_want != 0;
  }

  [[nodiscard]] usize
  pool_slab_size() const noexcept
  {
    return __nbufs != 0 ? __buf_sz : __pool_want_sz;
  }

  [[nodiscard]] u32
  nbufs() const noexcept
  {
    return __nbufs != 0 ? __nbufs : __pool_want;
  }

  // materialise the slab + register it. called on first acquire_buf; non-fatal on failure (the
  // pool simply stays disabled and acquire_buf reports -EAGAIN).
  [[gnu::noinline]] bool
  __realise_pool() noexcept
  {
    if ( __nbufs != 0 ) return true;
    if ( __pool_want == 0 || __pool_want_sz == 0 || !__r.live() ) return false;
    const u32 nb = __pool_want;
    const usize total = static_cast<usize>(nb) * __pool_want_sz;
    void *p = micron::mmap(nullptr, total, prot_read | prot_write, map_private | map_anonymous, -1, 0);
    if ( uring::ring::__map_failed(p) ) {
      __pool_want = 0;      // do not retry on every acquire
      return false;
    }
    __pool = reinterpret_cast<byte *>(p);
    __pool_len = total;
    micron::vector<uring::iovec> iov;
    iov.reserve(nb);
    for ( u32 i = 0; i < nb; i++ ) iov.push_back(uring::iovec{ __pool + static_cast<usize>(i) * __pool_want_sz, __pool_want_sz });
    if ( __r.register_buffers(iov.data(), nb) != 0 ) {
      __free_pool();
      __pool_want = 0;
      return false;
    }
    __nbufs = nb;
    __buf_sz = __pool_want_sz;
    __buf_free = nb >= 64 ? ~0ull : ((1ull << nb) - 1);
    return true;
  }

  // borrow a slab slot; -EAGAIN when exhausted (or when the pool cannot be materialised)
  i32
  acquire_buf(byte *&out, usize &cap, u16 &index) noexcept
  {
    if ( __nbufs == 0 ) [[unlikely]] {
      if ( !__realise_pool() ) return -error::try_again;
    }
    if ( __buf_free == 0 ) return -error::try_again;
    u32 i = static_cast<u32>(__builtin_ctzll(__buf_free));
    __buf_free &= ~(1ull << i);
    out = __pool + static_cast<usize>(i) * __buf_sz;
    cap = __buf_sz;
    index = static_cast<u16>(i);
    return 0;
  }

  void
  release_buf(u16 index) noexcept
  {
    if ( index < __nbufs ) __buf_free |= 1ull << index;
  }

  // %%%% fixed-file table

  [[nodiscard]] bool
  has_file_slots() const noexcept
  {
    return __nfiles != 0;
  }

  i32
  acquire_slot() noexcept
  {
    for ( u32 w = 0; w < 4; w++ ) {
      if ( __file_free[w] == 0 ) continue;
      u32 b = static_cast<u32>(__builtin_ctzll(__file_free[w]));
      __file_free[w] &= ~(1ull << b);
      return static_cast<i32>(w * 64 + b);
    }
    return -error::try_again;
  }

  void
  release_slot(u32 slot) noexcept
  {
    if ( slot < __nfiles ) __file_free[slot >> 6] |= 1ull << (slot & 63);
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// per-thread default engine. noinline: micthread fibers that migrate across threads must re-resolve
// the TLS address on every call (see micthread/tls.hpp) -- never cache the reference across a
// suspension point. flash calls do not suspend, so only preemptive migration could bite.

[[gnu::noinline]] inline engine &
default_engine() noexcept
{
  static thread_local engine __e{ engine_opts{} };
  return __e;
}

// borrowed fixed-buffer slab; returns its slot on destruction (move-only)
struct pool_buf {
  byte *data = nullptr;
  usize cap = 0;
  u16 index = 0;
  engine *__e = nullptr;

  pool_buf() = default;
  pool_buf(const pool_buf &) = delete;
  pool_buf &operator=(const pool_buf &) = delete;

  pool_buf(pool_buf &&o) noexcept : data(o.data), cap(o.cap), index(o.index), __e(o.__e)
  {
    o.__e = nullptr;
    o.data = nullptr;
  }

  pool_buf &
  operator=(pool_buf &&o) noexcept
  {
    if ( this != &o ) {
      if ( __e ) __e->release_buf(index);
      data = o.data;
      cap = o.cap;
      index = o.index;
      __e = o.__e;
      o.__e = nullptr;
      o.data = nullptr;
    }
    return *this;
  }

  ~pool_buf()
  {
    if ( __e ) __e->release_buf(index);
  }

  [[nodiscard]] bool
  valid() const noexcept
  {
    return __e != nullptr;
  }
};

inline micron::option<pool_buf, io::error_t>
acquire_buf(engine &eng = default_engine())
{
  pool_buf b;
  byte *d = nullptr;
  usize cap = 0;
  u16 idx = 0;
  i32 e = eng.acquire_buf(d, cap, idx);
  if ( e < 0 ) return micron::option<pool_buf, io::error_t>{ io::error_t(e) };
  b.data = d;
  b.cap = cap;
  b.index = idx;
  b.__e = &eng;
  return micron::option<pool_buf, io::error_t>{ micron::move(b) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// internal helpers

namespace __impl
{
inline constexpr u32 __max_chunk = 1u << 30;      // per-sqe transfer cap (sqe.len is u32)

// dead-engine guard
[[gnu::always_inline]] inline i32
__guard(engine &eng) noexcept
{
  return eng.live() ? 0 : -error::bad_syscall;
}

// one positional read/write op with short-io + EINTR continuation. op is op_read/op_write.
// off == -1 uses the file position (advances); otherwise absolute, resumed by += done.
template<bool Write>
inline max_t
__rw(engine &eng, i32 fd, void *buf, usize cnt, u64 off) noexcept
{
  if ( i32 e = __guard(eng) ) [[unlikely]]
    return e;
  byte *p = static_cast<byte *>(buf);
  usize done = 0;
  const bool positional = off != static_cast<u64>(-1);
  while ( done < cnt ) {
    u32 chunk = static_cast<u32>((cnt - done) < __max_chunk ? (cnt - done) : __max_chunk);
    uring::sqe *s = eng.sqe();
    if ( s == nullptr ) [[unlikely]]
      return done ? static_cast<max_t>(done) : -error::bad_syscall;
    u64 at = positional ? off + done : static_cast<u64>(-1);
    if constexpr ( Write )
      uring::prep_write(s, fd, p + done, chunk, at);
    else
      uring::prep_read(s, fd, p + done, chunk, at);
    u64 ud = eng.next_seq();
    s->user_data = ud;
    eng.advance();
    (void)eng.submit_wait(1);
    i32 r = eng.await(ud);
    if ( r < 0 ) [[unlikely]] {
      if ( r == -error::interrupted ) continue;
      if ( r == -error::try_again && done ) break;
      return done ? static_cast<max_t>(done) : r;
    }
    if ( r == 0 ) break;
    done += static_cast<usize>(r);
  }
  return static_cast<max_t>(done);
}

// initial whole-file read reservation. big enough that the overwhelming majority of files finish
// in one read, small enough not to waste a page on tiny ones.
inline constexpr usize __probe_size = 64 * 1024;

// __fill / __finish_large live in a reopened __impl below, after flash::statx is declared

// per-file state threaded through the batch waves
struct __bslot {
  i32 fd = -1;
  i32 err = 0;
  u64 want = 0;
  u64 done = 0;
};

// how many files to keep in flight. every in-flight slot pins a destination buffer, and
// parallelism saturates well before the sq limit.
[[gnu::always_inline]] inline u32
__batch_window(engine &eng, usize remaining) noexcept
{
  u32 w = eng.ring().__sq_entries;
  w = w > 8 ? w - 8 : 1;
  if ( w > 64 ) w = 64;
  if ( static_cast<usize>(w) < remaining ) return w ? w : 1;
  return remaining ? static_cast<u32>(remaining) : 1;
}

// %%%% wave runner
//
// the caller stages N sqes (eng.stage(0..N-1)), tags every one ud_wave(gen, slot, step), and calls
// eng.publish(N). this then submits and reaps them. three invariants, each fixing a real defect:
//
//   1. the wait count is THIS wave's published count minus whatever the kernel would not take,
//      never a raw loop counter and never submit()'s return value. enter() can short-submit and
//      re-credit to_submit, so `submit_wait(batch); while (got < batch) wait_cqe()` blocks forever
//      on completions that were never submitted. summing submit() instead is wrong in the other
//      direction: an sqpoll ring always reports 0 (the poll thread drains the sq itself), which
//      abandons the entire wave unreaped, and any sqes a queue_* left pending get counted as ours,
//      so the loop waits for completions that can never match and hangs.
//   2. completions are matched on tag+gen. anything else -- a stray from an abandoned op, a
//      reclaim close, a previous wave's leakage -- is consumed and dropped instead of being
//      written into this wave's result array.
//   3. the cq is drained in passes, so cq_head takes one release-store per pass rather than one
//      per completion.
//
// dispatch(slot, step, res) is invoked per matching completion, in arrival order.
// `expect` is the number of sqes THIS wave published (eng.publish(expect)).
template<typename Dispatch>
inline i32
__run_wave(engine &eng, u8 tag, u16 gen, u32 expect, Dispatch &&d) noexcept
{
  uring::ring &r = eng.ring();
  if ( expect == 0 ) return 0;

  // hand everything pending to the kernel. enter() re-credits a short submit, so keep going while
  // the queue is still draining and stop the moment it stalls rather than spinning.
  for ( ;; ) {
    const u32 before = r.to_submit;
    if ( before == 0 ) break;
    if ( r.submit() < 0 ) break;             // fatal
    if ( r.to_submit >= before ) break;      // the kernel took nothing this pass
  }

  // how many of OUR entries reached the kernel. this wave was published last, so anything still
  // pending is the tail of it; everything ahead belonged to whoever queued before us. on an sqpoll
  // ring enter() has already zeroed to_submit (the poll thread owns the sq), so left == 0 and the
  // whole wave is correctly treated as in flight.
  const u32 left = r.to_submit;
  const u32 in_flight = left >= expect ? 0u : expect - left;
  if ( in_flight == 0 ) return 0;

  u32 got = 0;
  u32 idle = 0;
  while ( got < in_flight ) {
    if ( r.cq_ready() == 0 ) {
      if ( r.submit_and_wait(1) < 0 ) return -error::bad_syscall;
    }
    u32 matched = 0;
    u32 seen = 0;
    if ( r.__cqe_mixed ) [[unlikely]] {
      // mixed rings have a variable cqe stride; for_each_cqe cannot walk them
      uring::cqe c{};
      while ( r.peek_cqe(&c) ) {
        seen++;
        if ( __ud::tag_of(c.user_data) == tag && __ud::gen_of(c.user_data) == gen ) {
          d(__ud::slot_of(c.user_data), __ud::step_of(c.user_data), c.res);
          matched++;
        }
      }
    } else {
      seen = r.for_each_cqe([&](const uring::cqe &c) {
        if ( __ud::tag_of(c.user_data) == tag && __ud::gen_of(c.user_data) == gen ) {
          d(__ud::slot_of(c.user_data), __ud::step_of(c.user_data), c.res);
          matched++;
        }
      });
    }
    got += matched;
    if ( seen == 0 ) {
      if ( ++idle > 3 ) break;      // the kernel says ready but yields nothing: bail rather than spin
    } else {
      idle = 0;
    }
  }
  return 0;
}

// stage up to `count` entries -- fill(sqe*, i) returns false to skip index i -- publish once, then
// submit and reap. staging never calls enter(), so the whole wave reaches the kernel in one
// advance_sq + one io_uring_enter regardless of count. returns the number staged.
//
// if the sq cannot hold the whole wave, staging simply stops early; the wave completes with what
// fits and the caller's loop picks up the remainder next pass. that is safe precisely because
// __run_wave derives its wait count from what enter() accepted rather than from `count`.
template<typename Fill, typename Dispatch>
inline u32
__stage_and_run(engine &eng, u32 count, u8 step, Fill &&fill, Dispatch &&d) noexcept
{
  const u16 gen = eng.next_gen();
  const u32 room = eng.sq_room();
  u32 staged = 0;
  for ( u32 i = 0; i < count && staged < room; i++ ) {
    uring::sqe *s = eng.stage(staged);
    if ( s == nullptr ) break;
    if ( !fill(s, i) ) continue;      // slot skipped: the staged slot is simply reused
    s->user_data = __ud::wave(gen, i, step);
    staged++;
  }
  if ( staged == 0 ) return 0;
  eng.publish(staged);
  (void)__run_wave(eng, __ud::tag_wave, gen, staged, d);
  return staged;
}
};      // namespace __impl

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// plumbing: single-op free fns (i32 + fd_t overload pairs, mirroring io.hpp)

template<typename T>
max_t
read(i32 fd, T *buf, usize cnt, engine &eng = default_engine())
{
  return __impl::__rw<false>(eng, fd, buf, cnt * sizeof(T), static_cast<u64>(-1));
}

template<typename T>
max_t
read(fd_t fd, T *buf, usize cnt, engine &eng = default_engine())
{
  return __impl::__rw<false>(eng, fd.fd, buf, cnt * sizeof(T), static_cast<u64>(-1));
}

template<typename T>
max_t
pread(i32 fd, T *buf, usize cnt, u64 off, engine &eng = default_engine())
{
  return __impl::__rw<false>(eng, fd, buf, cnt * sizeof(T), off);
}

template<typename T>
max_t
pread(fd_t fd, T *buf, usize cnt, u64 off, engine &eng = default_engine())
{
  return __impl::__rw<false>(eng, fd.fd, buf, cnt * sizeof(T), off);
}

template<typename T = byte>
max_t
write(i32 fd, const T *buf, usize cnt, engine &eng = default_engine())
{
  return __impl::__rw<true>(eng, fd, const_cast<T *>(buf), cnt * sizeof(T), static_cast<u64>(-1));
}

template<typename T = byte>
max_t
write(fd_t fd, const T *buf, usize cnt, engine &eng = default_engine())
{
  return __impl::__rw<true>(eng, fd.fd, const_cast<T *>(buf), cnt * sizeof(T), static_cast<u64>(-1));
}

template<typename T = byte>
max_t
pwrite(i32 fd, const T *buf, usize cnt, u64 off, engine &eng = default_engine())
{
  return __impl::__rw<true>(eng, fd, const_cast<T *>(buf), cnt * sizeof(T), off);
}

template<typename T = byte>
max_t
pwrite(fd_t fd, const T *buf, usize cnt, u64 off, engine &eng = default_engine())
{
  return __impl::__rw<true>(eng, fd.fd, const_cast<T *>(buf), cnt * sizeof(T), off);
}

// single-op that preps via a caller lambda and waits for the one completion
template<typename Fill>
inline max_t
__oneshot(engine &eng, Fill &&fill) noexcept
{
  if ( i32 e = __impl::__guard(eng) ) [[unlikely]]
    return e;
  uring::sqe *s = eng.sqe();
  if ( s == nullptr ) [[unlikely]]
    return -error::bad_syscall;
  fill(s);
  u64 ud = eng.next_seq();
  s->user_data = ud;
  eng.advance();
  (void)eng.submit_wait(1);
  return eng.await(ud);
}

inline max_t
fsync(i32 fd, engine &eng = default_engine())
{
  return __oneshot(eng, [fd](uring::sqe *s) { uring::prep_fsync(s, fd); });
}

inline max_t
fsync(fd_t fd, engine &eng = default_engine())
{
  return fsync(fd.fd, eng);
}

inline max_t
fdatasync(i32 fd, engine &eng = default_engine())
{
  return __oneshot(eng, [fd](uring::sqe *s) { uring::prep_fsync(s, fd, uring::fsync_datasync); });
}

inline max_t
fdatasync(fd_t fd, engine &eng = default_engine())
{
  return fdatasync(fd.fd, eng);
}

inline max_t
fallocate(i32 fd, i32 mode, u64 off, u64 len, engine &eng = default_engine())
{
  return __oneshot(eng, [=](uring::sqe *s) { uring::prep_fallocate(s, fd, mode, off, len); });
}

inline max_t
ftruncate(i32 fd, u64 len, engine &eng = default_engine())
{
  if ( !kernel::has(kernel::feature::uring_ftruncate) ) return -error::bad_syscall;
  return __oneshot(eng, [=](uring::sqe *s) { uring::prep_ftruncate(s, fd, len); });
}

inline max_t
fadvise(i32 fd, u64 off, u64 len, i32 advice, engine &eng = default_engine())
{
  return __oneshot(eng, [=](uring::sqe *s) { uring::prep_fadvise(s, fd, off, static_cast<u32>(len), advice); });
}

// statx into a caller struct (AT_EMPTY_PATH on the fd)
inline max_t
statx(i32 fd, posix::statx_t &out, engine &eng = default_engine())
{
  if ( !kernel::has(kernel::feature::uring_op_read) ) return -error::bad_syscall;
  return __oneshot(eng, [fd, &out](uring::sqe *s) { uring::prep_statx(s, fd, "", posix::at_empty_path, posix::statx_basic_stats, &out); });
}

// fixed-buffer io through the engine pool
inline max_t
read_fixed(i32 fd, pool_buf &b, usize cnt, u64 off, engine &eng = default_engine())
{
  if ( !b.valid() ) return -error::invalid_arg;
  u16 idx = b.index;
  return __oneshot(eng, [=, &b](uring::sqe *s) { uring::prep_read_fixed(s, fd, b.data, static_cast<u32>(cnt), off, idx); });
}

inline max_t
write_fixed(i32 fd, const pool_buf &b, usize cnt, u64 off, engine &eng = default_engine())
{
  if ( !b.valid() ) return -error::invalid_arg;
  u16 idx = b.index;
  return __oneshot(eng, [=, &b](uring::sqe *s) { uring::prep_write_fixed(s, fd, b.data, static_cast<u32>(cnt), off, idx); });
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// plumbing: queue layer (prep only, no syscall; user_data comes back in the cqe). buffers/paths
// must stay alive until the matching cqe is reaped.

inline i32
queue_read(engine &eng, i32 fd, void *buf, u32 n, u64 off, u64 user_data, u8 sqe_flags = 0)
{
  if ( i32 e = __impl::__guard(eng) ) [[unlikely]]
    return e;
  uring::sqe *s = eng.sqe();
  if ( s == nullptr ) [[unlikely]]
    return -error::bad_syscall;
  uring::prep_read(s, fd, buf, n, off);
  s->flags |= sqe_flags;
  s->user_data = user_data;
  eng.advance();
  return 0;
}

inline i32
queue_write(engine &eng, i32 fd, const void *buf, u32 n, u64 off, u64 user_data, u8 sqe_flags = 0)
{
  if ( i32 e = __impl::__guard(eng) ) [[unlikely]]
    return e;
  uring::sqe *s = eng.sqe();
  if ( s == nullptr ) [[unlikely]]
    return -error::bad_syscall;
  uring::prep_write(s, fd, buf, n, off);
  s->flags |= sqe_flags;
  s->user_data = user_data;
  eng.advance();
  return 0;
}

inline i32
queue_fsync(engine &eng, i32 fd, u64 user_data, u8 sqe_flags = 0)
{
  if ( i32 e = __impl::__guard(eng) ) [[unlikely]]
    return e;
  uring::sqe *s = eng.sqe();
  if ( s == nullptr ) [[unlikely]]
    return -error::bad_syscall;
  uring::prep_fsync(s, fd);
  s->flags |= sqe_flags;
  s->user_data = user_data;
  eng.advance();
  return 0;
}

inline i32
queue_statx(engine &eng, const char *path, posix::statx_t *out, u64 user_data, u8 sqe_flags = 0)
{
  if ( i32 e = __impl::__guard(eng) ) [[unlikely]]
    return e;
  uring::sqe *s = eng.sqe();
  if ( s == nullptr ) [[unlikely]]
    return -error::bad_syscall;
  uring::prep_statx(s, posix::at_fdcwd, path, 0, posix::statx_basic_stats, out);
  s->flags |= sqe_flags;
  s->user_data = user_data;
  eng.advance();
  return 0;
}

inline i32
submit(engine &eng)
{
  if ( i32 e = __impl::__guard(eng) ) [[unlikely]]
    return e;
  return eng.submit();
}

inline i32
submit_wait(engine &eng, u32 n)
{
  if ( i32 e = __impl::__guard(eng) ) [[unlikely]]
    return e;
  return eng.submit_wait(n);
}

[[nodiscard]] inline bool
reap(engine &eng, uring::cqe &out) noexcept
{
  return eng.live() && eng.ring().peek_cqe(&out);
}

inline i32
wait(engine &eng, uring::cqe &out) noexcept
{
  if ( i32 e = __impl::__guard(eng) ) [[unlikely]]
    return e;
  return eng.ring().wait_cqe(&out);
}

// wait for n cqes, invoke fn(user_data, res) per completion; returns 0 or the first enter error
template<typename Fn>
  requires micron::invocable<Fn, u64, i32>
i32
drain(engine &eng, u32 n, Fn &&fn)
{
  if ( i32 e = __impl::__guard(eng) ) [[unlikely]]
    return e;
  u32 got = 0;
  while ( got < n ) {
    uring::cqe c{};
    int rc = eng.ring().wait_cqe(&c);
    if ( rc < 0 ) return rc;
    fn(c.user_data, c.res);
    got++;
  }
  return 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// linked-sqe chain: ops run strictly in order in ONE submission. a failure severs the rest
// (-ECANCELED). open() targets a fixed-file slot on tier::fixed so later ops can name the fd before
// the open completes.

inline constexpr usize chain_max = 8;

class chain
{
  engine &__e;
  i32 __res[chain_max];
  u8 __tag[chain_max];      // 0=other, 1=open(slot), 2=close(slot)
  u32 __n = 0;
  i32 __slot = -1;
  i32 __build_err = 0;
  u16 __gen = 0;
  uring::sqe *__prev = nullptr;

  // stage (not publish) the next entry. staging is what makes the link flag safe: __prev is still
  // owned by userspace, so OR-ing sqe_io_link into it cannot race a kernel that has already been
  // handed the entry. the old code published each sqe as it was built, so a nested enter(0) inside
  // eng.sqe() could hand the kernel a half-linked chain.
  uring::sqe *
  __next(u8 tag) noexcept
  {
    if ( __build_err != 0 ) return nullptr;
    if ( __n >= chain_max ) {
      __build_err = -error::invalid_arg;
      return nullptr;
    }
    uring::sqe *s = __e.stage(__n);
    if ( s == nullptr ) {
      // the sq cannot hold the rest of this chain. a PARTIAL chain must never be submitted, so
      // record the failure and let run() report it without publishing anything.
      __build_err = -error::try_again;
      return nullptr;
    }
    if ( __prev != nullptr ) __prev->flags |= uring::sqe_io_link;      // link the PREVIOUS op to this one
    __tag[__n] = tag;
    __prev = s;
    return s;
  }

public:
  explicit chain(engine &eng = default_engine()) noexcept : __e(eng)
  {
    for ( usize i = 0; i < chain_max; i++ ) {
      __res[i] = -1;
      __tag[i] = 0;
    }
    if ( !eng.live() ) __build_err = -error::bad_syscall;
    __gen = eng.next_gen();
  }

  chain(const chain &) = delete;
  chain &operator=(const chain &) = delete;

  ~chain()
  {
    if ( __slot >= 0 ) __e.release_slot(static_cast<u32>(__slot));
  }

  // tier::fixed only: open into a fixed-file slot the chain owns
  chain &
  open(const char *path, i32 flags, u32 mode = posix::mode_file) noexcept
  {
    if ( __build_err != 0 ) return *this;
    // below tier::fixed there are no direct descriptors, so a later op cannot name the fd this
    // open will produce -- the chain is unbuildable, not merely degraded. previously this returned
    // silently and every following op fell back to fd -1, so the whole chain ran against -1 and
    // reported EBADF with no hint as to why.
    if ( __e.level() != tier::fixed ) {
      __build_err = -error::bad_syscall;
      return *this;
    }
    i32 slot = __e.acquire_slot();
    if ( slot < 0 ) {
      __build_err = -error::try_again;
      return *this;
    }
    __slot = slot;
    uring::sqe *s = __next(1);
    if ( s == nullptr ) return *this;
    uring::prep_openat_direct(s, posix::at_fdcwd, path, static_cast<u32>(flags), mode, static_cast<u32>(slot));
    s->user_data = __ud::chain_at(__gen, __n);
    __n++;      // staged only; run() publishes the whole chain with one advance_sq
    return *this;
  }

  chain &
  read(void *buf, u32 n, u64 off) noexcept
  {
    if ( __slot < 0 ) {      // no owning open() -- would have targeted fd -1
      if ( __build_err == 0 ) __build_err = -error::bad_syscall;
      return *this;
    }
    uring::sqe *s = __next(0);
    if ( s == nullptr ) return *this;
    uring::prep_read(s, __slot, buf, n, off);
    uring::sqe_set_fixed_file(s);
    s->user_data = __ud::chain_at(__gen, __n);
    __n++;      // staged only; run() publishes the whole chain with one advance_sq
    return *this;
  }

  chain &
  read(i32 fd, void *buf, u32 n, u64 off) noexcept
  {
    uring::sqe *s = __next(0);
    if ( s == nullptr ) return *this;
    uring::prep_read(s, fd, buf, n, off);
    s->user_data = __ud::chain_at(__gen, __n);
    __n++;      // staged only; run() publishes the whole chain with one advance_sq
    return *this;
  }

  chain &
  write(const void *buf, u32 n, u64 off) noexcept
  {
    if ( __slot < 0 ) {      // no owning open() -- would have targeted fd -1
      if ( __build_err == 0 ) __build_err = -error::bad_syscall;
      return *this;
    }
    uring::sqe *s = __next(0);
    if ( s == nullptr ) return *this;
    uring::prep_write(s, __slot, buf, n, off);
    uring::sqe_set_fixed_file(s);
    s->user_data = __ud::chain_at(__gen, __n);
    __n++;      // staged only; run() publishes the whole chain with one advance_sq
    return *this;
  }

  chain &
  write(i32 fd, const void *buf, u32 n, u64 off) noexcept
  {
    uring::sqe *s = __next(0);
    if ( s == nullptr ) return *this;
    uring::prep_write(s, fd, buf, n, off);
    s->user_data = __ud::chain_at(__gen, __n);
    __n++;      // staged only; run() publishes the whole chain with one advance_sq
    return *this;
  }

  chain &
  fsync() noexcept
  {
    if ( __slot < 0 ) {      // no owning open() -- would have targeted fd -1
      if ( __build_err == 0 ) __build_err = -error::bad_syscall;
      return *this;
    }
    uring::sqe *s = __next(0);
    if ( s == nullptr ) return *this;
    uring::prep_fsync(s, __slot);
    uring::sqe_set_fixed_file(s);
    s->user_data = __ud::chain_at(__gen, __n);
    __n++;      // staged only; run() publishes the whole chain with one advance_sq
    return *this;
  }

  chain &
  fsync(i32 fd) noexcept
  {
    uring::sqe *s = __next(0);
    if ( s == nullptr ) return *this;
    uring::prep_fsync(s, fd);
    s->user_data = __ud::chain_at(__gen, __n);
    __n++;      // staged only; run() publishes the whole chain with one advance_sq
    return *this;
  }

  chain &
  close() noexcept
  {
    if ( __slot < 0 ) {      // no owning open() -- would have targeted fd -1
      if ( __build_err == 0 ) __build_err = -error::bad_syscall;
      return *this;
    }
    uring::sqe *s = __next(2);
    if ( s == nullptr ) return *this;
    uring::prep_close_direct(s, static_cast<u32>(__slot));
    s->user_data = __ud::chain_at(__gen, __n);
    __n++;      // staged only; run() publishes the whole chain with one advance_sq
    return *this;
  }

  chain &
  close(i32 fd) noexcept
  {
    uring::sqe *s = __next(2);
    if ( s == nullptr ) return *this;
    uring::prep_close(s, fd);
    s->user_data = __ud::chain_at(__gen, __n);
    __n++;      // staged only; run() publishes the whole chain with one advance_sq
    return *this;
  }

  // submit the whole chain and reap all cqes. maps -ECANCELED (severed link) to the first real
  // -errno; on a mid-chain failure after the open succeeded, issues a compensating close of the
  // slot so nothing leaks. returns 0 or the first real error.
  i32
  run() noexcept
  {
    // a build failure means nothing was published; report it without submitting a partial chain
    if ( __build_err != 0 ) return __build_err;
    if ( i32 e = __impl::__guard(__e) ) [[unlikely]]
      return e;
    if ( __n == 0 ) return 0;

    __prev = nullptr;      // last op carries no link flag (we only set link on append)
    __e.publish(__n);      // ONE advance_sq for the whole chain

    u32 done = 0;      // bitmask of indices that actually completed
    bool open_ok = false;
    (void)__impl::__run_wave(__e, __ud::tag_chain, __gen, __n, [&](u32 idx, u8, i32 res) {
      if ( idx >= __n ) return;
      __res[idx] = res;
      done |= 1u << idx;
      if ( __tag[idx] == 1 && res >= 0 ) open_ok = true;
    });

    // first REAL error in chain order (-ECANCELED is just link severance, not the cause). scan by
    // index rather than arrival so the answer does not depend on completion ordering, and only
    // consider slots that actually landed -- __res defaults to -1, which would otherwise read as
    // EPERM for an op whose cqe never arrived.
    i32 first_err = 0;
    for ( u32 i = 0; i < __n; i++ )
      if ( ((done >> i) & 1u) != 0 && __res[i] < 0 && __res[i] != -error::operation_canceled ) {
        first_err = __res[i];
        break;
      }

    // the open slot leaks if the linked close was severed -- compensate. tagged reclaim so its
    // completion is dropped by every reap loop rather than mistaken for someone's result.
    if ( open_ok && __slot >= 0 ) {
      bool closed = false;
      for ( u32 i = 0; i < __n; i++ )
        if ( __tag[i] == 2 && __res[i] == 0 ) closed = true;
      if ( !closed ) {
        uring::sqe *s = __e.sqe();
        if ( s != nullptr ) {
          uring::prep_close_direct(s, static_cast<u32>(__slot));
          s->user_data = __ud::reclaim;
          __e.advance();
          (void)__e.submit_wait(1);
          uring::cqe cc{};
          (void)__e.ring().wait_cqe(&cc);
        }
      }
    }
    return first_err;
  }

  [[nodiscard]] i32
  res(usize i) const noexcept
  {
    return i < __n ? __res[i] : -1;
  }

  [[nodiscard]] usize
  size() const noexcept
  {
    return __n;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// availability

[[nodiscard]] inline bool
available(engine &eng = default_engine()) noexcept
{
  return eng.live() && eng.level() >= tier::basic;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// porcelain: class flash::file. standalone (not os_file-derived: needs a noexcept ctor and the fd_t
// inline-errno convention). move-only; explicit cursor (uring is positional; *_at forms leave it).

// the recorded name is a diagnostic label and nothing else -- file::name() has no caller in tree,
// and every operation goes through the fd. it used to be sstr<path_max>, which made sizeof(file)
// ~4128 bytes and cost a 4 KiB memset on construct plus another on destroy (micron::sstring zeroes
// under Sf=true, and its MOVE ctor memcpys then zeroes the source, so moving is no cheaper than
// copying). measured: 356 ns to build an sstr<4096> vs 49 ns for an sstr<128>, on every open.
inline constexpr usize name_cap = 128;

// copy the TRAILING name_cap-1 bytes -- for a long path the basename and its parent carry the
// diagnostic value, the leading directories do not.
inline micron::sstr<name_cap>
__short_name(const char *p) noexcept
{
  if ( p == nullptr ) return micron::sstr<name_cap>{};
  usize n = 0;
  while ( p[n] != '\0' ) n++;
  return micron::sstr<name_cap>(n < name_cap ? p : p + (n - (name_cap - 1)));
}

class file
{
  fd_t __handle{ posix::invalid_fd };
  engine *__eng = nullptr;
  u64 __cursor = 0;
  micron::sstr<name_cap> fname;

  i32
  __check() const noexcept
  {
    if ( __eng == nullptr || !__eng->live() ) return -error::bad_syscall;
    if ( __handle.fd < 0 ) return __handle.fd;
    return 0;
  }

public:
  file() = default;

  file(const io::path_t &p, const modes m = modes::read, const open_opts &opts = {}, engine &eng = default_engine())
      : __eng(&eng), fname(__short_name(p.c_str()))
  {
    if ( !eng.live() ) {
      __handle = fd_t{ -error::bad_syscall };
      return;
    }
    max_t r = __oneshot(eng, [&](uring::sqe *s) {
      uring::prep_openat(s, posix::at_fdcwd, p.c_str(), static_cast<u32>(compose_open_flags(m, opts)), opts.perms);
    });
    __handle = fd_t{ static_cast<i32>(r) };
  }

  file(fd_t adopt, const char *recorded_name, engine &eng = default_engine())
      : __handle(adopt), __eng(&eng), fname(__short_name(recorded_name))
  {
  }

  file(const file &) = delete;
  file &operator=(const file &) = delete;

  file(file &&o) noexcept : __handle(o.__handle), __eng(o.__eng), __cursor(o.__cursor), fname(o.fname)
  {
    o.__handle = fd_t{ posix::invalid_fd };
    o.__eng = nullptr;
  }

  file &
  operator=(file &&o) noexcept
  {
    if ( this != &o ) {
      close();
      __handle = o.__handle;
      __eng = o.__eng;
      __cursor = o.__cursor;
      fname = o.fname;
      o.__handle = fd_t{ posix::invalid_fd };
      o.__eng = nullptr;
    }
    return *this;
  }

  ~file() { close(); }

  [[nodiscard]] bool
  valid() const noexcept
  {
    return __handle.fd >= 0 && __eng != nullptr;
  }

  explicit
  operator bool() const noexcept
  {
    return valid();
  }

  [[nodiscard]] fd_t
  fd() const noexcept
  {
    return __handle;
  }

  [[nodiscard]] i32
  raw_fd() const noexcept
  {
    return __handle.fd;
  }

  // NOTE: truncated to the TRAILING name_cap-1 bytes for paths longer than that (see name_cap).
  // this is a diagnostic label, not a handle -- reopen from the path you opened with.
  [[nodiscard]] const micron::sstr<name_cap> &
  name() const noexcept
  {
    return fname;
  }

  [[nodiscard]] u64
  tell() const noexcept
  {
    return __cursor;
  }

  u64
  seek(u64 off) noexcept
  {
    __cursor = off;
    return __cursor;
  }

  void
  rewind() noexcept
  {
    __cursor = 0;
  }

  // raw byte io at the cursor / at an explicit offset
  max_t
  read(void *p, usize n)
  {
    if ( i32 e = __check() ) [[unlikely]]
      return e;
    max_t r = flash::pread(__handle.fd, static_cast<byte *>(p), n, __cursor, *__eng);
    if ( r > 0 ) __cursor += static_cast<u64>(r);
    return r;
  }

  max_t
  read_at(u64 off, void *p, usize n)
  {
    if ( i32 e = __check() ) [[unlikely]]
      return e;
    return flash::pread(__handle.fd, static_cast<byte *>(p), n, off, *__eng);
  }

  max_t
  write(const void *p, usize n)
  {
    if ( i32 e = __check() ) [[unlikely]]
      return e;
    max_t r = flash::pwrite(__handle.fd, static_cast<const byte *>(p), n, __cursor, *__eng);
    if ( r > 0 ) __cursor += static_cast<u64>(r);
    return r;
  }

  max_t
  write_at(u64 off, const void *p, usize n)
  {
    if ( i32 e = __check() ) [[unlikely]]
      return e;
    return flash::pwrite(__handle.fd, static_cast<const byte *>(p), n, off, *__eng);
  }

  // universal write tiers (bulk contiguous / literal / cstr / trivially-copyable object)
  template<typename T>
    requires(is_string<T> || (is_contiguous_container<T> && micron::is_trivially_copyable_v<typename T::value_type>))
  max_t
  write(const T &c)
  {
    return write(static_cast<const void *>(c.data()), c.size() * sizeof(typename T::value_type));
  }

  template<usize N>
  max_t
  write(const char (&lit)[N])
  {
    return write(static_cast<const void *>(lit), N - 1);
  }

  max_t
  write(const char *cstr)
  {
    usize n = 0;
    while ( cstr[n] ) n++;
    return write(static_cast<const void *>(cstr), n);
  }

  template<typename T>
    requires(micron::is_trivially_copyable_v<T> && !is_string<T> && !is_iterable_container<T> && !micron::is_pointer_v<T>
             && !micron::is_array_v<T> && !fn_like<T>)
  max_t
  write(const T &obj)
  {
    return write(static_cast<const void *>(&obj), sizeof(T));
  }

  // whole-remainder value read
  template<typename T>
    requires((is_string<T> || is_contiguous_container<T>) && micron::is_trivially_copyable_v<typename T::value_type>)
  T
  read()
  {
    T out{};
    if ( __check() != 0 ) [[unlikely]]
      return out;
    max_t sz = size();
    if ( sz > static_cast<max_t>(__cursor) ) {
      usize want = static_cast<usize>(sz - static_cast<max_t>(__cursor));
      out.reserve(want / sizeof(typename T::value_type) + 1);
      out.set_size(want / sizeof(typename T::value_type));
      max_t r = read(static_cast<void *>(out.data()), want);
      if ( r < 0 ) {
        out.set_size(0);
        return out;
      }
      out.set_size(static_cast<usize>(r) / sizeof(typename T::value_type));
      return out;
    }
    // size-0 (procfs/sysfs): chunk to EOF
    if constexpr ( sizeof(typename T::value_type) == 1 && requires(T t, typename T::value_type v) { t.push_back(v); } ) {
      byte chunk[4096];
      for ( ;; ) {
        max_t r = read(chunk, sizeof(chunk));
        if ( r <= 0 ) break;
        for ( max_t i = 0; i < r; i++ ) out.push_back(static_cast<typename T::value_type>(chunk[i]));
      }
    }
    return out;
  }

  // caller-sized fill
  template<typename T>
    requires((is_string<T> || is_contiguous_container<T>) && micron::is_trivially_copyable_v<typename T::value_type>)
  max_t
  read(T &out)
  {
    return read(static_cast<void *>(out.data()), out.size() * sizeof(typename T::value_type));
  }

  // stat / size
  i32
  stat(posix::statx_t &out)
  {
    if ( i32 e = __check() ) [[unlikely]]
      return e;
    max_t r = flash::statx(__handle.fd, out, *__eng);
    return static_cast<i32>(r);
  }

  max_t
  size()
  {
    posix::statx_t sx{};
    i32 e = stat(sx);
    if ( e < 0 ) return e;
    return static_cast<max_t>(sx.stx_size);
  }

  i32
  sync()
  {
    if ( i32 e = __check() ) [[unlikely]]
      return e;
    return static_cast<i32>(flash::fsync(__handle.fd, *__eng));
  }

  i32
  datasync()
  {
    if ( i32 e = __check() ) [[unlikely]]
      return e;
    return static_cast<i32>(flash::fdatasync(__handle.fd, *__eng));
  }

  i32
  allocate(u64 off, u64 len)
  {
    if ( i32 e = __check() ) [[unlikely]]
      return e;
    return static_cast<i32>(flash::fallocate(__handle.fd, 0, off, len, *__eng));
  }

  i32
  advise(u64 off, u64 len, i32 advice)
  {
    if ( i32 e = __check() ) [[unlikely]]
      return e;
    return static_cast<i32>(flash::fadvise(__handle.fd, off, len, advice, *__eng));
  }

  i32
  truncate(u64 len)
  {
    if ( i32 e = __check() ) [[unlikely]]
      return e;
    return static_cast<i32>(flash::ftruncate(__handle.fd, len, *__eng));
  }

  void
  close()
  {
    if ( __handle.fd >= 0 && __eng != nullptr && __eng->live() ) {
      (void)__oneshot(*__eng, [fd = __handle.fd](uring::sqe *s) { uring::prep_close(s, fd); });
    } else if ( __handle.fd >= 0 ) {
      (void)micron::syscall(SYS_close, __handle.fd);      // ring died mid-life: fall back to a raw close
    }
    __handle = fd_t{ posix::invalid_fd };
  }

  // %%%% functional members

  // producer: f.write([]{ return build(); })
  template<typename Fn>
    requires(fn_like<Fn> && micron::is_invocable_v<Fn> && !micron::is_void_v<micron::invoke_result_t<Fn>>)
  max_t
  write(Fn &&fn)
  {
    auto v = micron::forward<Fn>(fn)();
    return write(v);
  }

  // consumer: f.read([](string s){ ... }) -> option<R, error_t>
  template<typename Fn>
    requires(micron::is_invocable_v<Fn, micron::string>)
  auto
  read(Fn &&fn) -> micron::option<__unit_if_void_t<micron::invoke_result_t<Fn, micron::string>>, io::error_t>
  {
    using R = __unit_if_void_t<micron::invoke_result_t<Fn, micron::string>>;
    if ( i32 e = __check() ) [[unlikely]]
      return micron::option<R, io::error_t>{ io::error_t(e) };
    micron::string s = read<micron::string>();
    if constexpr ( micron::is_void_v<micron::invoke_result_t<Fn, micron::string>> ) {
      micron::forward<Fn>(fn)(micron::move(s));
      return micron::option<R, io::error_t>{ unit_t{} };
    } else {
      return micron::option<R, io::error_t>{ micron::forward<Fn>(fn)(micron::move(s)) };
    }
  }

  // in-place rewrite: T -> T
  template<typename Fn>
    requires(micron::is_invocable_v<Fn, micron::string> && is_string<micron::invoke_result_t<Fn, micron::string>>)
  max_t
  modify(Fn &&fn)
  {
    if ( i32 e = __check() ) [[unlikely]]
      return e;
    micron::string in = read<micron::string>();
    micron::string out = micron::forward<Fn>(fn)(micron::move(in));
    if ( i32 t = truncate(0); t < 0 ) return t;
    seek(0);
    return write(out);
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fsys-mirror free fns

namespace __impl
{
// read into buf[got .. cap) until it is full or EOF. returns bytes added, or -errno.
//
// ONE read_at call, deliberately: flash::pread already continues internally until the request is
// satisfied or EOF, so on return `got` is either cap (buffer full, maybe more file) or the true
// file size. an outer loop here would only add a redundant zero-length read round trip.
template<typename F, typename T>
inline max_t
__fill(F &f, T &buf, usize &got, usize cap) noexcept
{
  if ( got >= cap ) return 0;
  const max_t r = f.read_at(got, static_cast<void *>(buf.data() + got), cap - got);
  if ( r < 0 ) return r;
  got += static_cast<usize>(r);
  return r;
}

// the probe filled, so the file is bigger than __probe_size. ask for the exact size ONCE and size
// the buffer ONCE.
//
// this is where a statx earns its keep, and it is an ALLOCATION trade rather than a syscall one: a
// 1 MiB allocation costs ~1.5 ms in this allocator (measured -- read_file<T> at 1 MiB is 13x
// read_file(p,target), which differs only in reusing the caller's buffer), against ~19 us for the
// io-wq statx punt. geometric growth here measured 4.6x SLOWER than one exact reservation. small
// files never reach this path and so never pay the statx at all.
template<typename F, typename T>
inline i32
__finish_large(F &f, T &buf, usize &got, usize cap, engine &eng) noexcept
{
  posix::statx_t sx{};
  const max_t st = flash::statx(f.raw_fd(), sx, eng);
  if ( st >= 0 && sx.stx_size > 0 ) {
    const usize want = static_cast<usize>(sx.stx_size);
    if ( want <= got ) return 0;      // the probe already covers the whole file -- do NOT grow
    buf.reserve(want + 1);
    buf.set_size(want);
    const max_t r = __fill(f, buf, got, want);
    return r < 0 ? static_cast<i32>(r) : 0;
  }
  // no usable size (procfs and friends report 0) -- fall back to geometric growth to EOF
  while ( got == cap ) {
    cap *= 4;
    buf.reserve(cap + 1);
    buf.set_size(cap);
    const max_t r = __fill(f, buf, got, cap);
    if ( r < 0 ) return static_cast<i32>(r);
  }
  return 0;
}
};      // namespace __impl

inline flash::file
open_file(const io::path_t &p, const modes m = modes::read, const open_opts &opts = {}, engine &eng = default_engine())
{
  return flash::file(p, m, opts, eng);
}

inline flash::file
create_file(const io::path_t &p, u32 mode = posix::mode_file, engine &eng = default_engine())
{
  open_opts o{};
  o.perms = mode;
  return flash::file(p, modes::write, o, eng);
}

// whole-file read: open, then read to EOF into a geometrically-grown reservation.
//
// there is deliberately NO statx size probe. IORING_OP_STATX has no inline completion path -- the
// kernel always punts it to an io-wq worker, measured at ~19 us on 7.1.3 against ~1.3 us for a
// cached read on the same ring. the probe alone was roughly 60% of this function's cost, and it
// bought nothing a short read does not already tell us. it also made the size a TOCTOU guess: a
// file that grew between the statx and the read was silently truncated to the stale size.
//
// (batched statx is a different story -- io-wq punts run concurrently, so stat_many amortises them
// to ~1 us/file. read_files keeps its statx wave for exactly that reason.)
template<typename T = micron::string>
  requires((is_string<T> || is_contiguous_container<T>) && sizeof(typename T::value_type) == 1)
micron::option<T, io::error_t>
read_file(const io::path_t &p, engine &eng = default_engine())
{
  using Ret = micron::option<T, io::error_t>;
  if ( !available(eng) ) return Ret{ io::error_t(-error::bad_syscall) };

  flash::file f = open_file(p, modes::read, {}, eng);
  if ( !f.valid() ) return Ret{ io::error_t(f.raw_fd()) };

  T out{};
  usize cap = __impl::__probe_size;
  out.reserve(cap + 1);
  out.set_size(cap);
  usize got = 0;
  const max_t pr = __impl::__fill(f, out, got, cap);
  if ( pr < 0 ) return Ret{ io::error_t(static_cast<i32>(pr)) };
  if ( got < cap ) {      // EOF inside the probe: the common case, one allocation and no statx
    out.set_size(got);
    return Ret{ micron::move(out) };
  }
  if ( i32 e = __impl::__finish_large(f, out, got, cap, eng) ) return Ret{ io::error_t(e) };
  out.set_size(got);
  return Ret{ micron::move(out) };
}

// whole-file read into a caller-provided target (replace semantics; reuses target's capacity).
// growable targets mirror read_file<T>; fixed-capacity targets must hold the whole file or
// -error::file_too_big. returns bytes read or -errno; on read failure the target is emptied, on
// open/stat failure it is untouched
template<typename T>
  requires((is_string<T> || is_contiguous_container<T>) && sizeof(typename T::value_type) == 1)
max_t
read_file(const io::path_t &p, T &target, engine &eng = default_engine())
{
  if ( !available(eng) ) return -error::bad_syscall;

  flash::file f = open_file(p, modes::read, {}, eng);
  if ( !f.valid() ) return f.raw_fd();

  if constexpr ( requires(T t, usize n) {
                   t.reserve(n);
                   t.set_size(n);
                 } ) {
    // growable: probe-and-grow, starting from whatever the target already has reserved. reusing
    // the caller's capacity is the whole point of this overload -- for a 1 MiB file it is the
    // difference between one allocation per call and none.
    usize cap = __impl::__probe_size;
    if constexpr ( requires(T t) { t.capacity(); } ) {
      if ( target.capacity() > cap + 1 ) cap = target.capacity() - 1;
    }
    usize got = 0;
    target.reserve(cap + 1);
    target.set_size(cap);
    const max_t pr = __impl::__fill(f, target, got, cap);
    if ( pr < 0 ) {
      target.set_size(0);
      return pr;
    }
    if ( got == cap ) {
      if ( i32 e = __impl::__finish_large(f, target, got, cap, eng) ) {
        target.set_size(0);
        return e;
      }
    }
    target.set_size(got);
    return static_cast<max_t>(got);
  } else if constexpr ( requires(T t) { t.max_size(); } ) {
    // fixed-capacity fill: read up to capacity, then probe one more byte. without a size known up
    // front that probe is what distinguishes "exactly fits" from "file is larger" -- a silent
    // prefix would be indistinguishable from a smaller file.
    const usize cap = target.max_size();
    usize got = 0;
    while ( got < cap ) {
      const max_t r = f.read_at(got, static_cast<void *>(target.data() + got), cap - got);
      if ( r < 0 ) return r;
      if ( r == 0 ) break;
      got += static_cast<usize>(r);
    }
    if ( got == cap ) {
      byte probe = 0;
      if ( f.read_at(got, &probe, 1) > 0 ) [[unlikely]]
        return -error::file_too_big;
    }
    if constexpr ( requires(T t, usize n) { t.set_size(n); } ) target.set_size(got);
    return static_cast<max_t>(got);
  } else {
    return f.read(target);      // no reserve, no max_size (micron::buffer): caller-sized fill
  }
}

// create/truncate + write via a linked open->write->close chain (1 submission on tier::fixed). NO
// implicit fsync (see banner).
template<typename C>
max_t
write_file(const io::path_t &p, const C &c, engine &eng = default_engine())
{
  if ( !available(eng) ) return -error::bad_syscall;
  open_opts o{};
  flash::file f(p, modes::write, o, eng);
  if ( !f.valid() ) return f.raw_fd();
  return f.write(c);
}

// write_file + an fsync on the SAME handle before close. this used to call write_file (openat,
// write, close) and then reopen the file purely to fsync it -- six round trips where four do.
template<typename C>
max_t
write_file_sync(const io::path_t &p, const C &c, engine &eng = default_engine())
{
  if ( !available(eng) ) return -error::bad_syscall;
  open_opts o{};
  flash::file f(p, modes::write, o, eng);
  if ( !f.valid() ) return f.raw_fd();
  const max_t n = f.write(c);
  if ( n < 0 ) return n;
  if ( const max_t s = f.sync(); s < 0 ) return s;
  return n;
}

template<typename C>
max_t
append_file(const io::path_t &p, const C &c, engine &eng = default_engine())
{
  if ( !available(eng) ) return -error::bad_syscall;
  open_opts o{};
  flash::file f(p, modes::append, o, eng);
  if ( !f.valid() ) return f.raw_fd();
  // O_APPEND: write at the file position (off == -1)
  return f.write(c);
}

inline max_t
remove(const io::path_t &p, engine &eng = default_engine())
{
  if ( !available(eng) ) return -error::bad_syscall;
  return __oneshot(eng, [&](uring::sqe *s) { uring::prep_unlinkat(s, posix::at_fdcwd, p.c_str(), 0); });
}

inline max_t
rename(const io::path_t &from, const io::path_t &to, engine &eng = default_engine())
{
  if ( !available(eng) || !kernel::has(kernel::feature::uring_renameat) ) return -error::bad_syscall;
  return __oneshot(eng, [&](uring::sqe *s) { uring::prep_renameat(s, posix::at_fdcwd, from.c_str(), posix::at_fdcwd, to.c_str()); });
}

inline max_t
mkdir(const io::path_t &p, u32 mode = posix::mode_dir, engine &eng = default_engine())
{
  if ( !available(eng) || !kernel::has(kernel::feature::uring_mkdirat) ) return -error::bad_syscall;
  return __oneshot(eng, [&](uring::sqe *s) { uring::prep_mkdirat(s, posix::at_fdcwd, p.c_str(), mode); });
}

inline micron::option<posix::statx_t, io::error_t>
stat(const io::path_t &p, engine &eng = default_engine())
{
  using Ret = micron::option<posix::statx_t, io::error_t>;
  if ( !available(eng) ) return Ret{ io::error_t(-error::bad_syscall) };
  posix::statx_t sx{};
  max_t r = __oneshot(eng, [&](uring::sqe *s) { uring::prep_statx(s, posix::at_fdcwd, p.c_str(), 0, posix::statx_basic_stats, &sx); });
  if ( r < 0 ) return Ret{ io::error_t(static_cast<i32>(r)) };
  return Ret{ sx };
}

inline posix::off64_t
file_size(const io::path_t &p, engine &eng = default_engine())
{
  auto r = stat(p, eng);
  if ( r.is_second() ) return -1;
  return static_cast<posix::off64_t>(r.template cast<posix::statx_t>().stx_size);
}

inline bool
exists(const io::path_t &p, engine &eng = default_engine())
{
  return stat(p, eng).is_first();
}

// preferred O_DIRECT alignment (stx_dio_mem_align, >=6.1) or 4096 as a safe default
inline u32
dio_align(const io::path_t &p, engine &eng = default_engine())
{
  if ( !kernel::has(kernel::feature::statx_dio_align) ) return 4096;
  auto r = stat(p, eng);
  if ( r.is_second() ) return 4096;
  u32 a = r.template cast<posix::statx_t>().stx_dio_mem_align;
  return a ? a : 4096;
}

// slab ping-pong copy (uring has no file-to-file copy_file_range op). NO implicit fsync.
//
// prefers a registered pool slab (256 KiB by default) over a stack buffer: 4x fewer round trips
// than the old 64 KiB chunking, and no 64 KiB stack frame -- which mattered under -k freestanding,
// where that frame was a real hazard. the pool is materialised lazily, so this is also its first
// actual consumer.
//
// still strictly serialised read -> write -> read. overlapping them (fill slab B on the same wave
// that drains slab A -- independent ops, one enter for both) is a further ~2x and is the obvious
// next step; it is left out here because copy has exactly one regression test.
inline max_t
copy(const io::path_t &from, const io::path_t &to, engine &eng = default_engine())
{
  if ( !available(eng) ) return -error::bad_syscall;
  flash::file src = open_file(from, modes::read, {}, eng);
  if ( !src.valid() ) return src.raw_fd();
  open_opts o{};
  flash::file dst(to, modes::write, o, eng);
  if ( !dst.valid() ) return dst.raw_fd();

  const i32 sfd = src.raw_fd();
  const i32 dfd = dst.raw_fd();
  // the pool is optional (fixed_bufs == 0, or register_buffers lost to RLIMIT_MEMLOCK), so a failed
  // acquire is an ordinary outcome, not an error: fall back to the stack. is_first() is what makes
  // that a fallback rather than a reinterpret of the error alternative as a pool_buf.
  micron::option<pool_buf, io::error_t> got = acquire_buf(eng);
  pool_buf slab = got.is_first() ? micron::move(got.cast<pool_buf>()) : pool_buf{};
  byte fallback[16384];
  byte *buf = slab.valid() ? slab.data : fallback;
  const usize cap = slab.valid() ? slab.cap : sizeof(fallback);

  u64 total = 0;
  for ( ;; ) {
    const max_t r = flash::pread(sfd, buf, cap, total, eng);
    if ( r < 0 ) return r;
    if ( r == 0 ) break;
    usize wrote = 0;
    while ( wrote < static_cast<usize>(r) ) {
      const max_t w = flash::pwrite(dfd, buf + wrote, static_cast<usize>(r) - wrote, total + wrote, eng);
      if ( w < 0 ) return w;
      if ( w == 0 ) return -error::io_error;
      wrote += static_cast<usize>(w);
    }
    total += static_cast<u64>(wrote);
  }
  return static_cast<max_t>(total);
}

// renameat; on EXDEV, copy + unlink (all through the ring)
inline max_t
move(const io::path_t &from, const io::path_t &to, engine &eng = default_engine())
{
  max_t r = rename(from, to, eng);
  if ( r == 0 || -r != error::cross_device_link ) return r;
  if ( max_t c = copy(from, to, eng); c < 0 ) return c;
  return remove(from, eng);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// batch: the headline. N whole-file loads in flight on one ring.

// four batched waves per window -- statx / openat / read / close -- so a whole window of files
// costs ~4 io_uring_enters instead of the 1 + 3N it used to. the previous "wave 2" was a serial
// loop of open_file -> read -> dtor-close, three fully-blocking round trips per file, which made
// this API measurably SLOWER than a naive posix loop.
//
// deliberately waves rather than N linked open->read->close chains: a chain closes the fd before
// the caller learns the read was short, so a short read needs a reopen-and-continue repair path,
// and links only work at tier::fixed where a later op can name the fd via a direct descriptor.
// waves handle a short read by simply re-issuing at offset `done` with the fd still open, and run
// identically on tier::basic and tier::fixed. the marginal syscall saving of chains is ~1 enter
// per window.
template<typename T = micron::string>
  requires((is_string<T> || is_contiguous_container<T>) && sizeof(typename T::value_type) == 1)
micron::vector<micron::option<T, io::error_t>>
read_files(const micron::vector<io::path_t> &paths, engine &eng = default_engine())
{
  using Opt = micron::option<T, io::error_t>;
  micron::vector<Opt> out;
  const usize n = paths.size();
  out.reserve(n);
  if ( n == 0 ) return out;
  if ( !available(eng) ) {
    for ( usize i = 0; i < n; i++ ) out.push_back(Opt{ io::error_t(-error::bad_syscall) });
    return out;
  }

  // reserve-then-size, never vector(n): the ctor value-constructs (zeroes) every element and the
  // kernel is about to overwrite all of it anyway.
  micron::vector<posix::statx_t> sx;
  sx.reserve(n);
  sx.set_size(n);
  micron::vector<__impl::__bslot> st;
  st.reserve(n);
  for ( usize i = 0; i < n; i++ ) st.push_back(__impl::__bslot{});
  // destination buffers, allocated up front so the vector never reallocates while the kernel holds
  // pointers into the elements
  micron::vector<T> bufs;
  bufs.reserve(n);
  for ( usize i = 0; i < n; i++ ) bufs.push_back(T{});

  usize base = 0;
  while ( base < n ) {
    const u32 W = __impl::__batch_window(eng, n - base);

    // wave 1 -- statx. kept (unlike the single-file path, which drops it) precisely because io-wq
    // punts run concurrently: serial statx is ~19 us/file, batched it is ~1 us/file.
    (void)__impl::__stage_and_run(
        eng, W, __ud::st_statx,
        [&](uring::sqe *s, u32 i) {
          uring::prep_statx(s, posix::at_fdcwd, paths[base + i].c_str(), 0, posix::statx_size, &sx[base + i]);
          return true;
        },
        [&](u32 i, u8, i32 res) {
          if ( i >= W ) return;
          if ( res < 0 )
            st[base + i].err = res;
          else
            st[base + i].want = sx[base + i].stx_size;
        });

    // wave 2 -- openat
    (void)__impl::__stage_and_run(
        eng, W, __ud::st_open,
        [&](uring::sqe *s, u32 i) {
          if ( st[base + i].err != 0 ) return false;
          uring::prep_openat(s, posix::at_fdcwd, paths[base + i].c_str(), static_cast<u32>(posix::o_rdonly), 0u);
          return true;
        },
        [&](u32 i, u8, i32 res) {
          if ( i >= W ) return;
          if ( res < 0 )
            st[base + i].err = res;
          else
            st[base + i].fd = res;
        });

    // a slot that was staged but whose completion never landed still has err == 0 and fd == -1,
    // which every later wave reads as "nothing to do" -- it would fall out of the final assembly
    // as an empty buffer wrapped in the SUCCESS alternative. an unobserved open is a failure, not
    // an empty file. (slots skipped by the fill above already carry a real err.)
    for ( u32 i = 0; i < W; i++ ) {
      __impl::__bslot &s = st[base + i];
      if ( s.err == 0 && s.fd < 0 ) s.err = -error::io_error;
    }

    // size the destinations before anything is handed to the kernel
    for ( u32 i = 0; i < W; i++ ) {
      __impl::__bslot &s = st[base + i];
      if ( s.err != 0 || s.fd < 0 || s.want == 0 ) continue;
      bufs[base + i].reserve(static_cast<usize>(s.want) + 1);
      bufs[base + i].set_size(static_cast<usize>(s.want));
    }

    // wave 3 -- read, re-issued for any slot that came back short (fds are still open, so this is
    // just another wave at offset `done`; no reopen, no severed-link repair)
    for ( u32 round = 0; round < 16; round++ ) {
      bool progressed = false;
      bool interrupted = false;
      const u32 staged = __impl::__stage_and_run(
          eng, W, __ud::st_read,
          [&](uring::sqe *s_, u32 i) {
            __impl::__bslot &s = st[base + i];
            if ( s.err != 0 || s.fd < 0 || s.want == 0 || s.done >= s.want ) return false;
            const u64 rem = s.want - s.done;
            const u32 chunk = rem > __impl::__max_chunk ? __impl::__max_chunk : static_cast<u32>(rem);
            uring::prep_read(s_, s.fd, static_cast<byte *>(static_cast<void *>(bufs[base + i].data())) + s.done, chunk, s.done);
            return true;
          },
          [&](u32 i, u8, i32 res) {
            if ( i >= W ) return;
            __impl::__bslot &s = st[base + i];
            if ( res < 0 ) {
              // EINTR is retryable, not an error and not progress. it has to keep the round loop
              // alive on its own: falling out on !progressed would end the wave here and report
              // the partially filled buffer as a complete, successful read.
              if ( res == -error::interrupted )
                interrupted = true;
              else
                s.err = res;
            } else if ( res == 0 ) {
              s.want = s.done;      // EOF before the statx size: the file shrank under us
            } else {
              s.done += static_cast<u64>(res);
              progressed = true;
            }
          });
      if ( staged == 0 || (!progressed && !interrupted) ) break;
    }

    // out of rounds with bytes still owed, or a read whose completion never landed: either way the
    // buffer is short. reporting it as a success is indistinguishable from a genuinely small file.
    for ( u32 i = 0; i < W; i++ ) {
      __impl::__bslot &s = st[base + i];
      if ( s.err == 0 && s.fd >= 0 && s.want != 0 && s.done < s.want ) s.err = -error::io_error;
    }

    // wave 4 -- close. a close failure never fails the read.
    (void)__impl::__stage_and_run(
        eng, W, __ud::st_close,
        [&](uring::sqe *s_, u32 i) {
          if ( st[base + i].fd < 0 ) return false;
          uring::prep_close(s_, st[base + i].fd);
          return true;
        },
        [](u32, u8, i32) { });
    for ( u32 i = 0; i < W; i++ ) st[base + i].fd = -1;

    base += W;
  }

  for ( usize k = 0; k < n; k++ ) {
    if ( st[k].err != 0 ) {
      out.push_back(Opt{ io::error_t(st[k].err) });
      continue;
    }
    if ( st[k].want == 0 ) {
      // size-0 virtual file (procfs and friends): only the streaming reader can size it
      out.push_back(read_file<T>(paths[k], eng));
      continue;
    }
    bufs[k].set_size(static_cast<usize>(st[k].done));
    out.push_back(Opt{ micron::move(bufs[k]) });
  }
  return out;
}

template<typename T = micron::string, typename... P>
  requires(sizeof...(P) > 0 && (micron::same_as<micron::remove_cvref_t<P>, io::path_t> && ...))
micron::vector<micron::option<T, io::error_t>>
read_files(const P &...paths)
{
  micron::vector<io::path_t> v;
  (v.push_back(paths), ...);
  return read_files<T>(v);
}

// batch-load into a caller-provided results vector (appends one option per path, input order).
// returns the number of successful loads; per-file errors are reported inside target
template<typename T>
  requires((is_string<T> || is_contiguous_container<T>) && sizeof(typename T::value_type) == 1)
max_t
read_files(const micron::vector<io::path_t> &paths, micron::vector<micron::option<T, io::error_t>> &target, engine &eng = default_engine())
{
  micron::vector<micron::option<T, io::error_t>> results = read_files<T>(paths, eng);
  max_t ok = 0;
  target.reserve(target.size() + results.size());
  for ( usize i = 0; i < results.size(); i++ ) {
    if ( results[i].is_first() ) ok++;
    target.push_back(micron::move(results[i]));
  }
  return ok;
}

struct write_spec {
  io::path_t path;
  const byte *data;
  usize len;
};

// three batched waves -- openat(create|trunc) / write / close -- mirroring read_files. NO implicit
// fsync (see banner).
inline micron::vector<micron::option<max_t, io::error_t>>
write_files(const micron::vector<write_spec> &specs, engine &eng = default_engine())
{
  using Opt = micron::option<max_t, io::error_t>;
  micron::vector<Opt> out;
  const usize n = specs.size();
  out.reserve(n);
  if ( n == 0 ) return out;
  if ( !available(eng) ) {
    for ( usize i = 0; i < n; i++ ) out.push_back(Opt{ io::error_t(-error::bad_syscall) });
    return out;
  }

  micron::vector<__impl::__bslot> st;
  st.reserve(n);
  for ( usize i = 0; i < n; i++ ) {
    __impl::__bslot b{};
    b.want = specs[i].len;
    st.push_back(b);
  }

  usize base = 0;
  while ( base < n ) {
    const u32 W = __impl::__batch_window(eng, n - base);

    (void)__impl::__stage_and_run(
        eng, W, __ud::st_open,
        [&](uring::sqe *s, u32 i) {
          uring::prep_openat(s, posix::at_fdcwd, specs[base + i].path.c_str(),
                             static_cast<u32>(posix::o_wronly | posix::o_create | posix::o_trunc), posix::mode_file);
          return true;
        },
        [&](u32 i, u8, i32 res) {
          if ( i >= W ) return;
          if ( res < 0 )
            st[base + i].err = res;
          else
            st[base + i].fd = res;
        });

    // an open whose completion never landed leaves err == 0 / fd == -1, which the write wave skips
    // and the final loop reports as "0 bytes written, success" -- while the O_TRUNC has already
    // emptied the destination. see the matching guard in read_files.
    for ( u32 i = 0; i < W; i++ ) {
      __impl::__bslot &s = st[base + i];
      if ( s.err == 0 && s.fd < 0 ) s.err = -error::io_error;
    }

    for ( u32 round = 0; round < 16; round++ ) {
      bool progressed = false;
      bool interrupted = false;
      const u32 staged = __impl::__stage_and_run(
          eng, W, __ud::st_write,
          [&](uring::sqe *s_, u32 i) {
            __impl::__bslot &s = st[base + i];
            if ( s.err != 0 || s.fd < 0 || s.done >= s.want ) return false;
            const u64 rem = s.want - s.done;
            const u32 chunk = rem > __impl::__max_chunk ? __impl::__max_chunk : static_cast<u32>(rem);
            uring::prep_write(s_, s.fd, specs[base + i].data + s.done, chunk, s.done);
            return true;
          },
          [&](u32 i, u8, i32 res) {
            if ( i >= W ) return;
            __impl::__bslot &s = st[base + i];
            if ( res < 0 ) {
              if ( res == -error::interrupted )
                interrupted = true;      // retryable; must not end the loop as a short write
              else
                s.err = res;
            } else if ( res == 0 ) {
              s.want = s.done;      // no progress possible
            } else {
              s.done += static_cast<u64>(res);
              progressed = true;
            }
          });
      if ( staged == 0 || (!progressed && !interrupted) ) break;
    }

    // bytes still owed: a short write reported as success would tell the caller the file is
    // complete when it is truncated.
    for ( u32 i = 0; i < W; i++ ) {
      __impl::__bslot &s = st[base + i];
      if ( s.err == 0 && s.fd >= 0 && s.done < s.want ) s.err = -error::io_error;
    }

    (void)__impl::__stage_and_run(
        eng, W, __ud::st_close,
        [&](uring::sqe *s_, u32 i) {
          if ( st[base + i].fd < 0 ) return false;
          uring::prep_close(s_, st[base + i].fd);
          return true;
        },
        [](u32, u8, i32) { });
    for ( u32 i = 0; i < W; i++ ) st[base + i].fd = -1;

    base += W;
  }

  for ( usize k = 0; k < n; k++ ) {
    if ( st[k].err != 0 )
      out.push_back(Opt{ io::error_t(st[k].err) });
    else
      out.push_back(Opt{ static_cast<max_t>(st[k].done) });
  }
  return out;
}

inline micron::vector<micron::option<posix::statx_t, io::error_t>>
stat_many(const micron::vector<io::path_t> &paths, engine &eng = default_engine())
{
  using Opt = micron::option<posix::statx_t, io::error_t>;
  micron::vector<Opt> out;
  const usize n = paths.size();
  out.reserve(n);
  if ( !available(eng) ) {
    for ( usize i = 0; i < n; i++ ) out.push_back(Opt{ io::error_t(-error::bad_syscall) });
    return out;
  }
  micron::vector<posix::statx_t> sx;
  sx.reserve(n);
  sx.set_size(n);
  // seed with a failure, not 0. sx is deliberately left unconstructed (the kernel is about to
  // overwrite it), so 0 doubling as both "statx succeeded" and "no completion observed" would hand
  // back uninitialized statx bytes inside the SUCCESS alternative -- a caller reading stx_size off
  // that reserves a garbage 64-bit length. only a real completion may clear this.
  micron::vector<i32> res;
  res.reserve(n);
  for ( usize i = 0; i < n; i++ ) res.push_back(-error::io_error);

  usize base = 0;
  while ( base < n ) {
    const u32 W = __impl::__batch_window(eng, n - base);
    (void)__impl::__stage_and_run(
        eng, W, __ud::st_statx,
        [&](uring::sqe *s, u32 i) {
          uring::prep_statx(s, posix::at_fdcwd, paths[base + i].c_str(), 0, posix::statx_basic_stats, &sx[base + i]);
          return true;
        },
        [&](u32 i, u8, i32 r) {
          if ( i < W ) res[base + i] = r;
        });
    base += W;
  }
  for ( usize k = 0; k < n; k++ ) {
    if ( res[k] < 0 )
      out.push_back(Opt{ io::error_t(res[k]) });
    else
      out.push_back(Opt{ sx[k] });
  }
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// functional combinators (fp.hpp shapes)

template<typename Fn>
  requires(micron::invocable<Fn, flash::file &>
           && micron::distinct<__unit_if_void_t<micron::invoke_result_t<Fn, flash::file &>>, io::error_t>)
auto
with_file(const io::path_t &p, const modes m, Fn &&fn, engine &eng = default_engine())
    -> micron::option<__unit_if_void_t<micron::invoke_result_t<Fn, flash::file &>>, io::error_t>
{
  using Ret = micron::option<__unit_if_void_t<micron::invoke_result_t<Fn, flash::file &>>, io::error_t>;
  flash::file f = open_file(p, m, {}, eng);
  if ( !f.valid() ) [[unlikely]]
    return Ret{ io::error_t(f.raw_fd()) };
  if constexpr ( micron::is_void_v<micron::invoke_result_t<Fn, flash::file &>> ) {
    micron::forward<Fn>(fn)(f);
    return Ret{ unit_t{} };
  } else {
    return Ret{ micron::forward<Fn>(fn)(f) };
  }
}

template<typename Fn>
  requires micron::invocable<Fn, flash::file &>
auto
with_file(const io::path_t &p, Fn &&fn, engine &eng = default_engine())
{
  return with_file(p, modes::read, micron::forward<Fn>(fn), eng);
}

// batch-load, apply fn to each content; per-item option (fp counterpart of read_files)
template<typename T = micron::string, typename Fn>
  requires(micron::is_invocable_v<Fn, T &&> && micron::distinct<__unit_if_void_t<micron::invoke_result_t<Fn, T &&>>, io::error_t>)
auto
map_files(const micron::vector<io::path_t> &paths, Fn &&fn, engine &eng = default_engine())
    -> micron::vector<micron::option<__unit_if_void_t<micron::invoke_result_t<Fn, T &&>>, io::error_t>>
{
  using R = __unit_if_void_t<micron::invoke_result_t<Fn, T &&>>;
  using Opt = micron::option<R, io::error_t>;
  micron::vector<Opt> out;
  auto loaded = read_files<T>(paths, eng);
  out.reserve(loaded.size());
  for ( usize i = 0; i < loaded.size(); i++ ) {
    if ( loaded[i].is_second() ) {
      out.push_back(Opt{ loaded[i].template cast<io::error_t>() });
      continue;
    }
    if constexpr ( micron::is_void_v<micron::invoke_result_t<Fn, T &&>> ) {
      fn(micron::move(loaded[i].template cast<T>()));
      out.push_back(Opt{ unit_t{} });
    } else {
      out.push_back(Opt{ fn(micron::move(loaded[i].template cast<T>())) });
    }
  }
  return out;
}

// all-or-nothing left fold over loaded contents; first load error aborts and is returned
template<typename T = micron::string, typename R, typename Fn>
  requires(micron::distinct<R, io::error_t> && micron::is_invocable_v<Fn, R &&, const T &>)
auto
fold_files(const micron::vector<io::path_t> &paths, R init, Fn &&fn, engine &eng = default_engine()) -> micron::option<R, io::error_t>
{
  using Ret = micron::option<R, io::error_t>;
  auto loaded = read_files<T>(paths, eng);
  R acc = micron::move(init);
  for ( usize i = 0; i < loaded.size(); i++ ) {
    if ( loaded[i].is_second() ) return Ret{ loaded[i].template cast<io::error_t>() };
    acc = fn(micron::move(acc), loaded[i].template cast<T>());
  }
  return Ret{ micron::move(acc) };
}

template<typename Fn>
  requires requires(flash::file &f, Fn &&fn) { f.modify(micron::forward<Fn>(fn)); }
max_t
modify(const io::path_t &p, Fn &&fn, engine &eng = default_engine())
{
  flash::file f = open_file(p, modes::readwrite, {}, eng);
  if ( !f.valid() ) [[unlikely]]
    return f.raw_fd();
  return f.modify(micron::forward<Fn>(fn));
}

// %%%% curried (default_engine resolved at INVOCATION -> safe to pass across threads)

inline auto
write_file_c(io::path_t p)
{
  return [p = micron::move(p)](const auto &c) { return flash::write_file(p, c); };
}

inline auto
append_file_c(io::path_t p)
{
  return [p = micron::move(p)](const auto &c) { return flash::append_file(p, c); };
}

template<typename T = micron::string>
auto
read_file_c()
{
  return [](const io::path_t &p) { return flash::read_file<T>(p); };
}

template<typename Fn>
  requires fn_deducible<Fn>
auto
modify_c(Fn &&fn)
{
  return [fn = micron::forward<Fn>(fn)](const io::path_t &p) mutable { return flash::modify(p, fn); };
}

};      // namespace flash
};      // namespace io
};      // namespace micron
