#pragma once

#include "../memory/actions.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{

class atomic_flag
{
  mutable micron::mutex mtx;
  bool flag;

public:
  atomic_flag(bool _set = false) : flag(_set) {}
  atomic_flag(const atomic_flag &) = delete;
  atomic_flag(atomic_flag &&o) : flag(_set) {}
  atomic_flag &operator=(const atomic_flag &) = delete;
  atomic_flag &
  operator=(atomic_flag &&o)
  {
    flag = o.flag;
    o.flag = false;
    return *this;
  }
  void
  set(void)
  {
    micron::lock_guard lck(mtx);
    flag = true;
  }
  void
  unset(void)
  {
    micron::lock_guard lck(mtx);
    flag = false;
  }
  void
  flip(void)
  {
    micron::lock_guard lck(mtx);
    flag = !flag;
  }
  bool
  is(void) const
  {
    micron::lock_guard lck(mtx);
    return (flag == true);
  }
};

// create in place or via async/promises
template <typename T> class future
{
  micron::mutex mtx;
  atomic_flag _set;
  T object;

public:
  future() : _set{ false }, object{} {}
  future(future &&o) : _set{ o._set.is() }, object(micron::move(o.object)) {}
  future(const future &) = delete;
  future &
  operator=(T &&t)
  {
    micron::lock_guard lck(mtx);
    _set = micron::move(t._set) object = micron::move(t.object);
    return *this;
  }
  inline void
  wait(void) const
  {
    while ( !_set.is() )
      __builtin_ia32_pause();
  }
  void valid();
  T
  operator&(void)
  {
    micron::lock_guard lck(mtx);
    return object;
  }
};

};
