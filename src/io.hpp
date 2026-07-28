//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// standard io include file
// everything needed for (general) io should be here

#include "io/io.hpp"

// plumbing
#include "io/os/block.hpp"
#include "io/os/dir.hpp"
#include "io/os/os_file.hpp"
#include "io/os/volatile.hpp"

// paths
#include "io/paths.hpp"
#include "io/realpath.hpp"

// functional glue
#include "io/__lines.hpp"
#include "io/fn.hpp"

// file porcelain
#include "io/bin.hpp"
#include "io/cached_file.hpp"
#include "io/file.hpp"

// filesystems
#include "io/concurrent_filesystem.hpp"
#include "io/filesystem.hpp"
#include "io/fsys.hpp"
#include "io/ftw.hpp"

#include "io/fp.hpp"

// streams
#include "io/pipe.hpp"
#include "io/serial.hpp"
#include "io/stream.hpp"

// output
#include "io/console.hpp"
#include "io/echo.hpp"
#include "io/format.hpp"
#include "io/pecho.hpp"

// io_uring-native file io
#include "io/flash.hpp"

#include "io/stdin.hpp"
#include "io/stdio.hpp"
