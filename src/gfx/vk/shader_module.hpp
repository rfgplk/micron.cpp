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

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// shader_module (4 byte aligned)

class shader_module
{
  VkDevice __dev = nullptr;
  VkShaderModule __h = nullptr;

public:
  ~shader_module() { reset(); }

  shader_module() = default;

  shader_module(device &dev, const u32 *code, usize size_bytes) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateShaderModule ) throw except::library_error("vk::shader_module: vkCreateShaderModule unresolved");
    VkShaderModuleCreateInfo ci{};
    ci.sType = structure_type_of_v<VkShaderModuleCreateInfo>;
    ci.codeSize = size_bytes;
    ci.pCode = code;
    check_vk(vkCreateShaderModule(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateShaderModule");
  }

  shader_module(device &dev, const VkShaderModuleCreateInfo &ci) : __dev(dev.handle())
  {
    if ( !__dev || !vkCreateShaderModule ) throw except::library_error("vk::shader_module: vkCreateShaderModule unresolved");
    check_vk(vkCreateShaderModule(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateShaderModule");
  }

  shader_module(const shader_module &) = delete;

  shader_module(shader_module &&o) noexcept : __dev(o.__dev), __h(o.__h)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
  }

  shader_module &operator=(const shader_module &) = delete;

  shader_module &
  operator=(shader_module &&o) noexcept
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

  VkShaderModule
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
    if ( __h && __dev && vkDestroyShaderModule ) vkDestroyShaderModule(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
