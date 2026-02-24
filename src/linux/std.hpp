//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "sys/system.hpp"

namespace micron
{
using micron::posix::get_affinity;
using micron::posix::get_attrs;
using micron::posix::get_priority;
using micron::posix::get_scheduler;
using micron::posix::geteuid;
using micron::posix::getgid;
using micron::posix::getpid;
using micron::posix::getppid;
using micron::posix::getsid;
using micron::posix::gettid;
using micron::posix::getuid;
using micron::posix::kill;
using micron::posix::raise;
using micron::posix::set_affinity;
using micron::posix::set_attrs;
using micron::posix::set_priority;
using micron::posix::setgid;
using micron::posix::setpgid;
using micron::posix::setsid;
using micron::posix::setuid;
};     // namespace micron
