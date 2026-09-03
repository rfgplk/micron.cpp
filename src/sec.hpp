//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// microns linux security suite
//
// seccomp-bpf, landlock, namespaces and SELinux

// vocabulary
#include "sec/bits.hpp"
#include "sec/fn.hpp"

// syscall groups
#include "sec/groups.hpp"

// subsystems
#include "sec/landlock.hpp"
#include "sec/namespaces.hpp"
#include "sec/seccomp.hpp"
#include "sec/selinux.hpp"

#include "sec/policy.hpp"
#include "sec/sandbox.hpp"

#include "sec/fp.hpp"
