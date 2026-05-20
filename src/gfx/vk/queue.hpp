//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

#include "errors.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

class queue
{
  VkQueue __h = nullptr;
  u32 __family = ~0u;
  u32 __index = 0;

public:
  queue() = default;

  queue(VkQueue h, u32 family, u32 index = 0) noexcept : __h(h), __family(family), __index(index) { }

  VkQueue
  handle() const noexcept
  {
    return __h;
  }

  u32
  family() const noexcept
  {
    return __family;
  }

  u32
  index() const noexcept
  {
    return __index;
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
  submit2(const VkSubmitInfo2 *submits, u32 count, VkFence signal_fence = VK_NULL_HANDLE)
  {
    if ( !__h || !vkQueueSubmit2 ) throw except::library_error("vk::queue::submit2: queue null or vkQueueSubmit2 not loaded");
    check_vk(vkQueueSubmit2(__h, count, submits, signal_fence), "vkQueueSubmit2");
  }

  void
  submit(const VkSubmitInfo *submits, u32 count, VkFence signal_fence = VK_NULL_HANDLE)
  {
    if ( !__h || !vkQueueSubmit ) throw except::library_error("vk::queue::submit: queue null or vkQueueSubmit not loaded");
    check_vk(vkQueueSubmit(__h, count, submits, signal_fence), "vkQueueSubmit");
  }

  void
  wait_idle()
  {
    if ( !__h || !vkQueueWaitIdle ) return;
    check_vk(vkQueueWaitIdle(__h), "vkQueueWaitIdle");
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
