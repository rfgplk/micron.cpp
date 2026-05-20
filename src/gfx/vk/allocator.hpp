//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/allocation/abcmalloc/malloc.hpp"
#include "../../memory/cmemory.hpp"
#include "../../types.hpp"

#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Vulkan drivers support two memory types device memory and cpu/host memory
// cpu side allocations route through VkAllocationCallbacks passed to every vkCreate*
// if null, the driver picks the base internal allocator, otherwise will use the provided one

namespace __vk_host
{

inline constexpr usize __header_size = 16;      // size + offset_to_raw

inline void *
__alloc(usize size, usize align) noexcept
{
  if ( size == 0 ) return nullptr;
  const usize align_eff = align > __header_size ? align : __header_size;
  const usize total = size + align_eff + __header_size;
  byte *raw = abc::alloc(total);
  if ( !raw ) return nullptr;
  const uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw);
  const uintptr_t user_addr = (raw_addr + __header_size + align_eff - 1) & ~(static_cast<uintptr_t>(align_eff) - 1);
  byte *user = reinterpret_cast<byte *>(user_addr);
  *reinterpret_cast<usize *>(user - 16) = size;
  *reinterpret_cast<usize *>(user - 8) = user_addr - raw_addr;
  return user;
}

inline void
__free(void *user_ptr) noexcept
{
  if ( !user_ptr ) return;
  byte *user = reinterpret_cast<byte *>(user_ptr);
  const usize offset_to_raw = *reinterpret_cast<usize *>(user - 8);
  abc::dealloc(user - offset_to_raw);
}

inline usize
__size_of(void *user_ptr) noexcept
{
  return *reinterpret_cast<usize *>(reinterpret_cast<byte *>(user_ptr) - 16);
}

inline void *
__realloc(void *original, usize size, usize align) noexcept
{
  if ( !original ) return __alloc(size, align);
  if ( size == 0 ) {
    __free(original);
    return nullptr;
  }
  const usize old_size = __size_of(original);
  void *fresh = __alloc(size, align);
  if ( !fresh ) return nullptr;
  const usize copy = old_size < size ? old_size : size;
  micron::memcpy(reinterpret_cast<byte *>(fresh), reinterpret_cast<byte *>(original), copy);
  __free(original);
  return fresh;
}

// PFN_* signatures emitted in __vk_protos.hpp have VKAPI_PTR stripped
inline void *
__pfn_alloc(void * /*user*/, usize size, usize align, VkSystemAllocationScope /*scope*/) noexcept
{
  return __alloc(size, align);
}

inline void *
__pfn_realloc(void * /*user*/, void *original, usize size, usize align, VkSystemAllocationScope /*scope*/) noexcept
{
  return __realloc(original, size, align);
}

inline void
__pfn_free(void * /*user*/, void *p) noexcept
{
  __free(p);
}

inline void
__pfn_internal_alloc_notify(void * /*user*/, usize /*size*/, VkInternalAllocationType /*type*/, VkSystemAllocationScope /*scope*/) noexcept
{
  // the driver is telling us it allocated internally; noop
}

inline void
__pfn_internal_free_notify(void * /*user*/, usize /*size*/, VkInternalAllocationType /*type*/, VkSystemAllocationScope /*scope*/) noexcept
{
}

};      // namespace __vk_host

inline const VkAllocationCallbacks *
host_allocation_callbacks() noexcept
{
  static const VkAllocationCallbacks cb = {
    .pUserData = nullptr,
    .pfnAllocation = __vk_host::__pfn_alloc,
    .pfnReallocation = __vk_host::__pfn_realloc,
    .pfnFree = __vk_host::__pfn_free,
    .pfnInternalAllocation = __vk_host::__pfn_internal_alloc_notify,
    .pfnInternalFree = __vk_host::__pfn_internal_free_notify,
  };
  return &cb;
}

};      // namespace vk
};      // namespace gfx
};      // namespace micron
