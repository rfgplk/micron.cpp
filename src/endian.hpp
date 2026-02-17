//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

static constexpr int little_endian = 1234;
static constexpr int big_endian = 4321;
static constexpr int pdp_endian = 3412;

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
static constexpr int byte_order = big_endian;
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_PDP_ENDIAN__)
static constexpr int byte_order = pdp_endian;
#else
static constexpr int byte_order = little_endian;
#endif

static constexpr int float_word_order = byte_order;

static constexpr bool is_big_endian = float_word_order == big_endian;

static constexpr bool is_little_endian = float_word_order == little_endian;

static constexpr int high_half = is_big_endian ? 0 : 1;

static constexpr int low_half = is_big_endian ? 1 : 0;
