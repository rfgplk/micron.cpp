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

class render_pass
{
  VkDevice __dev = nullptr;
  VkRenderPass __h = nullptr;

public:
  ~render_pass() { reset(); }

  render_pass() = default;

  render_pass(device &dev, const VkRenderPassCreateInfo &ci) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateRenderPass ) throw except::library_error("vk::render_pass: vkCreateRenderPass unresolved");
    check_vk(vkCreateRenderPass(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateRenderPass");
  }

  render_pass(const render_pass &) = delete;

  render_pass(render_pass &&o) noexcept : __dev(o.__dev), __h(o.__h)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
  }

  render_pass &operator=(const render_pass &) = delete;

  render_pass &
  operator=(render_pass &&o) noexcept
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

  VkRenderPass
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
    if ( __h && __dev && vkDestroyRenderPass ) vkDestroyRenderPass(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
