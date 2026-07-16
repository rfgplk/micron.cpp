#include "../src/io/file.hpp"
#include "../src/io/serial.hpp"
#include "../src/list.hpp"
#include "../src/maps/heap_swiss.hpp"
#include "../src/vector/vector.hpp"
#include "../src/strings.hpp"


using namespace micron;

static int fails = 0;
#define CHECK(x)                                                                                                                           \
  do {                                                                                                                                     \
    if ( !(x) ) {                                                                                                                          \
      ++fails;                                                                                                                             \
      posix::write(2, "FAIL: " #x "\n", sizeof("FAIL: " #x "\n") - 1);                                                                     \
    }                                                                                                                                      \
  } while ( 0 )

int
main()
{
  const char *path = "/tmp/mfr1_test.bin";
  // tier a + b + d in one file, sequential
  {
    io::file f(path, io::rwc);
    micron::string s("hello-tier-a");
    micron::vector<u32> v;
    for ( u32 i = 0; i < 100; ++i ) v.push_back(i * 7);
    struct pod_t {
      i32 a;
      f32 b;
    } p{ 42, 2.5f };
    CHECK(f.write(s) == (max_t)s.size());
    CHECK(f.write(v) == (max_t)(100 * sizeof(u32)));
    CHECK(f.write(p) == (max_t)sizeof(p));
    f.seek(0);
    micron::string s2(s.size(), ' ');
    CHECK(f.read(s2) == (max_t)s.size());
    CHECK(s2 == s);
    micron::vector<u32> v2(100);
    CHECK(f.read(v2) == (max_t)(100 * sizeof(u32)));
    bool veq = true;
    for ( u32 i = 0; i < 100; ++i ) veq &= (v2[i] == i * 7);
    CHECK(veq);
    pod_t p2{};
    CHECK(f.read(p2) == (max_t)sizeof(p2));
    CHECK(p2.a == 42 && p2.b == 2.5f);
  }
  // tier c: list<int> node chain
  {
    io::file f(path, io::rwc);
    micron::list<i32> l;
    l.push_back(1);
    l.push_back(2);
    l.push_back(3);
    CHECK(f.write(l) > 0);
    f.seek(0);
    auto l2 = f.read<micron::list<i32>>();
    CHECK(l2.size() == 3);
    CHECK(l2.front() == 1 && l2.back() == 3);
  }
  // tier c: vector<string> (contiguous, non-TC elements)
  {
    io::file f(path, io::rwc);
    micron::vector<micron::string> vs;
    vs.push_back(micron::string("alpha"));
    vs.push_back(micron::string("beta"));
    vs.push_back(micron::string("gamma-longer-string-to-heap"));
    CHECK(f.write(vs) > 0);
    f.seek(0);
    auto vs2 = f.read<micron::vector<micron::string>>();
    CHECK(vs2.size() == 3);
    CHECK(vs2[0] == micron::string("alpha") && vs2[2] == micron::string("gamma-longer-string-to-heap"));
  }
  // tier c: heap swiss map<string is not TC... use u64->u64 first, then string values>
  {
    io::file f(path, io::rwc);
    micron::hswiss<u64, u64> m;
    for ( u64 i = 1; i <= 50; ++i ) m.insert(i, i * i);
    CHECK(f.write(m) > 0);
    f.seek(0);
    auto m2 = f.read<micron::hswiss<u64, u64>>();
    CHECK(m2.size() == 50);
    bool meq = true;
    for ( u64 i = 1; i <= 50; ++i ) {
      auto *hit = m2.find(i);
      meq &= (hit != nullptr);
    }
    CHECK(meq);
  }
  // serialize wrappers: frame/unframe option form
  {
    io::file f(path, io::rwc);
    micron::vector<micron::string> vs;
    vs.push_back(micron::string("one"));
    vs.push_back(micron::string("two"));
    CHECK(io::serialize::frame(f, vs) > 0);
    f.seek(0);
    auto r = io::serialize::unframe<micron::vector<micron::string>>(f);
    CHECK(r.is_first());
    if ( r.is_first() ) {
      auto &got = r.cast<micron::vector<micron::string>>();
      CHECK(got.size() == 2 && got[1] == micron::string("two"));
    }
  }
  // corruption detection: bad magic
  {
    byte junk[16] = { 'X', 'X', 'X', 'X', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    micron::vector<u32> dummy;
    CHECK(io::serialize::unframe_from(junk, sizeof(junk), dummy) < 0);
  }
  posix::unlink(path);
  if ( fails == 0 ) posix::write(1, "ALL PASS\n", 9);
  return fails;
}
