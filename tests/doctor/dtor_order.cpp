// dtor_order.cpp
// Regression suite for the abcmalloc teardown-order inversion (ABC-11) and the doctor's
// requested-vs-granted size accounting.
//
// Build BOTH gates -- the bug only aborted under -ke, but the inversion is present hosted too:
//   duck test tests/doctor/dtor_order.cpp --def ABCMALLOC_DOCTOR_HELP -o bin/doctor
//   duck test tests/doctor/dtor_order.cpp -ke --def ABCMALLOC_DOCTOR_HELP -o bin/doctor
//
// Test 1 has no in-main assertion by construction: it fires after main returns, and a regression
// shows up as SIGABRT from abc::fail_state() -- i.e. exit 134 instead of the 1 success sentinel.

#include "../../src/std.hpp"

#include "../../src/string/string.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"

using sb::require;
using sb::test_case;

// a namespace-scope string still holding its heap buffer when static destruction runs. Its dtor is
// drained AFTER the thread-dtor pass on both backends (glibc __call_tls_dtors, micron
// __run_exit_sequence), which is exactly when the arena used to be unbound.
static micron::string g_outlives_main;

int
main()
{
  sb::print("=== ABCMALLOC DTOR ORDER ===");

  test_case("a global string freed during static destruction does not fault");
  {
    g_outlives_main = "released after the thread-dtor pass has already run";
    require(g_outlives_main.size() != 0);
  }

  test_case("balloc grants a rounded block and accepts a free of chunk.len");
  {
    // 543 = 512 capacity + 16 header + 15 alignment slack, the shape micron::string's default
    // ctor produces; the small TLSF tier quantizes it to (543+64+31)&~31 = 608, user len 576
    micron::__chunk<byte> c = abc::balloc(543);
    require(c.ptr != nullptr);
    require(c.len >= 543u);
    require(c.len == 576u);
    abc::dealloc(c.ptr, c.len);      // the granted size, not the request -- must be accepted
  }

  test_case("a block filled to its full granted length is not a buffer overflow");
  {
    micron::__chunk<byte> c = abc::balloc(543);
    require(c.ptr != nullptr);
    for ( usize i = 0; i < c.len; ++i ) c.ptr[i] = static_cast<byte>(0xA7);
    require(abc::doctor::fsck() == 0u);      // a chunk consumer owning [req_size, len) is not an overflow
    abc::dealloc(c.ptr, c.len);
  }

  test_case("dealloc(ptr, chunk.len) from a thread that does not own the arena is accepted");
  {
    micron::__chunk<byte> c{ nullptr, 0 };
    auto t = micron::solo::spawn<micron::auto_thread<>>([&]() { c = abc::balloc(543); });
    micron::solo::join(t);

    require(c.ptr != nullptr);
    require(c.len == 576u);
    abc::dealloc(c.ptr, c.len);      // main's arena is not the owner: used to abort on req_size
  }

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
