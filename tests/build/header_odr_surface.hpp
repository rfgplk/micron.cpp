//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Public Micron dependencies used together by Railway. Both objects must include the full surface.
#include <micron/atomic/atomic.hpp>
#include <micron/concepts.hpp>
#include <micron/errno.hpp>
#include <micron/except.hpp>
#include <micron/hash/crc.hpp>
#include <micron/linux/io.hpp>
#include <micron/linux/io/ext.hpp>
#include <micron/linux/io/io_structs.hpp>
#include <micron/linux/process/system.hpp>
#include <micron/linux/std.hpp>
#include <micron/linux/sys/fcntl.hpp>
#include <micron/linux/sys/poll.hpp>
#include <micron/linux/sys/stat.hpp>
#include <micron/linux/sys/time.hpp>
#include <micron/memory/actions.hpp>
#include <micron/memory/mman.hpp>
#include <micron/slice.hpp>
#include <micron/std.hpp>
#include <micron/string/strings.hpp>
#include <micron/sync/futex.hpp>
#include <micron/sync/pause.hpp>
#include <micron/syscall.hpp>
#include <micron/type_traits.hpp>
#include <micron/types.hpp>
