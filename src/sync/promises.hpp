#pragma once

#include "../mutex/mutex.hpp"
#include "../mutex/locks.hpp"
namespace micron
{

class broken_promise{};
class promise
{
  micron::mutex mtx;

public:
  promise(void) {}
  promise(promise &&o) {}
  promise(const promise &) = delete;
  promise &operator=(const promise &) = delete;
  promise &
  operator=(promise &&o)
  {

    return *this;
  }
};


template <typename T, typename F, typename R, typename... Args>
inline bool
expect(const T &t, const F &f, R r, Args &&...args)
{
  if ( t == f ) {
    r(args...);
    return true;
  }
  return false;
}
template <typename T, typename F, typename... Args>
inline bool
expect(const T &t, const F &f)
{
  return (f == t);
}
};
