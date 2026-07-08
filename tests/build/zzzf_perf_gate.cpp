

#include "../snowball/snowball.hpp"

#include "../../src/chrono.hpp"
#include "../../src/hash/zzz.hpp"
#include "../../src/io/console.hpp"
#include "../../src/io/posix/file.hpp"
#include "../../src/std.hpp"
#include "../../src/string/strings.hpp"

int
main()
{
  constexpr u64 iters = 20'000;
  enable_scope()
  {
    auto rand = mc::io::virtual_file("/dev/urandom");
    mc::string str(1 << 20);
    str.set_size(rand.read(str));
    mc::console("buffer size: ", str.size(), " data ptr mod 32: ", (u64)((u64)(&str) & 31));

    alignas(32) u64 scratch[4] = {};
    volatile u64 sink = 0;

    mc::system_clock<mc::system_clocks::monotonic> clock;
    clock.start();
    for ( usize i = 0; i < iters; ++i ) {
      mc::hashes::zzzf(&str, 1234 + i, str.size(), scratch);
      sink = scratch[0];
    }
    mc::console("zzzf seconds: ", clock.lap());

    clock.reset();
    for ( usize i = 0; i < iters; ++i ) {
      mc::hashes::zzz(&str, 1234 + i, str.size(), scratch);
      sink = scratch[0];
    }
    mc::console("zzz seconds:  ", clock.lap());
    (void)sink;
  };
}
