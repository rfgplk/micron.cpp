//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../../src/mutex/locks.hpp"
#include "../../src/mutex/backoff.hpp"
#include "../../src/mutex/mutex.hpp"

#include "../../src/mutex/locks/clh_lock.hpp"
#include "../../src/mutex/locks/futex_mutex.hpp"
#include "../../src/mutex/locks/mcs_lock.hpp"
#include "../../src/mutex/locks/seqlock.hpp"
#include "../../src/mutex/locks/shared_mutex.hpp"
#include "../../src/mutex/locks/ticket_lock.hpp"
#include "../../src/mutex/locks/ttas_lock.hpp"

#include "../../src/atomic/atomic.hpp"
#include "../../src/concepts.hpp"
#include "../../src/memory/new.hpp"
#include "../../src/sync/inlet.hpp"

namespace
{

static_assert(micron::is_mutex<micron::mutex>);
static_assert(micron::is_mutex<micron::weak_mutex>);
static_assert(micron::is_mutex<micron::fast_mutex>);
static_assert(micron::is_mutex<micron::null_lock>);
static_assert(micron::is_mutex<micron::spin_lock>);
static_assert(micron::is_mutex<micron::recursive_lock>);
static_assert(micron::is_mutex<micron::ttas_lock>);
static_assert(micron::is_mutex<micron::ttas_spin_lock>);
static_assert(micron::is_mutex<micron::ticket_lock>);
static_assert(micron::is_mutex<micron::ticket_spin_lock>);
static_assert(micron::is_mutex<micron::mcs_lock>);
static_assert(micron::is_mutex<micron::clh_lock>);
static_assert(micron::is_mutex<micron::futex_mutex>);
static_assert(micron::is_mutex<micron::shared_mutex>);

// the reset-PMF shape the guards dispatch through. null_lock lacked it and so could not be used as
// the lock-policy parameter it exists to be; is_mutex does not catch that, since it only requires
// lock() to be callable
template<typename M>
concept ct_has_reset_pmf = requires(M m) {
  { m.retrieve() };
  { m() };
};
static_assert(ct_has_reset_pmf<micron::null_lock>);
static_assert(ct_has_reset_pmf<micron::mutex>);
static_assert(ct_has_reset_pmf<micron::weak_mutex>);
static_assert(ct_has_reset_pmf<micron::spin_lock>);
static_assert(ct_has_reset_pmf<micron::ttas_lock>);

static_assert(sizeof(micron::ticket_lock) == 2 * micron::cache_line_size(), "next and serving must not share a line");
static_assert(alignof(micron::clh_node) >= micron::cache_line_size(), "clh arena nodes must not share a line");

static_assert(sizeof(micron::clh_lock) >= 33 * sizeof(micron::clh_node), "clh_lock must carry its own 32 + 1 node arena");
static_assert(sizeof(micron::basic_clh_lock<2>) >= 3 * sizeof(micron::clh_node));
static_assert(alignof(micron::clh_lock) >= micron::cache_line_size(), "the queue tail must not share a line");

static_assert(!micron::is_copy_constructible_v<micron::ttas_lock>);
static_assert(!micron::is_move_constructible_v<micron::ttas_lock>);
static_assert(!micron::is_copy_constructible_v<micron::ticket_lock>);
static_assert(!micron::is_move_constructible_v<micron::ticket_lock>);
static_assert(!micron::is_copy_constructible_v<micron::mcs_lock>);
static_assert(!micron::is_move_constructible_v<micron::mcs_lock>);
static_assert(!micron::is_copy_constructible_v<micron::clh_lock>);
static_assert(!micron::is_move_constructible_v<micron::clh_lock>);
static_assert(!micron::is_copy_constructible_v<micron::futex_mutex>);
static_assert(!micron::is_move_constructible_v<micron::futex_mutex>);
static_assert(!micron::is_copy_constructible_v<micron::shared_mutex>);
static_assert(!micron::is_move_constructible_v<micron::shared_mutex>);
static_assert(!micron::is_copy_assignable_v<micron::mcs_lock>);
static_assert(!micron::is_move_assignable_v<micron::mcs_lock>);
static_assert(!micron::is_copy_assignable_v<micron::clh_lock>);
static_assert(!micron::is_move_assignable_v<micron::clh_lock>);

static_assert(!micron::is_trivially_destructible_v<micron::mcs_lock>, "mcs_lock must release its slot association at destruction");
static_assert(micron::is_trivially_destructible_v<micron::clh_lock>, "the clh pool is internal state; nothing to unwind");
static_assert(micron::__lock_slot_table<micron::__mcs_slot>::capacity() == MICRON_MCS_DEPTH);
static_assert(noexcept(micron::__lock_slot_table<micron::__mcs_slot>::find(nullptr, 0u)));

template<typename T>
consteval bool
constant_initializable()
{
  T t{};
  return sizeof(t) != 0;
}

static_assert(constant_initializable<micron::__mcs_slot>(), "the slot table's TLS must be constant-initialized");
static_assert(constant_initializable<micron::clh_node>(), "a clh arena is a member, so its nodes must be too");

static_assert(micron::is_same_v<micron::timed_mutex, micron::futex_mutex>);
static_assert(micron::is_same_v<micron::adaptive_mutex, micron::futex_mutex>);
static_assert(micron::is_same_v<micron::queuing_mutex_adapter, micron::mcs_lock>);
static_assert(micron::is_same_v<micron::clh_lock, micron::basic_clh_lock<32, micron::spin_yield>>);
static_assert(micron::is_same_v<micron::ticket_lock, micron::basic_ticket_lock<micron::spin_yield>>);
static_assert(micron::is_same_v<micron::shared_mutex, micron::basic_shared_mutex<micron::spin_yield>>);

struct pod {
  long a;
  long b;
};

micron::ttas_lock g_ttas;
micron::ticket_lock g_ticket;
micron::mcs_lock g_mcs;
micron::clh_lock g_clh;
micron::futex_mutex g_futex;
micron::shared_mutex g_shared;
micron::null_lock g_null;
micron::seqlock<pod> g_seq;                                 // now defaults to fast_mutex, not null_lock
micron::seqlock<pod, micron::ttas_lock> g_seq_locked;
micron::seqlock<pod, micron::null_lock> g_seq_unlocked;      // still available, explicitly single-writer

micron::basic_ttas_lock<micron::spin_only> g_ttas_only;
micron::basic_ttas_lock<micron::spin_yield> g_ttas_yield;
micron::basic_ttas_lock<micron::spin_park> g_ttas_park;
micron::basic_ticket_lock<micron::spin_only> g_ticket_only;
micron::basic_clh_lock<2> g_clh2;
micron::basic_clh_lock<64, micron::spin_only> g_clh64;
micron::basic_shared_mutex<micron::spin_only> g_shared_only;

micron::queuing_inlet<long> g_qinlet;

static_assert(noexcept(g_mcs.try_lock()));
static_assert(noexcept(g_clh.try_lock()));
static_assert(noexcept(g_ticket.try_lock()));
static_assert(noexcept(g_futex.try_lock()));
static_assert(noexcept(g_shared.try_lock()));
static_assert(noexcept(g_shared.try_lock_shared()));
static_assert(noexcept(g_mcs.unlock()));
static_assert(noexcept(g_clh.unlock()));
static_assert(noexcept(g_ticket.unlock()));
static_assert(noexcept(g_futex.unlock()));
static_assert(noexcept(g_shared.unlock()));
static_assert(noexcept(g_shared.unlock_shared()));
static_assert(noexcept(g_mcs.holds()));
static_assert(noexcept(g_clh.participants()));

void
touch_locks(void)
{

  {
    micron::lock_guard<micron::ttas_lock> a(g_ttas);
  }
  {
    micron::auto_guard<micron::ticket_lock> a(g_ticket);
  }
  {
    micron::unique_lock<micron::lock_starts::locked, micron::futex_mutex> a(g_futex);
  }
  {
    micron::shared_lock<micron::shared_mutex> a(g_shared);
  }
  {
    micron::shared_lock<micron::shared_mutex> a(g_shared, micron::defer_lock);
    a.lock();
    a.unlock();
    (void)a.owns_lock();
    (void)a.release();
  }
  {
    micron::lock_set<micron::ttas_lock, micron::ticket_lock, micron::mcs_lock> a(g_ttas, g_ticket, g_mcs);
  }

  // lock_starts::attempt: no constructor existed for this enumerator at all
  {
    micron::unique_lock<micron::lock_starts::attempt, micron::futex_mutex> a(g_futex);
    (void)a.owns_lock();
    (void)static_cast<bool>(a);
  }
  {
    micron::unique_lock<micron::lock_starts::attempt, micron::ttas_lock> a(&g_ttas);
    (void)a.owns_lock();
  }

  // null_lock through every guard form: it satisfies is_mutex, so it must actually instantiate
  {
    micron::lock_guard<micron::null_lock> a(g_null);
  }
  {
    micron::auto_guard<micron::null_lock> a(g_null);
  }
  {
    micron::unique_lock<micron::lock_starts::locked, micron::null_lock> a(g_null);
    (void)a.owns_lock();
  }
  {
    micron::unique_lock<micron::lock_starts::attempt, micron::null_lock> a(g_null);
    (void)a.owns_lock();
  }

  micron::lock_all(g_ttas, g_ticket);
  micron::unlock(g_ttas, g_ticket);
  (void)micron::try_lock_all(g_ttas, g_ticket, g_mcs);
  micron::unlock(g_ttas, g_ticket, g_mcs);

  // the unqualified pair now routes to lock_all/try_lock_all; the raw fold kept its own names
  micron::lock(g_ttas, g_ticket);
  micron::unlock(g_ttas, g_ticket);
  (void)micron::try_lock(g_ttas, g_ticket);
  micron::unlock(g_ttas, g_ticket);
  micron::lock_in_order(g_ttas, g_ticket);
  micron::unlock(g_ttas, g_ticket);
  (void)micron::try_lock_in_order(g_ttas, g_ticket);
  micron::unlock(g_ttas, g_ticket);
  micron::lock_all(g_ttas);
  g_ttas.unlock();
  micron::lock_all();

  {
    auto r = g_ttas.lock();
    (g_ttas.*r)();
  }
  {
    auto r = g_ticket();
    (g_ticket.*r)();
  }
  {
    auto r = g_mcs.retrieve();
    g_mcs.lock();
    (g_mcs.*r)();
  }
  {
    auto r = g_clh.lock();
    (g_clh.*r)();
  }
  {
    auto r = g_futex.lock();
    (g_futex.*r)();
  }
  {
    auto r = g_shared.lock();
    (g_shared.*r)();
  }

  if ( g_mcs.try_lock() ) g_mcs.unlock();
  if ( g_clh.try_lock() ) g_clh.unlock();
  if ( g_ticket.try_lock() ) g_ticket.unlock();
  g_ticket.unlock();
  g_mcs.unlock();
  g_clh.unlock();

  (void)g_futex.try_lock_for(1000);
  micron::timespec_t deadline{ 0, 0 };
  (void)g_futex.try_lock_until(deadline);

  g_shared.lock_shared();
  g_shared.unlock_shared();
  (void)g_shared.try_lock_shared();
  g_shared.unlock_shared();
  (void)g_shared.readers();
  (void)g_shared.writers_queued();
  (void)g_shared.is_writer_held();

  g_seq.store(pod{ 1, 2 });
  (void)g_seq.load();
  pod out{};
  (void)g_seq.try_load(out);
  g_seq.write([](pod &p) { p.a = 3; });
  (void)g_seq.sequence();
  (void)g_seq.writing();
  g_seq_locked.store(pod{ 4, 5 });
  (void)g_seq_locked.load();

  (void)g_ticket.queued();
  (void)g_ticket.serving();
  (void)g_mcs.holds();
  (void)g_mcs.enqueued();
  (void)g_clh.participants();
  (void)g_futex.contended();
  (void)g_ttas.stats().acquires();
  (void)g_ttas.stats().spins();
  (void)g_ttas.stats().yields();
  (void)g_ttas.stats().parks();

  g_qinlet.store(7);
  (void)g_qinlet.load();
  g_qinlet.apply([](long &v) { ++v; });

  micron::__lock_backoff<micron::spin_yield> bo;
  (void)bo.next();
  bo.relax();
  bo.reset();
  (void)bo.rounds();
  (void)bo.width();

  micron::__pause_backoff pb;
  pb.relax();
  pb.reset();
  (void)pb.width();
}

}      // namespace

int
main(void)
{
  touch_locks();
  return 1;
}
