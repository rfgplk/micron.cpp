//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron freestanding C++ exception runtime

#include "eh/eh_config.hpp"

#if defined(__micron_eh)

#include "eh/unwind.hpp"

#include "eh/cxa_eh_globals.hpp"
#include "eh/cxa_exception.hpp"
#include "eh/dwarf_enc.hpp"
#include "eh/typeinfo.hpp"

#if defined(__micron_eh_dwarf)
#include "eh/dwarf_cfi.hpp"
#include "eh/find_fde.hpp"
#include "eh/unwind_dwarf.hpp"
#endif

#if defined(__micron_eh_ehabi)
#include "eh/unwind_ehabi.hpp"
#endif

#include "eh/cxa_throw.hpp"
#include "eh/personality.hpp"
#include "eh/terminate.hpp"

namespace micron::eh
{

[[gnu::used, maybe_unused]] inline const void *const __eh_force_emit[] = {
  reinterpret_cast<const void *>(&__cxa_allocate_exception),
  reinterpret_cast<const void *>(&__cxa_free_exception),
  reinterpret_cast<const void *>(&__cxa_throw),
  reinterpret_cast<const void *>(&__cxa_begin_catch),
  reinterpret_cast<const void *>(&__cxa_end_catch),
  reinterpret_cast<const void *>(&__cxa_rethrow),
  reinterpret_cast<const void *>(&__cxa_get_exception_ptr),
  reinterpret_cast<const void *>(&__cxa_current_exception_type),
  reinterpret_cast<const void *>(&__cxa_get_globals),
  reinterpret_cast<const void *>(&__cxa_get_globals_fast),
  reinterpret_cast<const void *>(&__cxa_call_terminate),
  reinterpret_cast<const void *>(&__cxa_call_unexpected),
  reinterpret_cast<const void *>(&__cxa_pure_virtual),
  reinterpret_cast<const void *>(&__gxx_personality_v0),
  reinterpret_cast<const void *>(&_Unwind_RaiseException),
  reinterpret_cast<const void *>(&_Unwind_Resume),
  reinterpret_cast<const void *>(&_Unwind_Resume_or_Rethrow),
  reinterpret_cast<const void *>(&_Unwind_DeleteException),
  reinterpret_cast<const void *>(&_Unwind_ForcedUnwind),
#if defined(__micron_eh_ehabi)
  reinterpret_cast<const void *>(&__aeabi_unwind_cpp_pr0),
  reinterpret_cast<const void *>(&__aeabi_unwind_cpp_pr1),
  reinterpret_cast<const void *>(&__aeabi_unwind_cpp_pr2),
  reinterpret_cast<const void *>(&__gnu_unwind_frame),
  reinterpret_cast<const void *>(&__cxa_begin_cleanup),
  reinterpret_cast<const void *>(&__cxa_end_cleanup),
  reinterpret_cast<const void *>(&_Unwind_Complete),
  reinterpret_cast<const void *>(&_Unwind_VRS_Get),
  reinterpret_cast<const void *>(&_Unwind_VRS_Set),
#endif
};

#if defined(__micron_eh_dwarf)
[[gnu::constructor, maybe_unused]] static void
__eh_register_init() noexcept
{
  register_eh_frame();
}
#endif

}      // namespace micron::eh

#endif      // __micron_eh
