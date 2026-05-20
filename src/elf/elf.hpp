//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Module umbrella for src/elf/. Pulls in the high-level free-function API
// (open / list_symbols / execute) and the underlying Linux loader
// (handle_t / module_t / dyn_info_t / symbol_info_t). User code should
// prefer `#include "elf.hpp"` (the top-level umbrella one directory up).

#include "../linux/elf/elf.hpp"

#include "execute.hpp"
#include "open.hpp"
#include "symbols.hpp"
