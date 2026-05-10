//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

extern "C" {

extern void (*__preinit_array_start[])(void) __attribute__((weak, visibility("hidden")));
extern void (*__preinit_array_end[])(void) __attribute__((weak, visibility("hidden")));

extern void (*__init_array_start[])(void) __attribute__((weak, visibility("hidden")));
extern void (*__init_array_end[])(void) __attribute__((weak, visibility("hidden")));

extern void (*__fini_array_start[])(void) __attribute__((weak, visibility("hidden")));
extern void (*__fini_array_end[])(void) __attribute__((weak, visibility("hidden")));
};
