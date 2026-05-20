//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "allocator.hpp"
#include "device.hpp"
#include "errors.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

class memory_allocation
{
  VkDevice __dev = nullptr;
  VkDeviceMemory __h = nullptr;
  VkDeviceSize __size = 0;
  u32 __type_index = ~0u;
  void *__mapped = nullptr;
  bool __coherent = false;

  static u32
  __pick_type(const VkPhysicalDeviceMemoryProperties &mp, u32 type_mask, VkMemoryPropertyFlags required) noexcept
  {
    for ( u32 i = 0; i < mp.memoryTypeCount; ++i ) {
      if ( (type_mask & (1u << i)) == 0 ) continue;
      if ( (mp.memoryTypes[i].propertyFlags & required) != required ) continue;
      return i;
    }
    return ~0u;
  }

public:
  ~memory_allocation() { reset(); }

  memory_allocation() = default;

  memory_allocation(device &dev, const VkMemoryRequirements &req, VkMemoryPropertyFlags required_props) : __dev(dev.handle())
  {
    if ( !__dev || !vkAllocateMemory ) throw except::library_error("vk::memory_allocation: vkAllocateMemory unresolved");
    const auto &mp = dev.physical().memory_properties();
    __type_index = __pick_type(mp, req.memoryTypeBits, required_props);
    if ( __type_index == ~0u ) throw except::library_error("vk::memory_allocation: no memory type satisfies the requested property mask");
    __coherent = static_cast<bool>(mp.memoryTypes[__type_index].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateInfo info{};
    info.sType = structure_type_of_v<VkMemoryAllocateInfo>;
    info.allocationSize = req.size;
    info.memoryTypeIndex = __type_index;
    check_vk(vkAllocateMemory(__dev, &info, host_allocation_callbacks(), &__h), "vkAllocateMemory");
    __size = req.size;
  }

  memory_allocation(const memory_allocation &) = delete;

  memory_allocation(memory_allocation &&o) noexcept
      : __dev(o.__dev), __h(o.__h), __size(o.__size), __type_index(o.__type_index), __mapped(o.__mapped), __coherent(o.__coherent)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
    o.__mapped = nullptr;
  }

  memory_allocation &operator=(const memory_allocation &) = delete;

  memory_allocation &
  operator=(memory_allocation &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __dev = o.__dev;
      __h = o.__h;
      __size = o.__size;
      __type_index = o.__type_index;
      __mapped = o.__mapped;
      __coherent = o.__coherent;
      o.__dev = nullptr;
      o.__h = nullptr;
      o.__mapped = nullptr;
    }
    return *this;
  }

  VkDeviceMemory
  handle() const noexcept
  {
    return __h;
  }

  VkDeviceSize
  size() const noexcept
  {
    return __size;
  }

  u32
  type_index() const noexcept
  {
    return __type_index;
  }

  bool
  coherent() const noexcept
  {
    return __coherent;
  }

  bool
  mapped() const noexcept
  {
    return __mapped != nullptr;
  }

  void *
  mapped_ptr() const noexcept
  {
    return __mapped;
  }

  bool
  valid() const noexcept
  {
    return __h != nullptr;
  }

  explicit
  operator bool() const noexcept
  {
    return valid();
  }

  void *
  map(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE)
  {
    if ( !__h || !vkMapMemory ) throw except::library_error("vk::memory_allocation::map: not allocated or vkMapMemory missing");
    if ( __mapped ) return __mapped;
    check_vk(vkMapMemory(__dev, __h, offset, size, VkMemoryMapFlags{}, &__mapped), "vkMapMemory");
    return __mapped;
  }

  void
  unmap() noexcept
  {
    if ( !__mapped || !__h || !vkUnmapMemory ) return;
    vkUnmapMemory(__dev, __h);
    __mapped = nullptr;
  }

  void
  flush(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE) noexcept
  {
    if ( __coherent || !__h || !__mapped || !vkFlushMappedMemoryRanges ) return;
    VkMappedMemoryRange r{};
    r.sType = structure_type_of_v<VkMappedMemoryRange>;
    r.memory = __h;
    r.offset = offset;
    r.size = size;
    (void)vkFlushMappedMemoryRanges(__dev, 1, &r);
  }

  void
  invalidate(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE) noexcept
  {
    if ( __coherent || !__h || !__mapped || !vkInvalidateMappedMemoryRanges ) return;
    VkMappedMemoryRange r{};
    r.sType = structure_type_of_v<VkMappedMemoryRange>;
    r.memory = __h;
    r.offset = offset;
    r.size = size;
    (void)vkInvalidateMappedMemoryRanges(__dev, 1, &r);
  }

  void
  reset() noexcept
  {
    if ( __mapped ) unmap();
    if ( __h && __dev && vkFreeMemory ) vkFreeMemory(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
    __size = 0;
    __type_index = ~0u;
    __coherent = false;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
