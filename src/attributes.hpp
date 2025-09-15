//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#define restrict __restrict
#define chot __attribute__((hot))
#define ccold __attribute__((cold))
#define cinline __attribute__((always_inline))
#define cflatten __attribute__((flatten))
#define cpure __attribute__((pure))
#define cconst __attribute__((const))
#define cnoreorder __attribute__((no_reorder))
#define gconstructor_ __attribute__((constructor))
#define gconstructor(x) __attribute__((constructor(x)))
#define gdestructor_ __attribute__((destructor))
#define gdestructor(x) __attribute__((destructor(x)))

#define inline_fn(x) cinline x
#define hot_fn(x) chot x
#define cold_fn(x) ccold x
#define pure_fn(x) cpure x
#define start_fn(x, y) gconstructor(y) x
#define end_fn(x, y) gdestructor(y) x
#define naked_fn __attribute__((naked)) void
