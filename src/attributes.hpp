//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#define restrict __restrict
#define chot __attribute__ ((hot))
#define ccold __attribute__ ((cold))
#define cinline __attribute__ ((always_inline))
#define cflatten __attribute__ ((flatten))
#define cpure __attribute__ ((pure))
#define cconst __attribute__ ((const))
#define cnoreorder __attribute__ ((no_reorder))
#define gconstructor __attribute__ ((constructor))
#define gdestructor __attribute__ ((destructor))


#define disable_block() if constexpr(false)
