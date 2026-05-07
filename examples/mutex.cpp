// mutex.cpp
// Tour of micron's synchronisation primitives (src/mutex/).
//
// micron's mutex API has a small but unusual twist: the call operator
// `m()` (or `m.lock()`) does not return void — it returns a pointer to
// an "unlock handle", a member-function pointer that releases the lock
// when invoked. The RAII wrappers (lock_guard, unique_lock) hold that
// handle and call it on destruction. Most user code goes through the
// guards, but the underlying primitive is exposed if you want it.
//
// What's available:
//
//   mutex                — spin-locked, strong (acq_rel) memory order.
//   weak_mutex           — same shape, weaker memory order, faster on
//                          uncontended paths.
//   queuing_mutex        — MCS queue lock; the user supplies an mcs_node.
//   lock_guard<M>        — RAII: locks on construction, unlocks in dtor.
//   unique_lock<S, M>    — like lock_guard but with deferred / adopt /
//                          unlocked / locked start modes (parameter S).
//   do_once<F>           — call a free function exactly once across
//                          program lifetime; one specialisation per F,
//                          so each F gets its own static atomic_token.
//   full_barrier()       — compiler memory barrier macro.
//
// STL deltas:
//   - mutex.lock() returns the unlock handle, not void.
//   - unique_lock takes a "starts locked / unlocked / adopt / defer"
//     enum as a TEMPLATE param, not a runtime tag — so the lock policy
//     is part of the type.
//   - do_once<F> generates one specialisation per F at compile time;
//     std::call_once tracks state in a separate flag.

#include "../src/io/console.hpp"
#include "../src/mutex/locks.hpp"
#include "../src/mutex/mutex.hpp"
#include "../src/mutex/once.hpp"

// A free function for do_once<>. do_once forwards args to F, and its
// nullary ctor is deleted, so F must take at least one parameter.
static void
init_table(int seed)
{
  micron::io::println("  init_table(seed=", seed, ") ran");
}

int
main()
{
  // ================================================================
  // 1. mutex: manual lock / unlock
  // ================================================================
  micron::io::println("-- 1. mutex --");

  micron::mutex m;
  micron::io::println("is_locked before = ", m.is_locked());
  m.lock();
  micron::io::println("is_locked after lock = ", m.is_locked());
  m.unlock();
  micron::io::println("is_locked after unlock = ", m.is_locked());

  // ================================================================
  // 2. mutex: try_lock returns bool, no spin
  // ================================================================
  micron::io::println("-- 2. try_lock --");

  micron::mutex m2;
  bool first = m2.try_lock();
  bool second = m2.try_lock();     // already held → false
  micron::io::println("first try=", first, " second try=", second);
  m2.unlock();

  // ================================================================
  // 3. lock_guard — RAII wrapper
  // ----------------------------------------------------------------
  // Locks on construction, unlocks in destructor. Most code wants this.
  // ================================================================
  micron::io::println("-- 3. lock_guard --");

  micron::mutex m3;
  {
    micron::lock_guard<micron::mutex> g(m3);
    micron::io::println("inside scope, is_locked=", m3.is_locked());
    // ... protected section ...
  }     // g destroyed here -> m3 released
  micron::io::println("after scope, is_locked=", m3.is_locked());

  // ================================================================
  // 4. unique_lock<lock_starts, M> — flexible RAII
  // ----------------------------------------------------------------
  // The first template parameter chooses the construction-time
  // behaviour. lock_starts::locked means "lock now". Other options:
  // unlocked (don't lock yet), adopt (we already hold it), defer.
  // ================================================================
  micron::io::println("-- 4. unique_lock --");

  micron::mutex m4;
  {
    micron::unique_lock<micron::lock_starts::locked, micron::mutex> u(m4);
    micron::io::println("u locked, m4.is_locked=", m4.is_locked());
  }
  micron::io::println("after u scope, m4.is_locked=", m4.is_locked());

  // ================================================================
  // 5. weak_mutex — fast path with weaker memory order
  // ----------------------------------------------------------------
  // Use for hot uncontended sections where you don't need acq_rel.
  // ================================================================
  micron::io::println("-- 5. weak_mutex --");

  micron::weak_mutex wm;
  wm.lock();
  micron::io::println("weak.is_locked=", wm.is_locked());
  wm.unlock();

  // ================================================================
  // 6. queuing_mutex — MCS lock
  // ----------------------------------------------------------------
  // Each waiter brings its own node. Fairness is FIFO; this is the
  // mutex of choice when you need to scale to many cores.
  // ================================================================
  micron::io::println("-- 6. queuing_mutex --");

  micron::queuing_mutex qm;
  micron::mcs_node node;
  qm.lock(node);
  micron::io::println("qm.is_locked=", qm.is_locked());
  qm.unlock(node);

  // ================================================================
  // 7. do_once<F> — one-shot initialisation
  // ----------------------------------------------------------------
  // Constructing do_once<&fn>{args...} executes fn exactly once across
  // the whole program. The static atomic flag lives on the
  // specialisation, so do_once<&a> and do_once<&b> are independent.
  // (The nullary ctor is deleted; F must take at least one argument.)
  // ================================================================
  micron::io::println("-- 7. do_once --");

  micron::do_once<&init_table>{42};
  micron::do_once<&init_table>{99};     // no-op: F already ran
  micron::do_once<&init_table>{0};      // still no-op

  // ================================================================
  // 8. Memory barriers
  // ----------------------------------------------------------------
  // full_barrier() is a compiler barrier — emits no code, just stops
  // reordering across this point. Pair with atomics for the runtime
  // memory order you actually need.
  // ================================================================
  full_barrier();
  micron::io::println("-- 8. full_barrier emitted --");

  return 0;
}
