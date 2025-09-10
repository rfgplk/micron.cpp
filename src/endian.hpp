//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __PDP_ENDIAN 3412
#define __BYTE_ORDER __LITTLE_ENDIAN


#ifndef __FLOAT_WORD_ORDER
#define __FLOAT_WORD_ORDER __BYTE_ORDER
#endif

#ifdef __USE_MISC
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define BIG_ENDIAN __BIG_ENDIAN
#define PDP_ENDIAN __PDP_ENDIAN
#define BYTE_ORDER __BYTE_ORDER
#endif

#if __FLOAT_WORD_ORDER == __BIG_ENDIAN
#define BIG_ENDI 1
#undef LITTLE_ENDI
#define HIGH_HALF 0
#define LOW_HALF 1
#else
#if __FLOAT_WORD_ORDER == __LITTLE_ENDIAN
#undef BIG_ENDI
#define LITTLE_ENDI 1
#define HIGH_HALF 1
#define LOW_HALF 0
#endif
#endif
