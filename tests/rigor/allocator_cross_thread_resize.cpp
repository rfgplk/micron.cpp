//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

#include "../../src/allocator.hpp"
#include "../../src/atomic/atomic.hpp"
#include "../../src/sync/yield.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"
#include "../../src/vector.hpp"

#include "../snowball/snowball.hpp"

namespace
{

constexpr usize workers = 8;
constexpr usize over_alignment = 256;

micron::chunk<byte> blocks[workers]{};
bool valid[workers]{};
micron::atomic_token<u32> ready{ 0 };
micron::atomic_token<u32> go{ 0 };

micron::chunk<byte> direct_block{};
micron::chunk<byte> runtime_block{};
micron::chunk<byte> native_aligned_block{};
micron::chunk<byte> over_aligned_block{};
micron::chunk<byte> aligned_zero_block{};
micron::chunk<byte> retiring_block{};
byte *raw_block = nullptr;
bool operation_valid = false;

micron::chunk<byte> released_block{};
micron::chunk<byte> released_aligned{};
byte *released_raw = nullptr;

micron::vector<u32> transferred_vector;

constexpr byte
pattern(usize key, usize offset)
{
  return static_cast<byte>((key * 31 + offset * 17 + 3) & 0xffu);
}

void
fill(byte *ptr, usize bytes, usize key)
{
  for ( usize i = 0; i < bytes; ++i ) ptr[i] = pattern(key, i);
}

bool
check(const byte *ptr, usize bytes, usize key)
{
  for ( usize i = 0; i < bytes; ++i )
    if ( ptr[i] != pattern(key, i) ) return false;
  return true;
}

bool
owned_here(const byte *ptr)
{
  return ptr != nullptr && abc::__query_arena(ptr) == abc::__current_arena();
}

void
resize_foreign(usize worker)
{
  ready.fetch_add(1, micron::memory_order_acq_rel);
  while ( go.get(micron::memory_order_acquire) == 0 ) micron::yield();

  const usize old_len = blocks[worker].len;
  bool ok = true;
  for ( usize i = 0; i < 512; ++i ) ok = ok && abc::query_size(blocks[worker].ptr) >= old_len;

  micron::chunk<byte> memory = micron::allocator_small<>::resize<1>(blocks[worker], 4096 + worker * 32, old_len);
  ok = ok && owned_here(memory.ptr) && check(memory.ptr, old_len, worker);
  valid[worker] = ok;
  micron::allocator_small<>::destroy<1>(memory);
  blocks[worker] = {};
}

void
churn_owner_arena()
{
  constexpr usize slots = 32;
  micron::chunk<byte> churn[slots]{};
  for ( usize round = 0; round < 24; ++round ) {
    for ( usize slot = 0; slot < slots; ++slot ) {
      const usize bytes = 64 + ((round * 131 + slot * 977) % 70000);
      churn[slot] = abc::balloc(bytes);
    }
    for ( usize slot = slots; slot != 0; --slot ) abc::dealloc(churn[slot - 1].ptr, churn[slot - 1].len);
  }
}

void
resize_direct_foreign()
{
  direct_block = abc::resize_chunk(direct_block, 73, 41);
  operation_valid = owned_here(direct_block.ptr) && direct_block.len >= 73 && check(direct_block.ptr, 41, 41);
  abc::dealloc(direct_block.ptr, direct_block.len);
  direct_block = {};
}

void
resize_runtime_foreign()
{
  runtime_block = micron::allocator_small<>::resize(runtime_block, 2048, 90, 1);
  operation_valid = owned_here(runtime_block.ptr) && check(runtime_block.ptr, 90, 42);
  micron::allocator_small<>::destroy(runtime_block, 1);
  runtime_block = {};
}

void
resize_aligned_foreign()
{
  native_aligned_block = abc::aligned_resize(native_aligned_block, 4096, 111, abc::native_block_alignment);
  bool ok = owned_here(native_aligned_block.ptr) && check(native_aligned_block.ptr, 111, 43);

  over_aligned_block = abc::aligned_resize(over_aligned_block, 97, 53, over_alignment);
  ok = ok && owned_here(over_aligned_block.ptr) && (reinterpret_cast<uintptr_t>(over_aligned_block.ptr) & (over_alignment - 1)) == 0
       && check(over_aligned_block.ptr, 53, 44);

  aligned_zero_block = abc::aligned_resize(aligned_zero_block, 0, 0, over_alignment);
  ok = ok && aligned_zero_block.ptr == nullptr && aligned_zero_block.len == 0;

  abc::aligned_dealloc(native_aligned_block, abc::native_block_alignment);
  abc::aligned_dealloc(over_aligned_block, over_alignment);
  native_aligned_block = {};
  over_aligned_block = {};
  operation_valid = ok;
}

void
realloc_foreign()
{
  raw_block = static_cast<byte *>(abc::realloc(raw_block, 8192));
  bool ok = owned_here(raw_block) && abc::query_size(raw_block) >= 8192 && check(raw_block, 512, 45);
  raw_block = static_cast<byte *>(abc::realloc(raw_block, 73));
  ok = ok && raw_block != nullptr && abc::query_size(raw_block) >= 73 && check(raw_block, 73, 45);
  abc::dealloc(raw_block);
  raw_block = nullptr;
  operation_valid = ok;
}

void
resize_vector_foreign()
{
  micron::vector<u32> local(micron::move(transferred_vector));
  local.reserve(4096);
  bool ok = local.size() == 128 && owned_here(reinterpret_cast<const byte *>(local.data()));
  for ( usize i = 0; i < local.size(); ++i ) ok = ok && local[i] == static_cast<u32>(i * 13 + 7);
  operation_valid = ok;
}

void
resize_retiring_foreign()
{
  using retiring = micron::allocator_retiring<>;
  retiring_block = retiring::resize<over_alignment>(retiring_block, 4096, 97);
  operation_valid = owned_here(retiring_block.ptr) && (reinterpret_cast<uintptr_t>(retiring_block.ptr) & (over_alignment - 1)) == 0
                    && check(retiring_block.ptr, 97, 46);
  retiring::destroy<over_alignment>(retiring_block);
  retiring_block = {};
}

void
allocate_then_exit()
{
  released_block = micron::allocator_small<>::create<1>(96);
  fill(released_block.ptr, released_block.len, 47);

  released_aligned = abc::aligned_balloc(over_alignment, 211);
  fill(released_aligned.ptr, 211, 48);

  released_raw = abc::alloc(333);
  fill(released_raw, 333, 49);
}

};      // namespace

int
main()
{
  sb::print("=== ALLOCATOR CROSS THREAD RESIZE RIGOR ===");

  sb::test_case("foreign policy growth and query race migrate before arena mutation");
  {
    for ( usize worker = 0; worker < workers; ++worker ) {
      blocks[worker] = micron::allocator_small<>::create<1>(64);
      fill(blocks[worker].ptr, blocks[worker].len, worker);
    }

    micron::__thread_pointer<micron::auto_thread<>> threads[workers];
    for ( usize worker = 0; worker < workers; ++worker )
      threads[worker] = micron::solo::spawn<micron::auto_thread<>>(resize_foreign, worker);
    while ( ready.get(micron::memory_order_acquire) != workers ) micron::yield();
    go.store(1, micron::memory_order_release);
    churn_owner_arena();
    for ( usize worker = 0; worker < workers; ++worker ) micron::solo::join(threads[worker]);

    for ( usize worker = 0; worker < workers; ++worker ) sb::require_true(valid[worker]);
  }
  sb::end_test_case();

  sb::test_case("foreign direct shrink honors a partial preservation bound");
  {
    direct_block = abc::balloc(4096);
    fill(direct_block.ptr, 127, 41);
    operation_valid = false;
    auto thread = micron::solo::spawn<micron::auto_thread<>>(resize_direct_foreign);
    micron::solo::join(thread);
    sb::require_true(operation_valid);
  }
  sb::end_test_case();

  sb::test_case("runtime-alignment policy resize migrates foreign storage");
  {
    runtime_block = micron::allocator_small<>::create(333, 1);
    fill(runtime_block.ptr, 90, 42);
    operation_valid = false;
    auto thread = micron::solo::spawn<micron::auto_thread<>>(resize_runtime_foreign);
    micron::solo::join(thread);
    sb::require_true(operation_valid);
  }
  sb::end_test_case();

  sb::test_case("aligned_resize routes native and over-aligned foreign blocks");
  {
    native_aligned_block = abc::aligned_balloc(abc::native_block_alignment, 256);
    fill(native_aligned_block.ptr, 111, 43);
    over_aligned_block = abc::aligned_balloc(over_alignment, 4096);
    fill(over_aligned_block.ptr, 127, 44);
    aligned_zero_block = abc::aligned_balloc(over_alignment, 128);
    operation_valid = false;
    auto thread = micron::solo::spawn<micron::auto_thread<>>(resize_aligned_foreign);
    micron::solo::join(thread);
    sb::require_true(operation_valid);
  }
  sb::end_test_case();

  sb::test_case("raw realloc migrates a foreign block and preserves grow and shrink prefixes");
  {
    raw_block = abc::alloc(512);
    fill(raw_block, 512, 45);
    operation_valid = false;
    auto thread = micron::solo::spawn<micron::auto_thread<>>(realloc_foreign);
    micron::solo::join(thread);
    sb::require_true(operation_valid);
  }
  sb::end_test_case();

  sb::test_case("a container reserve migrates its static-origin buffer");
  {
    for ( u32 i = 0; i < 128; ++i ) transferred_vector.push_back(i * 13 + 7);
    operation_valid = false;
    auto thread = micron::solo::spawn<micron::auto_thread<>>(resize_vector_foreign);
    micron::solo::join(thread);
    sb::require_true(operation_valid);
    sb::require_true(transferred_vector.empty());
  }
  sb::end_test_case();

  sb::test_case("retiring resize routes a foreign tombstone without allowing reuse");
  {
    using retiring = micron::allocator_retiring<>;
    retiring_block = retiring::create<over_alignment>(333);
    fill(retiring_block.ptr, 97, 46);
    byte *const retired = retiring_block.ptr;
    operation_valid = false;
    auto thread = micron::solo::spawn<micron::auto_thread<>>(resize_retiring_foreign);
    micron::solo::join(thread);
    sb::require_true(operation_valid);

    micron::chunk<byte> next = retiring::create<over_alignment>(333);
    sb::require_true(next.ptr != retired);
    retiring::destroy<over_alignment>(next);
  }
  sb::end_test_case();

  sb::test_case("resize paths migrate blocks after their owner exits");
  {
    auto producer = micron::solo::spawn<micron::auto_thread<>>(allocate_then_exit);
    micron::solo::join(producer);

    const usize old_len = released_block.len;
    released_block = micron::allocator_small<>::resize<1>(released_block, 8192, old_len);
    sb::require_true(owned_here(released_block.ptr));
    sb::require_true(check(released_block.ptr, old_len, 47));
    micron::allocator_small<>::destroy<1>(released_block);
    released_block = {};

    released_aligned = abc::aligned_resize(released_aligned, 4096, 211, over_alignment);
    sb::require_true(owned_here(released_aligned.ptr));
    sb::require(reinterpret_cast<uintptr_t>(released_aligned.ptr) & (over_alignment - 1), uintptr_t{ 0 });
    sb::require_true(check(released_aligned.ptr, 211, 48));
    abc::aligned_dealloc(released_aligned, over_alignment);
    released_aligned = {};

    released_raw = static_cast<byte *>(abc::realloc(released_raw, 4096));
    sb::require_true(owned_here(released_raw));
    sb::require_true(check(released_raw, 333, 49));
    abc::dealloc(released_raw);
    released_raw = nullptr;
  }
  sb::end_test_case();

  sb::test_case("same-owner, null, and zero resize edges retain their contracts");
  {
    micron::chunk<byte> same = abc::balloc(512);
    fill(same.ptr, 128, 50);
    byte *const original = same.ptr;
    same = abc::resize_chunk(same, same.len - 1, 128);
    sb::require(same.ptr, original);
    sb::require_true(check(same.ptr, 128, 50));
    abc::dealloc(same.ptr, same.len);

    micron::chunk<byte> from_null = abc::resize_chunk({}, 129, 0);
    sb::require_true(from_null.ptr != nullptr && from_null.len >= 129);
    from_null = abc::resize_chunk(from_null, 0, 0);
    sb::require_true(from_null.ptr == nullptr && from_null.len == 0);

    micron::chunk<byte> aligned_null = abc::aligned_resize({}, 129, 0, over_alignment);
    sb::require_true(aligned_null.ptr != nullptr && aligned_null.len >= 129);
    sb::require(reinterpret_cast<uintptr_t>(aligned_null.ptr) & (over_alignment - 1), uintptr_t{ 0 });
    aligned_null = abc::aligned_resize(aligned_null, 0, 0, over_alignment);
    sb::require_true(aligned_null.ptr == nullptr && aligned_null.len == 0);

    sb::require_true(abc::realloc(nullptr, 0) == nullptr);
  }
  sb::end_test_case();

  sb::print("=== ALL ALLOCATOR CROSS THREAD RESIZE RIGOR PASSED ===");
  return 1;
}
