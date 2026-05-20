//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../memory/actions.hpp"
#include "../../memory/new.hpp"
#include "../../memory/pointers/bits.hpp"
#include "../../tags.hpp"
#include "../../types.hpp"

namespace micron
{
namespace gfx
{
namespace vk
{

template<typename Hnd, typename Dlt> class unique
{
  Hnd __h{};
  [[no_unique_address]] Dlt __d{};

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Hnd;
  using element_type = Hnd;
  using deleter_type = Dlt;

  ~unique() noexcept { __reset(); }

  unique() noexcept = default;

  explicit unique(Hnd h) noexcept : __h(h), __d{} { }

  unique(Hnd h, Dlt d) noexcept : __h(h), __d(micron::move(d)) { }

  unique(const unique &) = delete;
  unique &operator=(const unique &) = delete;

  unique(unique &&o) noexcept : __h(o.__h), __d(micron::move(o.__d)) { o.__h = Hnd{}; }

  unique &
  operator=(unique &&o) noexcept
  {
    if ( this != &o ) {
      __reset();
      __h = o.__h;
      __d = micron::move(o.__d);
      o.__h = Hnd{};
    }
    return *this;
  }

  Hnd
  handle() const noexcept
  {
    return __h;
  }

  Hnd
  get() const noexcept
  {
    return __h;
  }

  bool
  active() const noexcept
  {
    return __h != Hnd{};
  }

  explicit
  operator bool() const noexcept
  {
    return active();
  }

  bool
  operator!() const noexcept
  {
    return !active();
  }

  Hnd
  release() noexcept
  {
    Hnd h = __h;
    __h = Hnd{};
    return h;
  }

  void
  reset(Hnd h = Hnd{}) noexcept
  {
    __reset();
    __h = h;
  }

  void
  swap(unique &o) noexcept
  {
    Hnd th = __h;
    __h = o.__h;
    o.__h = th;
    Dlt td = micron::move(__d);
    __d = micron::move(o.__d);
    o.__d = micron::move(td);
  }

  const Dlt &
  get_deleter() const noexcept
  {
    return __d;
  }

  Dlt &
  get_deleter() noexcept
  {
    return __d;
  }

  template<is_pointer_class O>
  bool
  operator==(const O &o) const noexcept
  {
    return __h == o.get();
  }

  bool
  operator==(Hnd o) const noexcept
  {
    return __h == o;
  }

private:
  void
  __reset() noexcept
  {
    if ( __h != Hnd{} ) __d(__h);
    __h = Hnd{};
  }
};

template<typename Hnd, typename Dlt>
inline void
swap(unique<Hnd, Dlt> &a, unique<Hnd, Dlt> &b) noexcept
{
  a.swap(b);
}

template<typename Hnd, typename Dlt> struct __shared_handler {
  Hnd h;
  Dlt d;
  atomic_token<count_t> refs;

  __shared_handler(Hnd hh, Dlt dd) noexcept : h(hh), d(micron::move(dd)), refs(count_t{ 1 }) { }

  __shared_handler(const __shared_handler &) = delete;
  __shared_handler(__shared_handler &&) = delete;
  __shared_handler &operator=(const __shared_handler &) = delete;
  __shared_handler &operator=(__shared_handler &&) = delete;
};

template<typename Hnd, typename Dlt> class shared
{
  __shared_handler<Hnd, Dlt> *__c = nullptr;

  void
  __release() noexcept
  {
    if ( !__c ) return;
    const count_t old = __c->refs.fetch_sub(count_t{ 1 }, memory_order_acq_rel);
    if ( old == count_t{ 1 } ) {

      if ( __c->h ) __c->d(__c->h);
      micron::__delete(__c);
    }
    __c = nullptr;
  }

public:
  using pointer_type = owning_pointer_tag;
  using category_type = pointer_tag;
  using mutability_type = mutable_tag;
  using value_type = Hnd;
  using element_type = Hnd;
  using deleter_type = Dlt;

  ~shared() noexcept { __release(); }

  shared() noexcept = default;

  shared(Hnd h, Dlt d) : __c(h ? micron::__new<__shared_handler<Hnd, Dlt>>(h, micron::move(d)) : nullptr) { }

  shared(const shared &o) noexcept : __c(o.__c)
  {
    if ( __c ) (void)__c->refs.fetch_add(count_t{ 1 }, memory_order_relaxed);
  }

  shared(shared &&o) noexcept : __c(o.__c) { o.__c = nullptr; }

  shared &
  operator=(const shared &o) noexcept
  {
    if ( this != &o ) {

      auto *incoming = o.__c;
      if ( incoming ) (void)incoming->refs.fetch_add(count_t{ 1 }, memory_order_relaxed);
      __release();
      __c = incoming;
    }
    return *this;
  }

  shared &
  operator=(shared &&o) noexcept
  {
    if ( this != &o ) {
      __release();
      __c = o.__c;
      o.__c = nullptr;
    }
    return *this;
  }

  Hnd
  handle() const noexcept
  {
    return __c ? __c->h : Hnd{};
  }

  Hnd
  get() const noexcept
  {
    return handle();
  }

  count_t
  use_count() const noexcept
  {
    return __c ? __c->refs.get(memory_order_relaxed) : count_t{ 0 };
  }

  bool
  active() const noexcept
  {
    return __c != nullptr;
  }

  explicit
  operator bool() const noexcept
  {
    return active();
  }

  bool
  operator!() const noexcept
  {
    return __c == nullptr;
  }

  void
  reset() noexcept
  {
    __release();
  }

  void
  swap(shared &o) noexcept
  {
    auto *t = __c;
    __c = o.__c;
    o.__c = t;
  }

  template<is_pointer_class O>
  bool
  operator==(const O &o) const noexcept
  {
    return handle() == o.get();
  }

  bool
  operator==(Hnd o) const noexcept
  {
    return handle() == o;
  }
};

template<typename Hnd, typename Dlt>
inline void
swap(shared<Hnd, Dlt> &a, shared<Hnd, Dlt> &b) noexcept
{
  a.swap(b);
}

};      // namespace vk
};      // namespace gfx
};      // namespace micron
