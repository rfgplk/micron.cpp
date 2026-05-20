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

//%%%%%%%%%%%%%%%%%%%%%%%%%%
// -> fence        VkFence
//  -> semaphore   VkSemaphore
//  -> event       VkEvent

namespace micron
{
namespace gfx
{
namespace vk
{

class fence
{
  VkDevice __dev = nullptr;
  VkFence __h = nullptr;

public:
  ~fence() { reset(); }

  fence() = default;

  explicit fence(device &dev, bool start_signaled = false) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateFence ) throw except::library_error("vk::fence: vkCreateFence unresolved");
    VkFenceCreateInfo ci{};
    ci.sType = structure_type_of_v<VkFenceCreateInfo>;
    ci.flags = start_signaled ? VkFenceCreateFlags{ VK_FENCE_CREATE_SIGNALED_BIT } : VkFenceCreateFlags{};
    check_vk(vkCreateFence(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateFence");
  }

  fence(const fence &) = delete;

  fence(fence &&o) noexcept : __dev(o.__dev), __h(o.__h)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
  }

  fence &operator=(const fence &) = delete;

  fence &
  operator=(fence &&o) noexcept
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

  VkFence
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

  bool
  wait(u64 timeout_ns = ~0ull)
  {
    if ( !__h || !__dev || !vkWaitForFences ) return false;
    VkResult r = vkWaitForFences(__dev, 1, &__h, VK_TRUE, timeout_ns);
    if ( r == VK_SUCCESS ) return true;
    if ( r == VK_TIMEOUT ) return false;
    check_vk(r, "vkWaitForFences");
    return false;
  }

  void
  reset_state()
  {
    if ( !__h || !__dev || !vkResetFences ) return;
    check_vk(vkResetFences(__dev, 1, &__h), "vkResetFences");
  }

  bool
  is_signaled() const noexcept
  {
    if ( !__h || !__dev || !vkGetFenceStatus ) return false;
    return vkGetFenceStatus(__dev, __h) == VK_SUCCESS;
  }

  void
  reset() noexcept
  {
    if ( __h && __dev && vkDestroyFence ) vkDestroyFence(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
  }
};

struct timeline_t {
};

inline constexpr timeline_t timeline_tag{};

class semaphore
{
  VkDevice __dev = nullptr;
  VkSemaphore __h = nullptr;
  bool __is_timeline = false;

public:
  ~semaphore() { reset(); }

  semaphore() = default;

  explicit semaphore(device &dev) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateSemaphore ) throw except::library_error("vk::semaphore: vkCreateSemaphore unresolved");
    VkSemaphoreCreateInfo ci{};
    ci.sType = structure_type_of_v<VkSemaphoreCreateInfo>;
    check_vk(vkCreateSemaphore(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateSemaphore");
  }

  semaphore(device &dev, timeline_t, u64 initial_value = 0) : __dev(dev.handle()), __is_timeline(true)
  {
    if ( !__dev || !vkCreateSemaphore ) throw except::library_error("vk::semaphore: vkCreateSemaphore unresolved");
    VkSemaphoreTypeCreateInfo type_ci{};
    type_ci.sType = structure_type_of_v<VkSemaphoreTypeCreateInfo>;
    type_ci.semaphoreType = static_cast<VkSemaphoreType>(VK_SEMAPHORE_TYPE_TIMELINE);
    type_ci.initialValue = initial_value;

    VkSemaphoreCreateInfo ci{};
    ci.sType = structure_type_of_v<VkSemaphoreCreateInfo>;
    ci.pNext = &type_ci;
    check_vk(vkCreateSemaphore(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateSemaphore (timeline)");
  }

  semaphore(const semaphore &) = delete;

  semaphore(semaphore &&o) noexcept : __dev(o.__dev), __h(o.__h), __is_timeline(o.__is_timeline)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
    o.__is_timeline = false;
  }

  semaphore &operator=(const semaphore &) = delete;

  semaphore &
  operator=(semaphore &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __dev = o.__dev;
      __h = o.__h;
      __is_timeline = o.__is_timeline;
      o.__dev = nullptr;
      o.__h = nullptr;
      o.__is_timeline = false;
    }
    return *this;
  }

  VkSemaphore
  handle() const noexcept
  {
    return __h;
  }

  bool
  valid() const noexcept
  {
    return __h != nullptr;
  }

  bool
  is_timeline() const noexcept
  {
    return __is_timeline;
  }

  explicit
  operator bool() const noexcept
  {
    return valid();
  }

  u64
  current_value() const noexcept
  {
    if ( !__is_timeline || !__h || !__dev || !vkGetSemaphoreCounterValue ) return 0;
    u64 v = 0;
    (void)vkGetSemaphoreCounterValue(__dev, __h, &v);
    return v;
  }

  bool
  wait_value(u64 value, u64 timeout_ns = ~0ull)
  {
    if ( !__is_timeline || !__h || !__dev || !vkWaitSemaphores ) return false;
    VkSemaphoreWaitInfo info{};
    info.sType = structure_type_of_v<VkSemaphoreWaitInfo>;
    info.semaphoreCount = 1;
    info.pSemaphores = &__h;
    info.pValues = &value;
    VkResult r = vkWaitSemaphores(__dev, &info, timeout_ns);
    if ( r == VK_SUCCESS ) return true;
    if ( r == VK_TIMEOUT ) return false;
    check_vk(r, "vkWaitSemaphores");
    return false;
  }

  void
  signal_value(u64 value)
  {
    if ( !__is_timeline || !__h || !__dev || !vkSignalSemaphore )
      throw except::library_error("vk::semaphore::signal_value: not a timeline semaphore");
    VkSemaphoreSignalInfo info{};
    info.sType = structure_type_of_v<VkSemaphoreSignalInfo>;
    info.semaphore = __h;
    info.value = value;
    check_vk(vkSignalSemaphore(__dev, &info), "vkSignalSemaphore");
  }

  void
  reset() noexcept
  {
    if ( __h && __dev && vkDestroySemaphore ) vkDestroySemaphore(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
    __is_timeline = false;
  }
};

class event
{
  VkDevice __dev = nullptr;
  VkEvent __h = nullptr;

public:
  ~event() { reset(); }

  event() = default;

  explicit event(device &dev) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateEvent ) throw except::library_error("vk::event: vkCreateEvent unresolved");
    VkEventCreateInfo ci{};
    ci.sType = structure_type_of_v<VkEventCreateInfo>;
    check_vk(vkCreateEvent(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateEvent");
  }

  event(const event &) = delete;

  event(event &&o) noexcept : __dev(o.__dev), __h(o.__h)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
  }

  event &operator=(const event &) = delete;

  event &
  operator=(event &&o) noexcept
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

  VkEvent
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
  set()
  {
    if ( !__h || !__dev || !vkSetEvent ) return;
    check_vk(vkSetEvent(__dev, __h), "vkSetEvent");
  }

  void
  unset()
  {
    if ( !__h || !__dev || !vkResetEvent ) return;
    check_vk(vkResetEvent(__dev, __h), "vkResetEvent");
  }

  bool
  is_set() const noexcept
  {
    if ( !__h || !__dev || !vkGetEventStatus ) return false;
    return vkGetEventStatus(__dev, __h) == VK_EVENT_SET;
  }

  void
  reset() noexcept
  {
    if ( __h && __dev && vkDestroyEvent ) vkDestroyEvent(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
