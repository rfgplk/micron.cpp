//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "allocator.hpp"
#include "command_buffer.hpp"
#include "device.hpp"
#include "errors.hpp"
#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

class pipeline
{
  VkDevice __dev = nullptr;
  VkPipeline __h = nullptr;
  bind_point __bp = bind_point::graphics;

  static VkResult
  __create_graphics(VkDevice dev, const VkGraphicsPipelineCreateInfo &ci, VkPipeline *out) noexcept
  {
    if ( !vkCreateGraphicsPipelines ) return VK_ERROR_INITIALIZATION_FAILED;
    return vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &ci, nullptr, out);
  }

  static VkResult
  __create_compute(VkDevice dev, const VkComputePipelineCreateInfo &ci, VkPipeline *out) noexcept
  {
    if ( !vkCreateComputePipelines ) return VK_ERROR_INITIALIZATION_FAILED;
    return vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &ci, nullptr, out);
  }

public:
  enum class bind_point : u8 { graphics = 0, compute = 1 };

  ~pipeline() { reset(); }

  pipeline() = default;

  pipeline(device &dev, const VkGraphicsPipelineCreateInfo &ci) : __dev(dev.handle()), __bp(bind_point::graphics)
  {
    if ( !__dev ) throw except::logic_error("vk::pipeline: null device");
    check_vk(__create_graphics(__dev, ci, &__h), "vkCreateGraphicsPipelines");
  }

  pipeline(device &dev, const VkComputePipelineCreateInfo &ci) : __dev(dev.handle()), __bp(bind_point::compute)
  {
    if ( !__dev ) throw except::logic_error("vk::pipeline: null device");
    check_vk(__create_compute(__dev, ci, &__h), "vkCreateComputePipelines");
  }

  pipeline(const pipeline &) = delete;

  pipeline(pipeline &&o) noexcept : __dev(o.__dev), __h(o.__h), __bp(o.__bp)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
  }

  pipeline &operator=(const pipeline &) = delete;

  pipeline &
  operator=(pipeline &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __dev = o.__dev;
      __h = o.__h;
      __bp = o.__bp;
      o.__dev = nullptr;
      o.__h = nullptr;
    }
    return *this;
  }

  VkPipeline
  handle() const noexcept
  {
    return __h;
  }

  bind_point
  point() const noexcept
  {
    return __bp;
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
  bind(command_buffer &cb) const noexcept
  {
    if ( !__h ) return;
    const auto vk_bp
        = static_cast<VkPipelineBindPoint>(__bp == bind_point::graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE);
    cb.bind_pipeline(vk_bp, __h);
  }

  void
  bind(VkCommandBuffer cb) const noexcept
  {
    if ( !__h || !cb || !vkCmdBindPipeline ) return;
    const auto vk_bp
        = static_cast<VkPipelineBindPoint>(__bp == bind_point::graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE);
    vkCmdBindPipeline(cb, vk_bp, __h);
  }

  void
  reset() noexcept
  {
    if ( __h && __dev && vkDestroyPipeline ) vkDestroyPipeline(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
    __bp = bind_point::graphics;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
