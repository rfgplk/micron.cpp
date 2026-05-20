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

class descriptor_set_layout
{
  VkDevice __dev = nullptr;
  VkDescriptorSetLayout __h = nullptr;

public:
  ~descriptor_set_layout() { reset(); }

  descriptor_set_layout() = default;

  descriptor_set_layout(device &dev, const VkDescriptorSetLayoutCreateInfo &ci) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateDescriptorSetLayout )
      throw except::library_error("vk::descriptor_set_layout: vkCreateDescriptorSetLayout unresolved");
    check_vk(vkCreateDescriptorSetLayout(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateDescriptorSetLayout");
  }

  descriptor_set_layout(device &dev, const VkDescriptorSetLayoutBinding *bindings, u32 binding_count,
                        VkDescriptorSetLayoutCreateFlags flags = {})
      : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateDescriptorSetLayout )
      throw except::library_error("vk::descriptor_set_layout: vkCreateDescriptorSetLayout unresolved");
    VkDescriptorSetLayoutCreateInfo ci{};
    ci.sType = structure_type_of_v<VkDescriptorSetLayoutCreateInfo>;
    ci.flags = flags;
    ci.bindingCount = binding_count;
    ci.pBindings = bindings;
    check_vk(vkCreateDescriptorSetLayout(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateDescriptorSetLayout");
  }

  descriptor_set_layout(const descriptor_set_layout &) = delete;

  descriptor_set_layout(descriptor_set_layout &&o) noexcept : __dev(o.__dev), __h(o.__h)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
  }

  descriptor_set_layout &operator=(const descriptor_set_layout &) = delete;

  descriptor_set_layout &
  operator=(descriptor_set_layout &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __dev = o.__dev;
      __h = o.__h;
      o.__dev = nullptr;
      o.__h = nullptr;
    }
    return *this;
  }

  VkDescriptorSetLayout
  handle() const noexcept
  {
    return __h;
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

  void
  reset() noexcept
  {
    if ( __h && __dev && vkDestroyDescriptorSetLayout ) vkDestroyDescriptorSetLayout(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
