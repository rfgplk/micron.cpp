//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../except.hpp"
#include "../../types.hpp"

#include "vulkan.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

// primary errors + error handling file
// TODO: stub for now, expand greatly later

inline const char *
result_name(VkResult r) noexcept
{
  switch ( r ) {
  case VK_SUCCESS:
    return "VK_SUCCESS";
  case VK_NOT_READY:
    return "VK_NOT_READY";
  case VK_TIMEOUT:
    return "VK_TIMEOUT";
  case VK_EVENT_SET:
    return "VK_EVENT_SET";
  case VK_EVENT_RESET:
    return "VK_EVENT_RESET";
  case VK_INCOMPLETE:
    return "VK_INCOMPLETE";
  case VK_ERROR_OUT_OF_HOST_MEMORY:
    return "VK_ERROR_OUT_OF_HOST_MEMORY";
  case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
  case VK_ERROR_INITIALIZATION_FAILED:
    return "VK_ERROR_INITIALIZATION_FAILED";
  case VK_ERROR_DEVICE_LOST:
    return "VK_ERROR_DEVICE_LOST";
  case VK_ERROR_MEMORY_MAP_FAILED:
    return "VK_ERROR_MEMORY_MAP_FAILED";
  case VK_ERROR_LAYER_NOT_PRESENT:
    return "VK_ERROR_LAYER_NOT_PRESENT";
  case VK_ERROR_EXTENSION_NOT_PRESENT:
    return "VK_ERROR_EXTENSION_NOT_PRESENT";
  case VK_ERROR_FEATURE_NOT_PRESENT:
    return "VK_ERROR_FEATURE_NOT_PRESENT";
  case VK_ERROR_INCOMPATIBLE_DRIVER:
    return "VK_ERROR_INCOMPATIBLE_DRIVER";
  case VK_ERROR_TOO_MANY_OBJECTS:
    return "VK_ERROR_TOO_MANY_OBJECTS";
  case VK_ERROR_FORMAT_NOT_SUPPORTED:
    return "VK_ERROR_FORMAT_NOT_SUPPORTED";
  case VK_ERROR_FRAGMENTED_POOL:
    return "VK_ERROR_FRAGMENTED_POOL";
  case VK_ERROR_UNKNOWN:
    return "VK_ERROR_UNKNOWN";
  case VK_ERROR_OUT_OF_POOL_MEMORY:
    return "VK_ERROR_OUT_OF_POOL_MEMORY";
  case VK_ERROR_INVALID_EXTERNAL_HANDLE:
    return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
  case VK_ERROR_FRAGMENTATION:
    return "VK_ERROR_FRAGMENTATION";
  case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
    return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
  case VK_PIPELINE_COMPILE_REQUIRED:
    return "VK_PIPELINE_COMPILE_REQUIRED";
  case VK_ERROR_SURFACE_LOST_KHR:
    return "VK_ERROR_SURFACE_LOST_KHR";
  case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
    return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
  case VK_SUBOPTIMAL_KHR:
    return "VK_SUBOPTIMAL_KHR";
  case VK_ERROR_OUT_OF_DATE_KHR:
    return "VK_ERROR_OUT_OF_DATE_KHR";
  default:
    return "VK_RESULT_UNKNOWN";
  }
}

inline void
check_vk(VkResult r, const char *where)
{
  if ( r == VK_SUCCESS ) return;
  (void)where;
  throw except::library_error(result_name(r));
}

inline VkResult
check_vk_ok(VkResult r, const char *where)
{
  switch ( r ) {
  case VK_SUCCESS:
  case VK_NOT_READY:
  case VK_TIMEOUT:
  case VK_EVENT_SET:
  case VK_EVENT_RESET:
  case VK_INCOMPLETE:
  case VK_SUBOPTIMAL_KHR:
    return r;
  default:
    (void)where;
    throw except::library_error(result_name(r));
  }
}

};      // namespace vk
};      // namespace gfx
};      // namespace micron
