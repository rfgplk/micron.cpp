//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/allocation/wasmalloc/wasmalloc.hpp"
#include "../eh/cxa_eh_globals.hpp"
#include "../eh/dwarf_enc.hpp"
#include "eh_std.hpp"
#include "eh_typeinfo.hpp"
#include "runtime.hpp"

// LLVM 22's native Wasm EH payload is the address of this Itanium unwind header.
asm(".globl __cpp_exception\n.tagtype __cpp_exception i32\n__cpp_exception:");

namespace micron::wasm::eh
{
struct lpad_context {
  unsigned index;
  const byte *lsda;
  unsigned selector;
};

inline std::terminate_handler terminate_handler = nullptr;
alignas(16) inline byte emergency[16][2048]{};
inline unsigned emergency_used[16]{};
inline thread_local __cxxabiv1::__cxa_exception *allocations = nullptr;

inline void
link(__cxxabiv1::__cxa_exception *h) noexcept
{
  // This VM unwinder owns the private words; no native stack walk uses them.
  h->unwindHeader.private_1 = 0;
  h->unwindHeader.private_2 = reinterpret_cast<usize>(allocations);
  if ( allocations ) allocations->unwindHeader.private_1 = reinterpret_cast<usize>(h);
  allocations = h;
}

inline void
unlink(__cxxabiv1::__cxa_exception *h) noexcept
{
  auto *previous = reinterpret_cast<__cxxabiv1::__cxa_exception *>(h->unwindHeader.private_1);
  auto *next = reinterpret_cast<__cxxabiv1::__cxa_exception *>(h->unwindHeader.private_2);
  if ( previous )
    previous->unwindHeader.private_2 = reinterpret_cast<usize>(next);
  else
    allocations = next;
  if ( next ) next->unwindHeader.private_1 = reinterpret_cast<usize>(previous);
}

[[noreturn]] inline void
terminate() noexcept
{
  if ( auto fn = __atomic_load_n(&terminate_handler, __ATOMIC_ACQUIRE) ) fn();
  write_diag("micron: terminate called\n");
  panic(134);
}

inline bool
match(const std::type_info *caught, __cxxabiv1::__cxa_exception *h, void *&adjusted) noexcept
{
  if ( !caught ) return true;
  if ( std::__micron_ti_strcmp(h->exceptionType->name(), "Dn") == 0 && caught->name()[0] == 'M' ) {
    const auto *member = static_cast<const __cxxabiv1::__pbase_type_info *>(caught);
    auto *words = static_cast<usize *>(adjusted);
    words[0] = member->__pointee->__is_function_p() ? 0 : ~usize(0);
    words[1] = 0;
    return true;
  }
  if ( h->exceptionType->__is_pointer_p() ) adjusted = *static_cast<void **>(adjusted);
  return caught->__do_catch(h->exceptionType, &adjusted, 1);
}
}      // namespace micron::wasm::eh

extern "C" {
__attribute__((used)) inline thread_local micron::wasm::eh::lpad_context __wasm_lpad_context{};

__attribute__((weak, used)) void *
__cxa_allocate_exception(usize size) noexcept
{
  using namespace micron::wasm::eh;
  constexpr usize overhead = sizeof(__cxxabiv1::__cxa_exception);
  if ( size > ~usize(0) - overhead ) terminate();
  const usize total = overhead + (size < 8 ? 8 : size);
  void *p = micron::wasmalloc::try_alloc(total);
  if ( !p && total <= sizeof(emergency[0]) ) {
    for ( unsigned i = 0; i < 16; ++i ) {
      unsigned expected = 0;
      if ( __atomic_compare_exchange_n(&emergency_used[i], &expected, 1u, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED) ) {
        p = emergency[i];
        break;
      }
    }
  }
  if ( !p ) terminate();
  auto *h = static_cast<__cxxabiv1::__cxa_exception *>(p);
  *h = {};
  link(h);
  return h + 1;
}

__attribute__((weak, used)) void
__cxa_free_exception(void *obj) noexcept
{
  using namespace micron::wasm::eh;
  auto *p = static_cast<__cxxabiv1::__cxa_exception *>(obj) - 1;
  unlink(p);
  const usize a = reinterpret_cast<usize>(p), b = reinterpret_cast<usize>(emergency);
  if ( a >= b && a < b + sizeof(emergency) )
    __atomic_store_n(&emergency_used[(a - b) / sizeof(emergency[0])], 0u, __ATOMIC_RELEASE);
  else
    micron::wasmalloc::dealloc(p);
}

__attribute__((weak, used)) void
_Unwind_DeleteException(_Unwind_Exception *ue)
{
  if ( ue->exception_cleanup ) ue->exception_cleanup(_URC_FOREIGN_EXCEPTION_CAUGHT, ue);
}

__attribute__((weak, used)) _Unwind_Reason_Code
_Unwind_RaiseException(_Unwind_Exception *ue)
{
  __builtin_wasm_throw(0, ue);
  __builtin_unreachable();
}

__attribute__((weak, used, noreturn)) void
_Unwind_Resume(_Unwind_Exception *ue)
{
  __builtin_wasm_throw(0, ue);
  __builtin_unreachable();
}

__attribute__((weak, used)) _Unwind_Reason_Code
_Unwind_Resume_or_Rethrow(_Unwind_Exception *ue)
{
  __builtin_wasm_throw(0, ue);
  __builtin_unreachable();
}

inline void
__micron_wasm_exception_cleanup(_Unwind_Reason_Code, _Unwind_Exception *ue)
{
  auto *h = __cxxabiv1::__get_exception_header_from_ue(ue);
  void *obj = h + 1;
  const auto dtor = h->exceptionDestructor;
  h->exceptionDestructor = nullptr;
  // Wasm's C++ ABI returns this from complete/base destructors.
  if ( dtor ) (void)reinterpret_cast<void *(*)(void *)>(dtor)(obj);
  __cxa_free_exception(obj);
}

__attribute__((weak, used)) void
__micron_wasm_finish_exceptions() noexcept
{
  auto *g = __cxa_get_globals();
  g->caughtExceptions = nullptr;
  g->uncaughtExceptions = 0;
  // A checkpoint may abandon catches, propagation, or an unfinished throw
  // construction. Only fully constructed, not-yet-destroyed objects have a dtor.
  while ( auto *h = micron::wasm::eh::allocations ) __micron_wasm_exception_cleanup(_URC_NO_REASON, &h->unwindHeader);
  __wasm_lpad_context = {};
}

__attribute__((weak, used, noreturn)) void
__cxa_throw(void *obj, void *type, void (*dtor)(void *))
{
  auto *h = __cxxabiv1::__get_exception_header_from_object(obj);
  h->exceptionType = static_cast<std::type_info *>(type);
  h->exceptionDestructor = dtor;
  h->adjustedPtr = obj;
  h->unwindHeader.exception_class = __cxxabiv1::__gxx_exception_class;
  h->unwindHeader.exception_cleanup = __micron_wasm_exception_cleanup;
  ++__cxa_get_globals()->uncaughtExceptions;
  __builtin_wasm_throw(0, &h->unwindHeader);
  __builtin_unreachable();
}

__attribute__((weak, used, noreturn)) void
__cxa_bad_cast()
{
  throw std::bad_cast{};
}

__attribute__((weak, used, noreturn)) void
__cxa_bad_typeid()
{
  throw std::bad_typeid{};
}

__attribute__((weak, used)) void *
__cxa_get_exception_ptr(void *ue) noexcept
{
  return __cxxabiv1::__get_exception_header_from_ue(static_cast<_Unwind_Exception *>(ue))->adjustedPtr;
}

__attribute__((weak, used)) void *
__cxa_begin_catch(void *ue) noexcept
{
  auto *h = __cxxabiv1::__get_exception_header_from_ue(static_cast<_Unwind_Exception *>(ue));
  auto *g = __cxa_get_globals();
  h->handlerCount = h->handlerCount < 0 ? -h->handlerCount + 1 : h->handlerCount + 1;
  if ( g->caughtExceptions != h ) {
    h->nextException = static_cast<__cxxabiv1::__cxa_exception *>(g->caughtExceptions);
    g->caughtExceptions = h;
  }
  if ( g->uncaughtExceptions ) --g->uncaughtExceptions;
  return h->adjustedPtr;
}

__attribute__((weak, used)) void
__cxa_end_catch()
{
  auto *g = __cxa_get_globals();
  auto *h = static_cast<__cxxabiv1::__cxa_exception *>(g->caughtExceptions);
  if ( !h ) return;
  if ( h->handlerCount < 0 ) {
    if ( ++h->handlerCount == 0 ) g->caughtExceptions = h->nextException;
  } else if ( --h->handlerCount == 0 ) {
    g->caughtExceptions = h->nextException;
    _Unwind_DeleteException(&h->unwindHeader);
  }
}

__attribute__((weak, used, noreturn)) void
__cxa_rethrow()
{
  auto *g = __cxa_get_globals();
  auto *h = static_cast<__cxxabiv1::__cxa_exception *>(g->caughtExceptions);
  if ( !h ) micron::wasm::eh::terminate();
  ++g->uncaughtExceptions;
  h->handlerCount = -h->handlerCount;
  __builtin_wasm_throw(0, &h->unwindHeader);
  __builtin_unreachable();
}

__attribute__((weak, used)) void *
__cxa_current_exception_type() noexcept
{
  auto *h = static_cast<__cxxabiv1::__cxa_exception *>(__cxa_get_globals()->caughtExceptions);
  return h ? h->exceptionType : nullptr;
}

__attribute__((weak, used, noreturn)) void
__cxa_call_terminate(void *)
{
  micron::wasm::eh::terminate();
}

__attribute__((weak, used, noreturn)) void
__cxa_call_unexpected(void *)
{
  micron::wasm::eh::terminate();
}

__attribute__((weak, used)) int
_Unwind_CallPersonality(void *exception)
{
  using namespace micron::eh;
  auto &ctx = __wasm_lpad_context;
#if defined(MICRON_WASM_EH_TRACE)
  micron::wasm::write_diag("personality\n");
#endif
  ctx.selector = 0;
  if ( !ctx.lsda ) return _URC_CONTINUE_UNWIND;
  const byte *p = ctx.lsda;
  const u8 landing_encoding = *p++;
  if ( landing_encoding != DW_EH_PE_omit ) (void)read_encoded(landing_encoding, p, {});
  const u8 type_encoding = *p++;
  const byte *type_table = nullptr;
  if ( type_encoding != DW_EH_PE_omit ) {
    const usize off = read_uleb128(p);
    type_table = p + off;
  }
  const u8 call_encoding = *p++;
  if ( call_encoding != DW_EH_PE_uleb128 ) micron::wasm::eh::terminate();
  const usize call_length = read_uleb128(p);
  const byte *action_table = p + call_length;
  usize action = 0;
  bool found = false;
  usize ordinal = 0;
  while ( p < action_table ) {
    (void)read_uleb128(p);      // landing-pad number; the VM already selected it
    const usize a = read_uleb128(p);
    if ( ordinal++ == ctx.index ) {
      action = a;
      found = true;
      break;
    }
  }
  if ( !found || !action ) return _URC_CONTINUE_UNWIND;
  auto *ue = static_cast<_Unwind_Exception *>(exception);
  if ( ue->exception_class != __cxxabiv1::__gxx_exception_class ) micron::wasm::eh::terminate();
  auto *h = __cxxabiv1::__get_exception_header_from_ue(ue);
  p = action_table + action - 1;
  for ( ;; ) {
    const i64 filter = read_sleb128(p);
    const byte *next_base = p;
    const i64 next = read_sleb128(p);
    if ( filter > 0 ) {
      const usize width = encoded_size(type_encoding);
      if ( !type_table || !width ) micron::wasm::eh::terminate();
      const byte *entry = type_table - static_cast<usize>(filter) * width;
      const auto *caught = reinterpret_cast<const std::type_info *>(read_encoded(type_encoding, entry, {}));
#if defined(MICRON_WASM_EH_TRACE)
      micron::wasm::write_diag(caught ? caught->name() : "...");
      micron::wasm::write_diag(" <- ");
      micron::wasm::write_diag(h->exceptionType->name());
      micron::wasm::write_diag("\n");
#endif
      void *adjusted = h + 1;
      if ( micron::wasm::eh::match(caught, h, adjusted) ) {
        h->adjustedPtr = adjusted;
        ctx.selector = static_cast<unsigned>(filter);
        return _URC_HANDLER_FOUND;
      }
    } else if ( filter < 0 ) {
      // Dynamic exception specifications were removed before Micron's C++23 floor.
      micron::wasm::eh::terminate();
    }
    if ( !next ) break;
    p = next_base + next;
  }
  return _URC_CONTINUE_UNWIND;
}
}

namespace std
{
[[gnu::weak, gnu::used, noreturn]] void
terminate() noexcept
{
  micron::wasm::eh::terminate();
}

inline terminate_handler
set_terminate(terminate_handler h) noexcept
{
  return __atomic_exchange_n(&micron::wasm::eh::terminate_handler, h, __ATOMIC_ACQ_REL);
}

inline terminate_handler
get_terminate() noexcept
{
  return __atomic_load_n(&micron::wasm::eh::terminate_handler, __ATOMIC_ACQUIRE);
}

inline int
uncaught_exceptions() noexcept
{
  return static_cast<int>(__cxa_get_globals()->uncaughtExceptions);
}
}      // namespace std
