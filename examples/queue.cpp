// queue.cpp
// Tour of micron's queue family (src/queue/).
//
// micron has four queue flavours, picked by who pushes and pops and
// whether mutability is required:
//
//   queue<T, N>          — generic mutable FIFO. Heap-allocated.
//                          push/pop on the same thread.
//
//   spsc_queue<T, N>     — single-producer / single-consumer lock-free
//                          ring buffer. Cache-line padded, atomic head/tail.
//                          Capacity is rounded up to the next power of 2.
//
//   conqueue<T, N>       — concurrent multi-producer / multi-consumer.
//
//   lambda_queue<N>      — queue of erased callables (functors / lambdas).
//                          push(fn) stores it, execute() pops and calls.
//
//   iqueue<T, N>         — immutable variant; push/pop methods are const,
//                          internal mutation is in atomic state. Useful
//                          when you want a queue handle that can be passed
//                          by const reference to a worker.
//
// All of them share roughly the same "push/pop/peek/front" surface; the
// concurrent ones return bool to signal full/empty rather than throw.
//
// STL deltas:
//   - SPSC and concurrent variants are first-party (no std::queue equivalent).
//   - Capacity is fixed at type level, not grown on demand.
//   - lambda_queue's push has no STL analogue beyond a vector<function<void()>>.

#include "../src/io/console.hpp"
#include "../src/queue/conqueue.hpp"
#include "../src/queue/lambda_queue.hpp"
#include "../src/queue/queue.hpp"
#include "../src/queue/spsc_queue.hpp"

int
main()
{
  // ================================================================
  // 1. queue<T, N> — generic FIFO, single-threaded
  // ================================================================
  micron::io::println("-- 1. queue --");

  micron::queue<int, 16> q;
  q.push(10).push(20).push(30);     // chainable
  micron::io::println("size=", q.size(), " last=", q.last(), " front=", q.front());

  // pop returns void; the FIFO ordering is push-from-front, pop-from-back
  q.pop();
  micron::io::println("after pop: size=", q.size(), " last=", q.last());

  // ================================================================
  // 2. spsc_queue<T, N> — lock-free single-producer/single-consumer
  // ----------------------------------------------------------------
  // push/pop return bool. Capacity is rounded up to the next power
  // of 2 (so spsc_queue<int, 10> actually holds 16). head/tail are
  // cache-line padded to avoid false sharing between cores.
  //
  // Demoed single-threaded here for portability — in real code one
  // thread calls push(), another calls pop().
  // ================================================================
  micron::io::println("-- 2. spsc_queue --");

  micron::spsc_queue<int, 16> sp;
  for ( int i = 0; i < 5; ++i ) {
    bool ok = sp.push(static_cast<int &&>(i));
    if ( !ok ) micron::io::println("  push failed (full)");
  }
  micron::io::println("spsc size=", sp.size(), " front=", sp.front());

  int out;
  while ( sp.pop(out) ) micron::io::print(out, " ");
  micron::io::println("");

  // peek (non-destructive read) returns false if empty
  micron::io::println("after drain: peek returns ", sp.peek(out));

  // batch operations: push_batch / pop_batch are also available for
  // lock-free bulk transfer (see src/queue/spsc_queue.hpp)

  // ================================================================
  // 3. conqueue<T, N> — concurrent multi-producer/consumer
  // ----------------------------------------------------------------
  // Same push/pop API as queue<T,N>, with synchronisation built in.
  // Use this when more than one thread will be on each end.
  // ================================================================
  micron::io::println("-- 3. conqueue --");

  micron::conqueue<int> cq;
  cq.push(1).push(2).push(3);
  // size()/front() lock internally; call them via a non-const handle.
  micron::conqueue<int> &cqref = cq;
  micron::io::println("conqueue front=", cqref.front());
  // pop / push API mirrors queue<T,N> exactly — only the synchronisation
  // differs. Run from multiple threads in real code.

  // ================================================================
  // 4. lambda_queue<N> — queue of erased callables
  // ----------------------------------------------------------------
  // Push any callable; execute() pops and invokes one. Perfect for
  // "deferred work" — push from anywhere, drain on a worker thread.
  // ================================================================
  micron::io::println("-- 4. lambda_queue --");

  micron::lambda_queue<8> lq;
  lq.push([]() { micron::io::println("  task A ran"); });
  lq.push([]() { micron::io::println("  task B ran"); });
  lq.push([]() { micron::io::println("  task C ran"); });

  micron::io::println("queued size=", lq.size());
  while ( !lq.empty() ) lq.execute();
  micron::io::println("after drain: empty=", lq.empty());

  return 0;
}
