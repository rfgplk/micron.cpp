//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// compile gate for the vk stack. built, never run -- the vk* entry points are inline nullptr
// globals until loader.hpp dlopens them, so nothing here needs libvulkan on the link line.
//
// __bits/__vk_types.hpp is 4800 lines of generated record declarations and the ELF-class family of
// bug lives in exactly that shape: a struct that lays out differently at 32 bits and fails silently.
// this file is what puts it through the i386 and armv7 columns of the matrix.

// the allocator, the handle wrappers and the generated record headers are all exception-free and
// stay in the -k columns. the PORCELAIN is not: micron::gfx is a "setup failures throw" layer under
// hard rule 6 (gfx/vk/errors.hpp:93 throws from a plain inline function, so -fno-exceptions rejects
// it on inclusion, not on use). guarding it keeps this cell in every freestanding row instead of
// reddening 60-odd of them, and still covers everything this file exists to assert.
#include "../../src/gfx/vk/allocator.hpp"
#include "../../src/gfx/vk/pointers.hpp"

#if defined(__cpp_exceptions)
#include "../../src/gfx/vk.hpp"
#endif

namespace v = micron::gfx::vk;

// the header the host allocator hides below every block. these are the invariants the arithmetic in
// __alloc depends on, and all four break quietly rather than loudly if they stop holding.
static_assert(v::__vk_host::__off_raw < v::__vk_host::__header_size);
static_assert(v::__vk_host::__off_size < v::__vk_host::__header_size);
static_assert(v::__vk_host::__off_scope < v::__vk_host::__header_size);
static_assert(v::__vk_host::__off_magic <= v::__vk_host::__header_size);
static_assert(v::__vk_host::__off_raw >= sizeof(usize));
static_assert(v::__vk_host::__off_size >= v::__vk_host::__off_raw + sizeof(usize));
static_assert(v::__vk_host::__off_scope >= v::__vk_host::__off_size + sizeof(usize));
static_assert(v::__vk_host::__off_magic >= v::__vk_host::__off_scope + sizeof(usize));

// the mask in __alloc is a rounding op only for a power of two
static_assert(v::__vk_host::__min_align != 0 and (v::__vk_host::__min_align & (v::__vk_host::__min_align - 1)) == 0);
static_assert(v::__vk_host::__magic != 0);      // must survive truncation to a 32-bit usize

// scope is stored as usize and read back as the enum; the table must cover the spec's five values
static_assert(v::__vk_host::__scope_count == 5);
static_assert(static_cast<int>(v::VkSystemAllocationScope::COMMAND) == 0);
static_assert(static_cast<int>(v::VkSystemAllocationScope::INSTANCE) == 4);

// the callback table is plain C function pointers; a capture or a member would not convert
static_assert(sizeof(v::VkAllocationCallbacks) == 6 * sizeof(void *));

int
main()
{
  const v::VkAllocationCallbacks *cb = v::host_allocation_callbacks();
  (void)cb;

  // always compiles, both with and without MICRON_VK_HOST_STATS
  const v::host_alloc_stats_t st = v::host_alloc_stats();
  (void)st;

  // the deleters default their allocator to ours, so unique<> round-trips through one allocator
  v::unique<v::VkBuffer, v::buffer_deleter> ub{};
  (void)ub;

  // odr-uses host_allocation_callbacks() from a TU reached through vulkan.hpp. a forward
  // declaration would compile this and fail to LINK it -- see __bits/__vk_host_alloc.hpp
  const v::buffer_deleter bd{};
  if ( bd.alloc != v::host_allocation_callbacks() ) return 0;

  return 1;
}
