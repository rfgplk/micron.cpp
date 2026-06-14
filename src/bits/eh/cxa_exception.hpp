//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "eh_config.hpp"

#if defined(__micron_eh)

#include "../../types.hpp"
#include "unwind.hpp"

// NOTE: must be std
namespace std
{
class type_info;
typedef void (*terminate_handler)();
typedef void (*unexpected_handler)();
}      // namespace std

namespace __cxxabiv1
{

using std::type_info;

// per thread current exception bookkeeping (Itanium ABI 3.3.2)
struct __cxa_eh_globals {
  void *caughtExceptions;      // __cxa_exception*
  unsigned int uncaughtExceptions;
#if defined(__micron_eh_ehabi)
  void *propagatingExceptions;      // ARM: chain of exceptions being propagated
#endif
};

struct __cxa_exception {
  type_info *exceptionType;
  void (*exceptionDestructor)(void *);
  std::unexpected_handler unexpectedHandler;
  std::terminate_handler terminateHandler;
  __cxa_exception *nextException;

  int handlerCount;

#if defined(__micron_eh_ehabi)
  // ARM chains exceptions being propagated
  __cxa_exception *nextPropagatingException;
  int propagationCount;
#endif

  int handlerSwitchValue;
  const unsigned char *actionRecord;
  const unsigned char *languageSpecificData;
  _Unwind_Ptr catchTemp;
  void *adjustedPtr;

  _Unwind_Exception unwindHeader;      // MUST be last; object follows
};

struct __cxa_dependent_exception {
  type_info *exceptionType;
  void (*exceptionDestructor)(void *);
  std::unexpected_handler unexpectedHandler;
  std::terminate_handler terminateHandler;
  __cxa_exception *nextException;

  int handlerCount;

#if defined(__micron_eh_ehabi)
  __cxa_exception *nextPropagatingException;
  int propagationCount;
#endif

  int handlerSwitchValue;
  const unsigned char *actionRecord;
  const unsigned char *languageSpecificData;
  _Unwind_Ptr catchTemp;
  void *adjustedPtr;

  void *primaryException;      // the original __cxa_exception this depends on

  _Unwind_Exception unwindHeader;      // MUST be last
};

// the gnu g++ exception class tag: "GNUCC++\0" big-endian
constexpr _Unwind_Exception_Class __gxx_exception_class = 0x474e5543432b2b00ULL;                // "GNUCC++\0"
constexpr _Unwind_Exception_Class __gxx_dependent_exception_class = 0x474e5543432b2b01ULL;      // "GNUCC++\1"

inline __cxa_exception *
__get_exception_header_from_ue(_Unwind_Exception *ue) noexcept
{
  return reinterpret_cast<__cxa_exception *>(reinterpret_cast<byte *>(ue) - __builtin_offsetof(__cxa_exception, unwindHeader));
}

inline _Unwind_Exception *
__get_ue_from_object(void *obj) noexcept
{
  return &(reinterpret_cast<__cxa_exception *>(obj) - 1)->unwindHeader;
}

inline __cxa_exception *
__get_exception_header_from_object(void *obj) noexcept
{
  return reinterpret_cast<__cxa_exception *>(obj) - 1;
}

inline void *
__get_object_from_header(__cxa_exception *h) noexcept
{
  return reinterpret_cast<void *>(h + 1);
}

inline void *
__get_object_from_ue(_Unwind_Exception *ue) noexcept
{
  return __get_object_from_header(__get_exception_header_from_ue(ue));
}

}      // namespace __cxxabiv1

namespace abi = __cxxabiv1;

#endif      // __micron_eh
