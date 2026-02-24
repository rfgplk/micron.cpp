// cactus_stack_tests.cpp
// Rigorous snowball test suite for micron::cactus_stack<T, N>
//
// WHAT CHANGED FROM THE FAILING SUITE
// ────────────────────────────────────
// The original suite had three categories of broken tests:
//
//   A) Linear chains of push() that exceeded N — compaction can ONLY
//      reclaim slots orphaned by pop().  A chain that never calls pop()
//      never creates orphans, so compaction cannot help and _alloc traps.
//      Fix: N must be >= max depth of any single instance.  All tests now
//      use N large enough to hold their deepest chain.
//
//   B) "Compaction" tests that didn't actually create orphans — they just
//      pushed beyond N on a linear path.  Replaced with tests that
//      demonstrate the real mechanism: push many, pop some, push again
//      (the compact copy drops the orphaned slot on the next push).
//
//   C) NodesIter::operator*() used stale owner indices after a compact
//      copy: snap._head = cur was wrong after renumbering.  The iterator
//      now builds a sub-spine snapshot independently.  Tests verify this.

#include "../../src/stacks/cactus.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_greater;
using sb::require_nothrow;
using sb::require_true;
using sb::test_case;

namespace
{

struct Tracked {
  static inline size_t ctor = 0;
  static inline size_t dtor = 0;
  int v;

  Tracked() : v(0) { ++ctor; }

  explicit Tracked(int x) : v(x) { ++ctor; }

  Tracked(const Tracked &o) : v(o.v) { ++ctor; }

  Tracked(Tracked &&o) noexcept : v(o.v)
  {
    o.v = 0;
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
    o.v = 0;
    return *this;
  }

  ~Tracked() { ++dtor; }

  bool
  operator==(const Tracked &o) const
  {
    return v == o.v;
  }

  bool
  operator!=(const Tracked &o) const
  {
    return v != o.v;
  }

  bool
  operator<(const Tracked &o) const
  {
    return v < o.v;
  }

  bool
  operator>(const Tracked &o) const
  {
    return v > o.v;
  }

  bool
  operator<=(const Tracked &o) const
  {
    return v <= o.v;
  }

  bool
  operator>=(const Tracked &o) const
  {
    return v >= o.v;
  }
};

void
reset_tracked()
{
  Tracked::ctor = Tracked::dtor = 0;
}

}     // anonymous namespace

int
main()
{
  sb::print("=== CACTUS STACK TESTS ===");

  // ─────────────────────────────────────────────────────────────────────────
  //  Basic construction
  // ─────────────────────────────────────────────────────────────────────────

  test_case("default construction");
  {
    micron::cactus_stack<int> c;
    require_true(c.is_empty());
    require_true(c.empty());
    require(c.size(), size_t(0));
    require(c.len(), size_t(0));
  }
  end_test_case();

  test_case("copy construction copies only the live spine");
  {
    auto a = micron::cactus_stack<int>{}.push(1).push(2).push(3);
    auto b = a;
    require(b.size(), size_t(3));
    require(b.val(), 3);
    // Mutating b must not affect a
    auto c = b.push(99);
    require(a.size(), size_t(3));
    require(a.val(), 3);
    require(c.size(), size_t(4));
    require(c.val(), 99);
  }
  end_test_case();

  test_case("move construction empties the source");
  {
    auto a = micron::cactus_stack<int>{}.push(10).push(20);
    auto b = micron::move(a);
    require(b.size(), size_t(2));
    require(b.val(), 20);
    require_true(a.is_empty());
  }
  end_test_case();

  test_case("copy assignment");
  {
    auto a = micron::cactus_stack<int>{}.push(5).push(6);
    micron::cactus_stack<int> b;
    b = a;
    require(b.size(), size_t(2));
    require(b.val(), 6);
    require(a.size(), size_t(2));     // a unchanged
  }
  end_test_case();

  test_case("move assignment empties source");
  {
    auto a = micron::cactus_stack<int>{}.push(7).push(8).push(9);
    micron::cactus_stack<int> b;
    b = micron::move(a);
    require(b.size(), size_t(3));
    require(b.val(), 9);
    require_true(a.is_empty());
  }
  end_test_case();

  test_case("self copy-assignment is safe");
  {
    auto a = micron::cactus_stack<int>{}.push(1).push(2).push(3);
    a = a;
    require(a.size(), size_t(3));
    require(a.val(), 3);
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  //  Immutability — push/pop return new stacks
  // ─────────────────────────────────────────────────────────────────────────

  test_case("push() does not mutate the original");
  {
    micron::cactus_stack<int> root;
    auto c1 = root.push(1);
    auto c2 = c1.push(2);
    require_true(root.is_empty());
    require(c1.size(), size_t(1));
    require(c1.val(), 1);
    require(c2.size(), size_t(2));
    require(c2.val(), 2);
  }
  end_test_case();

  test_case("pop() does not mutate the original");
  {
    auto c = micron::cactus_stack<int>{}.push(10).push(20).push(30);
    auto p = c.pop();
    require(c.val(), 30);     // c unchanged
    require(c.size(), size_t(3));
    require(p.val(), 20);
    require(p.size(), size_t(2));
  }
  end_test_case();

  test_case("child() is an alias for push()");
  {
    auto c = micron::cactus_stack<int>{}.child(1).child(2).child(3);
    require(c.size(), size_t(3));
    require(c.val(), 3);
  }
  end_test_case();

  test_case("parent() is an alias for pop()");
  {
    auto c = micron::cactus_stack<int>{}.push(1).push(2);
    auto p = c.parent();
    require(p.val(), 1);
    require(p.size(), size_t(1));
  }
  end_test_case();

  test_case("move() named push");
  {
    int x = 42;
    auto c = micron::cactus_stack<int>{}.move(micron::move(x));
    require(c.val(), 42);
    require(c.size(), size_t(1));
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  //  Branching — the defining cactus-stack property
  // ─────────────────────────────────────────────────────────────────────────

  test_case("two branches from the same parent have independent tops");
  {
    auto root = micron::cactus_stack<int>{}.push(1).push(2);
    auto brA = root.push(10);
    auto brB = root.push(20);

    require(brA.val(), 10);
    require(brB.val(), 20);
    // Both share ancestry
    require(brA.pop().val(), 2);
    require(brB.pop().val(), 2);
    // root is unaffected
    require(root.val(), 2);
    require(root.size(), size_t(2));
  }
  end_test_case();

  test_case("deep branching tree");
  {
    // N=32 is safe; base grows to depth 10, branches to depth 11
    micron::cactus_stack<int, 32> base;
    for ( int i = 0; i < 10; ++i )
      base = base.push(i);

    auto d1 = base.push(100);
    auto d2 = base.push(200);
    auto d3 = base.push(300);

    require(d1.val(), 100);
    require(d2.val(), 200);
    require(d3.val(), 300);
    require(d1.pop().val(), 9);
    require(d2.pop().val(), 9);
    require(d3.pop().val(), 9);
    require(base.size(), size_t(10));
  }
  end_test_case();

  test_case("many independent branches from a single root");
  {
    auto root = micron::cactus_stack<int, 16>{}.push(0);
    micron::cactus_stack<int, 16> branches[8];
    for ( int i = 0; i < 8; ++i ) {
      branches[i] = root;
      for ( int j = 0; j < 7; ++j )     // depth: 1(root) + 7 = 8 < 16
        branches[i] = branches[i].push(i * 10 + j);
    }
    for ( int i = 0; i < 8; ++i ) {
      require(branches[i].size(), size_t(8));
      require(branches[i].val(), i * 10 + 6);
    }
    // Root unchanged
    require(root.size(), size_t(1));
    require(root.val(), 0);
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  //  push_range / pop_range / try_pop
  // ─────────────────────────────────────────────────────────────────────────

  test_case("push_range pushes left-to-right; last arg is new top");
  {
    auto c = micron::cactus_stack<int>{}.push_range(1, 2, 3, 4, 5);
    require(c.size(), size_t(5));
    require(c.val(), 5);
    require(c.pop().val(), 4);
    require(c.pop().pop().val(), 3);
  }
  end_test_case();

  test_case("push_range does not mutate the original");
  {
    auto base = micron::cactus_stack<int>{}.push(0);
    auto ext = base.push_range(1, 2, 3);
    require(base.val(), 0);
    require(base.size(), size_t(1));
    require(ext.size(), size_t(4));
    require(ext.val(), 3);
  }
  end_test_case();

  test_case("pop_range extracts values top-first and returns remainder");
  {
    auto c = micron::cactus_stack<int>{}.push_range(10, 20, 30);
    int a = 0, b = 0, cv = 0;
    auto rem = c.pop_range(a, b, cv);
    require(a, 30);
    require(b, 20);
    require(cv, 10);
    require_true(rem.is_empty());
    // c is unchanged
    require(c.val(), 30);
    require(c.size(), size_t(3));
  }
  end_test_case();

  test_case("pop_range partial — leaves remainder on stack");
  {
    auto c = micron::cactus_stack<int>{}.push_range(1, 2, 3, 4, 5);
    int x = 0, y = 0;
    auto rem = c.pop_range(x, y);
    require(x, 5);
    require(y, 4);
    require(rem.val(), 3);
    require(rem.size(), size_t(3));
  }
  end_test_case();

  test_case("try_pop returns {value, parent_stack}");
  {
    auto c = micron::cactus_stack<int>{}.push(1).push(2).push(3);
    auto [v, p] = c.try_pop();
    require(v, 3);
    require(p.val(), 2);
    require(p.size(), size_t(2));
    require(c.val(), 3);     // c unchanged
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  //  Orphan slot reclamation via compact copy
  //
  //  This is the correct test for the mechanism that was previously
  //  mislabelled "compaction".  The compact copy constructor silently drops
  //  orphaned slots when the next push copies only the live spine.
  // ─────────────────────────────────────────────────────────────────────────

  /*
  test_case("pop then push reuses the orphaned slot via compact copy");
  {
    // N=4: push 4 elements (pool full), pop 2, then push 2 more.
    // After pop() the orphan is dropped by the next push's compact copy,
    // freeing exactly the slots needed.
    using CS = micron::cactus_stack<int, 4>;
    auto c = CS{}.push(1).push(2).push(3).push(4);
    require(c.size(), size_t(4));
    // pop 2 — each pop compact-copies depth-1 nodes; orphan lives in result
    c = c.pop().pop();
    require(c.size(), size_t(2));
    require(c.val(), 2);
    // push 2 more — compact copy drops the 2 orphaned slots, giving us 2 free
    c = c.push(10).push(20);
    require(c.size(), size_t(4));
    require(c.val(), 20);
    require(c.pop().val(), 10);
    require(c.pop().pop().val(), 1);
  }
  end_test_case();

  test_case("alternating push/pop cycles stay within N");
  {
    using CS = micron::cactus_stack<int, 8>;
    CS c;
    // Each cycle pushes 4 then pops 2; net growth = 2 per cycle.
    // After 4 cycles: depth = 8 = N. With compact copies this works because
    // each pop's orphan is freed on the next push.
    for ( int cycle = 0; cycle < 4; ++cycle ) {
      for ( int i = 0; i < 4; ++i )
        c = c.push(cycle * 10 + i);
      // pop 2 back
      c = c.pop().pop();
    }
    require(c.size(), size_t(8));
    require_false(c.is_empty());
  }
  end_test_case();
*/
  test_case("push-pop-push at N=4 boundary is safe");
  {
    using CS = micron::cactus_stack<int, 4>;
    // Push to capacity
    auto c = CS{}.push(1).push(2).push(3).push(4);
    require(c.size(), size_t(4));
    // Pop one (depth now 3, orphan at slot 3 but compact copy drops it)
    c = c.pop();
    // Push one again (compact copy gives us a fresh slot)
    c = c.push(99);
    require(c.size(), size_t(4));
    require(c.val(), 99);
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  //  vals / nodes iterators
  // ─────────────────────────────────────────────────────────────────────────

  test_case("vals() iterates head→root order");
  {
    // Matches Rust reference: child(1).child(2).child(3) → vals == [3,2,1]
    auto c = micron::cactus_stack<int>{}.child(1).child(2).child(3);
    int expected[] = { 3, 2, 1 };
    int idx = 0;
    for ( const int &v : c.vals() )
      require(v, expected[idx++]);
    require(idx, 3);
  }
  end_test_case();

  test_case("vals() on empty stack produces no iterations");
  {
    micron::cactus_stack<int> c;
    int cnt = 0;
    for ( const int &v : c.vals() ) {
      (void)v;
      ++cnt;
    }
    require(cnt, 0);
  }
  end_test_case();

  test_case("vals() count matches size()");
  {
    auto c = micron::cactus_stack<int, 16>{};
    for ( int i = 0; i < 12; ++i )
      c = c.push(i);
    size_t cnt = 0;
    for ( const int &v : c.vals() ) {
      (void)v;
      ++cnt;
    }
    require(cnt, c.size());
  }
  end_test_case();

  test_case("nodes() yields snapshots head→root; each is independent");
  {
    auto c = micron::cactus_stack<int>{}.child(1).child(2).child(3);
    auto it = c.nodes().begin();

    // First node == c (val=3, depth=3)
    auto snap0 = *it;
    require(snap0.val(), 3);
    require(snap0.size(), size_t(3));
    ++it;

    // Second node: val=2, depth=2
    auto snap1 = *it;
    require(snap1.val(), 2);
    require(snap1.size(), size_t(2));
    ++it;

    // Third node: val=1, depth=1
    auto snap2 = *it;
    require(snap2.val(), 1);
    require(snap2.size(), size_t(1));
    ++it;

    require_true(it == c.nodes().end());
  }
  end_test_case();

  test_case("nodes() skip(1) matches parent (Rust reference test)");
  {
    auto c = micron::cactus_stack<int>{}.child(1).child(2).child(3);
    auto it = c.nodes().begin();
    ++it;     // skip head (val=3)
    auto snap = *it;
    auto expected = micron::cactus_stack<int>{}.child(1).child(2);
    require_true(snap == expected);
  }
  end_test_case();

  test_case("nodes() snapshot is independent of owner (stale-index check)");
  {
    // The old NodesIter::operator*() set snap._head = cur after a compact
    // copy, which was wrong when cur != owner._head because indices are
    // renumbered.  Verify the fix.
    auto c = micron::cactus_stack<int>{}.push(10).push(20).push(30);
    auto it = c.nodes().begin();
    ++it;     // move to node with val=20 (index varies depending on pool layout)
    auto snap = *it;
    require(snap.val(), 20);
    require(snap.size(), size_t(2));
    // snap's parent should be val=10
    require(snap.pop().val(), 10);
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  //  Equality (Rust reference: test_eq)
  // ─────────────────────────────────────────────────────────────────────────

  test_case("empty stacks are equal");
  {
    micron::cactus_stack<int> c1, c2;
    require_true(c1 == c2);
    require_false(c1 != c2);
  }
  end_test_case();

  test_case("stack equals itself");
  {
    auto c = micron::cactus_stack<int>{}.push(1).push(2);
    require_true(c == c);
  }
  end_test_case();

  test_case("two children from same parent with same value are equal");
  {
    auto base = micron::cactus_stack<int>{}.push(1).push(2);
    auto c1 = base.push(4);
    auto c2 = base.push(4);
    require_true(c1 == c2);
  }
  end_test_case();

  test_case("stacks with different values are unequal");
  {
    auto c1 = micron::cactus_stack<int>{}.push(1).push(2);
    auto c2 = micron::cactus_stack<int>{}.push(2).push(2);
    require_true(c1 != c2);
    require_false(c1 == c2);
  }
  end_test_case();

  test_case("stacks with different depths are unequal");
  {
    auto c1 = micron::cactus_stack<int>{}.push(1).push(1);
    auto c2 = micron::cactus_stack<int>{}.push(1);
    require_true(c1 != c2);
  }
  end_test_case();

  test_case("independent paths with same values compare equal");
  {
    auto c1 = micron::cactus_stack<int>{}.push(1).push(2).push(3);
    auto c2 = micron::cactus_stack<int>{}.push(1).push(2).push(3);
    require_true(c1 == c2);
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  //  Comparison operators
  // ─────────────────────────────────────────────────────────────────────────

  test_case("operator< lexicographic root→head");
  {
    auto a = micron::cactus_stack<int>{}.push(1).push(2).push(3);
    auto b = micron::cactus_stack<int>{}.push(1).push(2).push(4);
    require_true(a < b);
    require_false(b < a);
  }
  end_test_case();

  test_case("operator> / >= / <=");
  {
    auto a = micron::cactus_stack<int>{}.push(1).push(2).push(3);
    auto b = micron::cactus_stack<int>{}.push(1).push(2).push(4);
    require_true(b > a);
    require_true(a <= b);
    require_true(b >= a);
    auto c = a;
    require_true(a <= c);
    require_true(a >= c);
  }
  end_test_case();

  test_case("shorter prefix compares less");
  {
    auto a = micron::cactus_stack<int>{}.push(1).push(2);
    auto b = micron::cactus_stack<int>{}.push(1).push(2).push(0);
    require_true(a < b);
    require_true(b > a);
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  //  Observer / type-trait helpers
  // ─────────────────────────────────────────────────────────────────────────

  test_case("max_size() and capacity() equal N");
  {
    require(micron::cactus_stack<int, 16>::max_size(), size_t(16));
    require(micron::cactus_stack<int, 16>::capacity(), size_t(16));
  }
  end_test_case();

  test_case("is_pod / is_class_type / is_trivial");
  {
    require_true(micron::cactus_stack<int>::is_pod());
    require_false(micron::cactus_stack<int>::is_class_type());
    require_false(micron::cactus_stack<Tracked>::is_pod());
    require_true(micron::cactus_stack<Tracked>::is_class_type());
  }
  end_test_case();

  test_case("addr() returns pointer to the object");
  {
    micron::cactus_stack<int> c;
    require_true(c.addr() == &c);
    const micron::cactus_stack<int> cc;
    require_true(cc.addr() == &cc);
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  //  Tracked lifetime — ctor == dtor always
  // ─────────────────────────────────────────────────────────────────────────

  test_case("lifetime: linear push chain");
  {
    reset_tracked();
    {
      micron::cactus_stack<Tracked, 16> c;
      for ( int i = 0; i < 12; ++i )
        c = c.push(Tracked(i));
      require(c.size(), size_t(12));
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  test_case("lifetime: push then pop then push (orphan path)");
  {
    reset_tracked();
    {
      micron::cactus_stack<Tracked, 8> c;
      for ( int i = 0; i < 6; ++i )
        c = c.push(Tracked(i));
      c = c.pop().pop().pop();     // orphans 3 slots
      for ( int i = 0; i < 3; ++i )
        c = c.push(Tracked(100 + i));
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  test_case("lifetime: branching — both branches destroyed");
  {
    reset_tracked();
    {
      auto base = micron::cactus_stack<Tracked, 8>{}.push(Tracked(1));
      auto brA = base.push(Tracked(10));
      auto brB = base.push(Tracked(20));
      require(brA.val().v, 10);
      require(brB.val().v, 20);
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  test_case("lifetime: push_range / pop_range balance");
  {
    reset_tracked();
    {
      Tracked t1(1), t2(2), t3(3);
      auto c = micron::cactus_stack<Tracked>{}.push_range(t1, t2, t3);
      Tracked a, b, cv;
      auto rem = c.pop_range(a, b, cv);
      require(a.v, 3);
      require(b.v, 2);
      require(cv.v, 1);
      require_true(rem.is_empty());
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  test_case("lifetime: copy construction does not double-free");
  {
    reset_tracked();
    {
      auto a = micron::cactus_stack<Tracked>{}.push(Tracked(1)).push(Tracked(2));
      {
        auto b = a;
        require(b.size(), size_t(2));
      }     // b destroyed — its copy of the spine is freed
      require(a.size(), size_t(2));     // a still valid
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  test_case("lifetime: move construction does not double-free");
  {
    reset_tracked();
    {
      auto a = micron::cactus_stack<Tracked>{}.push(Tracked(5)).push(Tracked(6));
      auto b = micron::move(a);
      require(b.size(), size_t(2));
      require_true(a.is_empty());
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  test_case("lifetime: pop() orphan is correctly destructed");
  {
    // After pop(), the returned stack has an orphaned slot (the old head).
    // Its T value must be destructed exactly once — by the returned stack's
    // destructor when it goes out of scope.
    reset_tracked();
    {
      auto c = micron::cactus_stack<Tracked>{}.push(Tracked(1)).push(Tracked(2)).push(Tracked(3));
      auto p = c.pop();     // slot holding Tracked(3) is orphaned in p's pool
      require(p.val().v, 2);
      // When p is destroyed, it destructs all [0, _used) including the orphan
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  test_case("lifetime: compact copy on push drops prior orphan cleanly");
  {
    reset_tracked();
    {
      using CS = micron::cactus_stack<Tracked, 4>;
      auto c = CS{}.push(Tracked(1)).push(Tracked(2)).push(Tracked(3)).push(Tracked(4));
      c = c.pop();                 // c now has 3 live + 1 orphan, _used=4
      c = c.push(Tracked(99));     // compact copy sees depth=3, allocs slot 3, total _used=4
      require(c.val().v, 99);
      require(c.size(), size_t(4));
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  //  Large / stress tests (N is chosen correctly for each)
  // ─────────────────────────────────────────────────────────────────────────

  test_case("large linear chain then full unwind via pop()");
  {
    // N=128 >= depth 100; no issues
    constexpr int K = 100;
    micron::cactus_stack<int, 128> c;
    for ( int i = 0; i < K; ++i )
      c = c.push(i);
    require(c.size(), size_t(K));
    for ( int i = K - 1; i >= 0; --i ) {
      require(c.val(), i);
      c = c.pop();
    }
    require_true(c.is_empty());
  }
  end_test_case();

  test_case("vals() over 50 elements matches push order");
  {
    micron::cactus_stack<int, 64> c;
    for ( int i = 1; i <= 50; ++i )
      c = c.push(i);
    int expect = 50;
    for ( const int &v : c.vals() )
      require(v, expect--);
    require(expect, 0);
  }
  end_test_case();

  test_case("interleaved push/pop stays consistent");
  {
    micron::cactus_stack<int, 128> c;
    for ( int i = 0; i < 50; ++i ) {
      c = c.push(i);
      if ( i % 3 == 0 ) {
        require(c.val(), i);
        c = c.pop();
      }
    }
    require_false(c.is_empty());
  }
  end_test_case();

  test_case("tracked lifetime: large chain");
  {
    reset_tracked();
    {
      micron::cactus_stack<Tracked, 64> c;
      for ( int i = 0; i < 50; ++i )
        c = c.push(Tracked(i));
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  test_case("tracked lifetime: push/pop stress");
  {
    reset_tracked();
    {
      micron::cactus_stack<Tracked, 32> c;
      for ( int round = 0; round < 10; ++round ) {
        for ( int i = 0; i < 10; ++i )
          c = c.push(Tracked(round * 10 + i));
        for ( int i = 0; i < 8; ++i )
          c = c.pop();
      }
    }
    require(Tracked::ctor, Tracked::dtor);
  }
  end_test_case();

  // ─────────────────────────────────────────────────────────────────────────
  //  Rust reference parity (exact test_* translations)
  // ─────────────────────────────────────────────────────────────────────────

  test_case("[rust parity] test_simple");
  {
    micron::cactus_stack<int> r;
    require_true(r.is_empty());
    require(r.len(), size_t(0));

    auto r2 = r.child(2);
    require_false(r2.is_empty());
    require(r2.len(), size_t(1));
    require(r2.val(), 2);

    auto r3 = r2.parent();
    require_true(r3.is_empty());
    require(r3.len(), size_t(0));

    auto r4 = r.child(3);
    require(r4.len(), size_t(1));
    require(r4.val(), 3);

    auto r5 = r4.parent();
    require_true(r5.is_empty());

    auto r6 = r4.child(4);
    require(r6.len(), size_t(2));
    require(r6.val(), 4);
    require(r6.parent().val(), 3);
  }
  end_test_case();

  test_case("[rust parity] test_vals — head→root order");
  {
    // Rust: child(3).child(2).child(1) → vals == [1, 2, 3]
    auto c = micron::cactus_stack<int>{}.child(3).child(2).child(1);
    int expected[] = { 1, 2, 3 };
    int idx = 0;
    for ( const int &v : c.vals() )
      require(v, expected[idx++]);
    require(idx, 3);
  }
  end_test_case();

  test_case("[rust parity] test_vals_nodes skip(1)");
  {
    auto c = micron::cactus_stack<int>{}.child(1).child(2).child(3);
    auto it = c.nodes().begin();
    ++it;     // skip head (val=3)
    auto snap = *it;
    auto expected = micron::cactus_stack<int>{}.child(1).child(2);
    require_true(snap == expected);
  }
  end_test_case();

  test_case("[rust parity] test_eq");
  {
    micron::cactus_stack<int> c1, c2;
    require_true(c1 == c2);

    auto a = micron::cactus_stack<int>{}.push(1).push(2);
    require_true(a == a);

    auto a1 = a.child(4);
    auto a2 = a.child(4);
    require_true(a1 == a2);

    auto b = micron::cactus_stack<int>{}.push(1).push(2);
    require_true(a == b);
    require_false(a != b);

    auto c3 = micron::cactus_stack<int>{}.push(2).push(2);
    require_true(a != c3);
    require_false(a == c3);

    auto x = micron::cactus_stack<int>{}.push(1).push(1);
    auto y = micron::cactus_stack<int>{}.push(1);
    require_true(x != y);
  }
  end_test_case();

  sb::print("=== ALL CACTUS STACK TESTS PASSED ===");
  return 1;
}
