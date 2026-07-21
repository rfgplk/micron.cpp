//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "eh_config.hpp"

#if defined(__micron_eh)

#include "../../exit.hpp"
#include "../../syscall.hpp"
#include "../../types.hpp"
#include "cxa_eh_globals.hpp"
#include "cxa_exception.hpp"
#include "terminate.hpp"
#include "typeinfo.hpp"
#include "unwind.hpp"

namespace micron::eh
{

// this allocator is self contained; otherwise we'd risk pulling spaghettified abcmalloc and self recursing
constexpr int __eh_prot_rw = 0x1 | 0x2;             // PROT_READ | PROT_WRITE
constexpr int __eh_map_priv_anon = 0x2 | 0x20;      // MAP_PRIVATE | MAP_ANONYMOUS
constexpr usize __eh_page = 4096;
constexpr usize __eh_prefix = 16;      // stores the mapping size; keeps the object 16-aligned

inline void *
__eh_raw_map(usize sz) noexcept
{
#if defined(__micron_arch_width_32)
  const long r = micron::syscall(SYS_mmap2, 0, sz, __eh_prot_rw, __eh_map_priv_anon, -1, 0);
#else
  const long r = micron::syscall(SYS_mmap, 0, sz, __eh_prot_rw, __eh_map_priv_anon, -1, 0);
#endif
  if ( static_cast<unsigned long>(r) >= static_cast<unsigned long>(-4095) ) return nullptr;
  return reinterpret_cast<void *>(r);
}

inline void
__eh_raw_unmap(void *p, usize sz) noexcept
{
  micron::syscall(SYS_munmap, p, sz);
}

constexpr usize __emergency_slots = 16;
constexpr usize __emergency_slot_size = 1024;
alignas(16) inline byte __emergency_pool[__emergency_slots][__emergency_slot_size];
inline bool __emergency_used[__emergency_slots] = {};

inline bool
__is_emergency(void *p) noexcept
{
  return p >= reinterpret_cast<byte *>(__emergency_pool) && p < reinterpret_cast<byte *>(__emergency_pool) + sizeof(__emergency_pool);
}

};      // namespace micron::eh

extern "C" {

inline void *
__cxa_allocate_exception(usize thrown_size) noexcept
{
  using namespace micron::eh;
  // layout: [16B prefix: map_size][__cxa_exception header][thrown object]
  const usize need = __eh_prefix + sizeof(__cxxabiv1::__cxa_exception) + thrown_size;
  byte *base = nullptr;
  usize map_size = 0;

  const usize rounded = (need + __eh_page - 1) & ~(__eh_page - 1);
  base = reinterpret_cast<byte *>(__eh_raw_map(rounded));
  if ( base )
    map_size = rounded;
  else {
    // mmap failed: fall back to a static emergency slot if it fits
    if ( need <= __emergency_slot_size ) {
      for ( usize i = 0; i < __emergency_slots; ++i ) {
        if ( !__emergency_used[i] ) {
          __emergency_used[i] = true;
          base = __emergency_pool[i];
          map_size = 0;      // 0 marks an emergency (non-mapped) allocation
          break;
        }
      }
    }
  }
  if ( !base ) micron::eh::__terminate();

  *reinterpret_cast<usize *>(base) = map_size;
  byte *hdr = base + __eh_prefix;
  __builtin_memset(hdr, 0, sizeof(__cxxabiv1::__cxa_exception));
  return hdr + sizeof(__cxxabiv1::__cxa_exception);      // 16-aligned (page + 16 + multiple-of-16)
}

inline void
__cxa_free_exception(void *thrown_object) noexcept
{
  using namespace micron::eh;
  byte *hdr = reinterpret_cast<byte *>(__cxxabiv1::__get_exception_header_from_object(thrown_object));
  byte *base = hdr - __eh_prefix;
  const usize map_size = *reinterpret_cast<usize *>(base);
  if ( __is_emergency(base) ) {
    const usize idx = static_cast<usize>(base - &__emergency_pool[0][0]) / __emergency_slot_size;
    if ( idx < __emergency_slots ) __emergency_used[idx] = false;
  } else if ( map_size ) {
    __eh_raw_unmap(base, map_size);
  }
}

inline void
__gxx_exception_cleanup(_Unwind_Reason_Code, _Unwind_Exception *ue)
{
  __cxxabiv1::__cxa_exception *h = __cxxabiv1::__get_exception_header_from_ue(ue);
  void *obj = __cxxabiv1::__get_object_from_header(h);
  if ( h->exceptionDestructor ) h->exceptionDestructor(obj);
  __cxa_free_exception(obj);
}

[[noreturn]] inline void
__cxa_throw(void *thrown_object, void *tinfo, void (*dtor)(void *))
{
  using namespace micron::eh;
  __cxxabiv1::__cxa_exception *h = __cxxabiv1::__get_exception_header_from_object(thrown_object);
  h->exceptionType = reinterpret_cast<std::type_info *>(tinfo);
  h->exceptionDestructor = dtor;
  h->unexpectedHandler = micron::eh::__unexpected_handler_slot;
  h->terminateHandler = micron::eh::__terminate_handler_slot;
#if defined(__micron_eh_ehabi)
  __builtin_memcpy(h->unwindHeader.exception_class, &__cxxabiv1::__gxx_exception_class, 8);
#else
  h->unwindHeader.exception_class = __cxxabiv1::__gxx_exception_class;
#endif
  h->unwindHeader.exception_cleanup = __gxx_exception_cleanup;

  __cxxabiv1::__cxa_eh_globals *g = __cxa_get_globals();
  g->uncaughtExceptions += 1;

  _Unwind_RaiseException(&h->unwindHeader);

  micron::eh::__terminate();
}

inline void *
__cxa_get_exception_ptr(void *ue_) noexcept
{
  _Unwind_Exception *ue = reinterpret_cast<_Unwind_Exception *>(ue_);
  return __cxxabiv1::__get_exception_header_from_ue(ue)->adjustedPtr;
}

inline void *
__cxa_begin_catch(void *ue_) noexcept
{
  using namespace micron::eh;
  _Unwind_Exception *ue = reinterpret_cast<_Unwind_Exception *>(ue_);
  __cxxabiv1::__cxa_exception *h = __cxxabiv1::__get_exception_header_from_ue(ue);
  __cxxabiv1::__cxa_eh_globals *g = __cxa_get_globals();

  // bump handler count (negative count == currently being rethrown)
  h->handlerCount = h->handlerCount < 0 ? -h->handlerCount + 1 : h->handlerCount + 1;

  // push onto the caught stack (unless it is already the top, e.g. re-catch)
  if ( h != reinterpret_cast<__cxxabiv1::__cxa_exception *>(g->caughtExceptions) ) {
    h->nextException = reinterpret_cast<__cxxabiv1::__cxa_exception *>(g->caughtExceptions);
    g->caughtExceptions = h;
  }
  g->uncaughtExceptions -= 1;
  return h->adjustedPtr;
}

inline void
__cxa_end_catch()
{
  using namespace micron::eh;
  __cxxabiv1::__cxa_eh_globals *g = __cxa_get_globals_fast();
  __cxxabiv1::__cxa_exception *h = reinterpret_cast<__cxxabiv1::__cxa_exception *>(g->caughtExceptions);
  if ( !h ) return;

  int count = h->handlerCount;
  if ( count < 0 ) {
    // exception was rethrown; on the last matching end_catch, pop it
    if ( ++count == 0 ) g->caughtExceptions = h->nextException;
    h->handlerCount = count;
  } else if ( --count == 0 ) {
    g->caughtExceptions = h->nextException;
    h->handlerCount = count;
    _Unwind_DeleteException(&h->unwindHeader);      // runs dtor + frees
  } else {
    h->handlerCount = count;
  }
}

[[noreturn]] inline void
__cxa_rethrow()
{
  using namespace micron::eh;
  __cxxabiv1::__cxa_eh_globals *g = __cxa_get_globals();
  __cxxabiv1::__cxa_exception *h = reinterpret_cast<__cxxabiv1::__cxa_exception *>(g->caughtExceptions);
  if ( !h ) micron::eh::__terminate();      // rethrow with no active exception

  g->uncaughtExceptions += 1;
  h->handlerCount = -h->handlerCount;      // mark as being rethrown

  _Unwind_Resume_or_Rethrow(&h->unwindHeader);
  micron::eh::__terminate();
}

inline void *
__cxa_current_exception_type() noexcept
{
  using namespace micron::eh;
  __cxxabiv1::__cxa_eh_globals *g = __cxa_get_globals_fast();
  __cxxabiv1::__cxa_exception *h = reinterpret_cast<__cxxabiv1::__cxa_exception *>(g->caughtExceptions);
  return h ? h->exceptionType : nullptr;
}

}      // extern "C"

#endif      // __micron_eh
