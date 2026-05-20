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

class query_pool
{
  VkDevice __dev = nullptr;
  VkQueryPool __h = nullptr;
  VkQueryType __type = static_cast<VkQueryType>(0);
  u32 __count = 0;

public:
  ~query_pool() { reset(); }

  query_pool() = default;

  query_pool(device &dev, VkQueryType type, u32 count, VkQueryPipelineStatisticFlags pipeline_statistics = {})
      : __dev(dev.handle()), __type(type), __count(count)
  {
    if ( !__dev || !vkCreateQueryPool ) throw except::library_error("vk::query_pool: vkCreateQueryPool unresolved");
    VkQueryPoolCreateInfo ci{};
    ci.sType = structure_type_of_v<VkQueryPoolCreateInfo>;
    ci.queryType = type;
    ci.queryCount = count;
    ci.pipelineStatistics = pipeline_statistics;
    check_vk(vkCreateQueryPool(__dev, &ci, host_allocation_callbacks(), &__h), "vkCreateQueryPool");
  }

  query_pool(const query_pool &) = delete;

  query_pool(query_pool &&o) noexcept : __dev(o.__dev), __h(o.__h), __type(o.__type), __count(o.__count)
  {
    o.__dev = nullptr;
    o.__h = nullptr;
    o.__count = 0;
  }

  query_pool &operator=(const query_pool &) = delete;

  query_pool &
  operator=(query_pool &&o) noexcept
  {
    if ( this != &o ) {
      reset();
      __dev = o.__dev;
      __h = o.__h;
      __type = o.__type;
      __count = o.__count;
      o.__dev = nullptr;
      o.__h = nullptr;
      o.__count = 0;
    }
    return *this;
  }

  VkQueryPool
  handle() const noexcept
  {
    return __h;
  }

  VkQueryType
  type() const noexcept
  {
    return __type;
  }

  u32
  count() const noexcept
  {
    return __count;
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
  reset_range(u32 first, u32 count)
  {
    if ( !__h || !__dev || !vkResetQueryPool )
      throw except::library_error("vk::query_pool::reset_range: vkResetQueryPool unresolved (needs 1.2 host_query_reset)");
    vkResetQueryPool(__dev, __h, first, count);
  }

  VkResult
  get_results(u32 first, u32 count, void *out_buf, usize buf_size_bytes, VkDeviceSize stride, VkQueryResultFlags flags = {}) noexcept
  {
    if ( !__h || !__dev || !vkGetQueryPoolResults ) return VK_ERROR_INITIALIZATION_FAILED;
    return vkGetQueryPoolResults(__dev, __h, first, count, buf_size_bytes, out_buf, stride, flags);
  }

  void
  reset() noexcept
  {
    if ( __h && __dev && vkDestroyQueryPool ) vkDestroyQueryPool(__dev, __h, host_allocation_callbacks());
    __h = nullptr;
    __dev = nullptr;
    __count = 0;
  }
};

};      // namespace vk
};      // namespace gfx
};      // namespace micron
