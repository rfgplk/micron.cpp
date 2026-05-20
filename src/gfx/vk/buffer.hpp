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
#include "memory.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

enum class buffer_usage : VkBufferUsageFlags {
  none = 0,
  transfer_src = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
  transfer_dst = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
  uniform_texel = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
  storage_texel = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
  uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
  storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
  index = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
  vertex = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
  indirect = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
  shader_device_address = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
};

inline constexpr buffer_usage
operator|(buffer_usage a, buffer_usage b) noexcept
{
  return static_cast<buffer_usage>(static_cast<VkBufferUsageFlags>(a) | static_cast<VkBufferUsageFlags>(b));
}

inline constexpr buffer_usage &
operator|=(buffer_usage &a, buffer_usage b) noexcept
{
  a = a | b;
  return a;
}

inline constexpr bool
has(buffer_usage flags, buffer_usage bit) noexcept
{
  return (static_cast<VkBufferUsageFlags>(flags) & static_cast<VkBufferUsageFlags>(bit)) != 0;
}

class buffer
{
  VkDevice __dev = nullptr;
  VkBuffer __h = nullptr;
  VkDeviceSize __size = 0;
  buffer_usage __usage = buffer_usage::none;

public:
  ~buffer() { reset(); }

  buffer() = default;

  buffer(device &dev, VkDeviceSize size, buffer_usage usage, VkSharingMode sharing = static_cast<VkSharingMode>(VK_SHARING_MODE_EXCLUSIVE))
      : __dev(dev.handle()), __size(size), __usage(usage)
  {
    if ( !__dev || !vkCreateBuffer ) throw except::library_error("vk::buffer: vkCreateBuffer unresolved");
    VkBufferCreateInfo ci{};
    ci.sType = structure_type_of_v<VkBufferCreateInfo>;
    ci.size = size;
    ci.usage = static_cast<VkBufferUsageFlags>(usage);
    ci.sharingMode = sharing;
    check_vk(vkCreateBuffer(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateBuffer");
  }

  buffer(const buffer &) = delete;

  buffer(buffer &&o) noexcept : __dev(o.__dev), __h(o.__h), __size(o.__size), __usage(o.__usage)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
  }

  buffer &operator=(const buffer &) = delete;

  buffer &
  operator=(buffer &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __dev = o.__dev;
      __h = o.__h;
      __size = o.__size;
      __usage = o.__usage;
      o.__dev = nullptr;
      o.__h = nullptr;
    }
    return *this;
  }

  VkBuffer
  handle() const noexcept
  {
    return __h;
  }

  VkDeviceSize
  size() const noexcept
  {
    return __size;
  }

  buffer_usage
  usage() const noexcept
  {
    return __usage;
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

  VkMemoryRequirements
  memory_requirements() const noexcept
  {
    VkMemoryRequirements r{};
    if ( __h && __dev && vkGetBufferMemoryRequirements ) vkGetBufferMemoryRequirements(__dev, __h, &r);
    return r;
  }

  void
  bind(memory_allocation &mem, VkDeviceSize offset = 0)
  {
    if ( !__h || !__dev || !vkBindBufferMemory ) throw except::library_error("vk::buffer::bind: vkBindBufferMemory unresolved");
    check_vk(vkBindBufferMemory(__dev, __h, mem.handle(), offset), "vkBindBufferMemory");
  }

  VkDeviceAddress
  device_address() const noexcept
  {
    if ( !__h || !__dev || !vkGetBufferDeviceAddress ) return 0;
    VkBufferDeviceAddressInfo info{};
    info.sType = structure_type_of_v<VkBufferDeviceAddressInfo>;
    info.buffer = __h;
    return vkGetBufferDeviceAddress(__dev, &info);
  }

  void
  reset() noexcept
  {
    if ( __h && __dev && vkDestroyBuffer ) vkDestroyBuffer(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
    __size = 0;
    __usage = buffer_usage::none;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
