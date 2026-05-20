//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../memory/cstring.hpp"
#include "../../syscall.hpp"
#include "../../types.hpp"

#include "allocator.hpp"
#include "errors.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

inline VkBool32 VKAPI_PTR
default_debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT /*severity*/, VkDebugUtilsMessageTypeFlagsEXT /*types*/,
                                 const VkDebugUtilsMessengerCallbackDataEXT *data, void * /*user*/) noexcept
{
  if ( !data || !data->pMessage ) return VK_FALSE;
  const char *m = data->pMessage;
  usize n = micron::strlen(m);
  micron::syscall(SYS_write, 2, m, n);
  const char nl = '\n';
  micron::syscall(SYS_write, 2, &nl, 1);
  return VK_FALSE;
}

inline VkDebugUtilsMessengerCreateInfoEXT
default_messenger_info() noexcept
{
  VkDebugUtilsMessengerCreateInfoEXT ci{};
  ci.sType = structure_type_of_v<VkDebugUtilsMessengerCreateInfoEXT>;
  ci.messageSeverity = static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                                                        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
  ci.messageType = static_cast<VkDebugUtilsMessageTypeFlagsEXT>(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                                                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                                                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);
  ci.pfnUserCallback = &default_debug_messenger_callback;
  ci.pUserData = nullptr;
  return ci;
}

class messenger
{
private:
  VkInstance __inst = nullptr;
  VkDebugUtilsMessengerEXT __h = nullptr;

public:
  ~messenger() { reset(); }

  messenger() = default;

  explicit messenger(VkInstance inst) : __inst(inst)
  {
    if ( !inst || !vkCreateDebugUtilsMessengerEXT )
      throw except::library_error("vk::messenger: VK_EXT_debug_utils not loaded — was hints.debug enabled?");
    auto ci = default_messenger_info();
    VkResult r = vkCreateDebugUtilsMessengerEXT(inst, &ci, host_allocation_callbacks(), &__h);
    check_vk(r, "vkCreateDebugUtilsMessengerEXT");
  }

  messenger(VkInstance inst, const VkDebugUtilsMessengerCreateInfoEXT &ci) : __inst(inst)
  {
    if ( !inst || !vkCreateDebugUtilsMessengerEXT )
      throw except::library_error("vk::messenger: VK_EXT_debug_utils not loaded — was hints.debug enabled?");
    VkResult r = vkCreateDebugUtilsMessengerEXT(inst, &ci, host_allocation_callbacks(), &__h);
    check_vk(r, "vkCreateDebugUtilsMessengerEXT");
  }

  messenger(const messenger &) = delete;

  messenger(messenger &&o) noexcept : __inst(o.__inst), __h(o.__h)
  {
    o.__inst = nullptr;
    o.__h = nullptr;
  }

  messenger &operator=(const messenger &) = delete;

  messenger &
  operator=(messenger &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __inst = o.__inst;
      __h = o.__h;
      o.__inst = nullptr;
      o.__h = nullptr;
    }
    return *this;
  }

  VkDebugUtilsMessengerEXT
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
    if ( __h && __inst && vkDestroyDebugUtilsMessengerEXT ) vkDestroyDebugUtilsMessengerEXT(__inst, __h, host_allocation_callbacks());
    __h = nullptr;
    __inst = nullptr;
  }
};

inline void
set_object_name(VkDevice device, VkObjectType type, u64 handle, const char *name) noexcept
{
  if ( !device || !name || !vkSetDebugUtilsObjectNameEXT ) return;
  VkDebugUtilsObjectNameInfoEXT info{};
  info.sType = structure_type_of_v<VkDebugUtilsObjectNameInfoEXT>;
  info.objectType = type;
  info.objectHandle = handle;
  info.pObjectName = name;
  (void)vkSetDebugUtilsObjectNameEXT(device, &info);
}

inline void
begin_label(VkCommandBuffer cb, const char *name, const float color[4] = nullptr) noexcept
{
  if ( !cb || !name || !vkCmdBeginDebugUtilsLabelEXT ) return;
  VkDebugUtilsLabelEXT label{};
  label.sType = structure_type_of_v<VkDebugUtilsLabelEXT>;
  label.pLabelName = name;
  if ( color ) {
    label.color[0] = color[0];
    label.color[1] = color[1];
    label.color[2] = color[2];
    label.color[3] = color[3];
  }
  vkCmdBeginDebugUtilsLabelEXT(cb, &label);
}

inline void
end_label(VkCommandBuffer cb) noexcept
{
  if ( !cb || !vkCmdEndDebugUtilsLabelEXT ) return;
  vkCmdEndDebugUtilsLabelEXT(cb);
}

inline void
insert_label(VkCommandBuffer cb, const char *name, const float color[4] = nullptr) noexcept
{
  if ( !cb || !name || !vkCmdInsertDebugUtilsLabelEXT ) return;
  VkDebugUtilsLabelEXT label{};
  label.sType = structure_type_of_v<VkDebugUtilsLabelEXT>;
  label.pLabelName = name;
  if ( color ) {
    label.color[0] = color[0];
    label.color[1] = color[1];
    label.color[2] = color[2];
    label.color[3] = color[3];
  }
  vkCmdInsertDebugUtilsLabelEXT(cb, &label);
}

};      // namespace vk
};      // namespace gfx
};      // namespace micron
