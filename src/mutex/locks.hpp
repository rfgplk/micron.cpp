//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "mutex.hpp"

#include "../atomic/flag.hpp"

namespace micron
{

enum class lock_starts { defer, adopt, locked, unlocked, attempt };

struct adopt_lock_t {
  explicit adopt_lock_t() = default;
};

inline constexpr adopt_lock_t adopt_lock{};

struct defer_lock_t {
  explicit defer_lock_t() = default;
};

inline constexpr defer_lock_t defer_lock{};

struct try_to_lock_t {
  explicit try_to_lock_t() = default;
};

inline constexpr try_to_lock_t try_to_lock{};

template <typename... Locks>
bool
try_lock(Locks... locks)
{
  (locks.try_lock(), ...);
}

template <typename... Locks>
void
lock(Locks &...locks)
{
  (locks.lock(), ...);
}

// use with care
template <typename... Locks>
void
unlock(Locks &...locks)
{
  (locks.unlock(), ...);
}
};     // namespace micron

#include "locks/auto_lock.hpp"
#include "locks/guard_lock.hpp"
#include "locks/queue_lock.hpp"
#include "locks/recursive_lock.hpp"
#include "locks/spin_lock.hpp"
#include "locks/unique_lock.hpp"
