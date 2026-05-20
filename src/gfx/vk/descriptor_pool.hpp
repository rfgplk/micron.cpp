//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "allocator.hpp"
#include "descriptor_set_layout.hpp"
#include "device.hpp"
#include "errors.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

class descriptor_set;

class descriptor_pool
{
  VkDevice __dev = nullptr;
  VkDescriptorPool __h = nullptr;
  bool __free_allowed = false;

public:
  ~descriptor_pool() { reset(); }

  descriptor_pool() = default;

  descriptor_pool(device &dev, const VkDescriptorPoolCreateInfo &ci) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateDescriptorPool ) throw except::library_error("vk::descriptor_pool: vkCreateDescriptorPool unresolved");
    check_vk(vkCreateDescriptorPool(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateDescriptorPool");
    __free_allowed = static_cast<bool>(ci.flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
  }

  descriptor_pool(device &dev, u32 max_sets, const VkDescriptorPoolSize *sizes, u32 size_count, VkDescriptorPoolCreateFlags flags = {})
      : __dev(dev.handle()), __free_allowed(static_cast<bool>(flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT))
  {
    if ( !__dev || !vkCreateDescriptorPool ) throw except::library_error("vk::descriptor_pool: vkCreateDescriptorPool unresolved");
    VkDescriptorPoolCreateInfo ci{};
    ci.sType = structure_type_of_v<VkDescriptorPoolCreateInfo>;
    ci.flags = flags;
    ci.maxSets = max_sets;
    ci.poolSizeCount = size_count;
    ci.pPoolSizes = sizes;
    check_vk(vkCreateDescriptorPool(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateDescriptorPool");
  }

  descriptor_pool(const descriptor_pool &) = delete;

  descriptor_pool(descriptor_pool &&o) noexcept : __dev(o.__dev), __h(o.__h), __free_allowed(o.__free_allowed)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
    o.__free_allowed = false;
  }

  descriptor_pool &operator=(const descriptor_pool &) = delete;

  descriptor_pool &
  operator=(descriptor_pool &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __dev = o.__dev;
      __h = o.__h;
      __free_allowed = o.__free_allowed;
      o.__dev = nullptr;
      o.__h = nullptr;
      o.__free_allowed = false;
    }
    return *this;
  }

  VkDescriptorPool
  handle() const noexcept
  {
    return __h;
  }

  VkDevice
  device_handle() const noexcept
  {
    return __dev;
  }

  bool
  free_allowed() const noexcept
  {
    return __free_allowed;
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

  void allocate(const VkDescriptorSetLayout *layouts, u32 count, descriptor_set *out);

  void free(const descriptor_set *sets, u32 count) noexcept;

  void
  reset_pool(VkDescriptorPoolResetFlags flags = {})
  {
    if ( !__h || !__dev || !vkResetDescriptorPool ) return;
    check_vk(vkResetDescriptorPool(__dev, __h, flags), "vkResetDescriptorPool");
  }

  void
  reset() noexcept
  {
    if ( __h && __dev && vkDestroyDescriptorPool ) vkDestroyDescriptorPool(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
    __free_allowed = false;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
