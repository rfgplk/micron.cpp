//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/linux/elf/tls.hpp"
#include "../../src/linux/sys/resource.hpp"

#include "../snowball/snowball.hpp"

int
main()
{
#if !defined(__micron_freestanding)
  const int initial = 41;
  const u64 module = micron::elf::tls_register(reinterpret_cast<const u8 *>(&initial), sizeof(initial), sizeof(initial), alignof(int));
  sb::require(module != 0);
  micron::elf::tls_index_t index{ static_cast<unsigned long>(module), 0 };

  sb::test_case("key exhaustion fails without publishing a TLS record");
  pthread_key_t keys[2048]{};
  usize count = 0;
  while ( count < 2048 && pthread_key_create(&keys[count], nullptr) == 0 ) ++count;
  sb::require(count > 0 && count < 2048);
  sb::require(__tls_get_addr(&index) == nullptr);
  sb::require(micron::elf::__tls_thread_head == nullptr);

  sb::test_case("a failed key creation can be retried after capacity becomes available");
  sb::require(pthread_key_delete(keys[--count]) == 0);
  auto *value = static_cast<int *>(__tls_get_addr(&index));
  sb::require(value && *value == initial);
  micron::elf::__tls_release_thread();
  micron::elf::__tls_release_thread();
  sb::require(micron::elf::__tls_thread_head == nullptr);
  while ( count ) sb::require(pthread_key_delete(keys[--count]) == 0);

  sb::test_case("record mapping failure leaves no record or cleanup payload behind");
  micron::posix::rlimit_t old{};
  sb::require(micron::posix::getrlimit(micron::posix::limits::virtual_memory, old) == 0);
  auto limited = old;
  limited.rlim_cur = 0;
  sb::require(micron::posix::setrlimit(micron::posix::limits::virtual_memory, limited) == 0);
  void *failed = __tls_get_addr(&index);
  const int restored = micron::posix::setrlimit(micron::posix::limits::virtual_memory, old);
  sb::require(restored == 0 && failed == nullptr);
  sb::require(micron::elf::__tls_thread_head == nullptr);
  value = static_cast<int *>(__tls_get_addr(&index));
  sb::require(value && *value == initial);
  micron::elf::__tls_release_thread();
  micron::elf::tls_unregister(module);
#else
  sb::skip("pthread key exhaustion requires the hosted backend");
#endif
  return 1;
}
