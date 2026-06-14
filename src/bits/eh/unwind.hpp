//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "eh_config.hpp"

#if defined(__micron_eh)

#include "../../types.hpp"

extern "C" {

// reason codes returned by the unwinder and personality routine
typedef enum {
  _URC_NO_REASON = 0,
  _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
  _URC_FATAL_PHASE2_ERROR = 2,
  _URC_FATAL_PHASE1_ERROR = 3,
  _URC_NORMAL_STOP = 4,
  _URC_END_OF_STACK = 5,
  _URC_HANDLER_FOUND = 6,
  _URC_INSTALL_CONTEXT = 7,
  _URC_CONTINUE_UNWIND = 8,
  // ARM EHABI spells the success code differently for __gnu_unwind_frame
  _URC_OK = 0,
  _URC_FAILURE = 9
} _Unwind_Reason_Code;

// personality-routine "actions" bitmask
typedef int _Unwind_Action;
constexpr _Unwind_Action _UA_SEARCH_PHASE = 1;
constexpr _Unwind_Action _UA_CLEANUP_PHASE = 2;
constexpr _Unwind_Action _UA_HANDLER_FRAME = 4;
constexpr _Unwind_Action _UA_FORCE_UNWIND = 8;
constexpr _Unwind_Action _UA_END_OF_STACK = 16;

// word-sized integers as seen by the unwinder (pointer-sized on all our targets)
typedef usize _Unwind_Word;
typedef intptr_t _Unwind_Sword;
typedef usize _Unwind_Ptr;
typedef usize _Unwind_Internal_Ptr;
typedef u64 _Unwind_Exception_Class;

struct _Unwind_Exception;
struct _Unwind_Context;

typedef void (*_Unwind_Exception_Cleanup_Fn)(_Unwind_Reason_Code reason, _Unwind_Exception *exc);

// the exception object header the compiler/runtime pass around
#if defined(__micron_eh_ehabi)
// ARM EHABI _Unwind_Control_Block (aliased as _Unwind_Exception)
struct _Unwind_Exception {
  char exception_class[8];
  _Unwind_Exception_Cleanup_Fn exception_cleanup;

  struct {
    _Unwind_Word reserved1;      // init to 0, owned by the unwinder thereafter
    _Unwind_Word reserved2;
    _Unwind_Word reserved3;
    _Unwind_Word reserved4;
    _Unwind_Word reserved5;
  } unwinder_cache;

  struct {
    _Unwind_Word sp;
    _Unwind_Word bitpattern[5];
  } barrier_cache;

  struct {
    _Unwind_Word bitpattern[4];
  } cleanup_cache;

  struct {
    _Unwind_Word fnstart;         // function start address
    _Unwind_Word *ehtp;           // pointer to EHT entry header word
    _Unwind_Word additional;      // additional data
    _Unwind_Word reserved1;
  } pr_cache;

  long long int : 0;      // force 8-byte alignment
};
#else
struct _Unwind_Exception {
  _Unwind_Exception_Class exception_class;
  _Unwind_Exception_Cleanup_Fn exception_cleanup;
  _Unwind_Word private_1;
  _Unwind_Word private_2;
} __attribute__((__aligned__));
#endif

typedef _Unwind_Reason_Code (*_Unwind_Personality_Fn)(int version, _Unwind_Action actions, _Unwind_Exception_Class exception_class,
                                                      _Unwind_Exception *exception_object, _Unwind_Context *context);

_Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Exception *exception_object);
void _Unwind_Resume(_Unwind_Exception *exception_object) __attribute__((noreturn));
_Unwind_Reason_Code _Unwind_Resume_or_Rethrow(_Unwind_Exception *exception_object);
void _Unwind_DeleteException(_Unwind_Exception *exception_object);

typedef _Unwind_Reason_Code (*_Unwind_Stop_Fn)(int version, _Unwind_Action actions, _Unwind_Exception_Class exception_class,
                                               _Unwind_Exception *exception_object, _Unwind_Context *context, void *stop_parameter);
_Unwind_Reason_Code _Unwind_ForcedUnwind(_Unwind_Exception *exception_object, _Unwind_Stop_Fn stop, void *stop_parameter);

#if defined(__micron_eh_ehabi)
// EHABI: register access goes through the VRS (virtual register set) API
typedef enum { _UVRSC_CORE = 0, _UVRSC_VFP = 1, _UVRSC_WMMXD = 3, _UVRSC_WMMXC = 4 } _Unwind_VRS_RegClass;

typedef enum { _UVRSD_UINT32 = 0, _UVRSD_VFPX = 1, _UVRSD_UINT64 = 3, _UVRSD_FLOAT = 4, _UVRSD_DOUBLE = 5 } _Unwind_VRS_DataRepresentation;

typedef enum { _UVRSR_OK = 0, _UVRSR_NOT_IMPLEMENTED = 1, _UVRSR_FAILED = 2 } _Unwind_VRS_Result;

_Unwind_VRS_Result _Unwind_VRS_Get(_Unwind_Context *context, _Unwind_VRS_RegClass regclass, u32 regno, _Unwind_VRS_DataRepresentation repr,
                                   void *valuep);
_Unwind_VRS_Result _Unwind_VRS_Set(_Unwind_Context *context, _Unwind_VRS_RegClass regclass, u32 regno, _Unwind_VRS_DataRepresentation repr,
                                   void *valuep);
_Unwind_VRS_Result _Unwind_VRS_Pop(_Unwind_Context *context, _Unwind_VRS_RegClass regclass, u32 discriminator,
                                   _Unwind_VRS_DataRepresentation repr);
_Unwind_Reason_Code __gnu_unwind_frame(_Unwind_Exception *exception_object, _Unwind_Context *context);
#endif

_Unwind_Word _Unwind_GetGR(_Unwind_Context *context, int index);
void _Unwind_SetGR(_Unwind_Context *context, int index, _Unwind_Word value);
_Unwind_Ptr _Unwind_GetIP(_Unwind_Context *context);
_Unwind_Ptr _Unwind_GetIPInfo(_Unwind_Context *context, int *ip_before_insn);
void _Unwind_SetIP(_Unwind_Context *context, _Unwind_Ptr value);
_Unwind_Word _Unwind_GetCFA(_Unwind_Context *context);
_Unwind_Ptr _Unwind_GetLanguageSpecificData(_Unwind_Context *context);
_Unwind_Ptr _Unwind_GetRegionStart(_Unwind_Context *context);
_Unwind_Ptr _Unwind_GetDataRelBase(_Unwind_Context *context);
_Unwind_Ptr _Unwind_GetTextRelBase(_Unwind_Context *context);

};      // extern "C"

#endif      // __micron_eh
