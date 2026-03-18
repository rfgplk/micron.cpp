// spsc_queue_tests.cpp
// Rigorous snowball test suite for micron::spsc_queue<T, N>

#include "../../src/queue/spsc_queue.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"
#include "../../src/io/console.hpp"

#include <atomic>
#include <thread>
#include <vector>

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_nothrow;
using sb::require_throw;
using sb::require_true;
using sb::test_case;

namespace
{

// ------------------------------------------------------------------ //
//  Lifetime-tracking helper                                           //
// ------------------------------------------------------------------ //
struct Tracked {
  static inline size_t ctor = 0;
  static inline size_t dtor = 0;
  int v;

  Tracked() : v(0) { ++ctor; }

  explicit Tracked(int x) : v(x) { ++ctor; }

  Tracked(const Tracked &o) : v(o.v) { ++ctor; }

  Tracked(Tracked &&o) noexcept : v(o.v)
  {
    o.v = -1;
    ++ctor;
  }

  Tracked &
  operator=(const Tracked &o)
  {
    v = o.v;
    return *this;
  }

  Tracked &
  operator=(Tracked &&o) noexcept
  {
    v = o.v;
    o.v = -1;
    return *this;
  }

  ~Tracked() { ++dtor; }

  bool
  operator==(const Tracked &o) const
  {
    return v == o.v;
  }
};

void
reset_tracked()
{
  Tracked::ctor = 0;
  Tracked::dtor = 0;
}

// ------------------------------------------------------------------ //
//  Non-trivial move-only type                                         //
// ------------------------------------------------------------------ //
struct MoveOnly {
  int value;

  explicit MoveOnly(int v) : value(v) {}

  MoveOnly(const MoveOnly &) = delete;

  MoveOnly(MoveOnly &&o) noexcept : value(o.value) { o.value = -1; }

  MoveOnly &
  operator=(MoveOnly &&o) noexcept
  {
    value = o.value;
    o.value = -1;
    return *this;
  }

  MoveOnly &operator=(const MoveOnly &) = delete;
};

// ------------------------------------------------------------------ //
//  Large struct to stress alignment / padding logic                   //
// ------------------------------------------------------------------ //
struct BigPod {
  int a, b, c, d;
  char buf[64];

  bool
  operator==(const BigPod &o) const
  {
    return a == o.a && b == o.b && c == o.c && d == o.d;
  }
};

}     // anonymous namespace

// ------------------------------------------------------------------ //
//  main                                                               //
// ------------------------------------------------------------------ //
int
main()
{
  sb::print("=== SPSC_QUEUE TESTS ===");

  // ---------------------------------------------------------------- //
  test_case("default construction – empty and zero size");
  {
    micron::spsc_queue<int, 16> q;
    require_true(q.empty());
    require(q.size(), size_t(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("capacity is rounded up to next power of two");
  {
    // N=10 → capacity must be 16
    micron::spsc_queue<int, 10> q10;
    require(q10.capacity(), size_t(16));

    // N=16 stays 16
    micron::spsc_queue<int, 16> q16;
    require(q16.capacity(), size_t(16));

    // N=1 → capacity must be 1 (edge case: 2^0)
    micron::spsc_queue<int, 1> q1;
    require(q1.capacity(), size_t(1));

    // N=100 → capacity must be 128
    micron::spsc_queue<int, 100> q100;
    require(q100.capacity(), size_t(128));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("max_size equals capacity");
  {
    micron::spsc_queue<int, 32> q;
    require(q.max_size(), q.capacity());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("single push then pop round-trip – int");
  {
    micron::spsc_queue<int, 8> q;
    require_true(q.push(42));
    require_false(q.empty());
    require(q.size(), size_t(1));

    int out = 0;
    require_true(q.pop(out));
    require(out, 42);
    require_true(q.empty());
    require(q.size(), size_t(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("push rvalue and pop – move semantics preserved");
  {
    micron::spsc_queue<Tracked, 8> q;
    reset_tracked();

    Tracked t(99);
    q.push(micron::move(t));

    Tracked out;
    q.pop(out);
    require(out.v, 99);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("push const-ref does not steal from source");
  {
    micron::spsc_queue<int, 8> q;
    const int src = 77;
    q.push(src);
    int out = 0;
    q.pop(out);
    require(out, 77);
    require(src, 77);     // source must be untouched
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("fill queue to capacity – push returns false when full");
  {
    constexpr size_t CAP = 8;
    micron::spsc_queue<int, CAP> q;

    for ( size_t i = 0; i < CAP; ++i )
        require_true(q.push((int)i));
    // one more must fail
    require_false(q.push(999));
    require(q.size(), CAP);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pop on empty queue returns false");
  {
    micron::spsc_queue<int, 8> q;
    int out = 0;
    require_false(q.pop(out));
    require(out, 0);     // output must not be mutated
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pop(void) discards element without output param");
  {
    micron::spsc_queue<int, 8> q;
    q.push(55);
    require_true(q.pop());
    require_true(q.empty());
    require_false(q.pop());     // already empty
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("push(void) default-constructs element in place");
  {
    micron::spsc_queue<int, 8> q;
    require_true(q.push());
    require(q.size(), size_t(1));
    int out = 99;
    q.pop(out);
    require(out, 0);     // default int is 0
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("emplace constructs in place");
  {
    micron::spsc_queue<Tracked, 8> q;
    reset_tracked();

    require_true(q.emplace(42));
    Tracked out;
    q.pop(out);
    require(out.v, 42);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("peek returns element without consuming it");
  {
    micron::spsc_queue<int, 8> q;
    q.push(10);
    q.push(20);

    int peeked = 0;
    require_true(q.peek(peeked));
    require(peeked, 10);
    require(q.size(), size_t(2));     // still two elements

    int out = 0;
    q.pop(out);
    require(out, 10);
    q.pop(out);
    require(out, 20);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("peek on empty queue returns false");
  {
    micron::spsc_queue<int, 8> q;
    int out = 99;
    require_false(q.peek(out));
    require(out, 99);     // must not be touched
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("front() references head element");
  {
    micron::spsc_queue<int, 8> q;
    q.push(7);
    q.push(8);
    require(q.front(), 7);
    q.front() = 100;
    int out = 0;
    q.pop(out);
    require(out, 100);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("last() references tail element");
  {
    micron::spsc_queue<int, 8> q;
    q.push(1);
    q.push(2);
    q.push(3);
    require(q.last(), 3);
    q.last() = 99;
    // drain
    int out = 0;
    q.pop(out);
    require(out, 1);
    q.pop(out);
    require(out, 2);
    q.pop(out);
    require(out, 99);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("FIFO ordering preserved over full cycle");
  {
    constexpr int N = 64;
    micron::spsc_queue<int, 64> q;
    for ( int i = 0; i < N; ++i )
      q.push(i);
    for ( int i = 0; i < N; ++i ) {
      int out = -1;
      q.pop(out);
      require(out, i);
    }
    require_true(q.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("wrap-around: push/pop across index wrap boundary");
  {
    // capacity = 8; fill 4, drain 4, fill 8 – forces wrap
    micron::spsc_queue<int, 8> q;
    for ( int i = 0; i < 4; ++i )
      q.push(i);
    for ( int i = 0; i < 4; ++i ) {
      int o;
      q.pop(o);
    }
    // now tail and head have advanced; next push wraps the ring
    for ( int i = 10; i < 18; ++i )
      q.push(i);
    for ( int i = 10; i < 18; ++i ) {
      int o = -1;
      q.pop(o);
      require(o, i);
    }
    require_true(q.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("clear resets queue to empty state");
  {
    micron::spsc_queue<int, 16> q;
    for ( int i = 0; i < 10; ++i )
      q.push(i);
    q.clear();
    require_true(q.empty());
    require(q.size(), size_t(0));

    // should be reusable after clear
    q.push(42);
    require(q.size(), size_t(1));
    int out = 0;
    q.pop(out);
    require(out, 42);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("clear destroys tracked objects");
  {
    reset_tracked();
    {
      micron::spsc_queue<Tracked, 16> q;
      for ( int i = 0; i < 8; ++i )
        q.emplace(i);
      size_t before_clear_ctor = Tracked::ctor;
      q.clear();
      // every constructed element must have been destroyed
      require(Tracked::dtor, before_clear_ctor);
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("destructor destroys all remaining elements");
  {
    reset_tracked();
    {
      micron::spsc_queue<Tracked, 16> q;
      for ( int i = 0; i < 10; ++i )
        q.emplace(i);
      // 3 consumed
      Tracked tmp;
      q.pop(tmp);
      q.pop(tmp);
      q.pop(tmp);
    }     // destructor fires here
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("push_batch pushes up to available space");
  {
    micron::spsc_queue<int, 8> q;
    int items[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    size_t pushed = q.push_batch(items, 6);
    require(pushed, size_t(6));
    require(q.size(), size_t(6));

    // overfill: only 2 slots left
    size_t pushed2 = q.push_batch(items, 8);
    require(pushed2, size_t(2));
    require(q.size(), size_t(8));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("push_batch on empty queue returns 0");
  {
    micron::spsc_queue<int, 8> q;
    // fill first
    for ( int i = 0; i < 8; ++i )
      q.push(i);

    int items[4] = { 9, 10, 11, 12 };
    size_t pushed = q.push_batch(items, 4);
    require(pushed, size_t(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pop_batch pops up to available elements");
  {
    micron::spsc_queue<int, 16> q;
    for ( int i = 0; i < 10; ++i )
      q.push(i);

    int out[16] = {};
    size_t popped = q.pop_batch(out, 6);
    require(popped, size_t(6));
    for ( int i = 0; i < 6; ++i )
      require(out[i], i);

    require(q.size(), size_t(4));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("pop_batch on empty queue returns 0");
  {
    micron::spsc_queue<int, 8> q;
    int out[8] = {};
    size_t popped = q.pop_batch(out, 4);
    require(popped, size_t(0));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("push_batch then pop_batch preserves element order");
  {
    micron::spsc_queue<int, 32> q;
    int src[16];
    for ( int i = 0; i < 16; ++i )
      src[i] = i * 2;

    q.push_batch(src, 16);

    int dst[16] = {};
    q.pop_batch(dst, 16);

    for ( int i = 0; i < 16; ++i )
      require(dst[i], src[i]);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("initializer_list constructor populates in order");
  {
    micron::spsc_queue<int, 8> q{ 10, 20, 30 };
    require(q.size(), size_t(3));

    int out = 0;
    q.pop(out);
    require(out, 10);
    q.pop(out);
    require(out, 20);
    q.pop(out);
    require(out, 30);
    require_true(q.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("size tracks push/pop correctly");
  {
    micron::spsc_queue<int, 32> q;
    for ( int i = 1; i <= 20; ++i ) {
      q.push(i);
      require(q.size(), size_t(i));
    }
    for ( int i = 19; i >= 0; --i ) {
      int o;
      q.pop(o);
      require(q.size(), size_t(i));
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("repeated fill-drain cycles – no capacity leak");
  {
    micron::spsc_queue<int, 16> q;
    for ( int cycle = 0; cycle < 200; ++cycle ) {
      for ( int i = 0; i < 16; ++i )
        require_true(q.push(i));
      for ( int i = 0; i < 16; ++i ) {
        int o;
        require_true(q.pop(o));
        require(o, i);
      }
      require_true(q.empty());
    }
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("BigPod round-trip preserves all fields");
  {
    micron::spsc_queue<BigPod, 4> q;
    BigPod src{ 1, 2, 3, 4, {} };
    src.buf[0] = 'X';
    src.buf[63] = 'Z';
    q.push(src);

    BigPod dst{};
    q.pop(dst);
    require(dst.a, src.a);
    require(dst.b, src.b);
    require(dst.c, src.c);
    require(dst.d, src.d);
    require((int)dst.buf[0], (int)'X');
    require((int)dst.buf[63], (int)'Z');
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("move-only type push/pop via rvalue");
  {
    micron::spsc_queue<MoveOnly, 8> q;
    require_true(q.push(MoveOnly(42)));

    MoveOnly out(0);
    require_true(q.pop(out));
    require(out.value, 42);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("tracked: emplace does not copy – only constructs in place");
  {
    reset_tracked();
    micron::spsc_queue<Tracked, 8> q;
    q.emplace(7);

    // One construction (in-place), zero copies
    require(Tracked::ctor, size_t(1));
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("const front() and last() are accessible");
  {
    micron::spsc_queue<int, 8> q;
    q.push(1);
    q.push(2);
    q.push(3);

    const auto &cq = q;
    require(cq.front(), 1);
    require(cq.last(), 3);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: interleaved single-element push/pop");
  {
    micron::spsc_queue<int, 4> q;
    for ( int i = 0; i < 10000; ++i ) {
      require_true(q.push(i));
      int o = -1;
      require_true(q.pop(o));
      require(o, i);
    }
    require_true(q.empty());
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: producer/consumer thread correctness");
  {
    constexpr int TOTAL = 1 << 16;     // 65536 items
    micron::spsc_queue<int, 1024> q;
    std::atomic<bool> done{ false };

    std::thread producer([&]() {
      for ( int i = 0; i < TOTAL; ++i ) {
        while ( !q.push(i) )
          ;     // spin until slot available
      }
    });

    std::vector<int> received;
    received.reserve(TOTAL);

    std::thread consumer([&]() {
      int out;
      while ( (int)received.size() < TOTAL ) {
        if ( q.pop(out) )
          received.push_back(out);
      }
    });

    producer.join();
    consumer.join();

    require(received.size(), size_t(TOTAL));
    for ( int i = 0; i < TOTAL; ++i )
      require(received[i], i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("stress: batch producer/consumer thread correctness");
  {
    constexpr int TOTAL = 1 << 14;     // 16384 items
    constexpr int BATCH = 32;
    micron::spsc_queue<int, 256> q;

    std::thread producer([&]() {
      int src[BATCH];
      int sent = 0;
      while ( sent < TOTAL ) {
        int chunk = (TOTAL - sent < BATCH) ? (TOTAL - sent) : BATCH;
        for ( int i = 0; i < chunk; ++i )
          src[i] = sent + i;
        size_t pushed = 0;
        while ( (int)pushed < chunk )
          pushed += q.push_batch(src + pushed, chunk - pushed);
        sent += chunk;
      }
    });

    std::vector<int> received;
    received.reserve(TOTAL);

    std::thread consumer([&]() {
      int dst[BATCH];
      while ( (int)received.size() < TOTAL ) {
        size_t got = q.pop_batch(dst, BATCH);
        for ( size_t i = 0; i < got; ++i )
          received.push_back(dst[i]);
      }
    });

    producer.join();
    consumer.join();

    require(received.size(), size_t(TOTAL));
    for ( int i = 0; i < TOTAL; ++i )
      require(received[i], i);
  }
  end_test_case();

  // ---------------------------------------------------------------- //
  test_case("reuse after clear in threaded context");
  {
    micron::spsc_queue<int, 64> q;
    for ( int i = 0; i < 32; ++i )
      q.push(i);
    q.clear();
    require_true(q.empty());

    // Confirm fully operational after clear in single-thread
    for ( int i = 0; i < 64; ++i )
      require_true(q.push(i));
    require_false(q.push(999));

    for ( int i = 0; i < 64; ++i ) {
      int o = -1;
      q.pop(o);
      require(o, i);
    }
    require_true(q.empty());
  }
  end_test_case();

  sb::print("=== ALL SPSC_QUEUE TESTS PASSED ===");
  return 0;
}
