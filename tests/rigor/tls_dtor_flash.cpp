//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../../src/atomic/atomic.hpp"
#include "../../src/io/flash.hpp"
#include "../../src/io/fsys.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"
#include "../support/mt.hpp"

using namespace snowball;

namespace mf = micron::io::flash;
namespace mio = micron::io;
namespace px = micron::posix;

namespace
{

constexpr const char *__base = "/var/tmp/micron_tls_dtor_flash";
constexpr int __workers = 4;

micron::atomic_token<u32> g_worker_ok{ 0 };
micron::atomic_token<u32> g_worker_live{ 0 };

mio::path_t
P(int n)
{
  char buf[256];
  usize i = 0;
  for ( const char *p = __base; *p; ++p ) buf[i++] = *p;
  buf[i++] = '/';
  buf[i++] = 'w';
  buf[i++] = static_cast<char>('0' + (n % 10));
  buf[i] = '\0';
  return mio::path_t(buf);
}

i32
probe_fd(void) noexcept
{
  i32 fd = px::open("/dev/null", px::o_rdonly);
  if ( fd >= 0 ) px::close(fd);
  return fd;
}

}      // namespace

int
main(int, char **)
{
  sb::print("=== FLASH TLS TEARDOWN RIGOR ===");

  (void)mio::mkdir(mio::path_t(__base));

  if ( !mf::default_engine().live() ) {
    sb::print("io_uring unavailable; flash TLS teardown test SKIPPED");
    return 1;
  }

  const i32 fd_before = probe_fd();
  require(fd_before >= 0, true);

  test_case("each worker builds and tears down its own flash engine");
  {
    mtest::parallel(__workers, [](int id) {
      mf::engine &e = mf::default_engine();
      if ( !e.live() ) return;
      g_worker_live.fetch_add(1, micron::memory_order_acq_rel);

      const char payload[] = "micron flash tls teardown probe";
      if ( mf::write_file(P(id), payload) < 0 ) return;
      auto got = mf::read_file(P(id));
      if ( got.is_first() ) g_worker_ok.fetch_add(1, micron::memory_order_acq_rel);
    });

    require(g_worker_live.get(micron::memory_order_acquire), static_cast<u32>(__workers));
    require(g_worker_ok.get(micron::memory_order_acquire), static_cast<u32>(__workers));
  }
  end_test_case();

  test_case("every worker ring fd was closed on its owning thread");
  {

    const i32 fd_after = probe_fd();
    sb::print("fd probe before / after workers:");
    sb::print(fd_before);
    sb::print(fd_after);
    require(fd_after, fd_before);
  }
  end_test_case();

  for ( int i = 0; i < __workers; ++i ) (void)mio::remove(P(i));

  sb::print("=== FLASH TLS TEARDOWN PASSED ===");
  return 1;
}
