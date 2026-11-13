//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../eh/eh_config.hpp"

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

  // WARNING: the deleting dtors of this hierarchy would otherwise call the global sized
  // ::operator delete (_ZdlPvm / _ZdlPvj), which a -nostdlib link that never sees
  // memory/new.hpp cannot satisfy
  static void
  operator delete(void *, __SIZE_TYPE__) noexcept
  {
  }

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
  constexpr explicit type_info(const char *__n) : __name(__n) { }

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
  constexpr explicit __fundamental_type_info(const char *__n) : std::type_info(__n) { }

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

struct __upcast_result {
  void *object = nullptr;
  usize identity = 0;
  bool found = false, ambiguous = false, accessible = false;

  void
  add(void *p, usize id, bool access) noexcept
  {
    if ( !found ) {
      object = p;
      identity = id;
      found = true;
      accessible = access;
    } else if ( object != p || (!p && identity != id) )
      ambiguous = true;
    else
      accessible = accessible || access;
  }
};

class __class_type_info: public std::type_info
{
public:
  explicit __class_type_info(const char *__n) : std::type_info(__n) { }

  virtual ~__class_type_info();

  virtual bool __is_pointer_p() const;
  virtual bool __do_catch(const type_info *__thr_type, void **__thr_obj, unsigned __outer) const;
  virtual bool __do_upcast(const __class_type_info *__target, void **__obj_ptr) const;
  virtual void __collect(const __class_type_info *, void *, __upcast_result &, bool, usize) const;
  virtual void __walk(void *, bool, void (*)(const __class_type_info *, void *, bool, void *), void *) const;
};

class __si_class_type_info: public __class_type_info
{
public:
  explicit __si_class_type_info(const char *__n, const __class_type_info *__base) : __class_type_info(__n), __base_type(__base) { }

  virtual ~__si_class_type_info();

  virtual bool __do_upcast(const __class_type_info *__target, void **__obj_ptr) const;
  virtual void __collect(const __class_type_info *, void *, __upcast_result &, bool, usize) const;
  virtual void __walk(void *, bool, void (*)(const __class_type_info *, void *, bool, void *), void *) const;

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
  virtual void __collect(const __class_type_info *, void *, __upcast_result &, bool, usize) const;
  virtual void __walk(void *, bool, void (*)(const __class_type_info *, void *, bool, void *), void *) const;

  unsigned int __flags;
  unsigned int __base_count;
  __base_class_type_info __base_info[1];

  enum __flags_masks { __non_diamond_repeat_mask = 0x1, __diamond_shaped_mask = 0x2 };
};

class __pbase_type_info: public std::type_info
{
public:
  constexpr explicit __pbase_type_info(const char *__n, int __quals, const std::type_info *__type)
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
  constexpr explicit __pointer_type_info(const char *__n, int __quals, const std::type_info *__type)
      : __pbase_type_info(__n, __quals, __type)
  {
  }

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
[[gnu::weak]] void
__class_type_info::__collect(const __class_type_info *target, void *object, __upcast_result &result, bool access, usize identity) const
{
  if ( *this == *target ) result.add(object, identity, access);
}

[[gnu::weak]] void
__si_class_type_info::__collect(const __class_type_info *target, void *object, __upcast_result &result, bool access, usize identity) const
{
  if ( *this == *target )
    result.add(object, identity, access);
  else
    __base_type->__collect(target, object, result, access, identity);
}

[[gnu::weak]] void
__vmi_class_type_info::__collect(const __class_type_info *target, void *object, __upcast_result &result, bool access, usize identity) const
{
  if ( *this == *target ) {
    result.add(object, identity, access);
    return;
  }
  for ( unsigned i = 0; i < __base_count; ++i ) {
    const auto &base = __base_info[i];
    long offset = base.__offset_flags >> __base_class_type_info::__offset_shift;
    const bool virt = base.__offset_flags & __base_class_type_info::__virtual_mask;
    void *candidate = object;
    if ( candidate ) {
      if ( virt ) offset = *reinterpret_cast<const long *>(*reinterpret_cast<const char *const *>(candidate) + offset);
      candidate = static_cast<char *>(candidate) + offset;
    }
    const usize child_id = virt ? reinterpret_cast<usize>(base.__base_type) : (identity * 33u) ^ (i + 1);
    base.__base_type->__collect(target, candidate, result, access && (base.__offset_flags & __base_class_type_info::__public_mask),
                                child_id);
  }
}

[[gnu::weak]] void
__class_type_info::__walk(void *object, bool access, void (*visit)(const __class_type_info *, void *, bool, void *), void *state) const
{
  visit(this, object, access, state);
}

[[gnu::weak]] void
__si_class_type_info::__walk(void *object, bool access, void (*visit)(const __class_type_info *, void *, bool, void *), void *state) const
{
  visit(this, object, access, state);
  __base_type->__walk(object, access, visit, state);
}

[[gnu::weak]] void
__vmi_class_type_info::__walk(void *object, bool access, void (*visit)(const __class_type_info *, void *, bool, void *), void *state) const
{
  visit(this, object, access, state);
  for ( unsigned i = 0; i < __base_count; ++i ) {
    const auto &base = __base_info[i];
    long offset = base.__offset_flags >> __base_class_type_info::__offset_shift;
    if ( base.__offset_flags & __base_class_type_info::__virtual_mask )
      offset = *reinterpret_cast<const long *>(*reinterpret_cast<const char *const *>(object) + offset);
    base.__base_type->__walk(static_cast<char *>(object) + offset, access && (base.__offset_flags & __base_class_type_info::__public_mask),
                             visit, state);
  }
}

[[gnu::weak]] bool
__class_type_info::__do_upcast(const __class_type_info *target, void **object) const
{
  __upcast_result result;
  __collect(target, *object, result, true, 1);
  if ( !result.found || result.ambiguous || !result.accessible ) return false;
  *object = result.object;
  return true;
}

[[gnu::weak]] bool
__si_class_type_info::__do_upcast(const __class_type_info *target, void **object) const
{
  return __class_type_info::__do_upcast(target, object);
}

[[gnu::weak]] bool
__vmi_class_type_info::__do_upcast(const __class_type_info *target, void **object) const
{
  return __class_type_info::__do_upcast(target, object);
}

[[gnu::weak]] bool
__pbase_type_info::__do_catch(const type_info *__thr_type, void **__thr_obj, unsigned __outer) const
{
  if ( *this == *__thr_type ) return true;
  if ( std::__micron_ti_strcmp(__thr_type->name(), "Dn") == 0 && __is_pointer_p() ) {
    *__thr_obj = nullptr;
    return true;
  }
  if ( __thr_type->__is_pointer_p() != __is_pointer_p() || !(__outer & 1) ) return false;
  if ( !__is_pointer_p() && (name()[0] != 'M' || __thr_type->name()[0] != 'M') ) return false;
  const auto *thrown = static_cast<const __pbase_type_info *>(__thr_type);
  if ( (thrown->__flags & ~__flags) & (__const_mask | __volatile_mask | __restrict_mask) ) return false;
  if ( (__flags & __noexcept_mask) && !(thrown->__flags & __noexcept_mask) ) return false;
  if ( !(__flags & __const_mask) ) __outer &= ~1u;
  return __pointer_catch(thrown, __thr_obj, __outer);
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
  if ( __outer < 2 && std::__micron_ti_strcmp(__pointee->name(), "v") == 0 && !__thr_type->__pointee->__is_function_p() ) return true;
  return __pbase_type_info::__pointer_catch(__thr_type, __thr_obj, __outer);
}

[[gnu::weak]] bool
__pointer_to_member_type_info::__pointer_catch(const __pbase_type_info *__thr_type, void **__thr_obj, unsigned __outer) const
{
  const auto *other = static_cast<const __pointer_to_member_type_info *>(__thr_type);
  if ( *__context != *other->__context ) return false;
  return __pbase_type_info::__pointer_catch(__thr_type, __thr_obj, __outer);
}

};      // namespace __cxxabiv1

namespace micron::wasm::eh
{
struct dynamic_source {
  const void *source;
  const __cxxabiv1::__class_type_info *type;
  bool found = false;

  static void
  visit(const __cxxabiv1::__class_type_info *type, void *object, bool access, void *p)
  {
    auto &s = *static_cast<dynamic_source *>(p);
    if ( access && object == s.source && *type == *s.type ) s.found = true;
  }
};

struct dynamic_target {
  dynamic_source source;
  const __cxxabiv1::__class_type_info *target;
  void *found = nullptr;
  bool ambiguous = false;

  static void
  visit(const __cxxabiv1::__class_type_info *type, void *object, bool, void *p)
  {
    auto &s = *static_cast<dynamic_target *>(p);
    if ( *type != *s.target ) return;
    auto probe = s.source;
    type->__walk(object, true, dynamic_source::visit, &probe);
    if ( !probe.found ) return;
    if ( s.found && s.found != object )
      s.ambiguous = true;
    else
      s.found = object;
  }
};
}      // namespace micron::wasm::eh

extern "C" [[gnu::weak, gnu::used]] void *
__dynamic_cast(const void *source, const __cxxabiv1::__class_type_info *source_type, const __cxxabiv1::__class_type_info *target_type,
               __PTRDIFF_TYPE__) noexcept
{
  if ( !source ) return nullptr;
  const auto *vtable = *static_cast<const usize *const *>(source);
  const auto *complete_type = reinterpret_cast<const __cxxabiv1::__class_type_info *>(vtable[-1]);
  void *complete = const_cast<char *>(static_cast<const char *>(source)) + static_cast<__PTRDIFF_TYPE__>(vtable[-2]);
  micron::wasm::eh::dynamic_target search{ { source, source_type }, target_type };
  complete_type->__walk(complete, true, micron::wasm::eh::dynamic_target::visit, &search);
  if ( search.ambiguous ) return nullptr;
  if ( search.found ) return search.found;
  auto src = search.source;
  complete_type->__walk(complete, true, micron::wasm::eh::dynamic_source::visit, &src);
  if ( !src.found ) return nullptr;
  __cxxabiv1::__upcast_result target;
  complete_type->__collect(target_type, complete, target, true, 1);
  return target.found && target.accessible && !target.ambiguous ? target.object : nullptr;
}

// Compiler-owned fundamental RTTI is supplied by this freestanding runtime.
extern "C" {
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_v asm("_ZTIv")("v");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pv asm("_ZTIPv")("Pv", 0, &__micron_ti_v);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKv asm("_ZTIPKv")("PKv", 1, &__micron_ti_v);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_w asm("_ZTIw")("w");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pw asm("_ZTIPw")("Pw", 0, &__micron_ti_w);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKw asm("_ZTIPKw")("PKw", 1, &__micron_ti_w);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_b asm("_ZTIb")("b");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pb asm("_ZTIPb")("Pb", 0, &__micron_ti_b);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKb asm("_ZTIPKb")("PKb", 1, &__micron_ti_b);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_c asm("_ZTIc")("c");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pc asm("_ZTIPc")("Pc", 0, &__micron_ti_c);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKc asm("_ZTIPKc")("PKc", 1, &__micron_ti_c);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_a asm("_ZTIa")("a");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pa asm("_ZTIPa")("Pa", 0, &__micron_ti_a);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKa asm("_ZTIPKa")("PKa", 1, &__micron_ti_a);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_h asm("_ZTIh")("h");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Ph asm("_ZTIPh")("Ph", 0, &__micron_ti_h);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKh asm("_ZTIPKh")("PKh", 1, &__micron_ti_h);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_s asm("_ZTIs")("s");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Ps asm("_ZTIPs")("Ps", 0, &__micron_ti_s);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKs asm("_ZTIPKs")("PKs", 1, &__micron_ti_s);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_t asm("_ZTIt")("t");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pt asm("_ZTIPt")("Pt", 0, &__micron_ti_t);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKt asm("_ZTIPKt")("PKt", 1, &__micron_ti_t);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_i asm("_ZTIi")("i");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pi asm("_ZTIPi")("Pi", 0, &__micron_ti_i);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKi asm("_ZTIPKi")("PKi", 1, &__micron_ti_i);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_j asm("_ZTIj")("j");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pj asm("_ZTIPj")("Pj", 0, &__micron_ti_j);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKj asm("_ZTIPKj")("PKj", 1, &__micron_ti_j);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_l asm("_ZTIl")("l");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pl asm("_ZTIPl")("Pl", 0, &__micron_ti_l);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKl asm("_ZTIPKl")("PKl", 1, &__micron_ti_l);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_m asm("_ZTIm")("m");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pm asm("_ZTIPm")("Pm", 0, &__micron_ti_m);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKm asm("_ZTIPKm")("PKm", 1, &__micron_ti_m);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_x asm("_ZTIx")("x");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Px asm("_ZTIPx")("Px", 0, &__micron_ti_x);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKx asm("_ZTIPKx")("PKx", 1, &__micron_ti_x);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_y asm("_ZTIy")("y");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Py asm("_ZTIPy")("Py", 0, &__micron_ti_y);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKy asm("_ZTIPKy")("PKy", 1, &__micron_ti_y);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_n asm("_ZTIn")("n");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pn asm("_ZTIPn")("Pn", 0, &__micron_ti_n);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKn asm("_ZTIPKn")("PKn", 1, &__micron_ti_n);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_o asm("_ZTIo")("o");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Po asm("_ZTIPo")("Po", 0, &__micron_ti_o);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKo asm("_ZTIPKo")("PKo", 1, &__micron_ti_o);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_f asm("_ZTIf")("f");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pf asm("_ZTIPf")("Pf", 0, &__micron_ti_f);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKf asm("_ZTIPKf")("PKf", 1, &__micron_ti_f);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_d asm("_ZTId")("d");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pd asm("_ZTIPd")("Pd", 0, &__micron_ti_d);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKd asm("_ZTIPKd")("PKd", 1, &__micron_ti_d);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_e asm("_ZTIe")("e");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pe asm("_ZTIPe")("Pe", 0, &__micron_ti_e);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKe asm("_ZTIPKe")("PKe", 1, &__micron_ti_e);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_g asm("_ZTIg")("g");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_Pg asm("_ZTIPg")("Pg", 0, &__micron_ti_g);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKg asm("_ZTIPKg")("PKg", 1, &__micron_ti_g);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_Ds asm("_ZTIDs")("Ds");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PDs asm("_ZTIPDs")("PDs", 0, &__micron_ti_Ds);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKDs asm("_ZTIPKDs")("PKDs", 1, &__micron_ti_Ds);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_Di asm("_ZTIDi")("Di");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PDi asm("_ZTIPDi")("PDi", 0, &__micron_ti_Di);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKDi asm("_ZTIPKDi")("PKDi", 1, &__micron_ti_Di);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_Du asm("_ZTIDu")("Du");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PDu asm("_ZTIPDu")("PDu", 0, &__micron_ti_Du);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKDu asm("_ZTIPKDu")("PKDu", 1, &__micron_ti_Du);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_Dn asm("_ZTIDn")("Dn");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__fundamental_type_info __micron_ti_Dh asm("_ZTIDh")("Dh");
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PDh asm("_ZTIPDh")("PDh", 0, &__micron_ti_Dh);
[[gnu::weak, gnu::used]] constinit __cxxabiv1::__pointer_type_info __micron_ti_PKDh asm("_ZTIPKDh")("PKDh", 1, &__micron_ti_Dh);
}

#endif      // __micron_eh
