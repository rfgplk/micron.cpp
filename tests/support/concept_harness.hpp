//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Compile-time concept-satisfaction harness. Use REQUIRE_CONCEPT(Concept, T)
// to assert that T satisfies Concept; a failure is a static_assert with a
// readable message.

#include "../../src/concepts.hpp"
#include "../../src/type_traits.hpp"

namespace mtest
{

// Macros are variadic so callers can pass template types with embedded commas
// (e.g. `micron::svector<int, 32>`) without parenthesizing.
#define REQUIRE_CONCEPT(C, ...) static_assert(C<__VA_ARGS__>, "mtest: type does not satisfy concept '" #C "'")
#define REQUIRE_TRAIT_TRUE(Trait, ...) static_assert(Trait<__VA_ARGS__>::value, "mtest: trait '" #Trait "' is false")
#define REQUIRE_TRAIT_V_TRUE(Trait, ...) static_assert(Trait##_v<__VA_ARGS__>, "mtest: trait '" #Trait "_v' is false")
#define REQUIRE_SAME(A, B) static_assert(micron::is_same_v<A, B>, "mtest: types not the same")

};      // namespace mtest
