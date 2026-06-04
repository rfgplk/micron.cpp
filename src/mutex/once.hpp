//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once      // hehe

#include "../memory/actions.hpp"
#include "../sync/yield.hpp"
#include "mutex.hpp"

// NOTE: moved from /sync to /mutex

namespace micron
{
// naming it like this to avoid ridiculous conflicts
template<auto F> class do_once
{
  // WARN: one class + one static gate is instantiated per F -> binary bloat
  static constexpr u8 ST_UNINIT = 0;
  static constexpr u8 ST_RUNNING = 1;
  static constexpr u8 ST_DONE = 2;

  static auto &
  _gate(void)
  {
    static atomic_token<u8> g(ST_UNINIT);
    return g;
  }

  // NOTE: for the "exactly once" guarantee F must be effectively noexcept
  template<typename... Args>
  static void
  _run(Args &&...args)
  {
    auto &g = _gate();
    for ( ;; ) {
      const u8 s = g.get(memory_order::acquire);
      if ( s == ST_DONE ) return;      // completed: acquire pairs with the release store below -> F's writes visible
      if ( s == ST_UNINIT && g.compare_and_swap(ST_UNINIT, ST_RUNNING) ) {
        struct __unwind {
          atomic_token<u8> *gp;
          bool armed;

          ~__unwind()
          {
            if ( armed ) gp->store(ST_UNINIT, memory_order::release);
          }
        } guard{ &g, true };

        F(micron::forward<Args>(args)...);
        guard.armed = false;
        g.store(ST_DONE, memory_order::release);      // publish completion AFTER F
        return;
      }
      micron::yield();      // ST_RUNNING (or lost CAS): block until the winner publishes DONE
    }
  }

public:
  ~do_once() { }

  do_once() { _run(); }

  template<typename... Args> do_once(Args &&...args) { _run(micron::forward<Args>(args)...); }

  // delete everything else explicitly
  do_once(const do_once &) = delete;
  do_once(do_once &&) = delete;
  do_once &operator=(do_once &&) = delete;
  do_once &operator=(const do_once &) = delete;
};
};      // namespace micron
