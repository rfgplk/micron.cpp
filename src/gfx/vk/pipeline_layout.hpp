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

class pipeline_layout
{
  VkDevice __dev = nullptr;
  VkPipelineLayout __h = nullptr;

public:
  ~pipeline_layout() { reset(); }

  pipeline_layout() = default;

  explicit pipeline_layout(device &dev) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreatePipelineLayout ) throw except::library_error("vk::pipeline_layout: vkCreatePipelineLayout unresolved");
    VkPipelineLayoutCreateInfo ci{};
    ci.sType = structure_type_of_v<VkPipelineLayoutCreateInfo>;
    check_vk(vkCreatePipelineLayout(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreatePipelineLayout");
  }

  pipeline_layout(device &dev, const VkPipelineLayoutCreateInfo &ci) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreatePipelineLayout ) throw except::library_error("vk::pipeline_layout: vkCreatePipelineLayout unresolved");
    check_vk(vkCreatePipelineLayout(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreatePipelineLayout");
  }

  pipeline_layout(const pipeline_layout &) = delete;

  pipeline_layout(pipeline_layout &&o) noexcept : __dev(o.__dev), __h(o.__h)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
  }

  pipeline_layout &operator=(const pipeline_layout &) = delete;

  pipeline_layout &
  operator=(pipeline_layout &&o) noexcept
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

  VkPipelineLayout
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
    if ( __h && __dev && vkDestroyPipelineLayout ) vkDestroyPipelineLayout(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
