//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define MICRON_TEST_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) && !defined(MICRON_TEST_ASAN)
#define MICRON_TEST_ASAN 1
#endif

#include "../../src/allocator.hpp"
#include "../../src/hash/hash.hpp"
#include "../../src/heap/fibonacci_heap.hpp"
#include "../../src/heap/quake_heap.hpp"
#include "../../src/io/fsys.hpp"
#include "../../src/linux/process/fork.hpp"
#include "../../src/linux/process/wait.hpp"
#include "../../src/linux/sys/limits.hpp"
#include "../../src/maps.hpp"
#include "../../src/queue/crossbeam.hpp"
#include "../../src/queue/disruptor.hpp"
#include "../../src/queue/queue.hpp"
#include "../../src/queue/spsc_queue.hpp"
#include "../../src/string/string.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"
#include "../../src/trees/rb.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"
#include "../support/mock_allocators.hpp"

namespace
{
struct static_tag_a;
struct static_tag_b;
struct static_concurrent_tag;
struct monotonic_tag;
struct monotonic_concurrent_tag;

using static_a = micron::allocator_static<static_tag_a, 1024, 64>;
using static_b = micron::allocator_static<static_tag_b, 1024, 64>;
using static_concurrent = micron::allocator_static<static_concurrent_tag, 32768, 64>;
using monotonic_t = micron::allocator_monotonic<monotonic_tag, 4096, mtest::tracking_allocator<70>>;
using monotonic_concurrent = micron::allocator_monotonic<monotonic_concurrent_tag, 32768, mtest::tracking_allocator<71>>;

struct allocation_record {
  byte *ptr;
  usize len;
};

allocation_record static_records[256]{};
allocation_record monotonic_records[256]{};

void
static_worker(usize worker)
{
  const usize base = worker * 64;
  for ( usize i = 0; i < 64; ++i ) {
    micron::chunk<byte> memory = static_concurrent::create(17, 16);
    static_records[base + i] = { memory.ptr, memory.len };
  }
}

void
monotonic_worker(usize worker)
{
  const usize base = worker * 64;
  for ( usize i = 0; i < 64; ++i ) {
    micron::chunk<byte> memory = monotonic_concurrent::create(17, 16);
    monotonic_records[base + i] = { memory.ptr, memory.len };
  }
}

bool
non_overlapping(const allocation_record *records, usize count)
{
  for ( usize i = 0; i < count; ++i ) {
    if ( records[i].ptr == nullptr || records[i].len == 0 ) return false;
    const uintptr_t a0 = reinterpret_cast<uintptr_t>(records[i].ptr);
    const uintptr_t a1 = a0 + records[i].len;
    for ( usize j = i + 1; j < count; ++j ) {
      const uintptr_t b0 = reinterpret_cast<uintptr_t>(records[j].ptr);
      const uintptr_t b1 = b0 + records[j].len;
      if ( a0 < b1 && b0 < a1 ) return false;
    }
  }
  return true;
}

int
hex_value(char c) noexcept
{
  if ( c >= '0' && c <= '9' ) return c - '0';
  if ( c >= 'a' && c <= 'f' ) return c - 'a' + 10;
  if ( c >= 'A' && c <= 'F' ) return c - 'A' + 10;
  return -1;
}

bool
line_has_token(const micron::hstring<char> &text, usize begin, usize end, const char token[3])
{
  for ( usize i = begin; i + 1 < end; ++i ) {
    if ( text[i] != token[0] || text[i + 1] != token[1] ) continue;
    const bool left = i == begin || text[i - 1] == ' ' || text[i - 1] == ':';
    const bool right = i + 2 == end || text[i + 2] == ' ' || text[i + 2] == '\n';
    if ( left && right ) return true;
  }
  return false;
}

bool
secure_vm_flags(byte *address)
{
  micron::io::file file = micron::io::open_file("/proc/self/smaps");
  if ( !file.valid() ) return false;
  micron::hstring<char> text;
  file >> text;
  const uintptr_t target = reinterpret_cast<uintptr_t>(address);
  bool matched = false;

  for ( usize line = 0; line < text.size(); ) {
    usize end = line;
    while ( end < text.size() && text[end] != '\n' ) ++end;

    usize dash = line;
    uintptr_t lo = 0;
    int digit = dash < end ? hex_value(text[dash]) : -1;
    while ( digit >= 0 ) {
      lo = (lo << 4) | static_cast<uintptr_t>(digit);
      digit = ++dash < end ? hex_value(text[dash]) : -1;
    }
    if ( dash < end && text[dash] == '-' ) {
      uintptr_t hi = 0;
      usize at = dash + 1;
      digit = at < end ? hex_value(text[at]) : -1;
      while ( digit >= 0 ) {
        hi = (hi << 4) | static_cast<uintptr_t>(digit);
        digit = ++at < end ? hex_value(text[at]) : -1;
      }
      matched = target >= lo && target < hi;
    } else if ( matched && end - line >= 8 && text[line] == 'V' && text[line + 1] == 'm' && text[line + 2] == 'F' && text[line + 3] == 'l'
                && text[line + 4] == 'a' && text[line + 5] == 'g' && text[line + 6] == 's' && text[line + 7] == ':' ) {
      return line_has_token(text, line, end, "lo") && line_has_token(text, line, end, "dc") && line_has_token(text, line, end, "dd");
    }
    line = end < text.size() ? end + 1 : end;
  }
  return false;
}

struct alignas(256) over_aligned {
  u64 value;
};

struct self_referential {
  self_referential *self;
  int value;

  explicit self_referential(int v = 0) noexcept : self(this), value(v) { }

  self_referential(const self_referential &o) noexcept : self(this), value(o.value) { }

  self_referential(self_referential &&o) noexcept : self(this), value(o.value) { o.value = -1; }

  self_referential &
  operator=(const self_referential &o) noexcept
  {
    value = o.value;
    self = this;
    return *this;
  }

  self_referential &
  operator=(self_referential &&o) noexcept
  {
    value = o.value;
    o.value = -1;
    self = this;
    return *this;
  }
};

struct throwing_move_only {
  static inline int live = 0;
  static inline int moves = 0;
  static inline int trip = -1;
  int value;

  explicit throwing_move_only(int v = 0) : value(v) { ++live; }

  throwing_move_only(const throwing_move_only &) = delete;
  throwing_move_only &operator=(const throwing_move_only &) = delete;

  throwing_move_only(throwing_move_only &&o) : value(o.value)
  {
    o.value = -1;
    if ( trip >= 0 && moves++ == trip ) throw micron::runtime{ "throwing_move_only" };
    ++live;
  }

  throwing_move_only &
  operator=(throwing_move_only &&o)
  {
    value = o.value;
    o.value = -1;
    return *this;
  }

  ~throwing_move_only() { --live; }
};

class growth_probe_allocator: public mtest::tracking_allocator<72>
{
  using base = mtest::tracking_allocator<72>;

public:
  static inline usize recommendations = 0;

  static void
  reset() noexcept
  {
    base::reset();
    recommendations = 0;
  }

  static constexpr usize
  auto_size() noexcept
  {
    return sizeof(u32);
  }

  static usize
  recommend(usize, usize minimum) noexcept
  {
    ++recommendations;
    return minimum;
  }
};

template<int Tag, typename Fn>
void
allocator_route(Fn fn)
{
  using alloc = mtest::tracking_allocator<Tag>;
  alloc::reset();
  {
    fn.template operator()<alloc>();
    sb::require_true(alloc::allocations > 0);
  }
  sb::require(alloc::outstanding(), i64{ 0 });
}
}      // namespace

int
main()
{
  sb::print("=== ALLOCATOR TYPES RIGOR ===");

  sb::test_case("checked integer arithmetic and rational growth");
  {
    usize out = 0;
    sb::require_true(micron::allocation_checked_add(7, 9, out) && out == 16);
    sb::require_false(micron::allocation_checked_add(micron::__allocation_max, 1, out));
    sb::require_false(micron::allocation_checked_multiply(micron::__allocation_max, 2, out));
    sb::require_false(micron::allocation_checked_round_up(7, 0, out));
    sb::require_false(micron::allocation_checked_round_up(micron::__allocation_max, 2, out));

    const usize current = micron::__allocation_max / 2;
    const usize expected = current + current / 2 + current % 2;
    sb::require_true(micron::allocation_checked_growth(current, 0, 3, 2, out));
    sb::require(out, expected);
    sb::require_false(micron::allocation_checked_growth(current, 0, 1, 0, out));
  }
  sb::end_test_case();

  sb::test_case("policy capacity, exact capacity, alignment, and null destruction");
  {
    using policy = micron::allocation_policy<32, 16, 3, 2>;
    sb::require(micron::__allocation_policy_capacity<policy>(33), usize{ 48 });
    sb::require(micron::__allocation_policy_recommend<policy>(32, 33), usize{ 48 });

    micron::chunk<byte> exact = micron::allocator_exact<>::create(73, 256);
    sb::require(exact.len, usize{ 73 });
    sb::require(reinterpret_cast<uintptr_t>(exact.ptr) & 255u, uintptr_t{ 0 });
    exact.ptr[72] = 0x5a;
    micron::allocator_exact<>::destroy(exact, 256);

    micron::chunk<byte> serial = micron::allocator_serial<>::create(1);
    sb::require_true(serial.len >= 1);
    sb::require(reinterpret_cast<uintptr_t>(serial.ptr) & 63u, uintptr_t{ 0 });
    serial.ptr[serial.len - 1] = 0xa5;
    micron::allocator_serial<>::destroy(serial);

    bool invalid_alignment = false;
    try {
      (void)micron::allocator_exact<>::create(8, 3);
    } catch ( const micron::except::invalid_argument & ) {
      invalid_alignment = true;
    }
    sb::require_true(invalid_alignment);

    micron::allocator_serial<>::destroy({ nullptr, 0 });
    micron::allocator_small<>::destroy({ nullptr, 0 });
    micron::allocator_constrained<>::destroy({ nullptr, 0 });
    micron::allocator_exact<>::destroy({ nullptr, 0 });
    micron::map_allocator<>::destroy({ nullptr, 0 });
    micron::allocator_huge<>::destroy({ nullptr, 0 });
    micron::allocator_secure<>::destroy({ nullptr, 0 });
    micron::allocator_guarded<>::destroy({ nullptr, 0 });
  }
  sb::end_test_case();

  sb::test_case("resize preservation and allocation-failure strong guarantee");
  {
    micron::chunk<byte> memory = micron::allocator_exact<>::create(64, 64);
    for ( usize i = 0; i < memory.len; ++i ) memory.ptr[i] = static_cast<byte>(i ^ 0x5a);
    memory = micron::allocator_exact<>::resize(memory, 129, 37, 64);
    sb::require(memory.len, usize{ 129 });
    for ( usize i = 0; i < 37; ++i ) sb::require(memory.ptr[i], static_cast<byte>(i ^ 0x5a));
    micron::allocator_exact<>::destroy(memory, 64);

    using fail = mtest::throwing_allocator<73>;
    fail::reset();
    micron::chunk<byte> old = fail::create(64, 64);
    for ( usize i = 0; i < old.len; ++i ) old.ptr[i] = static_cast<byte>(i + 1);
    byte *old_ptr = old.ptr;
    fail::arm(0);
    bool threw = false;
    try {
      (void)fail::resize(old, 128, 64, 64);
    } catch ( const micron::except::memory_error & ) {
      threw = true;
    }
    fail::disarm();
    sb::require_true(threw);
    sb::require(old.ptr, old_ptr);
    for ( usize i = 0; i < old.len; ++i ) sb::require(old.ptr[i], static_cast<byte>(i + 1));
    fail::destroy(old, 64);
    sb::require(fail::outstanding(), i64{ 0 });
  }
  sb::end_test_case();

  sb::test_case("mmap resize preserves bytes and over-page alignment");
  {
    constexpr usize alignment = micron::page_size * 2;
    micron::chunk<byte> memory = micron::map_allocator<>::create(6000, alignment);
    sb::require(reinterpret_cast<uintptr_t>(memory.ptr) & (alignment - 1), uintptr_t{ 0 });
    for ( usize i = 0; i < 6000; ++i ) memory.ptr[i] = static_cast<byte>((i * 17u) & 0xffu);
    memory = micron::map_allocator<>::resize(memory, 20000, 6000, alignment);
    sb::require_true(memory.len >= 20000);
    sb::require(reinterpret_cast<uintptr_t>(memory.ptr) & (alignment - 1), uintptr_t{ 0 });
    for ( usize i = 0; i < 6000; ++i ) sb::require(memory.ptr[i], static_cast<byte>((i * 17u) & 0xffu));
    micron::map_allocator<>::destroy(memory, alignment);
  }
  sb::end_test_case();

  sb::test_case("strict huge-page allocator succeeds or reports capability absence");
  {
    bool available = false;
    try {
      micron::chunk<byte> memory = micron::allocator_huge<micron::map_huge_2mb>::create(1, micron::page_size);
      available = true;
      sb::require_true(memory.len >= (usize{ 1 } << 21));
      memory.ptr[0] = 0x11;
      memory.ptr[memory.len - 1] = 0x22;
      micron::allocator_huge<micron::map_huge_2mb>::destroy(memory, micron::page_size);
    } catch ( const micron::except::memory_error & ) {
      sb::print("allocator_huge: no configured 2 MiB pages; capability case skipped");
    }
    if ( available ) sb::print("allocator_huge: strict mapping available");
  }
  sb::end_test_case();

  sb::test_case("secure mapping is locked, non-dumpable, and absent after fork");
  {
    bool available = false;
    try {
      micron::chunk<byte> memory = micron::allocator_secure<>::create(micron::page_size, 64);
      available = true;
      sb::require_true(secure_vm_flags(memory.ptr));
      memory.ptr[0] = 0x7b;

#if !defined(MICRON_TEST_ASAN)
      const int pid = micron::fork();
      if ( pid == 0 ) {
        volatile byte value = memory.ptr[0];
        (void)value;
        micron::sys_exit(77);
      }
      sb::require_true(pid > 0);
      int status = 0;
      micron::waitpid(pid, &status, 0);
      sb::require_true(micron::wifsignaled(status));
      sb::require(micron::wtermsig(status), micron::posix::sig_segv);
#else
      sb::print("allocator_secure: fork-fault probe skipped under ASan");
#endif
      micron::allocator_secure<>::destroy(memory, 64);
    } catch ( const micron::except::memory_error & ) {
      sb::print("allocator_secure: memlock/advice unavailable; capability case skipped");
    }
    if ( available ) {
      micron::posix::rlimit64_t limit{};
      if ( micron::posix::get_process_limits(0, micron::posix::rlimit_memlock, limit) == 0
           && limit.rlim_cur != micron::posix::rlim64_infinity && limit.rlim_cur <= 64u * 1024u * 1024u ) {
        bool refused = false;
        const usize request = static_cast<usize>(limit.rlim_cur) + micron::page_size;
        try {
          micron::chunk<byte> too_large = micron::allocator_secure<>::create(request, 64);
          micron::allocator_secure<>::destroy(too_large, 64);
        } catch ( const micron::except::memory_error & ) {
          refused = true;
        }
        if ( refused ) {
          micron::chunk<byte> after = micron::allocator_secure<>::create(micron::page_size, 64);
          micron::allocator_secure<>::destroy(after, 64);
        }
      }
    }
  }
  sb::end_test_case();

  sb::test_case("trailing guard faults on the first byte beyond capacity");
  {
    micron::chunk<byte> memory = micron::allocator_guarded<>::create(37, 1);
    memory.ptr[memory.len - 1] = 0x44;
#if !defined(MICRON_TEST_ASAN)
    const int pid = micron::fork();
    if ( pid == 0 ) {
      volatile byte *ptr = memory.ptr;
      ptr[memory.len] = 0x55;
      micron::sys_exit(78);
    }
    sb::require_true(pid > 0);
    int status = 0;
    micron::waitpid(pid, &status, 0);
    sb::require_true(micron::wifsignaled(status));
    sb::require(micron::wtermsig(status), micron::posix::sig_segv);
#else
    sb::print("allocator_guarded: deliberate guard fault skipped under ASan");
#endif
    micron::allocator_guarded<>::destroy(memory, 1);
  }
  sb::end_test_case();

  sb::test_case("tagged static arenas isolate, exhaust, reset, and allocate concurrently");
  {
    static_a::reset();
    static_b::reset();
    micron::chunk<byte> a = static_a::create(32, 16);
    micron::chunk<byte> b = static_b::create(32, 16);
    sb::require_true(a.ptr != b.ptr);

    bool exhausted = false;
    try {
      for ( ;; ) (void)static_a::create(64, 64);
    } catch ( const micron::except::memory_error & ) {
      exhausted = true;
    }
    sb::require_true(exhausted);
    sb::require_true(static_a::used() <= 1024);
    static_a::reset();
    sb::require(static_a::used(), usize{ 0 });
    sb::require(static_a::create(32, 16).ptr, a.ptr);

    static_concurrent::reset();
    {
      micron::auto_thread<> t0(static_worker, usize{ 0 });
      micron::auto_thread<> t1(static_worker, usize{ 1 });
      micron::auto_thread<> t2(static_worker, usize{ 2 });
      micron::auto_thread<> t3(static_worker, usize{ 3 });
    }
    sb::require_true(non_overlapping(static_records, 256));
  }
  sb::end_test_case();

  sb::test_case("tagged monotonic arena resets, releases, and expands safely");
  {
    using upstream = mtest::tracking_allocator<70>;
    upstream::reset();
    monotonic_t::release();
    micron::chunk<byte> first = monotonic_t::create(64, 64);
    micron::chunk<byte> second = monotonic_t::create(64, 64);
    sb::require_true(first.ptr != second.ptr);
    monotonic_t::reset();
    sb::require(monotonic_t::create(64, 64).ptr, first.ptr);
    monotonic_t::release();
    sb::require(upstream::outstanding(), i64{ 0 });

    using concurrent_upstream = mtest::tracking_allocator<71>;
    concurrent_upstream::reset();
    monotonic_concurrent::release();
    {
      micron::auto_thread<> t0(monotonic_worker, usize{ 0 });
      micron::auto_thread<> t1(monotonic_worker, usize{ 1 });
      micron::auto_thread<> t2(monotonic_worker, usize{ 2 });
      micron::auto_thread<> t3(monotonic_worker, usize{ 3 });
    }
    sb::require_true(non_overlapping(monotonic_records, 256));
    monotonic_concurrent::release();
    sb::require(concurrent_upstream::outstanding(), i64{ 0 });
  }
  sb::end_test_case();

  sb::test_case("allocator telemetry is absent by default and coherent when enabled");
  {
    micron::allocator_exact<>::reset_stats();
    micron::chunk<byte> exact = micron::allocator_exact<>::create(73, 16);
    micron::allocator_exact<>::destroy(exact, 16);
    const micron::allocator_stats_snapshot exact_stats = micron::allocator_exact<>::stats();

    monotonic_t::release();
    monotonic_t::reset_stats();
    micron::chunk<byte> monotonic = monotonic_t::create<16>(32);
    byte *const monotonic_ptr = monotonic.ptr;
    monotonic = monotonic_t::resize<16>(monotonic, 96, 32);
    sb::require(monotonic.ptr, monotonic_ptr);
    monotonic_t::reset();
    monotonic_t::release();
    const micron::allocator_stats_snapshot monotonic_stats = monotonic_t::stats();
#if defined(MICRON_ALLOCATOR_STATS)
    sb::require_true(exact_stats.enabled);
    sb::require(exact_stats.allocations, u64{ 1 });
    sb::require(exact_stats.deallocations, u64{ 1 });
    sb::require(exact_stats.current_bytes, u64{ 0 });
    sb::require_true(monotonic_stats.enabled);
    sb::require_true(monotonic_stats.allocations >= 1 && monotonic_stats.resizes >= 1);
    sb::require(monotonic_stats.resets, u64{ 1 });
    sb::require(monotonic_stats.releases, u64{ 1 });
    sb::require(monotonic_stats.current_bytes, u64{ 0 });
#else
    sb::require_false(exact_stats.enabled);
    sb::require_false(monotonic_stats.enabled);
#endif
  }
  sb::end_test_case();

  sb::test_case("over-aligned and self-referential vector elements relocate as objects");
  {
    micron::vector<over_aligned, micron::allocator_exact<>> aligned;
    for ( u64 i = 0; i < 80; ++i ) aligned.push_back(over_aligned{ i });
    sb::require(reinterpret_cast<uintptr_t>(aligned.data()) & 255u, uintptr_t{ 0 });
    for ( u64 i = 0; i < 80; ++i ) sb::require(aligned[i].value, i);

    micron::vector<self_referential, micron::allocator_exact<>> refs;
    for ( int i = 0; i < 200; ++i ) refs.emplace_back(i);
    for ( int i = 0; i < 200; ++i ) {
      sb::require(refs[static_cast<usize>(i)].self, micron::addressof(refs[static_cast<usize>(i)]));
      sb::require(refs[static_cast<usize>(i)].value, i);
    }
  }
  sb::end_test_case();

  sb::test_case("throwing move-only relocation rolls back destination and keeps a valid source");
  {
    throwing_move_only::live = 0;
    throwing_move_only::moves = 0;
    throwing_move_only::trip = -1;
    {
      micron::vector<throwing_move_only, micron::allocator_exact<>> values;
      values.reserve(8);
      for ( int i = 0; i < 8; ++i ) values.emplace_back(i);
      throwing_move_only *old_data = values.data();
      const usize old_capacity = values.max_size();
      throwing_move_only::moves = 0;
      throwing_move_only::trip = 3;
      bool threw = false;
      try {
        values.reserve(32);
      } catch ( const micron::runtime & ) {
        threw = true;
      }
      throwing_move_only::trip = -1;
      sb::require_true(threw);
      sb::require(values.data(), old_data);
      sb::require(values.size(), usize{ 8 });
      sb::require(values.max_size(), old_capacity);
      sb::require(throwing_move_only::live, 8);
    }
    sb::require(throwing_move_only::live, 0);
  }
  sb::end_test_case();

  sb::test_case("explicit reserve is exact while automatic growth asks once");
  {
    growth_probe_allocator::reset();
    {
      micron::vector<u32, growth_probe_allocator> values;
      values.reserve(5);
      sb::require(growth_probe_allocator::recommendations, usize{ 0 });
      sb::require(values.max_size(), usize{ 5 });
      for ( u32 i = 0; i < 5; ++i ) values.push_back(i);
      values.push_back(5);
      sb::require(growth_probe_allocator::recommendations, usize{ 1 });
      sb::require(values.max_size(), usize{ 6 });

      micron::vector<u32, growth_probe_allocator> more;
      more.reserve(3);
      more.push_back(6);
      more.push_back(7);
      more.push_back(8);
      values.append(more);
      sb::require(growth_probe_allocator::recommendations, usize{ 2 });
      sb::require(values.max_size(), usize{ 9 });
    }
    sb::require(growth_probe_allocator::outstanding(), i64{ 0 });
  }
  sb::end_test_case();

  sb::test_case("move assignment releases destination and oversized reserve fails before allocation");
  {
    using alloc = mtest::tracking_allocator<74>;
    alloc::reset();
    {
      micron::vector<u64, alloc> destination;
      micron::vector<u64, alloc> source;
      destination.reserve(32);
      source.reserve(64);
      destination.push_back(1);
      source.push_back(2);
      destination = micron::move(source);
      sb::require(alloc::outstanding(), i64{ 1 });
      const usize calls = alloc::allocations;
      bool overflowed = false;
      try {
        destination.reserve(micron::__allocation_max / sizeof(u64) + 1);
      } catch ( const micron::except::length_error & ) {
        overflowed = true;
      }
      sb::require_true(overflowed);
      sb::require(alloc::allocations, calls);
      sb::require(destination[0], u64{ 2 });
    }
    sb::require(alloc::outstanding(), i64{ 0 });
  }
  sb::end_test_case();

  sb::test_case("allocator-aware maps, heaps, trees, and queue cells route all storage");
  {
    allocator_route<80>([]<class A> {
      micron::robin_map<u64, u64, A> map(32);
      map.insert_hash(7, u64{ 7 }, u64{ 1 });
    });
    allocator_route<81>([]<class A> {
      micron::heap_swiss_map<u64, u64, A> map(32);
      map.insert_hash(7, u64{ 7 }, u64{ 1 });
    });
    allocator_route<82>([]<class A> {
      micron::hopscotch_map<u64, u64, 32, micron::hopscotch_node<u64, u64>, A> map;
      map.insert_asis(7, u64{ 1 });
    });
    allocator_route<83>([]<class A> {
      micron::btree_map<u64, u64, A> map(16);
      map.insert_hash(7, u64{ 7 }, u64{ 1 });
    });
    allocator_route<84>([]<class A> {
      micron::rb_map<u64, u64, A> map;
      for ( u64 i = 0; i < 96; ++i ) map.insert_hash(i, u64{ i }, u64{ i + 1 });
    });
    allocator_route<85>([]<class A> {
      micron::conmap<u64, u64, 4, A> map(64);
      map.insert_hash(7, u64{ 7 }, u64{ 1 });
    });
    allocator_route<86>([]<class A> {
      micron::fibonacci_heap<int, A> heap;
      for ( int i = 0; i < 32; ++i ) heap.insert(int(i));
      while ( !heap.empty() ) (void)heap.pop();
    });
    allocator_route<87>([]<class A> {
      micron::quake_heap<int, micron::__quake_less<int>, A> heap;
      for ( int i = 0; i < 32; ++i ) heap.insert(int(i));
      while ( !heap.empty() ) (void)heap.extract_min();
    });
    allocator_route<88>([]<class A> {
      micron::rb_tree<int, micron::default_less<int>, A> tree;
      for ( int i = 0; i < 32; ++i ) tree.insert(i);
    });
    allocator_route<89>([]<class A> {
      micron::queue<int, 0, A> queue;
      for ( int i = 0; i < 32; ++i ) queue.push(i);
    });
    allocator_route<90>([]<class A> {
      micron::crossbeam<int, 32, A> queue;
      int value = 0;
      sb::require_true(queue.push(7));
      sb::require_true(queue.pop(value));
      sb::require(value, 7);
    });
    allocator_route<91>([]<class A> {
      micron::spsc_queue<int, 32, A> queue;
      int value = 0;
      sb::require_true(queue.push(7));
      sb::require_true(queue.pop(value));
    });
    allocator_route<92>([]<class A> {
      micron::disruptor<int, 32, A> queue;
      int value = 0;
      sb::require_true(queue.push(7));
      sb::require_true(queue.pop(value));
    });
  }
  sb::end_test_case();

  sb::print("=== ALL ALLOCATOR TYPES RIGOR PASSED ===");
  return 1;
}

#if defined(MICRON_TEST_ASAN)
#undef MICRON_TEST_ASAN
#endif
