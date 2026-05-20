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

class command_buffer;      // command_buffer.hpp

class command_pool
{
  VkDevice __dev = nullptr;
  VkCommandPool __h = nullptr;
  u32 __family = ~0u;

public:
  ~command_pool() { reset(); }

  command_pool() = default;

  command_pool(device &dev, u32 queue_family_index, VkCommandPoolCreateFlags flags = {}) : __dev(dev.handle()), __family(queue_family_index)
  {
    if ( !__dev || !vkCreateCommandPool ) throw except::library_error("vk::command_pool: vkCreateCommandPool unresolved");
    VkCommandPoolCreateInfo ci{};
    ci.sType = structure_type_of_v<VkCommandPoolCreateInfo>;
    ci.flags = flags;
    ci.queueFamilyIndex = queue_family_index;
    check_vk(vkCreateCommandPool(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateCommandPool");
  }

  command_pool(const command_pool &) = delete;

  command_pool(command_pool &&o) noexcept : __dev(o.__dev), __h(o.__h), __family(o.__family)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
    o.__family = ~0u;
  }

  command_pool &operator=(const command_pool &) = delete;

  command_pool &
  operator=(command_pool &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __dev = o.__dev;
      __h = o.__h;
      __family = o.__family;
      o.__dev = nullptr;
      o.__h = nullptr;
      o.__family = ~0u;
    }
    return *this;
  }

  VkCommandPool
  handle() const noexcept
  {
    return __h;
  }

  VkDevice
  device_handle() const noexcept
  {
    return __dev;
  }

  u32
  family() const noexcept
  {
    return __family;
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

  void allocate(VkCommandBufferLevel level, u32 count, command_buffer *out);

  void
  reset_pool(VkCommandPoolResetFlags flags = {})
  {
    if ( !__h || !__dev || !vkResetCommandPool ) return;
    check_vk(vkResetCommandPool(__dev, __h, flags), "vkResetCommandPool");
  }

  void free(const command_buffer *bufs, u32 count) noexcept;

  void
  reset() noexcept
  {
    if ( __h && __dev && vkDestroyCommandPool ) vkDestroyCommandPool(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
    __family = ~0u;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
