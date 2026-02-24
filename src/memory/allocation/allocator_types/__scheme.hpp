//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

/*
 * All allocators must provide these (public) methods
 * ~dest() = default;
 * const() = default;
 * copy(&) = default;
 * move(&&) = default;
 * &operator=(&) = default;
 * &operator=(&&o) = default;
 *
 * inline __attribute__((always_inline)) static constexpr size_t auto_size();
 * static chunk<byte> create(size_t n);
 * static chunk<byte> grow(chunk<byte> memory, size_t n);
 * static void destroy(const chunk<byte> mem);
 * byte *share(void);
 **/
