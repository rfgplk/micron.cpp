// addr_cast.cpp
// Rigorous snowball test suite for the extended cast API in src/memory/addr.hpp:
//   ptr_cast, fn_cast, mem_ptr_cast, cast, ref_cast, cref_cast, as_const, as_mutable.

#include "../../src/memory/addr.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

namespace
{

struct foo {
  int a;
  double b;
};

struct base_a {
  int a;
};

struct base_b {
  int b;
};

struct derived: base_a, base_b {
  int c;
};

struct members {
  int x;
  int y;
  double z;
};

int
fn_inc(int v)
{
  return v + 1;
}

void
fn_void()
{
}

}      // namespace

int
main()
{
  sb::print("=== ADDR CAST TESTS ===");

  test_case("ptr_cast preserves address across cv-qualification and type");
  {
    int x = 42;
    int *p = &x;
    const addr_t a = reinterpret_cast<addr_t>(p);

    const int *cp = micron::ptr_cast<const int *>(p);
    volatile int *vp = micron::ptr_cast<volatile int *>(p);
    void *vd = micron::ptr_cast<void *>(p);
    char *ch = micron::ptr_cast<char *>(p);
    int *back = micron::ptr_cast<int *>(cp);

    require_true(reinterpret_cast<addr_t>(cp) == a);
    require_true(reinterpret_cast<addr_t>(vp) == a);
    require_true(reinterpret_cast<addr_t>(vd) == a);
    require_true(reinterpret_cast<addr_t>(ch) == a);
    require_true(back == p);
    require_true(*back == 42);
  }
  end_test_case();

  test_case("ptr_cast round-trips through an unrelated type");
  {
    foo f{ 7, 1.5 };
    foo *fp = &f;
    int *ip = micron::ptr_cast<int *>(fp);
    foo *fb = micron::ptr_cast<foo *>(ip);
    require_true(reinterpret_cast<addr_t>(ip) == reinterpret_cast<addr_t>(fp));
    require_true(fb == fp);
    require_true(fb->a == 7);
  }
  end_test_case();

  test_case("cast<> uses static_cast for legal conversions, reinterpret otherwise");
  {
    int y = 7;
    int *yp = &y;

    const int *cyp = micron::cast<const int *>(yp);
    require_true(reinterpret_cast<addr_t>(cyp) == reinterpret_cast<addr_t>(yp));

    int *myp = micron::cast<int *>(cyp);
    require_true(myp == yp);
    require_true(*myp == 7);

    void *vp = micron::cast<void *>(yp);
    require_true(vp == static_cast<void *>(yp));

    int *np = micron::cast<int *>(nullptr);
    require_true(np == nullptr);
  }
  end_test_case();

  test_case("cast<> performs ordinary value conversions");
  {
    require_true(micron::cast<int>(3.9) == 3);
    require_true(micron::cast<double>(5) == 5.0);
    require_true(micron::cast<unsigned>(-1) == 0xFFFFFFFFu);
  }
  end_test_case();

  test_case("cast<> adjusts derived->base while ptr_cast does not");
  {
    derived d{};
    derived *dp = &d;
    base_b *adjusted = micron::cast<base_b *>(dp);
    base_b *raw = micron::ptr_cast<base_b *>(dp);

    require_true(reinterpret_cast<addr_t>(adjusted) == reinterpret_cast<addr_t>(static_cast<base_b *>(dp)));
    require_true(reinterpret_cast<addr_t>(raw) == reinterpret_cast<addr_t>(dp));
    require_true(reinterpret_cast<addr_t>(adjusted) != reinterpret_cast<addr_t>(raw));
  }
  end_test_case();

  test_case("fn_cast round-trips function pointers");
  {
    using fa = int (*)(int);
    using fb = void (*)();
    fa a = &fn_inc;
    fb b = micron::fn_cast<fb>(a);
    fa back = micron::fn_cast<fa>(b);
    require_true(back == &fn_inc);
    require_true(back(41) == 42);

    fa back2 = micron::cast<fa>(micron::cast<fb>(a));
    require_true(back2 == &fn_inc);

    fb vb = micron::fn_cast<fb>(&fn_void);
    require_true(vb == &fn_void);
  }
  end_test_case();

  test_case("mem_ptr_cast round-trips pointers-to-member");
  {
    int members::*pmx = &members::x;
    double members::*pmd = micron::mem_ptr_cast<double members::*>(pmx);
    int members::*back = micron::mem_ptr_cast<int members::*>(pmd);
    require_true(back == pmx);

    int members::*via_cast = micron::cast<int members::*>(pmd);
    require_true(via_cast == pmx);

    members m{ 3, 9, 2.0 };
    require_true(m.*back == 3);
  }
  end_test_case();

  test_case("ref_cast / cref_cast alias the same storage as another type");
  {
    f32 fv = 1.0f;

    require_true(static_cast<void *>(&micron::ref_cast<u32>(fv)) == static_cast<void *>(&fv));
    require_true(static_cast<const void *>(&micron::cref_cast<u32>(fv)) == static_cast<const void *>(&fv));

    require_true(micron::ref_cast<u32>(fv) == 0x3F800000u);
    require_true(micron::cref_cast<u32>(fv) == 0x3F800000u);
  }
  end_test_case();

  test_case("ref_cast strips const (same-type write through a const-viewed mutable object)");
  {
    int n = 4;
    const int &cv = n;
    int &mut = micron::ref_cast<int>(cv);
    mut = 11;
    require_true(n == 11);
    require_true(&mut == &n);
  }
  end_test_case();

  test_case("as_const adds const, as_mutable strips it");
  {
    int q = 8;
    const int &qc = micron::as_const(q);
    require_true(&qc == &q);
    require_true(qc == 8);

    int n = 4;
    const int &cn = n;
    int &mn = micron::as_mutable(cn);
    mn = 21;
    require_true(n == 21);
    require_true(&mn == &n);
  }
  end_test_case();

  sb::print("=== ALL ADDR CAST TESTS PASSED ===");
  return 1;
}
