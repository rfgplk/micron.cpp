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
#include "render_pass.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

class framebuffer
{
private:
  VkDevice __dev = nullptr;
  VkFramebuffer __h = nullptr;
  VkExtent2D __extent{};

public:
  ~framebuffer() { reset(); }

  framebuffer() = default;

  framebuffer(device &dev, const VkFramebufferCreateInfo &ci) : __dev(dev.handle()), __extent{ ci.width, ci.height }
  {
    if ( !__dev || !vkCreateFramebuffer ) throw except::library_error("vk::framebuffer: vkCreateFramebuffer unresolved");
    check_vk(vkCreateFramebuffer(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateFramebuffer");
  }

  framebuffer(device &dev, const render_pass &rp, const VkImageView *attachments, u32 attachment_count, u32 width, u32 height,
              u32 layers = 1)
      : __dev(dev.handle()), __extent{ width, height }
  {
    if ( !__dev || !vkCreateFramebuffer ) throw except::library_error("vk::framebuffer: vkCreateFramebuffer unresolved");
    VkFramebufferCreateInfo ci{};
    ci.sType = structure_type_of_v<VkFramebufferCreateInfo>;
    ci.renderPass = rp.handle();
    ci.attachmentCount = attachment_count;
    ci.pAttachments = attachments;
    ci.width = width;
    ci.height = height;
    ci.layers = layers;
    check_vk(vkCreateFramebuffer(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateFramebuffer");
  }

  framebuffer(const framebuffer &) = delete;

  framebuffer(framebuffer &&o) noexcept : __dev(o.__dev), __h(o.__h), __extent(o.__extent)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
  }

  framebuffer &operator=(const framebuffer &) = delete;

  framebuffer &
  operator=(framebuffer &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __dev = o.__dev;
      __h = o.__h;
      __extent = o.__extent;
      o.__dev = nullptr;
      o.__h = nullptr;
    }
    return *this;
  }

  VkFramebuffer
  handle() const noexcept
  {
    return __h;
  }

  VkExtent2D
  extent() const noexcept
  {
    return __extent;
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
    if ( __h && __dev && vkDestroyFramebuffer ) vkDestroyFramebuffer(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
    __extent = {};
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
