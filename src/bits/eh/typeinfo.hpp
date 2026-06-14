//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "eh_config.hpp"

#if defined(__micron_eh)

#include "../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// freestanding RTTI
//
// the compiler emits typeinfo for X objects (_ZTI...) whenever it sees a throw with -frrti,
//   _ZTVN10__cxxabiv117__class_type_infoE        (__class_type_info)
//   _ZTVN10__cxxabiv120__si_class_type_infoE     (__si_class_type_info)
//   _ZTVN10__cxxabiv123__fundamental_type_infoE  (__fundamental_type_info)
//   _ZTVN10__cxxabiv119__pointer_type_infoE      (__pointer_type_info)
// LAYOUTS and vtable VIRTUAL ORDERS are fixed by the Itanium C++ ABI

namespace __cxxabiv1
{
class __class_type_info;
};

// must be std namespace
namespace std
{

inline int
__micron_ti_strcmp(const char *a, const char *b) noexcept
{
  while ( *a && (*a == *b) ) {
    ++a;
    ++b;
  }
  return static_cast<int>(static_cast<unsigned char>(*a)) - static_cast<int>(static_cast<unsigned char>(*b));
}

// ABI layout: { vptr; const char* __name; }. sizeof == 2 words
class type_info
{
public:
  virtual ~type_info();

  const char *
  name() const noexcept
  {
    return __name[0] == '*' ? __name + 1 : __name;
  }

  bool
  before(const type_info &__arg) const noexcept
  {
    return (__name[0] == '*' && __arg.__name[0] == '*') ? __name < __arg.__name : std::__micron_ti_strcmp(__name, __arg.__name) < 0;
  }

  bool
  operator==(const type_info &__arg) const noexcept
  {
    if ( __name == __arg.__name ) return true;
    return __name[0] != '*' && std::__micron_ti_strcmp(__name, __arg.__name) == 0;
  }

  bool
  operator!=(const type_info &__arg) const noexcept
  {
    return !operator==(__arg);
  }

  virtual bool __is_pointer_p() const;
  virtual bool __is_function_p() const;
  virtual bool __do_catch(const type_info *__thr_type, void **__thr_obj, unsigned __outer) const;
  virtual bool __do_upcast(const __cxxabiv1::__class_type_info *__target, void **__obj_ptr) const;

  const char *__name;

protected:
  explicit type_info(const char *__n) : __name(__n) { }

private:
  type_info &operator=(const type_info &) = delete;
  type_info(const type_info &) = delete;
};

};      // namespace std

namespace __cxxabiv1
{

using std::type_info;

class __fundamental_type_info: public std::type_info
{
public:
  explicit __fundamental_type_info(const char *__n) : std::type_info(__n) { }

  virtual ~__fundamental_type_info();
};

class __array_type_info: public std::type_info
{
public:
  explicit __array_type_info(const char *__n) : std::type_info(__n) { }

  virtual ~__array_type_info();
};

class __function_type_info: public std::type_info
{
public:
  explicit __function_type_info(const char *__n) : std::type_info(__n) { }

  virtual ~__function_type_info();
  virtual bool __is_function_p() const;
};

class __enum_type_info: public std::type_info
{
public:
  explicit __enum_type_info(const char *__n) : std::type_info(__n) { }

  virtual ~__enum_type_info();
};

// %%% class types %%%
class __class_type_info: public std::type_info
{
public:
  explicit __class_type_info(const char *__n) : std::type_info(__n) { }

  virtual ~__class_type_info();

  virtual bool __is_pointer_p() const;
  virtual bool __do_catch(const type_info *__thr_type, void **__thr_obj, unsigned __outer) const;
  virtual bool __do_upcast(const __class_type_info *__target, void **__obj_ptr) const;
};

class __si_class_type_info: public __class_type_info
{
public:
  explicit __si_class_type_info(const char *__n, const __class_type_info *__base) : __class_type_info(__n), __base_type(__base) { }

  virtual ~__si_class_type_info();

  virtual bool __do_upcast(const __class_type_info *__target, void **__obj_ptr) const;

  const __class_type_info *__base_type;
};

struct __base_class_type_info {
  const __class_type_info *__base_type;
  long __offset_flags;

  enum __offset_flags_masks { __virtual_mask = 0x1, __public_mask = 0x2, __offset_shift = 8 };
};

class __vmi_class_type_info: public __class_type_info
{
public:
  explicit __vmi_class_type_info(const char *__n, int) : __class_type_info(__n) { }

  virtual ~__vmi_class_type_info();

  virtual bool __do_upcast(const __class_type_info *__target, void **__obj_ptr) const;

  unsigned int __flags;
  unsigned int __base_count;
  __base_class_type_info __base_info[1];

  enum __flags_masks { __non_diamond_repeat_mask = 0x1, __diamond_shaped_mask = 0x2 };
};

class __pbase_type_info: public std::type_info
{
public:
  explicit __pbase_type_info(const char *__n, int __quals, const std::type_info *__type)
      : std::type_info(__n), __flags(static_cast<unsigned>(__quals)), __pointee(__type)
  {
  }

  virtual ~__pbase_type_info();

  virtual bool __do_catch(const type_info *__thr_type, void **__thr_obj, unsigned __outer) const;

  unsigned int __flags;
  const std::type_info *__pointee;

  enum __masks {
    __const_mask = 0x1,
    __volatile_mask = 0x2,
    __restrict_mask = 0x4,
    __incomplete_mask = 0x8,
    __incomplete_class_mask = 0x10,
    __transaction_safe_mask = 0x20,
    __noexcept_mask = 0x40
  };

protected:
  virtual bool __pointer_catch(const __pbase_type_info *__thr_type, void **__thr_obj, unsigned __outer) const;
};

class __pointer_type_info: public __pbase_type_info
{
public:
  explicit __pointer_type_info(const char *__n, int __quals, const std::type_info *__type) : __pbase_type_info(__n, __quals, __type) { }

  virtual ~__pointer_type_info();

  virtual bool __is_pointer_p() const;

protected:
  virtual bool __pointer_catch(const __pbase_type_info *__thr_type, void **__thr_obj, unsigned __outer) const;
};

class __pointer_to_member_type_info: public __pbase_type_info
{
public:
  explicit __pointer_to_member_type_info(const char *__n, int __quals, const std::type_info *__type, const __class_type_info *__klass)
      : __pbase_type_info(__n, __quals, __type), __context(__klass)
  {
  }

  virtual ~__pointer_to_member_type_info();

  const __class_type_info *__context;

protected:
  virtual bool __pointer_catch(const __pbase_type_info *__thr_type, void **__thr_obj, unsigned __outer) const;
};

};      // namespace __cxxabiv1

namespace std
{

//  WARNING: ALL MEMBER FNS MUST BE GNU::WEAK

[[gnu::weak]] type_info::~type_info() { }

[[gnu::weak]] bool
type_info::__is_pointer_p() const
{
  return false;
}

[[gnu::weak]] bool
type_info::__is_function_p() const
{
  return false;
}

[[gnu::weak]] bool
type_info::__do_catch(const type_info *__thr_type, void **, unsigned) const
{
  return *this == *__thr_type;
}

[[gnu::weak]] bool
type_info::__do_upcast(const __cxxabiv1::__class_type_info *, void **) const
{
  return false;
}

};      // namespace std

namespace __cxxabiv1
{

[[gnu::weak]] __fundamental_type_info::~__fundamental_type_info() { }

[[gnu::weak]] __array_type_info::~__array_type_info() { }

[[gnu::weak]] __function_type_info::~__function_type_info() { }

[[gnu::weak]] __enum_type_info::~__enum_type_info() { }

[[gnu::weak]] __class_type_info::~__class_type_info() { }

[[gnu::weak]] __si_class_type_info::~__si_class_type_info() { }

[[gnu::weak]] __vmi_class_type_info::~__vmi_class_type_info() { }

[[gnu::weak]] __pbase_type_info::~__pbase_type_info() { }

[[gnu::weak]] __pointer_type_info::~__pointer_type_info() { }

[[gnu::weak]] __pointer_to_member_type_info::~__pointer_to_member_type_info() { }

[[gnu::weak]] bool
__function_type_info::__is_function_p() const
{
  return true;
}

[[gnu::weak]] bool
__class_type_info::__is_pointer_p() const
{
  return false;
}

[[gnu::weak]] bool
__class_type_info::__do_catch(const type_info *__thr_type, void **__thr_obj, unsigned __outer) const
{
  if ( *this == *__thr_type ) return true;
  if ( __outer >= 4 ) return false;      // refuse to chase through too many pointer levels
  return __thr_type->__do_upcast(this, __thr_obj);
}

// A plain class with no recorded bases: it is only itself.
[[gnu::weak]] bool
__class_type_info::__do_upcast(const __class_type_info *__target, void **) const
{
  return *this == *__target;
}

[[gnu::weak]] bool
__si_class_type_info::__do_upcast(const __class_type_info *__target, void **__obj_ptr) const
{
  if ( *this == *__target ) return true;
  return __base_type->__do_upcast(__target, __obj_ptr);
}

[[gnu::weak]] bool
__vmi_class_type_info::__do_upcast(const __class_type_info *__target, void **__obj_ptr) const
{
  // micron uses no multiple/virtual inheritance
  if ( *this == *__target ) return true;
  for ( unsigned i = 0; i < __base_count; ++i ) {
    if ( __base_info[i].__base_type->__do_upcast(__target, __obj_ptr) ) return true;
  }
  return false;
}

[[gnu::weak]] bool
__pbase_type_info::__do_catch(const type_info *__thr_type, void **__thr_obj, unsigned __outer) const
{
  if ( *this == *__thr_type ) return true;
  if ( !__thr_type->__is_pointer_p() ) return false;      // only pointer catches pointer
  if ( __outer & 1 ) return __pointer_catch(static_cast<const __pbase_type_info *>(__thr_type), __thr_obj, __outer);
  return false;
}

[[gnu::weak]] bool
__pbase_type_info::__pointer_catch(const __pbase_type_info *__thr_type, void **__thr_obj, unsigned __outer) const
{
  return __pointee->__do_catch(__thr_type->__pointee, __thr_obj, __outer + 2);
}

[[gnu::weak]] bool
__pointer_type_info::__is_pointer_p() const
{
  return true;
}

[[gnu::weak]] bool
__pointer_type_info::__pointer_catch(const __pbase_type_info *__thr_type, void **__thr_obj, unsigned __outer) const
{
  if ( __outer & 1 ) {
    if ( (__thr_type->__flags & ~__flags) & (__const_mask | __volatile_mask | __restrict_mask) ) return false;
  }
  return __pbase_type_info::__pointer_catch(__thr_type, __thr_obj, __outer);
}

[[gnu::weak]] bool
__pointer_to_member_type_info::__pointer_catch(const __pbase_type_info *__thr_type, void **__thr_obj, unsigned __outer) const
{
  return __pbase_type_info::__pointer_catch(__thr_type, __thr_obj, __outer);
}

};      // namespace __cxxabiv1

#endif      // __micron_eh
