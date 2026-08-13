// vector_defects.cpp

#include "../../src/std.hpp"

#include "../../src/vector/circle_vector.hpp"
#include "../../src/vector/convector.hpp"
#include "../../src/vector/fvector.hpp"
#include "../../src/vector/ivector.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace
{

struct low_byte {
  u32 v;

  bool
  operator==(const low_byte &o) const noexcept
  {
    return (v & 0xffu) == (o.v & 0xffu);
  }
};
};      // namespace

int
main()
{
  print("=== VECTOR DEFECT REGRESSIONS ===");

  test_case("circle_vector::pop() survives an over-pop");
  {
    micron::circle_vector<int, 8> r;
    r.push(10);
    r.push(20);
    (void)r.pop();
    (void)r.pop();
    (void)r.pop();
    r.push(99);
    require(r.size(), usize(1));
    require(r[0], 99);

    micron::circle_vector<int, 4> q;
    for ( int i = 0; i < 4; ++i ) q.push(i);
    (void)q.pop();
    (void)q.pop();
    (void)q.pop();
    (void)q.pop();
    (void)q.pop();
    q.push(7);
    require(q.size(), usize(1));
    require(q[0], 7);
  }
  end_test_case();

  test_case("reserve(capacity) does not reallocate");
  {
    micron::vector<int> v;
    const usize cap = v.max_size();
    const int *before = v.data();
    v.reserve(cap);
    require(v.max_size(), cap);
    require_true(v.data() == before);

    micron::fvector<int> f;
    const usize fcap = f.max_size();
    f.try_reserve(fcap);
    require(f.max_size(), fcap);
  }
  end_test_case();

  test_case("operator[] is bounded by length, not capacity");
  {
    micron::vector<int> v;
    v.push_back(1);
    require_true(v.max_size() > v.size() + 4);

    bool threw = false;
    try {
      volatile int x = v[v.size() + 4];
      (void)x;
    } catch ( ... ) {
      threw = true;
    }
    require(threw, true);

    require(v[0], 1);
  }
  end_test_case();

  test_case("fvector::set_size clamps to capacity");
  {
    micron::fvector<int> f(4);
    f.set_size(f.max_size() + 100000);
    require_true(f.size() <= f.max_size());
  }
  end_test_case();

  test_case("convector::remove() removes every occurrence, once");
  {
    micron::convector<int> c;
    for ( int i = 0; i < 30; ++i ) c.push_back(i % 3);
    c.remove(1);
    require(c.size(), usize(20));
    for ( usize i = 0; i < c.size(); ++i ) require_true(c[i] != 1);

    c.remove(9);
    require(c.size(), usize(20));
  }
  end_test_case();

  test_case("convector clone()/copy do not self-deadlock");
  {
    micron::convector<int> c;
    for ( int i = 0; i < 5; ++i ) c.push_back(i);
    auto d = c.clone();
    auto e = c;
    require(d.size(), usize(5));
    require(e.size(), usize(5));
    require(d[4], 4);
  }
  end_test_case();

  test_case("growth never returns less capacity than requested");
  {
    micron::vector<int> v;
    usize prev = v.max_size();
    for ( int i = 0; i < 8; ++i ) {
      const usize want = prev + 1;
      v.reserve(want);
      require_true(v.max_size() >= want);
      require_true(v.max_size() > prev);
      prev = v.max_size();
    }

    micron::vector<int> w;
    w.reserve(1u << 20);
    require_true(w.max_size() >= (1u << 20));
  }
  end_test_case();

  test_case("append does not over-reserve");
  {
    micron::vector<int> a;
    micron::vector<int> b;
    for ( int i = 0; i < 8; ++i ) b.push_back(i);
    a.reserve(64);
    const usize cap = a.max_size();
    require_true(cap >= 8 + a.size());
    a.append(b);
    require(a.max_size(), cap);
    require(a.size(), usize(8));
    for ( usize i = 0; i < 8; ++i ) require(a[i], int(i));
  }
  end_test_case();

  test_case("find() honours a non-bitwise operator==");
  {
    micron::vector<low_byte> v;
    v.push_back(low_byte{ 0x1100u });
    v.push_back(low_byte{ 0x2242u });
    v.push_back(low_byte{ 0x3307u });
    auto *p = v.find(low_byte{ 0xff42u });
    require_true(p != nullptr);
    require(p->v, u32(0x2242u));
    require_true(v.find(low_byte{ 0x00f0u }) == nullptr);

    micron::vector<u64> w;
    for ( u64 i = 0; i < 200; ++i ) w.push_back(i * 3u);
    require_true(w.find(297u) != nullptr);
    require(*w.find(297u), u64(297));
    require_true(w.find(298u) == nullptr);
  }
  end_test_case();

  test_case("fill()/resize(n,v) write the right values");
  {
    micron::vector<int> v;
    for ( int i = 0; i < 100; ++i ) v.push_back(i);
    v.fill(-7);
    require(v.size(), usize(100));
    for ( usize i = 0; i < v.size(); ++i ) require(v[i], -7);

    micron::vector<u64> w;
    w.resize(64, 0xdeadbeefu);
    require(w.size(), usize(64));
    for ( usize i = 0; i < w.size(); ++i ) require(w[i], u64(0xdeadbeefu));

    w.resize(4096, 5u);
    require(w.size(), usize(4096));
    require(w[0], u64(0xdeadbeefu));
    require(w[63], u64(0xdeadbeefu));
    require(w[64], u64(5));
    require(w[4095], u64(5));
  }
  end_test_case();

  test_case("drop() empties without disturbing capacity");
  {
    micron::vector<int> v;
    for ( int i = 0; i < 50; ++i ) v.push_back(i);
    const usize cap = v.max_size();
    v.drop();
    require(v.size(), usize(0));
    require(v.max_size(), cap);
    v.push_back(11);
    require(v.size(), usize(1));
    require(v[0], 11);

    micron::fvector<int> f(10);
    f.drop();
    require(f.size(), usize(0));
  }
  end_test_case();

  test_case("unchecked (Sf == false) vector still works");
  {
    micron::vector<int, micron::allocator_serial<>, false> u;
    for ( int i = 0; i < 10; ++i ) u.push_back(i);
    u.insert(usize(3), 99);
    require(u.size(), usize(11));
    require(u[3], 99);
    u.erase(usize(0));
    require(u.size(), usize(10));
    require(u[0], 1);
  }
  end_test_case();

  print("=== ALL VECTOR DEFECT REGRESSIONS PASSED ===");
  return 1;
}
