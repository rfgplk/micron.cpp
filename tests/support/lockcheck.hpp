//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// detectors for the lock suite  --  namespace lcheck
//
// every contention case in tests/rigor/mutex_* grades the same way: N threads each increment a
// plain int under the lock, then assert the total. that catches a lock that LOSES an increment. it
// does not catch a lock that admits two holders whose increments happen not to collide, it cannot
// see starvation at all (the total is the same however unfairly it was reached), and a deadlock
// shows up as the sweep hanging rather than as a graded failure.
//
//   exclusion_probe   -- asserts on double entry to a critical section
//   fairness_probe    -- per-thread acquisition counts, handoff order, starvation ratio
//   order_graph       -- records lock-order edges; DFS afterwards finds LATENT inversions,
//                        including ones this particular run got lucky and did not deadlock on
//   watchdog          -- a progress-stall detector, so a deadlock fails with a diagnosis
//
// this header pairs with tests/support/lifetime.hpp (ltest) and needs snowball for the watchdog's
// failure path, so include it AFTER snowball.hpp.

#include "../../src/atomic/atomic.hpp"
#include "../../src/bits/__pause.hpp"
#include "../../src/sync/pause.hpp"
#include "../../src/sync/yield.hpp"
#include "../../src/types.hpp"

#include "lifetime.hpp"

namespace lcheck
{

// i32/u32/u64/usize are GLOBAL typedefs in src/types.hpp, not micron:: members

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// mutual exclusion

// wrap the critical section: enter() on the way in, leave() on the way out. any moment two threads
// are inside at once is recorded, whether or not it corrupted anything.
struct exclusion_probe {
  micron::atomic_token<i32> inside{ 0 };
  micron::atomic_token<u64> entries{ 0 };
  micron::atomic_token<u64> violations{ 0 };
  micron::atomic_token<u64> peak{ 0 };

  void
  enter() noexcept
  {
    const i32 n = inside.add_fetch(1, micron::memory_order::acq_rel);
    entries.fetch_add(1, micron::memory_order::relaxed);
    if ( n != 1 ) {
      violations.fetch_add(1, micron::memory_order::relaxed);
      u64 p = peak.get(micron::memory_order::relaxed);
      while ( static_cast<u64>(n) > p ) {
        if ( peak.compare_exchange_weak(p, static_cast<u64>(n), micron::memory_order::acq_rel, micron::memory_order::relaxed) ) break;
      }
    }
  }

  void
  leave() noexcept
  {
    if ( inside.sub_fetch(1, micron::memory_order::acq_rel) != 0 ) violations.fetch_add(1, micron::memory_order::relaxed);
  }

  // hold the section for a moment so an overlap has time to be observable. a critical section that
  // is a single increment is nearly impossible to catch two threads inside of.
  void
  dwell(u32 pauses) const noexcept
  {
    for ( u32 i = 0; i < pauses; ++i ) __cpu_pause();
  }

  [[nodiscard]] bool
  clean() const noexcept
  {
    return violations.get(micron::memory_order::acquire) == 0
           and inside.get(micron::memory_order::acquire) == 0;
  }

  void
  reset() noexcept
  {
    inside.store(0, micron::memory_order::release);
    entries.store(0, micron::memory_order::release);
    violations.store(0, micron::memory_order::release);
    peak.store(0, micron::memory_order::release);
  }
};

// RAII form
template<typename Lock> struct guarded_section {
  Lock &lk;
  exclusion_probe &pr;

  guarded_section(Lock &l, exclusion_probe &p, u32 dwell_pauses = 0) : lk(l), pr(p)
  {
    lk.lock();
    pr.enter();
    pr.dwell(dwell_pauses);
  }

  ~guarded_section()
  {
    pr.leave();
    lk.unlock();
  }

  guarded_section(const guarded_section &) = delete;
  guarded_section &operator=(const guarded_section &) = delete;
};

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// fairness

inline constexpr u32 max_probe_threads = 64;
inline constexpr usize handoff_log_size = 4096;

struct fairness_probe {
  micron::atomic_token<u64> got[max_probe_threads];
  micron::atomic_token<u32> handoff[handoff_log_size];      // ring of acquiring thread ids
  micron::atomic_token<u64> handoffs{ 0 };

  void
  note(u32 tid) noexcept
  {
    if ( tid < max_probe_threads ) got[tid].fetch_add(1, micron::memory_order::relaxed);
    const u64 i = handoffs.fetch_add(1, micron::memory_order::acq_rel);
    if ( i < handoff_log_size ) handoff[i].store(tid, micron::memory_order::release);
  }

  [[nodiscard]] u64
  total(u32 n) const noexcept
  {
    u64 t = 0;
    for ( u32 i = 0; i < n and i < max_probe_threads; ++i ) t += got[i].get(micron::memory_order::acquire);
    return t;
  }

  [[nodiscard]] u64
  least(u32 n) const noexcept
  {
    u64 m = ~0ull;
    for ( u32 i = 0; i < n and i < max_probe_threads; ++i ) {
      const u64 v = got[i].get(micron::memory_order::acquire);
      if ( v < m ) m = v;
    }
    return m == ~0ull ? 0 : m;
  }

  [[nodiscard]] u64
  most(u32 n) const noexcept
  {
    u64 m = 0;
    for ( u32 i = 0; i < n and i < max_probe_threads; ++i ) {
      const u64 v = got[i].get(micron::memory_order::acquire);
      if ( v > m ) m = v;
    }
    return m;
  }

  // most/least. 1.0 is perfect. an unfair TTAS lock under load reaches the hundreds; a FIFO lock
  // stays at 1 or 2. returns 0 when some thread never acquired at all -- outright starvation.
  [[nodiscard]] double
  spread(u32 n) const noexcept
  {
    const u64 lo = least(n);
    if ( lo == 0 ) return 0.0;
    return static_cast<double>(most(n)) / static_cast<double>(lo);
  }

  // the i-th thread to acquire, in acquisition order (the note() call is inside the section)
  [[nodiscard]] u32
  at(u64 i) const noexcept
  {
    return i < handoff_log_size ? handoff[i].get(micron::memory_order::acquire) : 0xFFFFFFFFu;
  }

  [[nodiscard]] u64
  logged() const noexcept
  {
    const u64 h = handoffs.get(micron::memory_order::acquire);
    return h < handoff_log_size ? h : handoff_log_size;
  }

  // NOTE on what CANNOT be graded here: a free-running contention loop does not keep every thread
  // enqueued, so "no id repeats in a window of n" is not a property even a perfectly FIFO lock has
  // -- a thread that releases and re-queues while its peer is still descheduled legitimately
  // acquires twice. proving FIFO needs a KNOWN enqueue order, which is what the queue-order case in
  // mutex_ticket.cpp / mutex_mcs_lock.cpp builds by hand. spread() is the free-running metric.

  void
  reset() noexcept
  {
    for ( u32 i = 0; i < max_probe_threads; ++i ) got[i].store(0, micron::memory_order::release);
    for ( usize i = 0; i < handoff_log_size; ++i ) handoff[i].store(0, micron::memory_order::release);
    handoffs.store(0, micron::memory_order::release);
  }
};

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// lock-order inversions
//
// the point of this one: a soak that does NOT deadlock proves nothing about whether it could. every
// (held, wanted) pair a thread ever produces is an edge in the lock-order graph, and a cycle in
// that graph is a deadlock waiting for the right interleaving -- catchable in a run that never
// stalled at all.

inline constexpr usize max_graph_nodes = 32;

struct order_graph {
  micron::atomic_token<usize> node[max_graph_nodes];      // lock addresses as integers; the bitset
                                                          // rows below want an integral key anyway
  micron::atomic_token<u32> adj[max_graph_nodes];      // bitset row: node i was held while wanting j
  micron::atomic_token<u32> nodes{ 0 };
  micron::atomic_token<u64> overflow{ 0 };
  micron::atomic_token<u64> edges{ 0 };

  static_assert(max_graph_nodes <= 32, "adjacency rows are u32 bitsets");

  [[nodiscard]] u32
  intern(const void *ptr) noexcept
  {
    const usize p = reinterpret_cast<usize>(ptr);
    const u32 n = nodes.get(micron::memory_order::acquire);
    for ( u32 i = 0; i < n; ++i )
      if ( node[i].get(micron::memory_order::acquire) == p ) return i;

    for ( ;; ) {
      u32 slot = nodes.get(micron::memory_order::acquire);
      for ( u32 i = n; i < slot; ++i )
        if ( node[i].get(micron::memory_order::acquire) == p ) return i;
      if ( slot >= max_graph_nodes ) {
        overflow.fetch_add(1, micron::memory_order::relaxed);
        return max_graph_nodes;
      }
      if ( nodes.compare_exchange_weak(slot, slot + 1, micron::memory_order::acq_rel, micron::memory_order::relaxed) ) {
        node[slot].store(p, micron::memory_order::release);
        return slot;
      }
    }
  }

  void
  note(const void *held, const void *wanted) noexcept
  {
    if ( held == nullptr or held == wanted ) return;
    const u32 a = intern(held);
    const u32 b = intern(wanted);
    if ( a >= max_graph_nodes or b >= max_graph_nodes ) return;
    adj[a].fetch_or(1u << b, micron::memory_order::acq_rel);
    edges.fetch_add(1, micron::memory_order::relaxed);
  }

  // single-threaded, after the run
  [[nodiscard]] bool
  acyclic() const noexcept
  {
    const u32 n = nodes.get(micron::memory_order::acquire);
    u32 row[max_graph_nodes] = {};
    for ( u32 i = 0; i < n; ++i ) row[i] = adj[i].get(micron::memory_order::acquire);

    // iterative DFS with the usual white/grey/black colouring; a grey hit is a back edge
    u8 colour[max_graph_nodes] = {};
    u32 stack[max_graph_nodes];
    u32 next_edge[max_graph_nodes];

    for ( u32 root = 0; root < n; ++root ) {
      if ( colour[root] != 0 ) continue;
      u32 sp = 0;
      stack[sp] = root;
      next_edge[sp] = 0;
      colour[root] = 1;

      while ( true ) {
        const u32 v = stack[sp];
        u32 e = next_edge[sp];
        bool descended = false;
        for ( ; e < n; ++e ) {
          if ( (row[v] & (1u << e)) == 0u ) continue;
          if ( colour[e] == 1 ) return false;      // back edge: a cycle
          if ( colour[e] == 0 ) {
            next_edge[sp] = e + 1;
            colour[e] = 1;
            ++sp;
            stack[sp] = e;
            next_edge[sp] = 0;
            descended = true;
            break;
          }
        }
        if ( descended ) continue;
        next_edge[sp] = e;
        colour[v] = 2;
        if ( sp == 0 ) break;
        --sp;
      }
    }
    return true;
  }

  void
  reset() noexcept
  {
    for ( usize i = 0; i < max_graph_nodes; ++i ) {
      node[i].store(0, micron::memory_order::release);
      adj[i].store(0, micron::memory_order::release);
    }
    nodes.store(0, micron::memory_order::release);
    overflow.store(0, micron::memory_order::release);
    edges.store(0, micron::memory_order::release);
  }
};

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// progress watchdog
//
// a deadlocked test grades FAIL only because duck's --timeout kills it (124), which says nothing
// about WHERE. workers bump progress; a watcher samples it and, if it stops moving, prints what
// each thread was holding and wanting and then fails through snowball -- which aborts the whole
// group. that path matters: snowball.hpp:47-55 documents why a failing test must never sys_exit,
// since that reaps the caller only and leaves the parked workers to hang the grader anyway.

struct thread_state {
  micron::atomic_token<const void *> holding{ nullptr };
  micron::atomic_token<const void *> wanting{ nullptr };
  micron::atomic_token<u64> ops{ 0 };
};

struct watchdog {
  micron::atomic_token<u64> progress{ 0 };
  micron::atomic_token<u32> stop{ 0 };
  micron::atomic_token<u32> stalled{ 0 };
  thread_state per_thread[max_probe_threads];

  void
  bump() noexcept
  {
    progress.fetch_add(1, micron::memory_order::relaxed);
  }

  void
  note_want(u32 tid, const void *l) noexcept
  {
    if ( tid < max_probe_threads ) per_thread[tid].wanting.store(l, micron::memory_order::release);
  }

  void
  note_hold(u32 tid, const void *l) noexcept
  {
    if ( tid < max_probe_threads ) {
      per_thread[tid].holding.store(l, micron::memory_order::release);
      per_thread[tid].wanting.store(nullptr, micron::memory_order::release);
      per_thread[tid].ops.fetch_add(1, micron::memory_order::relaxed);
    }
  }

  void
  note_release(u32 tid) noexcept
  {
    if ( tid < max_probe_threads ) per_thread[tid].holding.store(nullptr, micron::memory_order::release);
  }

  // runs on its own thread; returns when stop is set or a stall is detected
  void
  watch(u64 stall_ms, u32 threads) noexcept
  {
    u64 last = progress.get(micron::memory_order::acquire);
    u64 quiet_ms = 0;
    while ( stop.get(micron::memory_order::acquire) == 0 ) {
      micron::sleep_for(25);
      const u64 now = progress.get(micron::memory_order::acquire);
      if ( now != last ) {
        last = now;
        quiet_ms = 0;
        continue;
      }
      quiet_ms += 25;
      if ( quiet_ms < stall_ms ) continue;

      stalled.store(1, micron::memory_order::release);
      snowball::print("     WATCHDOG: no progress for ", static_cast<usize>(quiet_ms), " ms after ", static_cast<usize>(now), " ops");
      for ( u32 i = 0; i < threads and i < max_probe_threads; ++i ) {
        snowball::print("       t", static_cast<usize>(i), " ops=", static_cast<usize>(per_thread[i].ops.get(micron::memory_order::acquire)),
                        " holding=", reinterpret_cast<usize>(per_thread[i].holding.get(micron::memory_order::acquire)),
                        " wanting=", reinterpret_cast<usize>(per_thread[i].wanting.get(micron::memory_order::acquire)));
      }
      // and take the process down here, from this thread. returning would only let the harness
      // block forever in the join -- the stalled workers are never coming back -- and the run would
      // grade 124 (timed out) with no indication of where. snowball's require() failure path is
      // abort(6), which flushes the io buffers so the dump above survives and then kills the whole
      // group; a sys_exit here would reap only the watchdog (snowball.hpp:47-55).
      snowball::require(false);
      return;
    }
  }

  void
  disarm() noexcept
  {
    stop.store(1, micron::memory_order::release);
  }

  [[nodiscard]] bool
  ok() const noexcept
  {
    return stalled.get(micron::memory_order::acquire) == 0;
  }

  void
  reset() noexcept
  {
    progress.store(0, micron::memory_order::release);
    stop.store(0, micron::memory_order::release);
    stalled.store(0, micron::memory_order::release);
    for ( u32 i = 0; i < max_probe_threads; ++i ) {
      per_thread[i].holding.store(nullptr, micron::memory_order::release);
      per_thread[i].wanting.store(nullptr, micron::memory_order::release);
      per_thread[i].ops.store(0, micron::memory_order::release);
    }
  }
};

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// misc

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// simultaneous start
//
// USE THIS IN EVERY CONTENTION CASE. mtest::parallel spawns serially and a clone3 costs tens of
// microseconds, so a short loop can run to completion in the time it takes to spawn the next
// thread -- a "contended" case then never contends at all. measured: 8 threads x 4000 contended
// atomic_ptr increments recorded ZERO CAS failures without a gate, which was enough to make a
// livelocking implementation of operator++ grade PASS.
struct start_gate {
  ltest::barrier_t bar;

  explicit start_gate(u32 threads) noexcept { bar.n = threads; }

  // each thread keeps its own sense word (the barrier is sense-reversing, so it is reusable)
  void
  wait(u32 &sense) noexcept
  {
    ltest::barrier_wait(bar, sense);
  }
};

// xorshift64; seeds are fixed hex literals at the call site, never time-derived
[[gnu::always_inline]] inline u64
xs64(u64 &s) noexcept
{
  s ^= s << 13;
  s ^= s >> 7;
  s ^= s << 17;
  return s;
}

// arch-scaled thread counts: arm32 under qemu cannot carry the wide cases in a sane wall clock
#if defined(__micron_arch_width_32)
inline constexpr u32 wide_threads = 4;
inline constexpr u32 over_threads = 8;
#else
inline constexpr u32 wide_threads = 8;
inline constexpr u32 over_threads = 24;
#endif

};      // namespace lcheck
