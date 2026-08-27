//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define MICRON_TEST_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) && !defined(MICRON_TEST_ASAN)
#define MICRON_TEST_ASAN 1
#endif

#include "../../src/allocator.hpp"
#include "../../src/linux/process/fork.hpp"
#include "../../src/linux/process/wait.hpp"

#include "../snowball/snowball.hpp"

namespace
{
bool
faulted_on_write(byte *address)
{
#if defined(MICRON_TEST_ASAN)
  (void)address;
  return true;
#else
  const int pid = micron::fork();
  if ( pid == 0 ) {
    *reinterpret_cast<volatile byte *>(address) = 0xff;
    micron::sys_exit(77);
  }
  if ( pid <= 0 ) return false;
  int status = 0;
  micron::waitpid(pid, &status, 0);
  return micron::wifsignaled(status) && micron::wtermsig(status) == micron::posix::sig_segv;
#endif
}
}      // namespace

int
main()
{
  sb::print("=== ADVANCED ALLOCATOR RIGOR ===");

  sb::test_case("abcmalloc aligned chunks use the native floor and one recoverable over-alignment prefix");
  {
    constexpr usize alignments[] = { 1, 16, 32, 64, 256, 4096 };
    for ( usize alignment : alignments ) {
      micron::chunk<byte> memory = abc::balloc(73, alignment);
      sb::require_true(memory.ptr != nullptr && memory.len >= 73);
      sb::require(reinterpret_cast<uintptr_t>(memory.ptr) & (alignment - 1), uintptr_t{ 0 });
      for ( usize i = 0; i < 73; ++i ) memory.ptr[i] = static_cast<byte>(i ^ alignment);
      for ( usize i = 0; i < 73; ++i ) sb::require(memory.ptr[i], static_cast<byte>(i ^ alignment));
      abc::dealloc(memory, alignment);
    }
  }
  sb::end_test_case();

  sb::test_case("external provenance grows across registry pages and reuses unregistered holes");
  {
    constexpr usize ranges = 384;
    constexpr usize bytes = ranges * micron::page_size;
    addr_t *mapping
        = micron::mmap(nullptr, bytes, micron::prot_read | micron::prot_write, micron::map_private | micron::map_anonymous, -1, 0);
    sb::require_false(micron::mmap_failed(mapping));
    byte *base = reinterpret_cast<byte *>(mapping);
    for ( usize i = 0; i < ranges; ++i )
      sb::require(abc::mark_at(base + i * micron::page_size, micron::page_size), base + i * micron::page_size);
    sb::require_true(abc::within(base + (ranges - 1) * micron::page_size + 17));
    for ( usize i = 0; i < ranges; i += 3 )
      sb::require(abc::unmark_at(base + i * micron::page_size, micron::page_size), base + i * micron::page_size);
    for ( usize i = 0; i < ranges; i += 3 )
      sb::require(abc::mark_at(base + i * micron::page_size, micron::page_size), base + i * micron::page_size);
    for ( usize i = 0; i < ranges; ++i )
      sb::require(abc::unmark_at(base + i * micron::page_size, micron::page_size), base + i * micron::page_size);
    sb::require_false(abc::within(base + 17));
    micron::munmap(mapping, bytes);
  }
  sb::end_test_case();

  sb::test_case("temporal laundering returns a chunk and makes aliasing explicit");
  {
    micron::chunk<byte> first = micron::allocator_temporal::launder<64>(96);
    first.ptr[0] = 0x5a;
    micron::chunk<byte> alias = micron::allocator_temporal::launder<64>(96);
    sb::require(alias.ptr, first.ptr);
    sb::require(alias.ptr[0], static_cast<byte>(0x5a));
    micron::allocator_temporal::retire<64>(first);

    byte *compatibility = abc::launder(48);
    compatibility[0] = 0xa7;
    byte *compatibility_alias = abc::launder(48);
    sb::require(compatibility_alias, compatibility);
    sb::require(compatibility_alias[0], static_cast<byte>(0xa7));
    abc::retire(compatibility);
  }
  sb::end_test_case();

  sb::test_case("retiring allocator tombstones aligned allocations instead of regranting their address");
  {
    using retiring = micron::allocator_retiring<>;
    micron::chunk<byte> first = retiring::create(97, 256);
    first.ptr[96] = 0x33;
    byte *retired = first.ptr;
    retiring::destroy(first, 256);
    micron::chunk<byte> next = retiring::create(97, 256);
    sb::require_true(next.ptr != retired);
    retiring::destroy(next, 256);
  }
  sb::end_test_case();

  sb::test_case("external provenance recognizes interiors but only bases are present and sizeable");
  {
    const usize bytes = micron::page_size * 2;
    addr_t *mapping
        = micron::mmap(nullptr, bytes, micron::prot_read | micron::prot_write, micron::map_private | micron::map_anonymous, -1, 0);
    sb::require_false(micron::mmap_failed(mapping));
    byte *base = reinterpret_cast<byte *>(mapping);
    sb::require(abc::mark_at(base, bytes), base);
    sb::require_true(abc::is_present(base));
    sb::require_false(abc::is_present(base + 17));
    sb::require_true(abc::within(base + bytes - 1));
    sb::require(abc::query_size(base), bytes);
    sb::require(abc::query_size(base + 1), usize{ 0 });

    bool overlap = false;
    try {
      (void)abc::mark_at(base + micron::page_size, micron::page_size);
    } catch ( const micron::except::memory_error & ) {
      overlap = true;
    }
    sb::require_true(overlap);
    sb::require(abc::unmark_at(base, bytes), base);
    sb::require_false(abc::within(base + 1));
    base[bytes - 1] = 0xa5;
    sb::require(base[bytes - 1], static_cast<byte>(0xa5));
    micron::munmap(mapping, bytes);
  }
  sb::end_test_case();

  sb::test_case("fixed mappings are exact, non-destructive, and provenance registered");
  {
    addr_t *reservation = micron::mmap(nullptr, micron::page_size, micron::prot_read | micron::prot_write,
                                       micron::map_private | micron::map_anonymous, -1, 0);
    sb::require_false(micron::mmap_failed(reservation));
    micron::munmap(reservation, micron::page_size);

    micron::chunk<byte> fixed = micron::fixed_map_allocator::create_at(reservation, 137);
    sb::require(fixed.ptr, reinterpret_cast<byte *>(reservation));
    sb::require(fixed.len, usize{ 137 });
    sb::require_true(abc::within(fixed.ptr + 136));
    fixed.ptr[0] = 0x71;

    bool occupied = false;
    try {
      (void)micron::fixed_map_allocator::create_at(reservation, micron::page_size);
    } catch ( const micron::except::memory_error & ) {
      occupied = true;
    }
    sb::require_true(occupied);
    sb::require(fixed.ptr[0], static_cast<byte>(0x71));
    micron::fixed_map_allocator::destroy(fixed);
    sb::require_false(abc::within(reinterpret_cast<byte *>(reservation)));
  }
  sb::end_test_case();

  sb::test_case("immutable sealing affects only its dedicated mapping");
  {
    addr_t *foreign_mapping = micron::mmap(nullptr, micron::page_size, micron::prot_read | micron::prot_write,
                                           micron::map_private | micron::map_anonymous, -1, 0);
    sb::require_false(micron::mmap_failed(foreign_mapping));
    byte *foreign = reinterpret_cast<byte *>(foreign_mapping);
    sb::require(abc::mark_at(foreign, micron::page_size), foreign);
    sb::require_false(micron::allocator_immutable::seal({ foreign, micron::page_size }));
    foreign[0] = 0x09;

    micron::chunk<byte> sealed = micron::allocator_immutable::create(257, 64);
    micron::chunk<byte> writable = micron::allocator_immutable::create(257, 64);
    byte *ordinary = abc::alloc(64);
    sealed.ptr[0] = 0x11;
    writable.ptr[0] = 0x22;
    ordinary[0] = 0x33;
    sb::require_true(micron::allocator_immutable::seal(sealed));
    sb::require_true(faulted_on_write(sealed.ptr));
    writable.ptr[0] = 0x44;
    ordinary[0] = 0x55;
    sb::require(writable.ptr[0], static_cast<byte>(0x44));
    sb::require(ordinary[0], static_cast<byte>(0x55));
    micron::allocator_immutable::destroy(sealed);
    micron::allocator_immutable::destroy(writable);
    abc::dealloc(ordinary);
    sb::require(abc::unmark_at(foreign, micron::page_size), foreign);
    micron::munmap(foreign_mapping, micron::page_size);
  }
  sb::end_test_case();

  sb::test_case("freeze_sheet keeps the explicitly dangerous sheet-wide behavior");
  {
#if !defined(MICRON_TEST_ASAN)
    const int pid = micron::fork();
    if ( pid == 0 ) {
      byte *first = abc::alloc(64);
      byte *neighbor = abc::alloc(64);
      first[0] = 1;
      neighbor[0] = 2;
      abc::freeze_sheet(first);
      *reinterpret_cast<volatile byte *>(neighbor) = 3;
      micron::sys_exit(78);
    }
    sb::require_true(pid > 0);
    int status = 0;
    micron::waitpid(pid, &status, 0);
    sb::require_true(micron::wifsignaled(status));
    sb::require(micron::wtermsig(status), micron::posix::sig_segv);
#else
    sb::print("freeze_sheet: deliberate fault skipped under ASan");
#endif
  }
  sb::end_test_case();

  sb::test_case("abcmalloc telemetry is explicit when compiled out or in");
  {
    abc::reset_stats();
    byte *memory = abc::alloc(48);
    abc::dealloc(memory);
    const abc::stats_t stats = abc::stats();
#if defined(MICRON_ABC_STATS)
    sb::require_true(stats.enabled);
    sb::require_true(stats.alloc_requests >= 1 && stats.dealloc_requests >= 1);
    sb::require(stats.current_memory_usage, u64{ 0 });
    sb::require(stats.total_memory_freed, stats.total_memory_throughput);

    abc::reset_stats();
    micron::chunk<byte> exact = micron::allocator_exact<>::create(73);
    micron::allocator_exact<>::destroy(exact);
    const abc::stats_t sized = abc::stats();
    sb::require_true(sized.alloc_requests >= 1 && sized.dealloc_requests >= 1);
    sb::require(sized.current_memory_usage, u64{ 0 });
    sb::require(sized.total_memory_freed, sized.total_memory_throughput);
#else
    sb::require_false(stats.enabled);
#endif
  }
  sb::end_test_case();

#if defined(MICRON_ABC_PERSISTENT)
  sb::test_case("persistent wrapper is only exposed with the persistent allocator configuration");
  {
    using persistent = micron::allocator_persistent<>;
    micron::chunk<byte> first = persistent::create(80, 32);
    byte *address = first.ptr;
    persistent::destroy(first, 32);
    micron::chunk<byte> next = persistent::create(80, 32);
    sb::require_true(next.ptr != address);
    persistent::destroy(next, 32);
  }
  sb::end_test_case();
#endif

  sb::print("=== ALL ADVANCED ALLOCATOR RIGOR PASSED ===");
  return 1;
}

#if defined(MICRON_TEST_ASAN)
#undef MICRON_TEST_ASAN
#endif
